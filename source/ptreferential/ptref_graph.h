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

#include "type/type.h"
#include <boost/graph/adjacency_list.hpp>
#include "utils/flat_enum_map.h"

namespace navitia { namespace ptref {

struct Edge {
    float weight;
    Edge() : weight(1){}
    Edge(float w) : weight(w){}
};

struct Jointures {
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, type::Type_e, Edge> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

    navitia::flat_enum_map<type::Type_e, vertex_t> vertex_map;
    Graph g;
    Jointures();
};

/// Trouve le chemin d'un type de données à un autre
/// Par exemple StopArea → StopPoint → JourneyPatternPoint
std::map<type::Type_e, type::Type_e> find_path(type::Type_e source);

}}
