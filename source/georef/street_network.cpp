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

#include "street_network.h"
#include "type/data.h"
#include "georef.h"
#include <boost/math/constants/constants.hpp>
#include <chrono>
#ifdef _DEBUG_DIJKSTRA_QUANTUM_
#include <boost/foreach.hpp>
#endif

namespace navitia { namespace georef {

//a bit of sugar
const auto source_e = ProjectionData::Direction::Source;
const auto target_e = ProjectionData::Direction::Target;

navitia::time_duration PathFinder::crow_fly_duration(const double distance) const {
    // For BSS we want the default speed of walking, because on extremities we walk !
    const auto mode_ = mode == nt::Mode_e::Bss ? nt::Mode_e::Walking : mode;
    return navitia::seconds(distance / (default_speed[mode_] * speed_factor));
}

static bool is_projected_on_same_edge(const ProjectionData& p1, const ProjectionData& p2){
    return (p1[source_e] == p2[source_e] && p1[target_e] == p2[target_e])
                || (p1[source_e] == p2[target_e] && p1[target_e] == p2[source_e]);
}

navitia::time_duration PathFinder::path_duration_on_same_edge(const ProjectionData& p1, const ProjectionData& p2){
    return crow_fly_duration(p1.real_coord.distance_to(p1.projected)
                             + p1.projected.distance_to(p2.projected)
                             + p2.projected.distance_to(p2.real_coord));
}


StreetNetwork::StreetNetwork(const GeoRef &geo_ref) :
    geo_ref(geo_ref),
    departure_path_finder(geo_ref),
    arrival_path_finder(geo_ref),
    direct_path_finder(geo_ref)
{}

void StreetNetwork::init(const type::EntryPoint& start, boost::optional<const type::EntryPoint&> end) {
    departure_path_finder.init(start.coordinates, start.streetnetwork_params.mode, start.streetnetwork_params.speed_factor);
    if (end) {
        arrival_path_finder.init((*end).coordinates, (*end).streetnetwork_params.mode, (*end).streetnetwork_params.speed_factor);
    }
}

bool StreetNetwork::departure_launched() const {return departure_path_finder.computation_launch;}
bool StreetNetwork::arrival_launched() const {return arrival_path_finder.computation_launch;}

routing::map_stop_point_duration
StreetNetwork::find_nearest_stop_points(navitia::time_duration radius,
                                        const proximitylist::ProximityList<type::idx_t>& pl,
                                        bool use_second) {
    // delegate to the arrival or departure pathfinder
    // results are store to build the routing path after the transportation routing computation
    return (use_second ? arrival_path_finder : departure_path_finder).find_nearest_stop_points(radius, pl);
}

navitia::time_duration StreetNetwork::get_distance(type::idx_t target_idx, bool use_second) {
    return (use_second ? arrival_path_finder : departure_path_finder).get_distance(target_idx);
}

Path StreetNetwork::get_path(type::idx_t idx, bool use_second) {
    Path result;
    if (! use_second) {
        result = departure_path_finder.get_path(idx);

        if (! result.path_items.empty()) {
            result.path_items.front().coordinates.push_front(departure_path_finder.starting_edge.projected);
        }
    } else {
        result = arrival_path_finder.get_path(idx);

        //we have to reverse the path
        std::reverse(result.path_items.begin(), result.path_items.end());
        int last_angle = 0;
        const auto& speed_factor = arrival_path_finder.speed_factor;
        for (auto& item : result.path_items) {
            std::reverse(item.coordinates.begin(), item.coordinates.end());

            //we have to reverse the directions too
            // the first direction become 0,
            // and we 'shift' all directions to the next path_item after reverting them
            int current_angle = -1 * item.angle;
            item.angle = last_angle;
            last_angle = current_angle;

            // FIXME: ugly temporary fix
            // while we don't use a boost::reverse_graph, the easiest way to handle
            // the bss rent/putback section in the arrival section is to swap them
            if (item.transportation == PathItem::TransportCaracteristic::BssTake) {
                item.transportation = PathItem::TransportCaracteristic::BssPutBack;
                item.duration = geo_ref.default_time_bss_putback / speed_factor;
            } else if (item.transportation == PathItem::TransportCaracteristic::BssPutBack) {
                item.transportation = PathItem::TransportCaracteristic::BssTake;
                item.duration = geo_ref.default_time_bss_pickup / speed_factor;
            }
            if (item.transportation == PathItem::TransportCaracteristic::CarPark) {
                item.transportation = PathItem::TransportCaracteristic::CarLeaveParking;
                item.duration = geo_ref.default_time_parking_leave / speed_factor;
            } else if (item.transportation == PathItem::TransportCaracteristic::CarLeaveParking) {
                item.transportation = PathItem::TransportCaracteristic::CarPark;
                item.duration = geo_ref.default_time_parking_park / speed_factor;
            }
        }

        if (! result.path_items.empty()) {
            //no direction for the first elt
            result.path_items.back().coordinates.push_back(arrival_path_finder.starting_edge.projected);
        }
    }

    return result;
}

Path
StreetNetwork::get_direct_path(const type::EntryPoint& origin, const type::EntryPoint& destination) {
    auto dest_mode = origin.streetnetwork_params.mode;
    if(dest_mode == type::Mode_e::Car){
        //on direct path with car we want to arrive on the walking graph
        dest_mode = type::Mode_e::Walking;
    }
    const auto dest_edge = ProjectionData(destination.coordinates,
                                          geo_ref,
                                          geo_ref.offsets[dest_mode],
                                          geo_ref.pl);
    if (! dest_edge.found) { return Path(); }
    const auto max_dur = origin.streetnetwork_params.max_duration
        + destination.streetnetwork_params.max_duration;
    direct_path_finder.init(origin.coordinates,
                            origin.streetnetwork_params.mode,
                            origin.streetnetwork_params.speed_factor);
    direct_path_finder.start_distance_dijkstra(max_dur);
    const auto dest_vertex = direct_path_finder.find_nearest_vertex(dest_edge, true);
    const auto res = direct_path_finder.get_path(dest_edge, dest_vertex);
    if (res.duration > max_dur) { return Path(); }
    return res;
}

PathFinder::PathFinder(const GeoRef& gref) : geo_ref(gref) {}

void PathFinder::init(const type::GeographicalCoord& start_coord, nt::Mode_e mode, const float speed_factor) {
    computation_launch = false;
    // we look for the nearest edge from the start coordinate
    // in the right transport mode (walk, bike, car, ...) (ie offset)
    this->mode = mode;
    this->speed_factor = speed_factor; //the speed factor is the factor we have to multiply the edge cost with
    nt::idx_t offset = this->geo_ref.offsets[mode];
    this->start_coord = start_coord;
    starting_edge = ProjectionData(start_coord, this->geo_ref, offset, this->geo_ref.pl);

    distance_to_entry_point.clear();
    //we initialize the distances to the maximum value
    size_t n = boost::num_vertices(geo_ref.graph);
    distances.assign(n, bt::pos_infin);
    //for the predecessors no need to clean the values, the important one will be updated during search
    predecessors.resize(n);

    if (starting_edge.found) {
        //durations initializations
        distances[starting_edge[source_e]] = crow_fly_duration(starting_edge.distances[source_e]); //for the projection, we use the default walking speed.
        distances[starting_edge[target_e]] = crow_fly_duration(starting_edge.distances[target_e]);
        predecessors[starting_edge[source_e]] = starting_edge[source_e];
        predecessors[starting_edge[target_e]] = starting_edge[target_e];

        //small enchancement, if the projection is done on a node, we disable the crow fly
        if (starting_edge.distances[source_e] < 0.01) {
            predecessors[starting_edge[target_e]] = starting_edge[source_e];
            auto e = boost::edge(starting_edge[source_e], starting_edge[target_e], geo_ref.graph).first;
            distances[starting_edge[target_e]] = geo_ref.graph[e].duration;
        } else if (starting_edge.distances[target_e] < 0.01) {
            predecessors[starting_edge[source_e]] = starting_edge[target_e];
            auto edge_pair = boost::edge(starting_edge[target_e], starting_edge[source_e], geo_ref.graph);
            if (edge_pair.second) {
                distances[starting_edge[source_e]] = geo_ref.graph[edge_pair.first].duration;
            } else {
                // since we reverse the edge (from target to source) the edge might not exists
                // (for one way street for example). we thus forbid to start from the source
                distances[starting_edge[source_e]] = bt::pos_infin;
            }
        }
    }
}

void PathFinder::start_distance_dijkstra(navitia::time_duration radius) {
    if (! starting_edge.found)
        return ;
    computation_launch = true;
    // We start dijkstra from source and target nodes
    try {
#ifndef _DEBUG_DIJKSTRA_QUANTUM_
        dijkstra(starting_edge[source_e], distance_visitor(radius, distances));
#else
        dijkstra(starting_edge[source_e], printer_distance_visitor(radius, distances, "source"));
#endif
    } catch(DestinationFound){}

    try {
#ifndef _DEBUG_DIJKSTRA_QUANTUM_
        dijkstra(starting_edge[target_e], distance_visitor(radius, distances));
#else
        dijkstra(starting_edge[target_e], printer_distance_visitor(radius, distances, "target"));
#endif
    } catch(DestinationFound){}

}

std::vector<std::pair<type::idx_t, type::GeographicalCoord>>
PathFinder::crow_fly_find_nearest_stop_points(navitia::time_duration radius,
                                              const proximitylist::ProximityList<type::idx_t>& pl) {
    // Searching for all the elements that are less than radius meters awyway in crow fly
    float crow_fly_dist = radius.total_seconds() * speed_factor * georef::default_speed[mode];
    return pl.find_within(start_coord, crow_fly_dist);
}

routing::map_stop_point_duration
PathFinder::find_nearest_stop_points(navitia::time_duration radius,
                                     const proximitylist::ProximityList<type::idx_t>& pl) {
    auto elements = crow_fly_find_nearest_stop_points(radius, pl);

    routing::map_stop_point_duration result;
    if (! starting_edge.found){

        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("Logger"), "starting_edge not found!");
        // if no street network, return stop_points that are within
        // radius distance (with sqrt(2) security factor)
        // if we are not dealing with 0,0 coordinates (incorrect data), allow crow fly
        if(start_coord != type::GeographicalCoord(0, 0)) {
            for (const auto& element: elements) {
                if (element.second == type::GeographicalCoord(0, 0)) {
                    continue;
                }
                navitia::time_duration duration =
                        crow_fly_duration(start_coord.distance_to(element.second)) * sqrt(2);
                // if the radius is still ok with sqrt(2) factor
                auto sp_idx = routing::SpIdx(element.first);
                if (duration < radius && distance_to_entry_point.count(sp_idx) == 0) {
                    result[sp_idx] = duration;
                    distance_to_entry_point[sp_idx] = duration;
                }
            }
        }
        return result;
    }

