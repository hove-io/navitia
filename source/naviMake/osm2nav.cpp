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

namespace navitia { namespace georef {

struct OSMHouseNumber{
    type::GeographicalCoord coord;
    int number;

    OSMHouseNumber(): number(-1){}

};

struct Node {
public:
    uint32_t uses;
    int32_t idx;
    uint64_t ref_node; // Identifiant OSM du noeud
    navitia::type::GeographicalCoord coord;
    Node() : uses(0), idx(-1),ref_node(-1){}

    /// Incrémente le nombre d'utilisations, on lui passe l'index qu'il devra avoit
    /// Retourne vrai si c'est un nouveau nœud (càd utilisé au moins 2 fois et n'avait pas encore d'idx)
    /// Ca permet d'éviter d'utiliser deux fois le même nœud
    bool increment_use(int idx){
        uses++;
        if(this->idx == -1 && uses > 1){
            this->idx = idx;
            return true;
        } else {
            return false;
        }
    }
};

struct OSMWay {
    const static uint8_t CYCLE_FWD = 0;
    const static uint8_t CYCLE_BWD = 1;
    const static uint8_t CAR_FWD = 2;
    const static uint8_t CAR_BWD = 3;
    const static uint8_t FOOT_FWD = 4;
    const static uint8_t FOOT_BWD = 5;

    /// Propriétés du way : est-ce qu'on peut "circuler" dessus
    std::bitset<8> properties;

    /// Nodes qui composent le way
    std::vector<uint64_t> refs;

    /// Idx du way dans navitia
    /// N'est pas toujours renseigné car tous les ways dans OSM ne sont pas des rues
    navitia::type::idx_t idx;
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
    log4cplus::Logger logger;
    std::unordered_map<uint64_t,References> references;
    std::unordered_map<uint64_t, Node> nodes;
    std::unordered_map<uint64_t, OSMHouseNumber> housenumbers;
    std::unordered_map<uint64_t, OSMAdminRef> OSMAdminRefs;
    int total_ways;
    int total_house_number;

    std::unordered_map<uint64_t, OSMWay> ways;
    georef::GeoRef & geo_ref;
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
            Node node;
            node.coord.set_lat(lat);
            node.coord.set_lon(lon);
            node.ref_node = osmid;
            this->nodes[osmid] = node;
            add_osm_housenumber(osmid, tags);
    }

    void way_callback(uint64_t osmid, const Tags &tags, const std::vector<uint64_t> &refs){
        total_ways++;

        OSMWay w;
        w.refs = refs;
        w.properties = parse_way_tags(tags);
        // Est-ce que au moins une propriété fait que la rue est empruntable (voiture, vélo, piéton)
        // Alors on le garde comme way navitia
        if(w.properties.any()){
            georef::Way gr_way;
            gr_way.idx = geo_ref.ways.size();
            gr_way.uri = std::to_string(w.idx);
            if(tags.find("name") != tags.end())
                gr_way.name = tags.at("name");
            geo_ref.ways.push_back(gr_way);
            w.idx = gr_way.idx;
        }else{
            // Dans le cas où la maison est dessinée par un way et non pas juste par un node
             add_osm_housenumber(refs.front(), tags);
        }
        ways[osmid] = w;
    }

    // Once all the ways and nodes are read, we count how many times a node is used to detect intersections
    void count_nodes_uses() {
        int node_idx = 0;
        for(const auto & w : ways){
            if(w.second.properties.any()) {
                for(uint64_t ref : w.second.refs){
                    if(nodes[ref].increment_use(node_idx)){
                        Vertex v;
                        node_idx++;
                        v.coord = nodes[ref].coord;
                        boost::add_vertex(v, geo_ref.graph);
                    }
                }

                uint64_t ref = w.second.refs.front();
                // make sure that the last node is considered as an extremity
                if(nodes[ref].increment_use(node_idx)){
                    node_idx++;
                    Vertex v;
                    v.coord =  nodes[ref].coord;
                    boost::add_vertex(v, geo_ref.graph);
                }

                ref = w.second.refs.back();
                if(nodes[ref].increment_use(node_idx)){
                    node_idx++;
                    Vertex v;
                    v.coord =  nodes[ref].coord;
                    boost::add_vertex(v, geo_ref.graph);
                }
            }
        }
        std::cout << "On a : " << boost::num_vertices(geo_ref.graph) << " nœuds" << std::endl;
    }

