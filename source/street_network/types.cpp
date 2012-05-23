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

#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <iostream>
#include <unordered_map>

namespace pt = boost::posix_time;
using navitia::type::City;
using navitia::type::idx_t;
namespace navitia{ namespace streetnetwork{

// Exception levée dès que l'on trouve une destination
struct DestinationFound{};

// Visiteur qui lève une exception dès qu'une des cibles souhaitées est atteinte
struct my_visitor : public boost::dijkstra_visitor<> {
    const std::vector<vertex_t> & destinations;
    my_visitor(const std::vector<vertex_t> & destinations) : destinations(destinations){}
    void finish_vertex(vertex_t u, Graph){
        if(std::find(destinations.begin(), destinations.end(), u) != destinations.end())
            throw DestinationFound();
    }
};

Path StreetNetwork::compute(std::vector<vertex_t> starts, std::vector<vertex_t> destinations, std::vector<float> zeros) {
    if(starts.size() == 0 || destinations.size() == 0)
        throw NotFound();

    if(zeros.size() != starts.size())
        zeros = std::vector<float>(starts.size(), 0);

    size_t n = boost::num_vertices(this->graph);

    // Tableau des prédécesseurs de chaque nœuds
    // si pred[v] == v, c'est soit qu'il n'y a pas de chemin possible, soit c'est l'origine
    std::vector<vertex_t> preds(n);
    for(size_t i = 0; i<preds.size(); ++i)
        preds[i] = i;

    // Tableau des distances des nœuds à l'origine, par défaut à l'infini
    std::vector<float> dists(n, std::numeric_limits<float>::max());

    for(size_t i = 0; i < starts.size(); ++i){
        boost::two_bit_color_map<> color(n);
        vertex_t start = starts[i];
        dists[start] = zeros[i];    

        // On effectue un Dijkstra sans ré-initialiser les tableaux de distances et de prédécesseur
        try {
            boost::dijkstra_shortest_paths_no_init(this->graph, start, &preds[0], &dists[0],
                                                   boost::get(&Edge::length, this->graph), // weigth map
                                                   boost::identity_property_map(),
                                                   //boost::get(boost::vertex_index, this->graph),
                                                   std::less<float>(), boost::closed_plus<float>(),
                                                   0,
                                                   my_visitor(destinations),
                                                   color
                                                   );
        } catch (DestinationFound) {}
    }

    // On cherche la destination la plus proche
    vertex_t best_destination;
    float best_distance = std::numeric_limits<float>::max();
    BOOST_FOREACH(vertex_t destination, destinations){
        if(dists[destination] < best_distance) {
            best_distance = dists[destination];
            best_destination = destination;
        }
    }

    // Si un chemin existe
    Path p;
    if(best_distance < std::numeric_limits<float>::max()){
        p.length = best_distance;
        // On reconstruit le chemin, on part de la destination pour remonter à l'arrivée
        // Il est donc à l'envers
        std::vector<vertex_t> reverse_path;
        while(best_destination != preds[best_destination]){
            reverse_path.push_back(best_destination);
            best_destination = preds[best_destination];
        }
        reverse_path.push_back(best_destination);

        // On reparcourre tout dans le bon ordre
        nt::idx_t last_way =  type::invalid_idx;
        PathItem path_item;
        p.coordinates.push_back(graph[reverse_path.back()].coord);
        for(size_t i = reverse_path.size(); i > 1; --i){
            vertex_t v = reverse_path[i-2];
            vertex_t u = reverse_path[i-1];
            p.coordinates.push_back(graph[v].coord);

            edge_t e = boost::edge(u, v, graph).first;
            Edge edge = graph[e];
            if(edge.way_idx != last_way && last_way != type::invalid_idx){
                p.path_items.push_back(path_item);
                path_item = PathItem();
            }
            last_way = edge.way_idx;
            path_item.way_idx = edge.way_idx;
            path_item.segments.push_back(e);
            path_item.length += edge.length;
        }
        if(reverse_path.size() > 1)
            p.path_items.push_back(path_item);
    } else {
        throw NotFound();
    }

    return p;
}

Path StreetNetwork::compute(const type::GeographicalCoord & start_coord, const type::GeographicalCoord & end_coord){
    edge_t start = this->nearest_edge(start_coord);
    edge_t dest = this->nearest_edge(end_coord);

    std::vector<vertex_t> starts = {boost::source(start, this->graph), boost::target(start, this->graph)};
    std::vector<vertex_t> dests = {boost::source(dest, this->graph), boost::source(dest, this->graph)};
    return compute(starts, dests);
}

void StreetNetwork::build_proximity_list(){
    BOOST_FOREACH(vertex_t u, boost::vertices(this->graph)){
        pl.add(graph[u].coord, u);
    }
    pl.build();
}

edge_t StreetNetwork::nearest_edge(const type::GeographicalCoord & coordinates) const {
    vertex_t u = pl.find_nearest(coordinates);
    type::GeographicalCoord coord_u, coord_v;
    coord_u = this->graph[u].coord;
    float dist = std::numeric_limits<float>::max();
    edge_t best;
    bool found = false;
    BOOST_FOREACH(edge_t e, boost::out_edges(u, this->graph)){
        vertex_t v = boost::target(e, this->graph);
        coord_v = this->graph[v].coord;
        // Petite approximation de la projection : on ne suit pas le tracé de la voirie !
        auto projected = project(coordinates, coord_u, coord_v);
        if(projected.second < dist){
            found = true;
            dist = projected.second;
            best = e;
        }
    }
    if(!found)
        throw NotFound();
    else
        return best;
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
    type::GeographicalCoord coord(x,y,false);
    if(it  == this->vertex_map.end()){
        v = boost::add_vertex(this->street_network.graph);
        vertex_map[node_name] = v;
    } else {
        v = it->second;
    }

    this->street_network.graph[v].coord = coord;
    this->street_network.pl.add(coord, v);
    this->street_network.pl.build();
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

    boost::add_edge(source, target, edge, this->street_network.graph);

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
    boost::tie(e,b) =  boost::edge(source, target, this->street_network.graph);
    if(!b) throw NotFound();
    else return e;
}

std::pair<type::GeographicalCoord, float> project(type::GeographicalCoord point, type::GeographicalCoord segment_start, type::GeographicalCoord segment_end){
    std::pair<type::GeographicalCoord, float> result;
    result.first.degrees = point.degrees;

    float length = segment_start.distance_to(segment_end);
    float u;

    // On gère le cas où le segment est particulièrement court, et donc ça peut poser des problèmes (à cause de la division par length²)
    if(length < 1){ // moins d'un mètre, on projette sur une extrémité
        if(point.distance_to(segment_start) < point.distance_to(segment_end))
            u = 0;
        else
            u = 1;
    } else {
        u = ((point.x - segment_start.x)*(segment_end.x - segment_start.x)
             + (point.y - segment_start.y)*(segment_end.y - segment_start.y) )/
                (length * length);
    }

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
