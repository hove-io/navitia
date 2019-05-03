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
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/astar_search.hpp>

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

    distance_visitor(const time_duration& max_dur, const std::vector<time_duration>& dur)
        : max_duration(max_dur), durations(dur) {}
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

#ifdef _DEBUG_DIJKSTRA_QUANTUM_
struct printer_distance_or_target_visitor : virtual public printer_distance_visitor, virtual public target_all_visitor {
    printer_distance_or_target_visitor(const time_duration& max_dur,
                                       const std::vector<time_duration>& dur,
                                       const std::vector<vertex_t>& destinations,
                                       const std::string& name)
        : printer_distance_visitor(max_dur, dur, name), target_all_visitor(destinations) {}
    printer_distance_or_target_visitor(const printer_distance_or_target_visitor& other) = default;
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

using dijkstra_distance_visitor = distance_visitor<boost::dijkstra_visitor<>>;
using dijkstra_target_all_visitor = target_all_visitor<boost::dijkstra_visitor<>>;
using dijkstra_distance_or_target_visitor = distance_or_target_visitor<boost::dijkstra_visitor<>>;

using astar_distance_visitor = distance_visitor<boost::astar_visitor<>>;
using astar_target_all_visitor = target_all_visitor<boost::astar_visitor<>>;
using astar_distance_or_target_visitor = distance_or_target_visitor<boost::astar_visitor<>>;

}  // namespace georef
}  // namespace navitia
