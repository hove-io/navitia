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

#pragma once
#include "georef.h"
#include "dijkstra_shortest_paths_with_heap.h"
#include "routing/raptor_utils.h"
#include "type/time_duration.h"
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/two_bit_color_map.hpp>
#include <boost/format.hpp>

namespace bt = boost::posix_time;

namespace navitia {
namespace georef {

struct SpeedDistanceCombiner
    : public std::binary_function<navitia::time_duration, navitia::time_duration, navitia::time_duration> {
    /// speed factor compared to the default speed of the transportation mode
    /// speed_factor = 2 means the speed is twice the default speed of the given transportation mode
    float speed_factor;
    SpeedDistanceCombiner(float speed_) : speed_factor(speed_) {}
    inline navitia::time_duration operator()(navitia::time_duration a, navitia::time_duration b) const {
        if (a == bt::pos_infin || b == bt::pos_infin)
            return bt::pos_infin;
        return a + b / speed_factor;
    }
};
template <typename T>
using map_by_mode = flat_enum_map<type::Mode_e, T>;
/**
 * create a map of map of boolean from a map of map of allowed mode
 * (because it's simpler to define with only the allowed mode, but  more efficient with the boolean masks)
 */
inline map_by_mode<map_by_mode<bool>> create_from_allowedlist(map_by_mode<std::vector<nt::Mode_e>> allowed_modes) {
    map_by_mode<map_by_mode<bool>> res;
    for (auto modes_pair : allowed_modes) {
        res[modes_pair.first] = {{{}}};  // force false initialization of all members
        for (auto mode : modes_pair.second) {
            res[modes_pair.first][mode] = true;
        }
    }
    return res;
}

const auto allowed_transportation_mode = create_from_allowedlist({{{
    {type::Mode_e::Walking},                      // for walking, only walking is allowed
    {type::Mode_e::Bike},                         // for biking, only bike
    {type::Mode_e::Car, type::Mode_e::Walking},   // for car, only car and walking is allowed
    {type::Mode_e::Walking, type::Mode_e::Bike},  // for vls, walking and bike is allowed
    {type::Mode_e::Car}                           // for CarNoPark, only Car is allowed
}}});

struct TransportationModeFilter {
    flat_enum_map<type::Mode_e, bool> acceptable_modes;  // map associating a boolean to a mode,
    type::idx_t nb_vertex_by_mode;
    TransportationModeFilter() = default;
    TransportationModeFilter(type::Mode_e mode, const georef::GeoRef& geo_ref)
        : acceptable_modes(allowed_transportation_mode[mode]),
          nb_vertex_by_mode(
              geo_ref.nb_vertex_by_mode)  // the second elt in the offset array is the number of edge by mode
    {
        BOOST_ASSERT_MSG(nb_vertex_by_mode != 0, "there should be vertexes in the graph");
    }

    template <typename vertex_t>
    bool operator()(const vertex_t& e) const {
        int graph_number = e / nb_vertex_by_mode;

        //        std::cout << "for node " << e << " in graph " << graph_number << " we can " <<
        //        (acceptable_modes[graph_number] ? " " : "not ") << "go" << std::endl;
        return acceptable_modes[graph_number];
    }
};

enum class RoutingStatus_e { reached = 0, unreached = 1, unknown = 2 };

struct RoutingElement {
    navitia::time_duration time_duration;
    RoutingStatus_e routing_status = RoutingStatus_e::reached;
    RoutingElement(navitia::time_duration time_duration = navitia::time_duration(),
                   RoutingStatus_e routing_status = RoutingStatus_e::reached)
        : time_duration(time_duration), routing_status(routing_status) {}
};

struct PathFinder {
    const GeoRef& geo_ref;

    bool computation_launch = false;

    /// starting point
    type::GeographicalCoord start_coord;
    ProjectionData starting_edge;

    /// Transportation mode
    nt::Mode_e mode;
    float speed_factor = 0.;

    /// Distance map between entry point and stop point
    std::map<routing::SpIdx, navitia::time_duration> distance_to_entry_point;