    // Returns the source and target node of the edges
    void edges(){
        for(const auto & w : ways){
            if(w.second.properties.any() && w.second.refs.size() > 0){
                Node n = nodes[w.second.refs[0]];
                type::idx_t source = n.idx;
                type::GeographicalCoord prev = n.coord;
                float length = 0;
                for(size_t i = 1; i < w.second.refs.size(); ++i){
                    Node current_node = nodes[w.second.refs[i]];
                    type::GeographicalCoord current = current_node.coord;
                    length += current.distance_to(prev);
                    prev = current;
                    // If a node is used more than once, it is an intersection, hence it's a node of the street network graph
                    try{
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
                    }catch(...){
                        LOG4CPLUS_WARN(logger, "L'arc n'est pas importée car la rue : [" + std::to_string(w.first) + "] n'existe pas.");
                    }
                }
            }
        }
        std::cout << "On a " << boost::num_edges(geo_ref.graph) << " arcs" << std::endl;
    }

    /// Chargement des adresses
    void HouseNumbers(){        
        type::idx_t idx;
        type::idx_t Count = 0; // Nombre d'adresses non importées
        georef::HouseNumber gr_hn;
        geo_ref.build_proximity_list();
        for(auto hn : housenumbers){
            try{
                Node n = nodes[hn.first];
                if ((n.coord.lon() == 0) && (n.coord.lat())){
                    ++Count;
                }else{
                gr_hn.number = hn.second.number;
                gr_hn.coord.set_lon(n.coord.lon());
                gr_hn.coord.set_lat(n.coord.lat());
                idx = geo_ref.graph[geo_ref.nearest_edge(gr_hn.coord)].way_idx;
                if (idx <= geo_ref.ways.size()){
                    geo_ref.ways[idx].add_house_number(gr_hn);
                    total_house_number ++;
                } // Message ?
                }
            } catch(navitia::proximitylist::NotFound) {
                std::cout << "Attention, l'adresse n'est pas importée dont le numéro et les coordonnées sont : [" << gr_hn.number<<";"<< gr_hn.coord.lon()<< ";"<< gr_hn.coord.lat()<< "] Impossible de trouver le segment le plus proche.  " << std::endl;
                ++Count;
            }catch(...){
                std::cout << "Attention, l'adresse n'est pas importée dont le numéro et les coordonnées sont : [" << gr_hn.number<<";"<< gr_hn.coord.lon()<< ";"<< gr_hn.coord.lat()<< "].  " << std::endl;
                ++Count;
            }
        }
        std::cout<<"Nombre d'adresses non importées : "<<Count<<std::endl;
    }

    /// récupération des coordonnées du noued "admin_centre"
    type::GeographicalCoord admin_centre_coord(const References & refs){
        type::GeographicalCoord best;        
        for(Reference ref : refs){
            if (ref.member_type == OSMPBF::Relation_MemberType_NODE){
                try{                    
                    Node n = nodes[ref.member_id];
                    best.set_lon(n.coord.lon());
                    best.set_lat(n.coord.lat());
                    break;
                }catch(...){
                    std::cout << "Attention, le noued  : [" << ref.member_id<< " est introuvable].  " << std::endl;;
                }
            }
        }
        return best;
    }

