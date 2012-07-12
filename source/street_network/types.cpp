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

// Exception levée dès que l'on trouve une destination
struct DestinationFound{};

// Visiteur qui lève une exception dès qu'une des cibles souhaitées est atteinte
struct target_visitor : public boost::dijkstra_visitor<> {
    const std::vector<vertex_t> & destinations;
    target_visitor(const std::vector<vertex_t> & destinations) : destinations(destinations){}
    void finish_vertex(vertex_t u, Graph){
        if(std::find(destinations.begin(), destinations.end(), u) != destinations.end())
            throw DestinationFound();
    }
};

// Visiteur qui s'arrête au bout d'une certaine distance
struct distance_visitor : public boost::dijkstra_visitor<> {
    double max_distance;
    const std::vector<float> & distances;
    distance_visitor(float max_distance, const std::vector<float> & distances) : max_distance(max_distance), distances(distances){}
    void finish_vertex(vertex_t u, Graph){
        if(distances[u] > max_distance)
            throw DestinationFound();
    }
};


void StreetNetwork::init(std::vector<float> &distances, std::vector<vertex_t> &predecessors) const{
    size_t n = boost::num_vertices(this->graph);
    distances.assign(n, std::numeric_limits<float>::max());
    predecessors.resize(n);
    for(size_t i = 0; i<n; ++i)
        predecessors[i] = i;
}

Path StreetNetwork::compute(std::vector<vertex_t> starts, std::vector<vertex_t> destinations, std::vector<double> start_zeros, std::vector<double> dest_zeros) {
    if(starts.size() == 0 || destinations.size() == 0)
        throw NotFound();

    if(start_zeros.size() != starts.size())
        start_zeros.assign(starts.size(), 0);

    if(dest_zeros.size() != destinations.size())
        dest_zeros.assign(destinations.size(), 0);

    // Tableau des prédécesseurs de chaque nœuds
    // si pred[v] == v, c'est soit qu'il n'y a pas de chemin possible, soit c'est l'origine
    std::vector<vertex_t> preds;

    // Tableau des distances des nœuds à l'origine, par défaut à l'infini
    std::vector<float> dists;

    this->init(dists, preds);

    for(size_t i = 0; i < starts.size(); ++i){
        vertex_t start = starts[i];
        dists[start] = start_zeros[i];
        // On effectue un Dijkstra sans ré-initialiser les tableaux de distances et de prédécesseur
        try {
            this->dijkstra(start, dists, preds, target_visitor(destinations));
        } catch (DestinationFound) {}
    }

    // On cherche la destination la plus proche
    vertex_t best_destination;
    float best_distance = std::numeric_limits<float>::max();
    for(size_t i = 0; i < destinations.size(); ++i){
        vertex_t destination = destinations[i];
        dists[i] += dest_zeros[i];
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

        // On reparcourt tout dans le bon ordre
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

ProjectionData::ProjectionData(const type::GeographicalCoord & coord, const StreetNetwork & sn){
    this->edge = sn.nearest_edge(coord);
    // On cherche les coordonnées des extrémités de ce segment
    vertex_t vertex1 = boost::source(edge, sn.graph);
    vertex_t vertex2 = boost::target(edge, sn.graph);
    type::GeographicalCoord vertex1_coord = sn.graph[vertex1].coord;
    type::GeographicalCoord vertex2_coord = sn.graph[vertex2].coord;
    // On projette le nœud sur le segment
    this->projected = project(coord, vertex1_coord, vertex2_coord).first;
    // On calcule la distance « initiale » déjà parcourue avant d'atteindre ces extrémité d'où on effectue le calcul d'itinéraire
    this->vertices = {vertex1, vertex2};
    this->distances = {projected.distance_to(vertex1_coord), projected.distance_to(vertex2_coord)};
}

Path StreetNetwork::compute(const type::GeographicalCoord & start_coord, const type::GeographicalCoord & dest_coord){
    ProjectionData start(start_coord, *this);
    ProjectionData dest(dest_coord, *this);

    Path p = compute(start.vertices, dest.vertices, start.distances, dest.distances);

    // On rajoute les bouts de coordonnées manquants à partir et vers le projeté de respectivement le départ et l'arrivée
    std::vector<type::GeographicalCoord> coords = {start.projected};
    coords.resize(p.coordinates.size() + 2);
    std::copy(p.coordinates.begin(), p.coordinates.end(), coords.begin() + 1);
    coords.back() = dest.projected;
    p.coordinates = coords;

    return p;
}

std::vector< std::pair<idx_t, double> > StreetNetwork::find_nearest(const type::GeographicalCoord & start_coord, const ProximityList<idx_t> & pl, double radius){
    ProjectionData start(start_coord, *this);
    // Tableau des prédécesseurs de chaque nœuds
    // si pred[v] == v, c'est soit qu'il n'y a pas de chemin possible, soit c'est l'origine
    std::vector<vertex_t> preds;

    // Tableau des distances des nœuds à l'origine, par défaut à l'infini
    std::vector<float> dists;
    this->init(dists, preds);

    // On lance un dijkstra depuis les deux nœuds de départ
    dists[start.vertices[0]] = start.distances[0];
    try{
        this->dijkstra(start.vertices[0], dists, preds, distance_visitor(radius, dists));
    }catch(DestinationFound){}

    dists[start.vertices[1]] = start.distances[1];
    try{
        this->dijkstra(start.vertices[0], dists, preds, distance_visitor(radius, dists));
    }catch(DestinationFound){}

    std::vector< std::pair<idx_t, double> > result;
    // On trouve tous les élements à moins radius mètres en vol d'oiseau
    std::vector< std::pair<idx_t, type::GeographicalCoord> > elements = pl.find_within(start_coord, radius);

    // À chaque fois on regarde la distance réelle en suivant le filaire de voirie
    BOOST_FOREACH(auto element, elements){
        ProjectionData current(element.second, *this);
        size_t best = dists[current.vertices[0]] < dists[current.vertices[1]] ? 0 : 1;

        if(dists[current.vertices[best]] < radius){
            result.push_back(std::make_pair(element.first, dists[current.vertices[best]] + current.distances[best]));
        }
    }
    return result;
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