    /// Distance array for the Dijkstra
    std::vector<navitia::time_duration> distances;

    /// Predecessors array for the Dijkstra
    std::vector<vertex_t> predecessors;

    /// helper for dijkstra internal heap (to avoid extra alloc)
    std::vector<std::size_t> index_in_heap_map;

    /// Color map for the dijkstra shortest path (to avoid extra alloc)
    boost::two_bit_color_map<> color;

    PathFinder(const GeoRef& geo_ref);

    /**
     *  Update the structure for a given starting point and transportation mode
     *  The init HAS to be called before any other methods
     */
    void init(const type::GeographicalCoord& start_coord, nt::Mode_e mode, const float speed_factor);

    void start_distance_dijkstra(const navitia::time_duration& radius);
    void start_distance_or_target_dijkstra(const navitia::time_duration& radius,
                                           const std::vector<vertex_t>& destinations);

    /// compute the reachable stop points within the radius
    routing::map_stop_point_duration find_nearest_stop_points(const navitia::time_duration& radius,
                                                              const proximitylist::ProximityList<type::idx_t>& pl);
    using coord_uri = std::string;
    boost::container::flat_map<coord_uri, georef::RoutingElement> get_duration_with_dijkstra(
        const navitia::time_duration& radius,
        const std::vector<type::GeographicalCoord>& entry_points);

    /// compute the distance from the starting point to the target stop point
    navitia::time_duration get_distance(type::idx_t target_idx);

    /// return the path from the starting point to the target. the target has to have been previously visited.
    Path get_path(type::idx_t idx);

    /// Add the starting point projection the the path. Add a new way if needed
    void add_projections_to_path(Path& p, bool append_to_begin) const;

    /**
     * Launch a dijkstra without initializing the data structure
     * Warning, it modifies the distances and the predecessors
     **/
    template <class Visitor>
    void dijkstra(const vertex_t source, const vertex_t target, const Visitor& visitor) {
        // Note: the predecessors have been updated in init

        std::array<georef::vertex_t, 2> vertex{{source, target}};

        // Fill color map in white before dijkstra
        std::fill(color.data.get(),
                  color.data.get() + (color.n + color.elements_per_char - 1) / color.elements_per_char, 0);

        // we filter the graph to only use certain mean of transport
        using filtered_graph = boost::filtered_graph<georef::Graph, boost::keep_all, TransportationModeFilter>;
        boost::dijkstra_shortest_paths_no_init_with_heap(
            filtered_graph(geo_ref.graph, {}, TransportationModeFilter(mode, geo_ref)), vertex.cbegin(), vertex.cend(),
            &predecessors[0], &distances[0], boost::get(&Edge::duration, geo_ref.graph),  // weigth map
            std::less<navitia::time_duration>(),
            SpeedDistanceCombiner(speed_factor),  // we multiply the edge duration by a speed factor
            navitia::seconds(0), visitor, color, &index_in_heap_map[0]);
    }

    // shouldn't be used outside of class apart from tests
    Path get_path(const ProjectionData& target,
                  const std::pair<navitia::time_duration, ProjectionData::Direction>& nearest_edge);

    // shouldn't be used outside of class apart from tests
    /** compute the path to the target and update the distances/pred
     *  return a pair with the edge corresponding to the target and the distance
     */
    std::pair<navitia::time_duration, ProjectionData::Direction> update_path(const ProjectionData& target);

    /// find the nearest vertex from the projection. return the distance to this vertex and the vertex
    std::pair<navitia::time_duration, ProjectionData::Direction> find_nearest_vertex(const ProjectionData& target,
                                                                                     bool handle_on_node = false) const;

    // return the duration between two projection on the same edge
    navitia::time_duration path_duration_on_same_edge(const ProjectionData& p1, const ProjectionData& p2);

    // return the real geometry between two projection on the same edge
    type::LineString path_coordinates_on_same_edge(const Edge& e, const ProjectionData& p1, const ProjectionData& p2);

private:
    /// return the time the travel the distance at the current speed (used for projections)
    navitia::time_duration crow_fly_duration(const double val) const;

