#include "osm2nav.h"
#include <stdio.h>

#include <iostream>
#include <boost/lexical_cast.hpp>
#include "type/data.h"
#include "third_party/osmpbfreader/osmpbfreader.h"
#include "osm_tags_reader.h"
#include "georef/georef.h"
#include "georef/adminref.h"
#include "utils/functions.h"
//#include "utils/csv.h"

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
    std::bitset<8> properties;
    type::idx_t idx;
};

using namespace CanalTP;

struct OSMAdminRef{
    std::string level;
    std::string insee;
    std::string name;
    std::string postcode;
    type::GeographicalCoord coord;
    References refs;
};



struct Visitor{
    std::unordered_map<uint64_t, Node> nodes;
    std::unordered_map<uint64_t, OSMHouseNumber> housenumbers;
    std::unordered_map<uint64_t, OSMAdminRef> OSMAdminRefs;
    int total_ways;
    int total_house_number;

    std::unordered_map<uint64_t, OSMWay> ways;
    georef::GeoRef & geo_ref;
//    georef::AdminRef & admin_ref;
    //Pour charger les données administratives
    navitia::georef::Levels levellist;
    std::map<std::string,std::string>::iterator iter;

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

        OSMWay w;
        w.idx = ways.size();
        w.refs = refs;
        ways[osmid] = w;

