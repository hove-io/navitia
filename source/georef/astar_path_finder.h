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

#include "path_finder.h"
#include "visitor.h"

#include <boost/graph/astar_search.hpp>
#include <boost/graph/filtered_graph.hpp>

namespace navitia {
namespace georef {

struct astar_distance_heuristic : public boost::astar_heuristic<Graph, navitia::seconds> {
    const Graph& g;
    const type::GeographicalCoord& dest_coord;
    const double inv_speed;

    astar_distance_heuristic(const Graph& graph, const vertex_t& destination, const double inv_speed)
        : g(graph), dest_coord(graph[destination].coord), inv_speed(inv_speed) {}

    navitia::seconds operator()(const vertex_t& v) const {
        auto const dist_to_target = dest_coord.distance_to(g[v].coord);
        return navitia::seconds(dist_to_target * inv_speed);
    }
};

class AstarPathFinder : public PathFinder {
public:
    // Distance array for the Astar
    std::vector<navitia::time_duration> costs;

    AstarPathFinder(const GeoRef& geo_ref) : PathFinder(geo_ref) {}

    void init(const type::GeographicalCoord& start_coord,
              const type::GeographicalCoord& dest_coord,
              nt::Mode_e mode,
              const float speed_factor) {
        PathFinder::init(start_coord, mode, speed_factor);

        // we initialize the costs to the maximum value
        size_t n = boost::num_vertices(geo_ref.graph);
        costs.assign(n, bt::pos_infin);

        auto const distance_to_dest = start_coord.distance_to(dest_coord);
        auto const duration_to_dest = navitia::seconds(distance_to_dest / double(default_speed[mode] * speed_factor));
        costs[starting_edge[source_e]] = duration_to_dest;
        costs[starting_edge[target_e]] = duration_to_dest;
    }

    void start_distance_or_target_astar(const navitia::time_duration& radius,
                                        const std::vector<vertex_t>& destinations);

    /**
     * Launch an astar without initializing the data structure
     * Warning, it modifies the distances and the predecessors
     **/
    void astar(const std::array<georef::vertex_t, 2>& origin_vertexes,
               const astar_distance_heuristic& heuristic,
               const astar_distance_or_target_visitor& visitor);

private:
    template <class Graph, class WeightMap, class Compare = std::less<navitia::time_duration>>
    void astar_shortest_paths_no_init_with_heap(const Graph& g,
                                                const vertex_t* s_begin,
                                                const vertex_t* s_end,
                                                const astar_distance_heuristic& h,
                                                const astar_distance_or_target_visitor& vis,
                                                const WeightMap& weight,
                                                const SpeedDistanceCombiner& combine,
                                                const Compare& compare = Compare());
};

}  // namespace georef
}  // namespace navitia
