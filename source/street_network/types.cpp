#include "types.h"

#include <boost/foreach.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>

#include <fstream>
#include "fastlz_filter/filter.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "third_party/eos_portable_archive/portable_iarchive.hpp"
#include "third_party/eos_portable_archive/portable_oarchive.hpp"


#include <iostream>
#include <unordered_map>

namespace pt = boost::posix_time;
using navitia::type::City;
using navitia::type::idx_t;
namespace navitia{ namespace streetnetwork{



Path StreetNetwork::compute(std::vector<vertex_t> starts, std::vector<vertex_t> destinations) {
    vertex_t start = starts.front();
    Path p;
    // Tableau des prédécesseurs de chaque nœuds
    std::vector<vertex_t> preds(boost::num_vertices(this->graph));

    // Tableau des distances des nœuds à l'origine
    // si pred[v] == v, c'est soit qu'il n'y a pas de chemin possible, soit c'est l'origine
    std::vector<float> dists(boost::num_vertices(this->graph));

    std::cout << "On lance dijkstra" << std::endl;
    boost::dijkstra_shortest_paths(this->graph,
                                   start, // nœud de départ
                                   boost::predecessor_map(&preds[0])
                                   .distance_map(&dists[0])
                                   .weight_map(boost::get(&Edge::length, this->graph))
                                   );

    std::cout << "Dijkstra fini" << std::endl;
    // On cherche la destination la plus proche
    vertex_t best_destination;
    float best_distance = std::numeric_limits<float>::max();
    BOOST_FOREACH(vertex_t destination, destinations){
        if(dists[destination] < best_distance) {
            best_distance = dists[destination];
            best_destination = destination;
        }
    }

    std::cout << "On a trouvé le plus court chemin : " << best_distance << " mètres" << std::endl;
    // Si un chemin existe
    if(best_distance < std::numeric_limits<float>::max()){
        p.length = best_distance;
        // On reconstruit le chemin, on part de la destination pour remonter à l'arrivée
        // Il est donc à l'envers
        std::vector<vertex_t> reverse_path;
        while(best_destination != preds[best_destination]){
            reverse_path.push_back(best_destination);
            best_destination = preds[best_destination];
        }

        // On reparcourre tout dans le bon ordre
        nt::idx_t last_way =  std::numeric_limits<nt::idx_t>::max();
        PathItem path_item;
        for(size_t i = reverse_path.size(); i > 1; --i){
            vertex_t u = reverse_path[i-2];
            vertex_t v = reverse_path[i-1];
            Edge edge = graph[boost::edge(u, v, graph).first];
            p.coordinates.push_back(graph[v].coord);
            path_item.way_idx = edge.way_idx;


            path_item.length += edge.length;
            if(edge.way_idx != last_way){
                p.path_items.push_back(path_item);
                last_way = edge.way_idx;
                path_item = PathItem();
            }
        }
        if(reverse_path.size() > 0)
            p.path_items.push_back(path_item);
    }

    return p;
}

void StreetNetwork::build_proximity_list(){
    BOOST_FOREACH(vertex_t u, boost::vertices(this->graph)){
        pl.add(graph[u].coord, u);
    }
    pl.build();
}


void StreetNetwork::save(const std::string & filename) {
    std::ofstream ofs(filename.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa << *this;
}

void StreetNetwork::load(const std::string & filename) {
    std::ifstream ifs(filename.c_str());
    boost::archive::text_iarchive ia(ifs);
    ia >> *this;
}

void StreetNetwork::load_flz(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(FastLZDecompressor(2048*500),8192*500, 8192*500);
    in.push(ifs);
    eos::portable_iarchive ia(in);
    ia >> *this;
}

void StreetNetwork::save_flz(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(FastLZCompressor(2048*500), 1024*500, 1024*500);
    out.push(ofs);
    eos::portable_oarchive oa(out);
    oa << *this;
}

GraphBuilder & GraphBuilder::add_vertex(std::string node_name, float x, float y){
    auto it = this->vertex_map.find(node_name);
    vertex_t v;
    type::GeographicalCoord coord(x,y);
    if(it  == this->vertex_map.end()){
        v = boost::add_vertex(this->graph);
        vertex_map[node_name] = v;
    } else {
        v = it->second;
    }

    graph[v].coord.x = x;
    graph[v].coord.y = y;
    return *this;
}

GraphBuilder & GraphBuilder::add_edge(std::string source_name, std::string target_name, float length){
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

    boost::add_edge(source, target, edge, this->graph);

    return *this;
}

std::pair<type::GeographicalCoord, float> project(type::GeographicalCoord point, type::GeographicalCoord segment_start, type::GeographicalCoord segment_end){
    std::pair<type::GeographicalCoord, float> result;
    result.first.degrees = point.degrees;

    float length = segment_start.distance_to(segment_end);
    float u = ((point.x - segment_start.x)*(segment_end.x - segment_start.x)
            + (point.y - segment_start.y)*(segment_end.y - segment_start.y) )/
            (length * length);

    // Les deux cas où le projeté tombe en dehors
    if(u < 0)
        result = std::make_pair(segment_start, segment_start.distance_to(point));
    else if(u > 1)
        result = std::make_pair(segment_end, segment_end.distance_to(point));
    else {
        result.first.x = segment_start.x + u * (segment_end.x - segment_start.x);
        result.first.y = segment_start.y + u * (segment_end.y - segment_start.y);
        result.second = point.distance_to(result.first);
    }

    return result;
}


}}
