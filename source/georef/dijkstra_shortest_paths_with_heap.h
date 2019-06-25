/* Copyright © 2001-2017, Canal TP and/or its affiliates. All rights reserved.

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

#include <boost/graph/dijkstra_shortest_paths.hpp>

namespace boost {
// Call breadth first search
// Allow to pass color map so that user deals with the allocation (and white init)
template <class Graph,
          class SourceInputIter,
          class DijkstraVisitor,
          class PredecessorMap,
          class DistanceMap,
          class WeightMap,
          class Compare,
          class Combine,
          class DistZero,
          class ColorMap,
          class IndexInHeapMap>
inline void dijkstra_shortest_paths_no_init_with_heap(const Graph& g,
                                                      SourceInputIter s_begin,
                                                      SourceInputIter s_end,
                                                      PredecessorMap predecessor,
                                                      DistanceMap distance,
                                                      WeightMap weight,
                                                      Compare compare,
                                                      Combine combine,
                                                      DistZero zero,
                                                      DijkstraVisitor vis,
                                                      ColorMap color,
                                                      IndexInHeapMap index_in_heap) {
    typedef indirect_cmp<DistanceMap, Compare> IndirectCmp;
    IndirectCmp icmp(distance, compare);

    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
    typedef d_ary_heap_indirect<Vertex, 4, IndexInHeapMap, DistanceMap, Compare> MutableQueue;
    MutableQueue Q(distance, index_in_heap, compare);

    detail::dijkstra_bfs_visitor<DijkstraVisitor, MutableQueue, WeightMap, PredecessorMap, DistanceMap, Combine,
                                 Compare>
        bfs_vis(vis, Q, weight, predecessor, distance, combine, compare, zero);

    breadth_first_visit(g, s_begin, s_end, Q, bfs_vis, color);
}

}  // namespace boost