    void add_custom_projections_to_path(Path& p,
                                        bool append_to_begin,
                                        const ProjectionData& projection,
                                        ProjectionData::Direction d) const;

    /// Build a path with a destination and the predecessors list
    Path build_path(vertex_t best_destination) const;

    /// compute the reachable stop points within the radius with a simple crow fly
    std::vector<std::pair<type::idx_t, type::GeographicalCoord>> crow_fly_find_nearest_stop_points(
        const navitia::time_duration& radius,
        const proximitylist::ProximityList<type::idx_t>& pl);

    template <typename K, typename U, typename G>
    boost::container::flat_map<K, georef::RoutingElement> start_dijkstra_and_fill_duration_map(
        const navitia::time_duration& radius,
        const std::vector<U>& destinations,
        const G& projection_getter);

#ifdef _DEBUG_DIJKSTRA_QUANTUM_
    void dump_dijkstra_for_quantum(const ProjectionData& target);
#endif
};

/** Structure managing the computation on the streetnetwork */
struct StreetNetwork {
    StreetNetwork(const GeoRef& geo_ref);

    void init(const type::EntryPoint& start_coord, boost::optional<const type::EntryPoint&> end_coord = {});

    bool departure_launched() const;
    bool arrival_launched() const;

    routing::map_stop_point_duration find_nearest_stop_points(const navitia::time_duration& radius,
                                                              const proximitylist::ProximityList<type::idx_t>& pl,
                                                              bool use_second);

    navitia::time_duration get_distance(type::idx_t target_idx, bool use_second = false);

    Path get_path(type::idx_t idx, bool use_second = false);

    /**
     * Build the direct path between the start and the end
     **/
    Path get_direct_path(const type::EntryPoint& origin, const type::EntryPoint& destination);

    const GeoRef& geo_ref;
    PathFinder departure_path_finder;
    PathFinder arrival_path_finder;
    PathFinder direct_path_finder;
};

/// Build a path from a reverse path list
Path create_path(const GeoRef& georef, const std::vector<vertex_t>& reverse_path, bool add_one_elt, float speed_factor);

/// Compute the angle between the last segment of the path and the next point
int compute_directions(const navitia::georef::Path& path, const nt::GeographicalCoord& c_coord);

// Exception thrown when a destination is found in the djisktra
struct DestinationFound {};
struct DestinationNotFound {};

// Visitor who stops (throw a DestinationFound exception) when one of the targets have been reached
struct target_visitor : public boost::dijkstra_visitor<> {
    const std::vector<vertex_t>& destinations;
    target_visitor(const std::vector<vertex_t>& destinations) : destinations(destinations) {}
    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type&) {
        if (std::find(destinations.begin(), destinations.end(), u) != destinations.end())
            throw DestinationFound();
    }
};

// Visitor who stops (throw a DestinationFound exception) when a certain distance is reached
struct distance_visitor : virtual public boost::dijkstra_visitor<> {
    navitia::time_duration max_duration;
    const std::vector<navitia::time_duration>& durations;

    distance_visitor(const time_duration& max_dur, const std::vector<time_duration>& dur)
        : max_duration(max_dur), durations(dur) {}
    distance_visitor(const distance_visitor& other) = default;
    virtual ~distance_visitor();

    /*
     * stop when we can't find any vertex such that distances[v] <= max_duration
     */
    template <typename G>
    void examine_vertex(typename boost::graph_traits<G>::vertex_descriptor u, const G&) {
        if (durations[u] > max_duration)
            throw DestinationFound();
    }
};

#ifdef _DEBUG_DIJKSTRA_QUANTUM_

struct printer_distance_visitor : public distance_visitor {
    std::ofstream file_vertex, file_edge;
    size_t cpt_v = 0, cpt_e = 0;
    std::string name;

    void init_files() {
        file_vertex.open((boost::format("vertexes_%s.csv") % name).str());
        file_vertex << std::setprecision(16) << "idx; lat; lon; vertex_id" << std::endl;
        file_edge.open((boost::format("edges_%s.csv") % name).str());
        file_edge << std::setprecision(16) << "idx; lat from; lon from; lat to; long to; wkt; duration; edge"
                  << std::endl;
    }

