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
#include "type/time_duration.h"
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/two_bit_color_map.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

namespace bt = boost::posix_time;

namespace navitia { namespace georef {

struct SpeedDistanceCombiner : public std::binary_function<navitia::time_duration, navitia::time_duration, navitia::time_duration> {
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
        res[modes_pair.first] = {{{}}}; //force false initialization of all members
        for (auto mode : modes_pair.second) {
            res[modes_pair.first][mode] = true;
        }
    }
    return res;
}

const auto allowed_transportation_mode = create_from_allowedlist({{{
                                                                {type::Mode_e::Walking}, //for walking, only walking is allowed
                                                                {type::Mode_e::Bike}, //for biking, only bike
                                                                {type::Mode_e::Car}, //for car, only car is allowed (for the moment, to handle parking we could allow walking)
                                                                {type::Mode_e::Walking, type::Mode_e::Bike} //for vls, walking and bike is allowed
                                                          }}});

struct TransportationModeFilter {
    flat_enum_map<type::Mode_e, bool> acceptable_modes; //map associating a boolean to a mode,
    type::idx_t nb_vertex_by_mode;
    TransportationModeFilter() = default;
    TransportationModeFilter(type::Mode_e mode, const georef::GeoRef& geo_ref) :
        acceptable_modes(allowed_transportation_mode[mode]),
        nb_vertex_by_mode(geo_ref.nb_vertex_by_mode) //the second elt in the offset array is the number of edge by mode
    {
        BOOST_ASSERT_MSG(nb_vertex_by_mode != 0, "there should be vertexes in the graph");
    }

    template <typename vertex_t>
    bool operator()(const vertex_t& e) const {
        int graph_number = e / nb_vertex_by_mode;

//        std::cout << "for node " << e << " in graph " << graph_number << " we can " << (acceptable_modes[graph_number] ? " " : "not ") << "go" << std::endl;
        return acceptable_modes[graph_number];
    }
};

struct PathFinder {
    const GeoRef & geo_ref;

    bool computation_launch = false;

    /// starting point
    type::GeographicalCoord start_coord;
    ProjectionData starting_edge;

    /// Transportation mode
    nt::Mode_e mode;
    float speed_factor = 0.;

    /// Distance array for the Dijkstra
    std::vector<navitia::time_duration> distances;

    /// Predecessors array for the Dijkstra
    std::vector<vertex_t> predecessors;

    PathFinder(const GeoRef& geo_ref);

    /**
     *  Update the structure for a given starting point and transportation mode
     *  The init HAS to be called before any other methods
     */
    void init(const type::GeographicalCoord& start_coord, nt::Mode_e mode, const float speed_factor);

    void start_distance_dijkstra(navitia::time_duration radius);

    /// compute the reachable stop points within the radius
    std::vector<std::pair<type::idx_t, navitia::time_duration>> find_nearest_stop_points(navitia::time_duration radius,
                                                                         const proximitylist::ProximityList<type::idx_t>& pl);

    /// Compute the path from the starting point to the the target geographical coord
    Path compute_path(const type::GeographicalCoord& target_coord);

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
    template<class Visitor>
    void dijkstra(vertex_t start, Visitor visitor) {
        // Note: the predecessors have been updated in init
        boost::two_bit_color_map<> color(boost::num_vertices(geo_ref.graph));

        //we filter the graph to only use certain mean of transport
        using filtered_graph = boost::filtered_graph<georef::Graph, boost::keep_all, TransportationModeFilter>;
        boost::dijkstra_shortest_paths_no_init(filtered_graph(geo_ref.graph, {}, TransportationModeFilter(mode, geo_ref)),
                                               start, &predecessors[0], &distances[0],
                                               boost::get(&Edge::duration, geo_ref.graph), // weigth map
                                               boost::identity_property_map(),
                                               std::less<navitia::time_duration>(),
                                               SpeedDistanceCombiner(speed_factor), //we multiply the edge duration by a speed factor
                                               navitia::seconds(0),
                                               visitor,
                                               color
                                               );
    }
private:
    Path get_path(const ProjectionData& target, std::pair<navitia::time_duration, ProjectionData::Direction> nearest_edge);