    /// Retourne les nœuds des rues
    std::unordered_map<uint64_t,std::vector<Node> >get_list_ways(const References & refs){
        std::unordered_map<uint64_t,std::vector<Node> >  node_list;
        for(Reference ref : refs){
            uint64_t osmid = boost::lexical_cast<uint64_t>(ref.member_id);
            if (ref.member_type == OSMPBF::Relation_MemberType_WAY){

                auto osmway_it = ways.find(ref.member_id);
                if(osmway_it == ways.end()){
                    std::cout << "Rue introuvable :"+ std::to_string(osmid) << std::endl;
                } else if(node_list.find(osmid) == node_list.end()){
                    std::vector<Node> & current_List = node_list[osmid];
                    for(auto node : osmway_it->second.refs){
                        current_List.push_back(nodes[node]);
                    }
                }
            } else if (ref.member_type == OSMPBF::Relation_MemberType_RELATION && ref.role != "subarea"){
                for(auto pair : get_list_ways(references.at(osmid))){
                    node_list[pair.first].insert(node_list[pair.first].end(), pair.second.begin(), pair.second.end());
                }
            }
        }
        return node_list;
    }


/// Ordonner les nœuds et les rues
std::vector<uint64_t> order_nodes(std::unordered_map<uint64_t, std::vector<Node> > & node_list){
    std::vector<uint64_t> added;
    for(auto & jt : node_list) {
        added.push_back(jt.first);
    }

    for(size_t i = 1; i < added.size(); ++i){
        std::vector<Node> & vect1 = node_list.at(added[i-1]);
        for(size_t j = i; j< added.size(); ++j){
            std::vector<Node> & vect2 = node_list.at(added[j]);
            if( vect1.back().ref_node == vect2.front().ref_node || vect1.back().ref_node == vect2.back().ref_node){
                if(i != j){
                    if(vect1.back().ref_node == vect2.back().ref_node){
                        std::reverse(vect2.begin(), vect2.end());
                    }
                    std::swap(added[i], added[j]);
                    break;
                }else{
                    if(vect1.back().ref_node == vect2.back().ref_node){
                        std::reverse(vect2.begin(),  vect2.end());
                    }
                }
            }
        }
    }
    return added;
}

/// Gestion des limites des communes
void manage_boundary(const References & refs, navitia::georef::Admin& admin){
    std::unordered_map<uint64_t,std::vector<Node>> node_list = get_list_ways(refs);
    std::vector<uint64_t> Added = order_nodes(node_list);
    for(auto osmid : Added){
        for(Node & node : node_list.at(osmid)) {
            boost::geometry::append(admin.boundary, node.coord);
        }
    }
}

/// construction des informations administratives
void AdminRef(){
    for(auto ar : OSMAdminRefs){
        navitia::georef::Admin admin;
        admin.insee = ar.second.insee;
        admin.idx = geo_ref.admins.size();
        admin.post_code = ar.second.postcode;
        admin.name = ar.second.name;
        admin.level = boost::lexical_cast<int>( ar.second.level);
        admin.coord = admin_centre_coord(ar.second.refs);
        manage_boundary(ar.second.refs, admin);        
        geo_ref.admins.push_back(admin);
     }
}

/// Chargement des communes des rues
void fillAdminWay(){
    for(navitia::georef::Way& way : geo_ref.ways){
        navitia::type::GeographicalCoord coord = way.barycentre(geo_ref.graph);
        std::vector<navitia::type::idx_t> vect_idx = geo_ref.find_admins(coord);
        for(navitia::type::idx_t id : vect_idx){
            way.admins.push_back(id);
        }
    }
}

    void relation_callback(uint64_t osmid, const Tags & tags, const References & refs){
        references[osmid] = refs;
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
    std::cout<<"Chargement des adresses"<<std::endl;
    v.HouseNumbers();
    std::cout<<"Chargement des données administratives"<<std::endl;
    v.AdminRef();
    std::cout<<"Chargement des données administratives des adresses"<<std::endl;
    //v.fillAdminWay();
    std::cout << "On a : " << v.total_house_number << " adresses" << std::endl;
    std::cout << "On a : " << v.geo_ref.admins.size() << " données administratives" << std::endl;
}

}}



