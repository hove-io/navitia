#include "osm2nav.h"
#include <stdio.h>

#include <iostream>
#include <boost/lexical_cast.hpp>
#include "type/data.h"
#include "third_party/osmpbfreader/osmpbfreader.h"
#include "osm_tags_reader.h"
#include "georef/georef.h"
#include "utils/functions.h"

namespace navitia { namespace georef {

struct OSMHouseNumber{
    type::GeographicalCoord coord;
    int number;

    OSMHouseNumber(): number(-1){}

};

struct Node {
public:
    double lon() const {return static_cast<double>(this->lon_m) / precision;}
    double lat() const {return static_cast<double>(this->lat_m) / precision;}
    uint32_t uses;
    int32_t idx;

    Node(double lon = 0, double lat = 0) : uses(0), idx(-1), lon_m(lon * precision), lat_m(lat * precision){}
    bool increment_use(int idx){
        uses++;
        if(this->idx == -1 && uses > 1){
            this->idx = idx;
            return true;
        } else {
            return false;
        }
    }

private:
    int32_t lon_m;
    int32_t lat_m;
    static constexpr double precision = 10e6;
};

struct OSMWay {
    const static uint8_t CYCLE_FWD = 0;
    const static uint8_t CYCLE_BWD = 1;
    const static uint8_t CAR_FWD = 2;
    const static uint8_t CAR_BWD = 3;
    const static uint8_t FOOT_FWD = 4;
    const static uint8_t FOOT_BWD = 5;
    std::vector<uint64_t> refs;
    uint64_t id;
    std::bitset<8> properties;
    type::idx_t idx;
};

using namespace CanalTP;

struct Visitor{
    std::unordered_map<uint64_t, Node> nodes;
    std::unordered_map<uint64_t, OSMHouseNumber> housenumbers;
    int total_ways;
    int total_house_number;

    std::vector<OSMWay> ways;
    georef::GeoRef & geo_ref;

    Visitor(GeoRef & to_fill) : total_ways(0), total_house_number(0), geo_ref(to_fill){}

    void add_osm_housenumber(uint64_t osmid, const Tags & tags){
        if(tags.find("addr:housenumber") != tags.end()){
            OSMHouseNumber osm_hn;
            osm_hn.number = str_to_int(tags.at("addr:housenumber"));
            if (osm_hn.number > 0 ){
                this->housenumbers[osmid] = osm_hn;
            }
        }
    }

    void node_callback(uint64_t osmid, double lon, double lat, const Tags & tags){
        this->nodes[osmid] = Node(lon, lat);
        add_osm_housenumber(osmid, tags);
    }

    void way_callback(uint64_t osmid, const Tags &tags, const std::vector<uint64_t> &refs){
        total_ways++;
        std::bitset<8> properties = parse_way_tags(tags);
        if(properties.any()){
            OSMWay w;
            w.idx = ways.size();
            w.refs = refs;
            w.id = osmid;
            ways.push_back(w);

            georef::Way gr_way;
            gr_way.idx = w.idx;
            gr_way.uri = std::to_string(w.idx);
            gr_way.city_idx = type::invalid_idx;
            if(tags.find("name") != tags.end())
                gr_way.name = tags.at("name");             
            geo_ref.ways.push_back(gr_way);
        }else{
             add_osm_housenumber(refs.front(), tags);
        }
    }

