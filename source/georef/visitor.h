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

#include "georef.h"
#include "type/time_duration.h"

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/astar_search.hpp>
#include <utility>

namespace navitia {
namespace georef {

// Exception thrown when a destination is found in the djisktra
struct DestinationFound {};
struct DestinationNotFound {};

// Visitor who stops (throw a DestinationFound exception) when a certain distance is reached
template <class Base>
struct distance_visitor : virtual public Base {
    navitia::time_duration max_duration;
    const std::vector<navitia::time_duration>& durations;

    distance_visitor(time_duration max_dur, const std::vector<time_duration>& dur)
        : max_duration(std::move(max_dur)), durations(dur) {}
    distance_visitor(const distance_visitor& other) = default;

    /*
     * stop when we can't find any vertex such that distances[v] <= max_duration
     */
    template <typename G>
    void examine_vertex(typename boost::graph_traits<G>::vertex_descriptor u, const G&) {
        if (durations[u] > max_duration)
            throw DestinationFound();
    }
};

// Visitor who stops (throw a DestinationFound exception) when all targets has been visited
template <class Base>
struct target_all_visitor : virtual public Base {
    std::vector<vertex_t> destinations;
    size_t nbFound = 0;

    target_all_visitor(const std::vector<vertex_t>& destinations)
        : destinations(destinations.begin(), destinations.end()) {}

    target_all_visitor(const target_all_visitor& other) = default;

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
template <class Base>
struct target_unique_visitor : public Base {
    const vertex_t& destination;

    target_unique_visitor(const vertex_t& destination) : destination(destination) {}

    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type&) {
        if (u == destination)
            throw DestinationFound();
    }
};

// Visitor who stops when a target has been visited or a certain distance is reached
template <class Base>
struct distance_or_target_visitor : virtual public distance_visitor<Base>, virtual public target_all_visitor<Base> {
    distance_or_target_visitor(const time_duration& max_dur,
                               const std::vector<time_duration>& dur,
                               const std::vector<vertex_t>& destinations)
        : distance_visitor<Base>(max_dur, dur), target_all_visitor<Base>(destinations) {}
    distance_or_target_visitor(const distance_or_target_visitor& other) = default;
    template <typename graph_type>
    void finish_vertex(vertex_t u, const graph_type& g) {
        target_all_visitor<Base>::finish_vertex(u, g);
    }

    template <typename G>
    void examine_vertex(typename boost::graph_traits<G>::vertex_descriptor u, const G& g) {
        distance_visitor<Base>::examine_vertex(u, g);
    }
};

using dijkstra_distance_visitor = distance_visitor<boost::dijkstra_visitor<>>;
using dijkstra_target_all_visitor = target_all_visitor<boost::dijkstra_visitor<>>;

using astar_distance_or_target_visitor = distance_or_target_visitor<boost::astar_visitor<>>;

}  // namespace georef
}  // namespace navitia
