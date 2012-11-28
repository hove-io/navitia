#include "builder.h"

namespace navitia { namespace georef {

GraphBuilder & GraphBuilder::add_vertex(std::string node_name, float x, float y){
    auto it = this->vertex_map.find(node_name);
    vertex_t v;
    type::GeographicalCoord coord;
    coord.set_xy(x,y);
    if(it  == this->vertex_map.end()){
        v = boost::add_vertex(this->geo_ref.graph);
        vertex_map[node_name] = v;
    } else {
        v = it->second;
    }

    this->geo_ref.graph[v].coord = coord;
    this->geo_ref.pl.add(coord, v);
    this->geo_ref.pl.build();
    return *this;
}

GraphBuilder & GraphBuilder::add_edge(std::string source_name, std::string target_name, float length, bool bidirectionnal){
    vertex_t source, target;
    auto it = this->vertex_map.find(source_name);
    if(it == this->vertex_map.end())
        this->add_vertex(source_name, 0, 0);
    source = this->vertex_map[source_name];

    it = this->vertex_map.find(target_name);
    if(it == this->vertex_map.end())
        this->add_vertex(target_name, 0, 0);
    target= this->vertex_map[target_name];

    Edge edge;
    edge.length = length >= 0? length : 0;

    boost::add_edge(source, target, edge, this->geo_ref.graph);
    if(bidirectionnal)
        boost::add_edge(target, source, edge, this->geo_ref.graph);

    return *this;
}

vertex_t GraphBuilder::get(const std::string &node_name){
    return this->vertex_map[node_name];
}

edge_t GraphBuilder::get(const std::string &source_name, const std::string &target_name){
    vertex_t source = this->get(source_name);
    vertex_t target = this->get(target_name);
    edge_t e;
    bool b;
    boost::tie(e,b) =  boost::edge(source, target, this->geo_ref.graph);
    if(!b) throw proximitylist::NotFound();
    else return e;
}

}}
