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
    /// Graphe que l'on veut construire
    //StreetNetwork & street_network;
    GeoRef & geo_ref;

    /// Associe une chaine de caractères à un nœud
    std::map<std::string, vertex_t> vertex_map;

    /// Le constructeur : on précise sur quel graphe on va construire
    //GraphBuilder(StreetNetwork & street_network) : street_network(street_network){}
    GraphBuilder(GeoRef & geo_ref) : geo_ref(geo_ref){}

    /// Ajoute un nœud, s'il existe déjà, les informations sont mises à jour
    GraphBuilder & add_vertex(std::string node_name, float x, float y);

    /// Ajoute un arc. Si un nœud n'existe pas, il est créé automatiquement
    /// Si la longueur n'est pas précisée, il s'agit de la longueur à vol d'oiseau
    GraphBuilder & add_edge(std::string source_name, std::string target_name, boost::posix_time::time_duration dur = {}, bool bidirectionnal = false);

    /// Surchage de la création de nœud pour plus de confort
    GraphBuilder & operator()(std::string node_name, float x, float y){ return add_vertex(node_name, x, y);}


    /// Surchage de la création d'arc pour plus de confort
    GraphBuilder & operator()(std::string source_name, std::string target_name, boost::posix_time::time_duration dur = {}, bool bidirectionnal = false){
        return add_edge(source_name, target_name, dur, bidirectionnal);
    }


    /// Retourne le nœud demandé, jette une exception si on ne trouve pas
    vertex_t get(const std::string & node_name);

    /// Retourne l'arc demandé, jette une exception si on ne trouve pas
    edge_t get(const std::string & source_name, const std::string & target_name);
};

}}