    // Once all the ways and nodes are read, we count how many times a node is used to detect intersections
    void count_nodes_uses() {
        int count = 0;        
        for(auto w : ways){
            for(uint64_t ref : w.refs){
                if(nodes[ref].increment_use(count)){
                    Vertex v;
                    count++;
                    v.coord = type::GeographicalCoord( nodes[ref].lon(), nodes[ref].lat());
                    boost::add_vertex(v, geo_ref.graph);
                }
            }
            // make sure that the last node is considered as an extremity
            if(nodes[w.refs.front()].increment_use(count)){
                count++;
                Vertex v;
                v.coord = type::GeographicalCoord(nodes[w.refs.front()].lon(), nodes[w.refs.front()].lat());
                boost::add_vertex(v, geo_ref.graph);
            }
            if(nodes[w.refs.back()].increment_use(count)){
                count++;
                Vertex v;
                v.coord = type::GeographicalCoord(nodes[w.refs.back()].lon(), nodes[w.refs.back()].lat());
                boost::add_vertex(v, geo_ref.graph);
            }
        }
        std::cout << "On a : " << boost::num_vertices(geo_ref.graph) << " nœuds" << std::endl;
    }

    // Returns the source and target node of the edges
    void edges(){
        for(OSMWay w : ways){
            if(w.refs.size() > 0){
                Node n = nodes[w.refs[0]];
                type::idx_t source = n.idx;
                type::GeographicalCoord prev(n.lon(), n.lat());
                float length = 0;
                for(size_t i = 1; i < w.refs.size(); ++i){
                    Node current_node = nodes[w.refs[i]];
                    type::GeographicalCoord current(current_node.lon(), current_node.lat());
                    length += current.distance_to(prev);
                    prev = current;
                    // If a node is used more than once, it is an intersection, hence it's a node of the street network graph
                    if(current_node.uses > 1){
                        type::idx_t target = current_node.idx;
                        georef::Edge e;
                        e.length = length;
                        e.way_idx = w.idx;

                        e.cyclable = w.properties[CYCLE_FWD];
                        boost::add_edge(source, target, e, geo_ref.graph);
                        geo_ref.ways[w.idx].edges.push_back(std::make_pair(source, target));

                        e.cyclable = w.properties[CYCLE_BWD];
                        boost::add_edge(target, source, e, geo_ref.graph);
                        geo_ref.ways[w.idx].edges.push_back(std::make_pair(target, source));
                        source = target;
                        length = 0;
                    }
                }
            }
        }
        std::cout << "On a " << boost::num_edges(geo_ref.graph) << " arcs" << std::endl;
    }

    void HouseNumbers(){        
        type::idx_t idx;
        georef::HouseNumber gr_hn;
        geo_ref.build_proximity_list();
        for(auto hn : housenumbers){
            try{
                Node n = nodes[hn.first];
                gr_hn.number = hn.second.number;
                gr_hn.coord.set_lon(n.lon());
                gr_hn.coord.set_lat(n.lat());
                idx = geo_ref.graph[geo_ref.nearest_edge(gr_hn.coord)].way_idx;
                geo_ref.ways[idx].add_house_number(gr_hn);
                total_house_number ++;
            } catch(navitia::proximitylist::NotFound) {
                std::cout << "Attention, l'adresse n'est pas importée dont le numéro et les coordonnées sont : [" << gr_hn.number<<";"<< gr_hn.coord.lon()<< ";"<< gr_hn.coord.lat()<< "] Impossible de trouver le segment le plus proche.  " << std::endl;
            }catch(...){
                std::cout << "Attention, l'adresse n'est pas importée dont le numéro et les coordonnées sont : [" << gr_hn.number<<";"<< gr_hn.coord.lon()<< ";"<< gr_hn.coord.lat()<< "].  " << std::endl;
            }
        }
    }
    // We don't care about relations
    void relation_callback(uint64_t /*osmid*/, const Tags &/*tags*/, const References & /*refs*/){}
};

void fill_from_osm(GeoRef & geo_ref_to_fill, const std::string & osm_pbf_filename){
    Visitor v(geo_ref_to_fill);
    CanalTP::read_osm_pbf(osm_pbf_filename, v);    
    std::cout << v.nodes.size() << " nodes, " << v.ways.size() << " ways/" << v.total_ways << std::endl;
    v.count_nodes_uses();
    v.edges();
    v.HouseNumbers();
    std::cout << "On a " << v.total_house_number << " adresses" << std::endl;
}
}}