        if(properties.any()){
            georef::Way gr_way;
            gr_way.idx = w.idx;
            gr_way.uri = std::to_string(w.idx);
            gr_way.city_idx = type::invalid_idx;
            if(tags.find("name") != tags.end())
                gr_way.name = tags.at("name");
            geo_ref.ways.push_back(gr_way);
        }else{
            // Dans le cas où la maison est dessinée par un way et non pas juste par un node
             add_osm_housenumber(refs.front(), tags);
        }
    }

    // Once all the ways and nodes are read, we count how many times a node is used to detect intersections
    void count_nodes_uses() {
        int count = 0;        
        for(auto w : ways){
            for(uint64_t ref : w.second.refs){
                if(nodes[ref].increment_use(count)){
                    Vertex v;
                    count++;
                    v.coord = type::GeographicalCoord( nodes[ref].lon(), nodes[ref].lat());
                    boost::add_vertex(v, geo_ref.graph);
                }
            }
            // make sure that the last node is considered as an extremity
            if(nodes[w.second.refs.front()].increment_use(count)){
                count++;
                Vertex v;
                v.coord = type::GeographicalCoord(nodes[w.second.refs.front()].lon(), nodes[w.second.refs.front()].lat());
                boost::add_vertex(v, geo_ref.graph);
            }
            if(nodes[w.second.refs.back()].increment_use(count)){
                count++;
                Vertex v;
                v.coord = type::GeographicalCoord(nodes[w.second.refs.back()].lon(), nodes[w.second.refs.back()].lat());
                boost::add_vertex(v, geo_ref.graph);
            }
        }
        std::cout << "On a : " << boost::num_vertices(geo_ref.graph) << " nœuds" << std::endl;
    }

    // Returns the source and target node of the edges
    void edges(){
        for(auto w : ways){
            if(w.second.refs.size() > 0){
                Node n = nodes[w.second.refs[0]];
                type::idx_t source = n.idx;
                type::GeographicalCoord prev(n.lon(), n.lat());
                float length = 0;
                for(size_t i = 1; i < w.second.refs.size(); ++i){
                    Node current_node = nodes[w.second.refs[i]];
                    type::GeographicalCoord current(current_node.lon(), current_node.lat());
                    length += current.distance_to(prev);
                    prev = current;
                    // If a node is used more than once, it is an intersection, hence it's a node of the street network graph
                    if(current_node.uses > 1){
                        type::idx_t target = current_node.idx;
                        georef::Edge e;
                        e.length = length;
                        e.way_idx = w.second.idx;
                        e.cyclable = w.second.properties[CYCLE_FWD];
                        boost::add_edge(source, target, e, geo_ref.graph);
                        geo_ref.ways[e.way_idx].edges.push_back(std::make_pair(source, target));

                        e.cyclable = w.second.properties[CYCLE_BWD];
                        boost::add_edge(target, source, e, geo_ref.graph);
                        geo_ref.ways[e.way_idx].edges.push_back(std::make_pair(target, source));
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
                if (idx <= geo_ref.ways.size()){
                    geo_ref.ways[idx].add_house_number(gr_hn);
                    total_house_number ++;
                } // Message ?
            } catch(navitia::proximitylist::NotFound) {
                std::cout << "Attention, l'adresse n'est pas importée dont le numéro et les coordonnées sont : [" << gr_hn.number<<";"<< gr_hn.coord.lon()<< ";"<< gr_hn.coord.lat()<< "] Impossible de trouver le segment le plus proche.  " << std::endl;
            }catch(...){
                std::cout << "Attention, l'adresse n'est pas importée dont le numéro et les coordonnées sont : [" << gr_hn.number<<";"<< gr_hn.coord.lon()<< ";"<< gr_hn.coord.lat()<< "].  " << std::endl;
            }
        }
    }

    /// récupération des coordonnées du noued "admin_centre"
    type::GeographicalCoord admin_centre_coord(const References & refs){
        type::GeographicalCoord best;
        for(auto ref : refs){
            if (ref.first == OSMPBF::Relation_MemberType_NODE){
                try{
                    Node n = nodes[ref.second];
                    best.set_lon(n.lon());
                    best.set_lat(n.lat());
                    break;
                }catch(...){
                    std::cout << "Attention, le noued  : [" << ref.second<< " est introuvable].  " << std::endl;;
                }
            }
        }
        return best;
    }

    void get_list_ways(const References & refs, std::unordered_map<uint64_t,std::vector<Node>> & node_list){

        for(auto ref : refs){
            if (ref.first == OSMPBF::Relation_MemberType_WAY){
                uint64_t osmid = boost::lexical_cast<int>(ref.second);                
                auto osmway_it = ways.find(ref.second);
                if(osmway_it == ways.end()){
                    std::cout<<"Rue introuvable :"+boost::lexical_cast<std::string>(osmid)<<std::endl;
                }else{
                if (node_list.size() == 0){
                    std::vector<Node> Current_List;
                    for(auto node : osmway_it->second.refs){
                        Current_List.push_back(nodes[node]);
                    }
                    node_list[osmid] = Current_List;
                }else{
                    auto wt_node = node_list.find(osmid);
                    if(wt_node == node_list.end()){
                        std::vector<Node> Current_List;
                        for(auto node : osmway_it->second.refs){
                             Current_List.push_back(nodes[node]);
                         }
                         node_list[osmid] = Current_List;
                    }
                }
                }

            }else{
                if (ref.first == OSMPBF::Relation_MemberType_RELATION){
                    get_list_ways(refs, node_list);
                }
            }
        }
    }
void reverse(std::vector<Node> & nodes){
    int i = 0;
    int j = nodes.size() -1 ;
    while (i<j){
        Node temp = nodes[i];
        nodes[i] = nodes[j];
        nodes[j] = temp;
        ++i;
        --j;
    }
}

void add_boundary(std::vector<Node>& nodes, navitia::georef::Admin& admin){
    for(auto node : nodes){
        navitia::type::GeographicalCoord coord;
        coord.set_lon(node.lon());
        coord.set_lat(node.lat());
        admin.boundary.push_back(coord);
    }
}

void order_nodes(std::unordered_map<uint64_t,std::vector<Node>> & node_list, std::vector<uint64_t>& Added){

    typedef std::unordered_map<uint64_t, std::vector<Node>> map_vect;
    for(map_vect::iterator jt=node_list.begin(); jt!=node_list.end(); ++jt){
        Added.push_back(jt->first);
    }
    std::vector<Node> vect1, vect2;
    if (Added.size() > 1){
        for(size_t i = 1; i<Added.size(); ++i){
            vect1 = node_list.at(Added[i-1]);
            for(size_t j = i; j<Added.size(); ++j){
                vect2 = node_list.at(Added[j]);
                if((vect1.back().idx == vect2.front().idx) || (vect1.back().idx == vect2.back().idx)){
                    if(i != j){
                        if(vect1.back().idx == vect2.back().idx){
                            reverse(node_list.at(Added[j]));
                        }
                        std::swap(Added[i], Added[j]);
                        break;
                    }else{
                        if(vect1.back().idx == vect2.back().idx){
                            reverse(node_list.at(Added[j]));
                        }
                    }
                }
            }
        }
    }
}

void manage_boundary(const References & refs, navitia::georef::Admin& admin){
    std::unordered_map<uint64_t,std::vector<Node>> node_list;   // Identifiant de la rue
                                                                // liste des nodes dans l'ordre
    typedef std::unordered_map<uint64_t, std::vector<Node>> map_vect;
    std::vector<uint64_t> Added;

    get_list_ways(refs, node_list);
    order_nodes(node_list, Added);
    for(auto osmid : Added){
        std::vector<Node> vect = node_list.at(osmid);
        add_boundary(vect, admin);
    }
    /// A voir !!
    navitia::type::GeographicalCoord Start_Coord = admin.boundary.front();
    navitia::type::GeographicalCoord End_Coord = admin.boundary.back();
    if (Start_Coord != End_Coord){
        admin.boundary.push_back(Start_Coord);
    }
}
    /// construction des informations administratives
    void AdminRef(){
        for(auto ar : OSMAdminRefs){
            navitia::georef::Admin admin;
            admin.insee = ar.second.insee;
            admin.post_code = ar.second.postcode;
            admin.name = ar.second.name;
            admin.level = boost::lexical_cast<int>( ar.second.level);
            admin.coord = admin_centre_coord(ar.second.refs);
            manage_boundary(ar.second.refs, admin);

            geo_ref.admins.push_back(admin);
        }
    }

    void relation_callback(uint64_t osmid, const Tags & tags, const References & refs){
        if(tags.find("admin_level") != tags.end()){
            iter= levellist.LevelList.find(tags.at("admin_level"));
           if (iter != levellist.LevelList.end()){
                OSMAdminRef inf;
                inf.level = tags.at("admin_level");
                if(tags.find("ref:INSEE") != tags.end()){
                    inf.insee = tags.at("ref:INSEE");
                }
                if(tags.find("name") != tags.end()){
                    inf.name = tags.at("name");
                }
                if(tags.find("addr:postcode") != tags.end()){
                    inf.postcode = tags.at("addr:postcode");
                }
                inf.refs = refs;
                OSMAdminRefs[osmid]=inf;
            }
        }
    }
};

void fill_from_osm(GeoRef & geo_ref_to_fill, const std::string & osm_pbf_filename){
    Visitor v(geo_ref_to_fill);
    CanalTP::read_osm_pbf(osm_pbf_filename, v);    
    std::cout << v.nodes.size() << " nodes, " << v.ways.size() << " ways/" << v.total_ways << std::endl;
    v.count_nodes_uses();
    v.edges();
    v.HouseNumbers();    
    v.AdminRef();
    std::cout << "On a : " << v.total_house_number << " adresses" << std::endl;
    std::cout << "On a : " << v.geo_ref.admins.size() << " données administratives" << std::endl;
}

}}



