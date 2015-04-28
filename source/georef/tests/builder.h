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

#include "georef/georef.h"

namespace navitia { namespace georef {
/** Permet de construire un graphe de manière simple

    C'est essentiellement destiné aux tests unitaires
  */
struct GraphBuilder{
    GeoRef & geo_ref;

    std::map<std::string, vertex_t> vertex_map;

    GraphBuilder(GeoRef & geo_ref) : geo_ref(geo_ref){}

    /// Add or update a vertex
    GraphBuilder & add_vertex(std::string node_name, float x, float y);

    /// add an edge. create the vertex if it does not exists.
    /// if no duration provided a default one is taken (crow fly)
    GraphBuilder & add_edge(std::string source_name, std::string target_name, navitia::time_duration dur = {}, bool bidirectionnal = false);

    /// Node creation
    GraphBuilder & operator()(std::string node_name, float x, float y){ return add_vertex(node_name, x, y);}


    /// Edge creation
    GraphBuilder & operator()(std::string source_name, std::string target_name, navitia::time_duration dur = {}, bool bidirectionnal = false){
        return add_edge(source_name, target_name, dur, bidirectionnal);
    }

    /// edge creation for different mode.
    /// The vertex has to be created before and the init method called
    typedef std::pair<std::string, nt::Mode_e> edge_and_mode;
    GraphBuilder & operator()(edge_and_mode source, edge_and_mode target, navitia::time_duration dur = {}, bool bidirectionnal = false);

    vertex_t get(const std::string & node_name);

    edge_t get(const std::string & source_name, const std::string & target_name);
};

}}
