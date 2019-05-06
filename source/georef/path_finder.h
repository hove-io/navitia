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
#include "routing/raptor_utils.h"

#include <boost/graph/two_bit_color_map.hpp>

namespace bt = boost::posix_time;

namespace navitia {
namespace georef {

// a bit of sugar
const auto source_e = ProjectionData::Direction::Source;
const auto target_e = ProjectionData::Direction::Target;

enum class RoutingStatus_e { reached = 0, unreached = 1, unknown = 2 };

struct RoutingElement {
    navitia::time_duration time_duration;
    RoutingStatus_e routing_status = RoutingStatus_e::reached;
    RoutingElement(navitia::time_duration time_duration = navitia::time_duration(),
                   RoutingStatus_e routing_status = RoutingStatus_e::reached)
        : time_duration(time_duration), routing_status(routing_status) {}
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
        return acceptable_modes[graph_number];
    }
};

struct SpeedDistanceCombiner
    : public std::binary_function<navitia::time_duration, navitia::time_duration, navitia::time_duration> {
    // speed factor compared to the default speed of the transportation mode
    // speed_factor = 2 means the speed is twice the default speed of the given transportation mode
    // inv_speed_factor = 1 / speed_factor to avoid division
    float inv_speed_factor;
    SpeedDistanceCombiner(float speed_) : inv_speed_factor(1.f / speed_) {}
    inline navitia::time_duration operator()(navitia::time_duration a, navitia::time_duration b) const {
        if (a == bt::pos_infin || b == bt::pos_infin)
            return bt::pos_infin;
        return a + b * inv_speed_factor;
    }
};

class PathFinder {
public:
    const GeoRef& geo_ref;

    bool computation_launch = false;

    // starting point
    type::GeographicalCoord start_coord;
    ProjectionData starting_edge;

    // Transportation mode
    nt::Mode_e mode;
    float speed_factor = 0.;

    // Distance map between entry point and stop point
    std::map<routing::SpIdx, navitia::time_duration> distance_to_entry_point;

    // Distance array for the Dijkstra
    std::vector<navitia::time_duration> distances;

    // Predecessors array for the Dijkstra
    std::vector<vertex_t> predecessors;

    // helper for dijkstra internal heap (to avoid extra alloc)
    std::vector<std::size_t> index_in_heap_map;

    // Color map for the dijkstra shortest path (to avoid extra alloc)
    boost::two_bit_color_map<> color;

    PathFinder(const GeoRef& geo_ref);

    // Virtual destructor, to allow use as a public base class,
    // but pure to ensure object itself isn't instantiated
    virtual ~PathFinder() = 0;

    /**
     *  Update the structure for a given starting point and transportation mode
     *  The init HAS to be called before any other methods
     */
    void init(const type::GeographicalCoord& start_coord, nt::Mode_e mode, const float speed_factor);

    // return the path from the starting point to the target. the target has to have been previously visited.
    Path get_path(type::idx_t idx);

    // Add the starting point projection the the path. Add a new way if needed
    void add_projections_to_path(Path& p, bool append_to_begin) const;

    // shouldn't be used outside of class apart from tests
    Path get_path(const ProjectionData& target,
                  const std::pair<navitia::time_duration, ProjectionData::Direction>& nearest_edge);

    // find the nearest vertex from the projection. return the distance to this vertex and the vertex
    std::pair<navitia::time_duration, ProjectionData::Direction> find_nearest_vertex(const ProjectionData& target,
                                                                                     bool handle_on_node = false) const;

    // return the duration between two projection on the same edge
    navitia::time_duration path_duration_on_same_edge(const ProjectionData& p1, const ProjectionData& p2);

    // return the real geometry between two projection on the same edge
    type::LineString path_coordinates_on_same_edge(const Edge& e, const ProjectionData& p1, const ProjectionData& p2);

protected:
    // return the time the travel the distance at the current speed (used for projections)
    navitia::time_duration crow_fly_duration(const double val) const;

    void add_custom_projections_to_path(Path& p,
                                        bool append_to_begin,
                                        const ProjectionData& projection,
                                        ProjectionData::Direction d) const;

    // Build a path with a destination and the predecessors list
    Path build_path(vertex_t best_destination) const;
};

bool is_projected_on_same_edge(const ProjectionData& p1, const ProjectionData& p2);

Path create_path(const GeoRef& geo_ref,
                 const std::vector<vertex_t>& reverse_path,
                 bool add_one_elt,
                 float speed_factor);

}  // namespace georef
}  // namespace navitia