    /** compute the path to the target and update the distances/pred
     *  return a pair with the edge corresponding to the target and the distance
     */
    std::pair<navitia::time_duration, ProjectionData::Direction> update_path(const ProjectionData& target);

    /// find the nearest vertex from the projection. return the distance to this vertex and the vertex
    std::pair<navitia::time_duration, ProjectionData::Direction> find_nearest_vertex(const ProjectionData& target) const;

    ///return the time the travel the distance at the current speed (used for projections)
    navitia::time_duration crow_fly_duration(const double val) const;

    void add_custom_projections_to_path(Path& p, bool append_to_begin, const ProjectionData& projection, ProjectionData::Direction d) const;

    /// Build a path with a destination and the predecessors list
    Path build_path(vertex_t best_destination) const;

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
    std::vector<std::pair<type::idx_t, navitia::time_duration>> find_nearest_stop_points(
                                                    navitia::time_duration radius,
                                                    const proximitylist::ProximityList<type::idx_t>& pl,
                                                    bool use_second);

    navitia::time_duration get_distance(type::idx_t target_idx, bool use_second = false);

    Path get_path(type::idx_t idx, bool use_second = false);

    /**
     * Build the direct path between the start and the end by connecting the 2 sub path (from departure and from arrival).
     * If the 2 sub path does not connect return an empty path
     **/
    Path get_direct_path(const type::EntryPoint& origin, const type::EntryPoint& destination);

    const GeoRef & geo_ref;
    PathFinder departure_path_finder;
    PathFinder arrival_path_finder;

private:
    /// Combine 2 pathes
    Path combine_path(const vertex_t best_destination, std::vector<vertex_t> preds, std::vector<vertex_t> successors) const;
};

/// Build a path from a reverse path list
Path create_path(const GeoRef& georef, std::vector<vertex_t> reverse_path, bool add_one_elt);

/// Compute the angle between the last segment of the path and the next point
int compute_directions(const navitia::georef::Path& path, const nt::GeographicalCoord& c_coord);

// Exception levée dès que l'on trouve une destination
struct DestinationFound{};
struct DestinationNotFound{};

// Visiteur qui lève une exception dès qu'une des cibles souhaitées est atteinte
struct target_visitor : public boost::dijkstra_visitor<> {
    const std::vector<vertex_t> & destinations;
    target_visitor(const std::vector<vertex_t> & destinations) : destinations(destinations){}
    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type&){
        if(std::find(destinations.begin(), destinations.end(), u) != destinations.end())
            throw DestinationFound();
    }
};

// Visiteur qui s'arrête au bout d'une certaine distance
struct distance_visitor : public boost::dijkstra_visitor<> {
    navitia::time_duration max_duration;
    const std::vector<navitia::time_duration>& durations;
    distance_visitor(navitia::time_duration max_dur, const std::vector<navitia::time_duration> & dur) :
        max_duration(max_dur), durations(dur){}

    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type&){
        if(durations[u] > max_duration)
            throw DestinationFound();
    }
};

// Visitor who throw a DestinationFound exception when all target has been visited
struct target_all_visitor : public boost::dijkstra_visitor<> {
    std::vector<vertex_t> destinations;
    size_t nbFound = 0;
    target_all_visitor(std::vector<vertex_t> destinations) : destinations(destinations){}
    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type&){
        if (std::find(destinations.begin(), destinations.end(), u) != destinations.end()) {
            nbFound++;
            if (nbFound == destinations.size()) {
                throw DestinationFound();
            }
        }
    }
};

// Visiteur qui lève une exception dès que la cible souhaitée est atteinte
struct target_unique_visitor : public boost::dijkstra_visitor<> {
    const vertex_t & destination;

    target_unique_visitor(const vertex_t & destination) :
        destination(destination){}

    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type&){
        if(u == destination)
            throw DestinationFound();
    }
};


}}//namespace navitia::georef