    if (elements.empty()) {
        return result;
    }
    start_distance_dijkstra(radius);
#ifdef _DEBUG_DIJKSTRA_QUANTUM_
    dump_dijkstra_for_quantum(starting_edge);
#endif

    const auto max = bt::pos_infin;
    for (auto element: elements) {
        const ProjectionData& projection = this->geo_ref.projected_stop_points[element.first][mode];
        // the stop point has been projected on the graph?

        if(projection.found){
            //if our two points are projected on the same edge the Dijkstra won't give us the correct value
            // we need to handle this case separately
            if(is_projected_on_same_edge(starting_edge, projection)){
                //We calculate the duration for going to the edge, then to the projected destination on the edge
                //and finally to the destination
                auto duration = path_duration_on_same_edge(starting_edge, projection);
                if(duration <= radius){
                    result[routing::SpIdx(element.first)] = duration;
                }
            }else{
                navitia::time_duration best_dist = max;
                if (distances[projection[source_e]] < max) {
                    best_dist = distances[projection[source_e]] + crow_fly_duration(projection.distances[source_e]);
                }
                if (distances[projection[target_e]] < max) {
                    best_dist = std::min(best_dist, distances[projection[target_e]] + crow_fly_duration(projection.distances[target_e]));
                }

                if (best_dist <= radius) {
                    result[routing::SpIdx(element.first)] = best_dist;
                }
            }
        }
    }
    return result;
}

navitia::time_duration PathFinder::get_distance(type::idx_t target_idx) {
    constexpr auto max = bt::pos_infin;

    if (! starting_edge.found)
        return max;
    assert(boost::edge(starting_edge[source_e], starting_edge[target_e], geo_ref.graph).second);

    ProjectionData target = this->geo_ref.projected_stop_points[target_idx][mode];

    auto nearest_edge = update_path(target);

    return nearest_edge.first;
}

std::pair<navitia::time_duration, ProjectionData::Direction> PathFinder::find_nearest_vertex(const ProjectionData& target, bool handle_on_node) const {
    constexpr auto max = bt::pos_infin;
    if (! target.found)
        return {max, source_e};

    if (distances[target[source_e]] == max) //if one distance has not been reached, both have not been reached
        return {max, source_e};

    if (handle_on_node) {
        //handle if the projection is done on a node
        if (target.distances[source_e] < 0.01) {
            return {distances[target[source_e]], source_e};
        } else if (target.distances[target_e] < 0.01) {
            return {distances[target[target_e]], target_e};
        }
    }

    auto source_dist = distances[target[source_e]] + crow_fly_duration(target.distances[source_e]);
    auto target_dist = distances[target[target_e]] + crow_fly_duration(target.distances[target_e]);

    if (target_dist < source_dist)
        return {target_dist, target_e};

    return {source_dist, source_e};
}

Path PathFinder::get_path(type::idx_t idx) {
    if (! computation_launch)
        return {};
    ProjectionData projection = this->geo_ref.projected_stop_points[idx][mode];

    auto nearest_edge = find_nearest_vertex(projection);

    return get_path(projection, nearest_edge);
}

void PathFinder::add_custom_projections_to_path(Path& p, bool append_to_begin, const ProjectionData& projection, ProjectionData::Direction d) const {
    auto item_to_update = [append_to_begin](Path& p) -> PathItem& { return (append_to_begin ? p.path_items.front() : p.path_items.back()); };
    auto add_in_path = [append_to_begin](Path& p, const PathItem& item) {
        return (append_to_begin ? p.path_items.push_front(item) : p.path_items.push_back(item));
    };

    edge_t start_e = boost::edge(projection[source_e], projection[target_e], geo_ref.graph).first;
    Edge start_edge = geo_ref.graph[start_e];

    auto duration = crow_fly_duration(projection.distances[d]);

    //we need to update the total length
    p.duration += duration;

    //we aither add the starting coordinate to the first path item or create a new path item if it was another way
    nt::idx_t first_way_idx = (p.path_items.empty() ? type::invalid_idx : item_to_update(p).way_idx);
    if (start_edge.way_idx != first_way_idx || first_way_idx == type::invalid_idx) {
        //there can be an item with no way, so we will update this item
        if (! p.path_items.empty() && item_to_update(p).way_idx == type::invalid_idx) {
            item_to_update(p).way_idx = start_edge.way_idx;
            item_to_update(p).duration += duration;
        }
        else {
            PathItem item;
            item.way_idx = start_edge.way_idx;
            item.duration = duration;

            if (!p.path_items.empty()) {
                //still complexifying stuff... TODO: simplify this
                //we want the projection to be done with the previous transportation mode
                switch (item_to_update(p).transportation) {
                case georef::PathItem::TransportCaracteristic::Walk:
                case georef::PathItem::TransportCaracteristic::Car:
                case georef::PathItem::TransportCaracteristic::Bike:
                    item.transportation = item_to_update(p).transportation;
                    break;
                    //if we were switching between walking and biking, we need to take either
                    //the previous or the next transportation mode depending on 'append_to_begin'
                case georef::PathItem::TransportCaracteristic::BssTake:
                    item.transportation = (append_to_begin ? georef::PathItem::TransportCaracteristic::Walk
                                                           : georef::PathItem::TransportCaracteristic::Bike);
                    break;
                case georef::PathItem::TransportCaracteristic::BssPutBack:
                    item.transportation = (append_to_begin ? georef::PathItem::TransportCaracteristic::Bike
                                                           : georef::PathItem::TransportCaracteristic::Walk);
                    break;
                case georef::PathItem::TransportCaracteristic::CarLeaveParking:
                    item.transportation = (append_to_begin ? georef::PathItem::TransportCaracteristic::Walk
                                                           : georef::PathItem::TransportCaracteristic::Car);
                    break;
                case georef::PathItem::TransportCaracteristic::CarPark:
                    item.transportation = (append_to_begin ? georef::PathItem::TransportCaracteristic::Car
                                                           : georef::PathItem::TransportCaracteristic::Walk);
                    break;
                default:
                    throw navitia::recoverable_exception("unhandled transportation carac case");
                }
            }
            add_in_path(p, item);
        }
    } else {
        //we just need to update the duration
        item_to_update(p).duration += duration;
    }

    auto& coord_list = item_to_update(p).coordinates;
    if (append_to_begin) {
        if (coord_list.empty() || coord_list.front() != projection.projected) {
            coord_list.push_front(projection.projected);
        }
    }
    else {
        if (coord_list.empty() || coord_list.back() != projection.projected) {
            coord_list.push_back(projection.projected);
        }
    }
}

Path PathFinder::get_path(const ProjectionData& target, std::pair<navitia::time_duration, ProjectionData::Direction> nearest_edge) {
    if (! computation_launch || ! target.found || nearest_edge.first == bt::pos_infin)
        return {};

    Path result;
    if(is_projected_on_same_edge(starting_edge, target)){
        PathItem item;
        item.duration = path_duration_on_same_edge(starting_edge, target);
        item.coordinates.push_back(starting_edge.projected);
        item.coordinates.push_back(target.projected);

        auto edge_pair = boost::edge(starting_edge[source_e], starting_edge[target_e], geo_ref.graph);
        if (! edge_pair.second) {
            throw navitia::exception("impossible to find an edge");
        }
        Edge edge = geo_ref.graph[edge_pair.first];
        item.way_idx = edge.way_idx;
        item.transportation = geo_ref.get_caracteristic(edge_pair.first);
        result.path_items.push_back(item);
        result.duration += item.duration;
    }else{
        result = this->build_path(target[nearest_edge.second]);
        add_projections_to_path(result, true);

        //we need to put the end projections too
        add_custom_projections_to_path(result, false, target, nearest_edge.second);
    }


    return result;
}

void PathFinder::add_projections_to_path(Path& p, bool append_to_begin) const {
    //we need to find out which side of the projection has been used to compute the right length
    assert(! p.path_items.empty());

    const auto& path_item_to_consider = append_to_begin ? p.path_items.front(): p.path_items.back();
    const auto& coord_to_consider = append_to_begin ? path_item_to_consider.coordinates.front(): path_item_to_consider.coordinates.back();

    ProjectionData::Direction direction;
    if (coord_to_consider == geo_ref.graph[starting_edge[source_e]].coord) {
        direction = source_e;
    } else if (coord_to_consider == geo_ref.graph[starting_edge[target_e]].coord) {
        direction = target_e;
    } else {
        throw navitia::exception("by construction, should never happen");
    }

    add_custom_projections_to_path(p, append_to_begin, starting_edge, direction);
}

std::pair<navitia::time_duration, ProjectionData::Direction> PathFinder::update_path(const ProjectionData& target) {
    constexpr auto max = bt::pos_infin;
    if (! target.found)
        return {max, source_e};
    assert(boost::edge(target[source_e], target[target_e], geo_ref.graph).second );

    computation_launch = true;

    if (distances[target[source_e]] == max || distances[target[target_e]] == max) {
        bool found = false;
        try {
            dijkstra(starting_edge[source_e], target_all_visitor({target[source_e], target[target_e]}));
        } catch(DestinationFound) { found = true; }

        //if no way has been found, we can stop the search
        if ( ! found ) {
            LOG4CPLUS_WARN(log4cplus::Logger::getInstance("Logger"), "unable to find a way from start edge ["
                           << starting_edge[source_e] << "-" << starting_edge[target_e]
                           << "] to [" << target[source_e] << "-" << target[target_e] << "]");

#ifdef _DEBUG_DIJKSTRA_QUANTUM_
            dump_dijkstra_for_quantum(target);
#endif

            return {max, source_e};
        }
        try {
            dijkstra(starting_edge[target_e], target_all_visitor({target[source_e], target[target_e]}));
        } catch(DestinationFound) { found = true; }

    }
    //if we succeded in the first search, we must have found one of the other distances
    assert(distances[target[source_e]] != max && distances[target[target_e]] != max);

    return find_nearest_vertex(target);
}

Path PathFinder::build_path(vertex_t best_destination) const {
    std::vector<vertex_t> reverse_path;
    while (best_destination != predecessors[best_destination]){
        reverse_path.push_back(best_destination);
        best_destination = predecessors[best_destination];
    }
    reverse_path.push_back(best_destination);

    return create_path(geo_ref, reverse_path, true, speed_factor);
}

static edge_t get_best_edge(vertex_t u, vertex_t v, const GeoRef& georef) {
    const auto& g = georef.graph;
    boost::optional<edge_t> best_edge;
    for (auto range = out_edges(u, g); range.first != range.second; ++range.first) {
        if (target(*range.first, g) != v) { continue; }
        if (! best_edge || g[*range.first].duration < g[*best_edge].duration) {
            best_edge = *range.first;
        }
    }
    if (! best_edge) {
        throw navitia::exception("impossible to find an edge");
    }
    return *best_edge;
}

Path create_path(const GeoRef& geo_ref,
                 const std::vector<vertex_t>& reverse_path,
                 bool add_one_elt,
                 double speed_factor) {
    Path p;

    // On reparcourt tout dans le bon ordre
    nt::idx_t last_way = type::invalid_idx;
    boost::optional<PathItem::TransportCaracteristic> last_transport_carac{};
    PathItem path_item;
    path_item.coordinates.push_back(geo_ref.graph[reverse_path.back()].coord);

    for (size_t i = reverse_path.size(); i > 1; --i) {
        bool path_item_changed = false;
        vertex_t v = reverse_path[i-2];
        vertex_t u = reverse_path[i-1];
        edge_t e = get_best_edge(u, v, geo_ref);

        Edge edge = geo_ref.graph[e];
        PathItem::TransportCaracteristic transport_carac = geo_ref.get_caracteristic(e);
        if ((edge.way_idx != last_way && last_way != type::invalid_idx) || (last_transport_carac && transport_carac != *last_transport_carac)) {
            p.path_items.push_back(path_item);
            path_item = PathItem();
            path_item_changed = true;
        }

        nt::GeographicalCoord coord = geo_ref.graph[v].coord;
        path_item.coordinates.push_back(coord);
        last_way = edge.way_idx;
        last_transport_carac = transport_carac;
        path_item.way_idx = edge.way_idx;
        path_item.transportation = transport_carac;
        path_item.duration += edge.duration / speed_factor;
        p.duration += edge.duration / speed_factor;
        if (path_item_changed) {
            //we update the last path item
            path_item.angle = compute_directions(p, coord);
        }
    }
    //in some case we want to add even if we have only one vertex (which means there is no valid edge)
    size_t min_nb_elt_to_add = add_one_elt ? 1 : 2;
    if (reverse_path.size() >= min_nb_elt_to_add){
        p.path_items.push_back(path_item);
    }
    return p;
}


/**
  * Compute the angle between the last segment of the path and the next point
  *
  * A-------B
  *  \)
  *   \
  *    \
  *     C
  *
  *                       l(AB)² + l(AC)² - l(BC)²
  *the angle ABC = cos-1(_________________________)
  *                          2 * l(AB) * l(AC)
  *
  * with l(AB) = length OF AB
  *
  * The computed angle is 180 - the angle ABC, ie we compute the turn angle of the path
 */
int compute_directions(const navitia::georef::Path& path, const nt::GeographicalCoord& c_coord) {
    if (path.path_items.empty()) {
        return 0;
    }
    nt::GeographicalCoord b_coord, a_coord;
    const PathItem& last_item = path.path_items.back();
    a_coord = last_item.coordinates.back();
    if (last_item.coordinates.size() > 1) {
        b_coord = last_item.coordinates[last_item.coordinates.size() - 2];
    } else {
        if ( path.path_items.size() < 2 ) {
            return 0; //we don't have 2 previous coordinate, we can't compute an angle
        }
        const PathItem& previous_item = *(++(path.path_items.rbegin()));
        b_coord = previous_item.coordinates.back();
    }
    if (a_coord == b_coord || b_coord == c_coord || a_coord == c_coord) {
        return 0;
    }

    double len_ab = a_coord.distance_to(b_coord);
    double len_bc = b_coord.distance_to(c_coord);
    double len_ac = a_coord.distance_to(c_coord);

    double numerator = pow(len_ab, 2) + pow(len_ac, 2) - pow(len_bc, 2);
    double ab_lon = b_coord.lon() - a_coord.lon();
    double bc_lat = c_coord.lat() - b_coord.lat();
    double ab_lat = b_coord.lat() - a_coord.lat();
    double bc_lon = c_coord.lon() - b_coord.lon();

    double denominator =  2 * len_ab * len_ac;
    double raw_angle = acos(numerator / denominator);

    double det = ab_lon * bc_lat - ab_lat * bc_lon;

    //conversion into angle
    raw_angle *= 360 / (2 * boost::math::constants::pi<double>());

    int rounded_angle = std::round(raw_angle);

    rounded_angle = 180 - rounded_angle;
    if ( det < 0 )
        rounded_angle *= -1.0;

//    std::cout << "angle : " << rounded_angle << std::endl;

    return rounded_angle;
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

    printer_all_visitor(std::vector<vertex_t> destinations) :
        target_all_visitor(destinations) {
        init_files();
    }

    ~printer_all_visitor() {
        file_vertex.close();
        file_edge.close();
    }

    printer_all_visitor(const printer_all_visitor& o) : target_all_visitor(o) {
        init_files();
    }

    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type& g) {
        file_vertex << cpt_v++ << ";" << g[u].coord << ";" << u << std::endl;
        target_all_visitor::finish_vertex(u, g);
    }

