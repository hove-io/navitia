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

#include "astar_path_finder.h"
#include "visitor.h"

#include <boost/graph/astar_search.hpp>
#include <boost/graph/filtered_graph.hpp>

namespace navitia {
namespace georef {

void AstarPathFinder::start_distance_or_target_astar(const navitia::time_duration& radius,
                                                     const std::vector<vertex_t>& destinations) {
    if (!starting_edge.found)
        return;
    computation_launch = true;
    // We start astar from source and target nodes
    try {
        astar({starting_edge[source_e], starting_edge[target_e]},
              astar_distance_heuristic(geo_ref.graph, destinations.back(), 1. / (default_speed[mode] * speed_factor)),
              astar_distance_or_target_visitor(radius, distances, destinations));
    } catch (DestinationFound&) {
    }
}

/**
 * Launch an astar without initializing the data structure
 * Warning, it modifies the distances and the predecessors
 **/
void AstarPathFinder::astar(const std::array<georef::vertex_t, 2>& origin_vertexes,
                            const astar_distance_heuristic& heuristic,
                            const astar_distance_or_target_visitor& visitor) {
    // Note: the predecessors have been updated in init

    // Fill color map in white before A*
    std::fill(color.data.get(), color.data.get() + (color.n + color.elements_per_char - 1) / color.elements_per_char,
              0);

    // we filter the graph to only use certain mean of transport
    using filtered_graph = boost::filtered_graph<georef::Graph, boost::keep_all, TransportationModeFilter>;
    auto g = filtered_graph(geo_ref.graph, {}, TransportationModeFilter(mode, geo_ref));
    auto weight_map = boost::get(&Edge::duration, geo_ref.graph);
    auto combiner = SpeedDistanceCombiner(speed_factor);

    astar_shortest_paths_no_init_with_heap(g, origin_vertexes.cbegin(), origin_vertexes.cend(), heuristic, visitor,
                                           weight_map, combiner);
}

template <class Graph, class WeightMap, class Compare>
void AstarPathFinder::astar_shortest_paths_no_init_with_heap(const Graph& g,
                                                             const vertex_t* s_begin,
                                                             const vertex_t* s_end,
                                                             const astar_distance_heuristic& h,
                                                             const astar_distance_or_target_visitor& vis,
                                                             const WeightMap& weight,
                                                             const SpeedDistanceCombiner& combine,
                                                             const Compare& compare) {
    typedef boost::d_ary_heap_indirect<vertex_t, 4, vertex_t*, navitia::time_duration*, Compare> MutableQueue;
    MutableQueue Q(&costs[0], &index_in_heap_map[0], compare);

    boost::detail::astar_bfs_visitor<astar_distance_heuristic, astar_distance_or_target_visitor, MutableQueue,
                                     vertex_t*, navitia::time_duration*, navitia::time_duration*, WeightMap,
                                     boost::two_bit_color_map<>, SpeedDistanceCombiner, Compare>
        bfs_vis(h, vis, Q, &predecessors[0], &costs[0], &distances[0], weight, color, combine, compare,
                navitia::seconds(0));

    breadth_first_visit(g, s_begin, s_end, Q, bfs_vis, color);
}

}  // namespace georef
}  // namespace navitia
