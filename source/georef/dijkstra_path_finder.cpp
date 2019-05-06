/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "dijkstra_path_finder.h"

#include "utils/logger.h"

namespace navitia {
namespace georef {

void DijkstraPathFinder::start_distance_dijkstra(const navitia::time_duration& radius) {
    if (!starting_edge.found)
        return;
    computation_launch = true;
    // We start dijkstra from source and target nodes
    try {
#ifndef _DEBUG_DIJKSTRA_QUANTUM_
        dijkstra(starting_edge[source_e], starting_edge[target_e], dijkstra_distance_visitor(radius, distances));
#else
        dijkstra(starting_edge[source_e], starting_edge[target_e],
                 printer_distance_visitor(radius, distances, "source"));
#endif
    } catch (DestinationFound) {
    }
}

void DijkstraPathFinder::start_distance_or_target_dijkstra(const navitia::time_duration& radius,
                                                           const std::vector<vertex_t>& destinations) {
    if (!starting_edge.found)
        return;
    computation_launch = true;
    // We start dijkstra from source and target nodes
    try {
#ifndef _DEBUG_DIJKSTRA_QUANTUM_
        dijkstra(starting_edge[source_e], starting_edge[target_e],
                 dijkstra_distance_or_target_visitor(radius, distances, destinations));
#else
        dijkstra(starting_edge[source_e], starting_edge[target_e],
                 printer_distance_or_target_visitor(radius, distances, destinations, "direct_path_source"));
#endif
    } catch (DestinationFound&) {
    }
}

static routing::SpIdx get_id(const routing::SpIdx& idx) {
    return idx;
}
static std::string get_id(const type::GeographicalCoord& coord) {
    return coord.uri();
}

template <typename K, typename U, typename G>
boost::container::flat_map<K, georef::RoutingElement> DijkstraPathFinder::start_dijkstra_and_fill_duration_map(
    const navitia::time_duration& radius,
    const std::vector<U>& destinations,
    const G& projection_getter) {
    boost::container::flat_map<K, georef::RoutingElement> result;
    std::vector<std::pair<K, georef::ProjectionData>> projection_found_dests;
    for (const auto& dest : destinations) {
        auto projection = projection_getter(dest);
        // the stop point has been projected on the graph?
        if (projection.found) {
            projection_found_dests.push_back({get_id(dest), projection});
        } else {
            result[get_id(dest)] = georef::RoutingElement(navitia::time_duration(), georef::RoutingStatus_e::unknown);
        }
    }
    // if there are no stop_points projected on the graph, there is no need to start the dijkstra
    if (projection_found_dests.empty()) {
        return result;
    }

    start_distance_dijkstra(radius);

#ifdef _DEBUG_DIJKSTRA_QUANTUM_
    dump_dijkstra_for_quantum(starting_edge);
#endif
    for (const auto& dest : projection_found_dests) {
        // if our two points are projected on the same edge the
        // Dijkstra won't give us the correct value we need to handle
        // this case separately
        auto& projection = dest.second;
        auto& id = dest.first;
        navitia::time_duration duration;
        if (is_projected_on_same_edge(starting_edge, projection)) {
            // We calculate the duration for going to the edge, then to
            // the projected destination on the edge and finally to the
            // destination
            duration = path_duration_on_same_edge(starting_edge, projection);
        } else {
            duration = find_nearest_vertex(projection, true).first;
        }
        if (duration <= radius) {
            result[id] = georef::RoutingElement(duration, georef::RoutingStatus_e::reached);
        } else {
            result[id] = georef::RoutingElement(navitia::time_duration(), georef::RoutingStatus_e::unreached);
        }
    }
    return result;
}

std::vector<std::pair<type::idx_t, type::GeographicalCoord>> DijkstraPathFinder::crow_fly_find_nearest_stop_points(
    const navitia::time_duration& max_duration,
    const proximitylist::ProximityList<type::idx_t>& pl) {
    // Searching for all the elements that are less than radius meters awyway in crow fly
    float crow_fly_dist = max_duration.total_seconds() * speed_factor * georef::default_speed[mode];
    return pl.find_within(start_coord, double(crow_fly_dist));
}

struct ProjectionGetterByCache {
    const nt::Mode_e mode;
    const std::vector<GeoRef::ProjectionByMode>& projection_cache;
    const georef::ProjectionData& operator()(const routing::SpIdx& idx) const {
        return projection_cache[idx.val][mode];
    }
};

routing::map_stop_point_duration DijkstraPathFinder::find_nearest_stop_points(
    const navitia::time_duration& max_duration,
    const proximitylist::ProximityList<type::idx_t>& pl) {
    if (max_duration == navitia::seconds(0)) {
        return {};
    }

    auto elements = crow_fly_find_nearest_stop_points(max_duration, pl);
    if (elements.empty()) {
        return {};
    }

    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    routing::map_stop_point_duration result;
    // case 1 : start coord is not an edge (crow fly)
    if (!starting_edge.found) {
        LOG4CPLUS_DEBUG(logger, "starting_edge not found!");
        // if no street network, return stop_points that are within
        // radius distance (with sqrt(2) security factor)
        // if we are not dealing with 0,0 coordinates (incorrect data), allow crow fly
        if (start_coord != type::GeographicalCoord(0, 0)) {
            for (const auto& element : elements) {
                if (element.second == type::GeographicalCoord(0, 0)) {
                    continue;
                }
                navitia::time_duration duration = crow_fly_duration(start_coord.distance_to(element.second)) * sqrt(2);
                // if the radius is still ok with sqrt(2) factor
                auto sp_idx = routing::SpIdx(element.first);
                if (duration < max_duration && distance_to_entry_point.count(sp_idx) == 0) {
                    result[sp_idx] = duration;
                    distance_to_entry_point[sp_idx] = duration;
                }
            }
        }
    }
    // case 2 : start coord is an edge (dijkstra)
    else {
        std::vector<routing::SpIdx> dest_sp_idx;
        for (const auto& e : elements) {
            dest_sp_idx.push_back(routing::SpIdx{e.first});
        }
        ProjectionGetterByCache projection_getter{mode, geo_ref.projected_stop_points};
        auto resp = start_dijkstra_and_fill_duration_map<routing::SpIdx, routing::SpIdx, ProjectionGetterByCache>(
            max_duration, dest_sp_idx, projection_getter);
        for (const auto& r : resp) {
            if (r.second.routing_status == RoutingStatus_e::reached) {
                result[r.first] = r.second.time_duration;
            }
        }
    }
    return result;
}

struct ProjectionGetterOnFly {
    const GeoRef& geo_ref;
    const nt::idx_t offset;
    const georef::ProjectionData operator()(const type::GeographicalCoord& coord) const {
        return georef::ProjectionData{coord, geo_ref, offset, geo_ref.pl};
    }
};

boost::container::flat_map<DijkstraPathFinder::coord_uri, georef::RoutingElement>
DijkstraPathFinder::get_duration_with_dijkstra(const navitia::time_duration& radius,
                                               const std::vector<type::GeographicalCoord>& dest_coords) {
    if (dest_coords.empty()) {
        return {};
    }
    nt::idx_t offset;
    if (mode == type::Mode_e::Car) {
        // on direct path with car we want to arrive on the walking graph
        offset = geo_ref.offsets[nt::Mode_e::Walking];
    } else {
        offset = geo_ref.offsets[mode];
    }

    ProjectionGetterOnFly projection_getter{geo_ref, offset};
    return start_dijkstra_and_fill_duration_map<DijkstraPathFinder::coord_uri, type::GeographicalCoord,
                                                ProjectionGetterOnFly>(radius, dest_coords, projection_getter);
}

std::pair<navitia::time_duration, ProjectionData::Direction> DijkstraPathFinder::update_path(
    const ProjectionData& target) {
    constexpr auto max = bt::pos_infin;
    if (!target.found)
        return {max, source_e};
    assert(boost::edge(target[source_e], target[target_e], geo_ref.graph).second);

    computation_launch = true;
    if (distances[target[source_e]] == max || distances[target[target_e]] == max) {
        bool found = false;
        try {
            dijkstra(starting_edge[source_e], starting_edge[target_e],
                     dijkstra_target_all_visitor({target[source_e], target[target_e]}));
        } catch (DestinationFound) {
            found = true;
        }

        // if no way has been found, we can stop the search
        if (!found) {
            LOG4CPLUS_WARN(log4cplus::Logger::getInstance("Logger"),
                           "unable to find a way from start edge ["
                               << starting_edge[source_e] << "-" << starting_edge[target_e] << "] to ["
                               << target[source_e] << "-" << target[target_e] << "]");

#ifdef _DEBUG_DIJKSTRA_QUANTUM_
            dump_dijkstra_for_quantum(target);
#endif

            return {max, source_e};
        }
    }
    // if we succeded in the first search, we must have found one of the other distances
    assert(distances[target[source_e]] != max && distances[target[target_e]] != max);

    return find_nearest_vertex(target);
}

navitia::time_duration DijkstraPathFinder::get_distance(type::idx_t target_idx) {
    constexpr auto max = bt::pos_infin;

    if (!starting_edge.found)
        return max;
    assert(boost::edge(starting_edge[source_e], starting_edge[target_e], geo_ref.graph).second);

    ProjectionData target = this->geo_ref.projected_stop_points[target_idx][mode];

    auto nearest_edge = update_path(target);

    return nearest_edge.first;
}

/**
  The _DEBUG_DIJKSTRA_QUANTUM_ activate at compil time some dump used in quantum to analyze
  the street network.
  WARNING, it will slow A LOT the djisktra and must be used only for debug
  */
#ifdef _DEBUG_DIJKSTRA_QUANTUM_
/**
 * Visitor to dump the visited edges and vertexes
 */
struct printer_all_visitor : public target_all_visitor {
    std::ofstream file_vertex, file_edge;
    size_t cpt_v = 0, cpt_e = 0;