    template <typename graph_type>
    void examine_edge(edge_t e, graph_type& g) {
        file_edge << cpt_e++ << ";" << g[boost::source(e, g)].coord << ";" << g[boost::target(e, g)].coord
                  << "; LINESTRING(" << g[boost::source(e, g)].coord.lon() << " " << g[boost::source(e, g)].coord.lat()
                  << ", " << g[boost::target(e, g)].coord.lon() << " " << g[boost::target(e, g)].coord.lat() << ")"
                     << ";" << e
                  << std::endl;
        target_all_visitor::examine_edge(e, g);
    }
};

void PathFinder::dump_dijkstra_for_quantum(const ProjectionData& target) {
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
    BOOST_FOREACH(edge_t e, boost::out_edges(target[source_e], geo_ref.graph)) {
        out_edge << "source;" << geo_ref.graph[boost::target(e, geo_ref.graph)].coord << std::endl;
    }
    BOOST_FOREACH(edge_t e, boost::out_edges(target[target_e], geo_ref.graph)) {
        out_edge << "target;" << geo_ref.graph[boost::target(e, geo_ref.graph)].coord << std::endl;
    }
    try {
        dijkstra(starting_edge[source_e], printer_all_visitor({target[source_e], target[target_e]}));
    } catch(DestinationFound) { }
}
#endif
}}
