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
#include "dijkstra_shortest_paths_with_heap.h"

#include <boost/graph/filtered_graph.hpp>

namespace navitia {
namespace georef {

class DijkstraPathFinder : public PathFinder {
public:
    DijkstraPathFinder(const GeoRef& geo_ref) : PathFinder(geo_ref) {}
    DijkstraPathFinder(const DijkstraPathFinder& o) = default;
    virtual ~DijkstraPathFinder();

    void start_distance_dijkstra(const navitia::time_duration& radius);
    void start_distance_or_target_dijkstra(const navitia::time_duration& radius,
                                           const std::vector<vertex_t>& destinations);

    // compute the reachable stop points within the radius
    routing::map_stop_point_duration find_nearest_stop_points(const navitia::time_duration& radius,
                                                              const proximitylist::ProximityList<type::idx_t>& pl);

    using coord_uri = std::string;
    boost::container::flat_map<coord_uri, georef::RoutingElement> get_duration_with_dijkstra(
        const navitia::time_duration& radius,
        const std::vector<type::GeographicalCoord>& entry_points);

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
        navitia::dijkstra_shortest_paths_no_init_with_heap(
            filtered_graph(geo_ref.graph, {}, TransportationModeFilter(mode, geo_ref)), vertex.cbegin(), vertex.cend(),
            &predecessors[0], &distances[0], boost::get(&Edge::duration, geo_ref.graph),  // weigth map
            std::less<navitia::time_duration>(),
            SpeedDistanceCombiner(speed_factor),  // we multiply the edge duration by a speed factor
            navitia::seconds(0), visitor, color, &index_in_heap_map[0]);
    }

    // shouldn't be used outside of class apart from tests
    /** compute the path to the target and update the distances/pred
     *  return a pair with the edge corresponding to the target and the distance
     */
    std::pair<navitia::time_duration, ProjectionData::Direction> update_path(const ProjectionData& target);

    // compute the distance from the starting point to the target stop point
    navitia::time_duration get_distance(type::idx_t target_idx);

private:
    template <typename K, typename U, typename G>
    boost::container::flat_map<K, georef::RoutingElement> start_dijkstra_and_fill_duration_map(
        const navitia::time_duration& radius,
        const std::vector<U>& destinations,
        const G& projection_getter);

    // compute the reachable stop points within the radius with a simple crow fly
    std::vector<std::pair<type::idx_t, type::GeographicalCoord>> crow_fly_find_nearest_stop_points(
        const navitia::time_duration& radius,
        const proximitylist::ProximityList<type::idx_t>& pl);
};

}  // namespace georef
}  // namespace navitia
