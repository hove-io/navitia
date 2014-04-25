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

#include "data_cleaner.h"
#include "ed/data.h"
#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>

namespace ed { namespace connectors {

void data_cleaner::clean() {
    //we merge the ways
    LOG4CPLUS_INFO(logger, "begin merge way");
    fusion_ways();
    LOG4CPLUS_INFO(logger, "End : merge way " << this->ways_fusionned << "/" << this->data.ways.size() << " fusionned.");
}

// We group the way with the same name and the same admin
void data_cleaner::fusion_ways() {
    typedef std::unordered_map<std::string, std::vector<types::Edge*>> wayname_ways;
    std::unordered_map<std::string, wayname_ways> admin_wayname_way;
    for(auto way : this->data.ways) {
        if(way.second->admin == nullptr || way.second->edges.empty() || way.second->name == "") {
            continue;
        }
        auto admin_it = admin_wayname_way.find(way.second->admin->insee);
        if(admin_it == admin_wayname_way.end()) {
            admin_wayname_way[way.second->admin->insee] = wayname_ways();
            admin_it = admin_wayname_way.find(way.second->admin->insee);
        }
        auto way_it = admin_it->second.find(way.second->name);
        if(way_it == admin_it->second.end()) {
            admin_wayname_way[way.second->admin->insee][way.second->name] = std::vector<types::Edge*>();
            way_it = admin_it->second.find(way.second->name);
        }
        auto &way_vector = admin_wayname_way[way.second->admin->insee][way.second->name];

        way_vector.insert(way_vector.begin(), way.second->edges.begin(),  way.second->edges.end());
    }
    for(auto wayname_ways_it : admin_wayname_way){
        for(auto edges_it : wayname_ways_it.second){
            this->fusion_ways_list(edges_it.second);
        }
    }
}

//we affect the first way for every edges of a component
void data_cleaner::fusion_ways_list(const std::vector<ed::types::Edge*>& edges) {
    auto begin = edges.begin();
    ed::types::Way* way_ref = (*begin)->way;
    this->data.fusion_ways[way_ref->uri] = way_ref;
    ++begin;
    for(auto it=begin; it != edges.end(); ++it) {
        ed::types::Way* way = (*it)->way;
        way->is_used = false;
        this->data.fusion_ways[way->uri] = way_ref;
        (*it)->way = way_ref;
        this->ways_fusionned++;
    }
}


//We make a graph out of each edge of every edges
//We look for connected components in this graph
void data_cleaner::fusion_ways_by_graph(const std::vector<types::Edge*>& edges) {
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, size_t, ed::types::Edge*> Graph;
    Graph graph;
    std::unordered_map<uint64_t, idx_t> node_map_temp;
    // Graph building
    for(ed::types::Edge* edge : edges){
        if(node_map_temp.find(edge->source->id) == node_map_temp.end()){
            node_map_temp[edge->source->id] = boost::add_vertex(graph);
        }
        if(node_map_temp.find(edge->target->id) == node_map_temp.end()){
            node_map_temp[edge->target->id] = boost::add_vertex(graph);
        }
        boost::add_edge(node_map_temp[edge->source->id], node_map_temp[edge->target->id], edge, graph);
        boost::add_edge(node_map_temp[edge->target->id], node_map_temp[edge->source->id], edge, graph);
    }
    std::vector<size_t> vertex_component(boost::num_vertices(graph));
    boost::connected_components(graph, &vertex_component[0]);
    std::map<size_t, std::vector<ed::types::Edge*>> component_edges;
    for(size_t i = 0; i<vertex_component.size(); ++i) {
        auto component = vertex_component[i];
        if(component_edges.find(component) != component_edges.end()) {
            component_edges[component] = std::vector<ed::types::Edge*>();
        }
        auto begin_end = boost::out_edges(i, graph);
        for(auto it = begin_end.first; it != begin_end.second; ++it) {
            component_edges[component].push_back(graph[*it]);
        }
    }
    for(auto component_edges_ : component_edges) {
        this->fusion_ways_list(component_edges_.second);
    }
}

}
}