    void init_files() {
        file_vertex.open("vertexes.csv");
        file_vertex << "idx; lat; lon; vertex_id" << std::endl;
        file_edge.open("edges.csv");
        file_edge << "idx; lat from; lon from; lat to; long to" << std::endl;
    }

    printer_all_visitor(std::vector<vertex_t> destinations) : target_all_visitor(destinations) { init_files(); }

    ~printer_all_visitor() {
        file_vertex.close();
        file_edge.close();
    }

    printer_all_visitor(const printer_all_visitor& o) : target_all_visitor(o) { init_files(); }

    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type& g) {
        file_vertex << cpt_v++ << ";" << g[u].coord << ";" << u << std::endl;
        target_all_visitor::finish_vertex(u, g);
    }

    template <typename graph_type>
    void examine_edge(edge_t e, graph_type& g) {
        file_edge << cpt_e++ << ";" << g[boost::source(e, g)].coord << ";" << g[boost::target(e, g)].coord
                  << ";LINESTRING(" << g[boost::source(e, g)].coord.lon() << " " << g[boost::source(e, g)].coord.lat()
                  << ", " << g[boost::target(e, g)].coord.lon() << " " << g[boost::target(e, g)].coord.lat() << ")"
                  << ";" << e << std::endl;
        target_all_visitor::examine_edge(e, g);
    }
};

void DijkstraPathFinder::dump_dijkstra_for_quantum(const ProjectionData& target) {
    /* for debug in quantum gis, we dump 4 files :
     * - one for the start edge (start.csv)
     * - one for the destination edge (desitination.csv)
     * - one with all visited edges (edges.csv)
     * - one with all visited vertex (vertex.csv)
     * - one with the out edges of the target (out_edges.csv)
     *
     * the files are to be open in quantum with the csv layer
     * */
    std::ofstream start, destination, out_edge;
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"), "genrating debug trace for streetnetwork");
    start.open("start.csv");
    destination.open("destination.csv");
    start << "x;y;mode transport" << std::endl
          << geo_ref.graph[starting_edge[source_e]].coord << ";" << (int)(mode) << std::endl
          << geo_ref.graph[starting_edge[target_e]].coord << ";" << (int)(mode) << std::endl;
    destination << "x;y;" << std::endl
                << geo_ref.graph[target[source_e]].coord << std::endl
                << geo_ref.graph[target[target_e]].coord << std::endl;

    out_edge.open("out_edges.csv");
    out_edge << "target;x;y;" << std::endl;
    BOOST_FOREACH (edge_t e, boost::out_edges(target[source_e], geo_ref.graph)) {
        out_edge << "source;" << geo_ref.graph[boost::target(e, geo_ref.graph)].coord << std::endl;
    }
    BOOST_FOREACH (edge_t e, boost::out_edges(target[target_e], geo_ref.graph)) {
        out_edge << "target;" << geo_ref.graph[boost::target(e, geo_ref.graph)].coord << std::endl;
    }
    try {
        dijkstra(starting_edge[source_e], printer_all_visitor({target[source_e], target[target_e]}));
    } catch (DestinationFound) {
    }
}
#endif
}  // namespace georef
}  // namespace navitia