    printer_distance_visitor(time_duration max_dur, const std::vector<time_duration>& dur, const std::string& name)
        : distance_visitor(max_dur, dur), name(name) {
        init_files();
    }

    ~printer_distance_visitor() {
        file_vertex.close();
        file_edge.close();
    }

    printer_distance_visitor(const printer_distance_visitor& o) : distance_visitor(o), name(o.name) { init_files(); }

    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type& g) {
        file_vertex << cpt_v++ << ";" << g[u].coord << ";" << u << std::endl;
        distance_visitor::finish_vertex(u, g);
    }

    template <typename graph_type>
    void examine_edge(edge_t e, graph_type& g) {
        distance_visitor::examine_edge(e, g);
        file_edge << cpt_e++ << ";" << g[boost::source(e, g)].coord << ";" << g[boost::target(e, g)].coord
                  << ";LINESTRING(" << g[boost::source(e, g)].coord.lon() << " " << g[boost::source(e, g)].coord.lat()
                  << ", " << g[boost::target(e, g)].coord.lon() << " " << g[boost::target(e, g)].coord.lat() << ")"
                  << ";" << this->durations[boost::source(e, g)].total_seconds() << ";" << e << std::endl;
    }
};
#endif

// Visitor who stops (throw a DestinationFound exception) when all targets has been visited
struct target_all_visitor : virtual public boost::dijkstra_visitor<> {
    std::vector<vertex_t> destinations;
    size_t nbFound = 0;
    target_all_visitor(const std::vector<vertex_t>& destinations)
        : destinations(destinations.begin(), destinations.end()) {}
    target_all_visitor(const target_all_visitor& other) = default;
    virtual ~target_all_visitor();
    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type&) {
        if (std::find(destinations.begin(), destinations.end(), u) != destinations.end()) {
            nbFound++;
            if (nbFound == destinations.size()) {
                throw DestinationFound();
            }
        }
    }
};

// Visitor who stops (throw a DestinationFound exception) when a target has been visited
struct target_unique_visitor : public boost::dijkstra_visitor<> {
    const vertex_t& destination;

    target_unique_visitor(const vertex_t& destination) : destination(destination) {}

    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type&) {
        if (u == destination)
            throw DestinationFound();
    }
};

// Visitor who stops when a target has been visited or a certain distance is reached
struct distance_or_target_visitor : virtual public distance_visitor, virtual public target_all_visitor {
    distance_or_target_visitor(const time_duration& max_dur,
                               const std::vector<time_duration>& dur,
                               const std::vector<vertex_t>& destinations)
        : distance_visitor(max_dur, dur), target_all_visitor(destinations) {}
    distance_or_target_visitor(const distance_or_target_visitor& other) = default;
    virtual ~distance_or_target_visitor();
    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type& g) {
        target_all_visitor::finish_vertex(u, g);
    }

    template <typename G>
    void examine_vertex(typename boost::graph_traits<G>::vertex_descriptor u, const G& g) {
        distance_visitor::examine_vertex(u, g);
    }
};

#ifdef _DEBUG_DIJKSTRA_QUANTUM_
struct printer_distance_or_target_visitor : virtual public printer_distance_visitor, virtual public target_all_visitor {
    printer_distance_or_target_visitor(const time_duration& max_dur,
                                       const std::vector<time_duration>& dur,
                                       const std::vector<vertex_t>& destinations,
                                       const std::string& name)
        : printer_distance_visitor(max_dur, dur, name), target_all_visitor(destinations) {}
    printer_distance_or_target_visitor(const printer_distance_or_target_visitor& other) = default;
    virtual ~printer_distance_or_target_visitor();
    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type& g) {
        printer_distance_visitor::finish_vertex(u, g);
        target_all_visitor::finish_vertex(u, g);
    }

    template <typename G>
    void examine_vertex(typename boost::graph_traits<G>::vertex_descriptor u, const G& g) {
        printer_distance_visitor::examine_vertex(u, g);
    }
};
#endif

}  // namespace georef
}  // namespace navitia
