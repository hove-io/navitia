/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
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

    astar_distance_heuristic(const Graph& graph, const type::GeographicalCoord& dest_projected, const double inv_speed)
        : g(graph), dest_coord(dest_projected), inv_speed(inv_speed) {}

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
    AstarPathFinder(const AstarPathFinder& o) = default;
    ~AstarPathFinder() override;

    void init(const type::GeographicalCoord& start_coord,
              const type::GeographicalCoord& dest_projected_coord,
              nt::Mode_e mode,
              const float speed_factor);

    void start_distance_or_target_astar(const navitia::time_duration& radius,
                                        const type::GeographicalCoord& dest_projected,
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
                                                const vertex_t& s_begin,
                                                const vertex_t& s_end,
                                                const astar_distance_heuristic& h,
                                                const astar_distance_or_target_visitor& vis,
                                                const WeightMap& weight,
                                                const SpeedDistanceCombiner& combine,
                                                const Compare& compare = Compare());

    navitia::time_duration compute_cost_from_starting_edge_to_dist(const vertex_t& v,
                                                                   const type::GeographicalCoord& dest_coord) const;
};

}  // namespace georef
}  // namespace navitia
