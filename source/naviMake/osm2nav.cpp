#include "osm2nav.h"
#include <stdio.h>

#include <iostream>
#include <boost/lexical_cast.hpp>


#include "osm_tags_reader.h"
#include "georef/georef.h"
#include "georef/adminref.h"
#include "utils/functions.h"

namespace navitia { namespace georef {

void Visitor::node_callback(uint64_t osmid, double lon, double lat, const CanalTP::Tags & tags){
    Node node;
    node.coord.set_lat(lat);
    node.coord.set_lon(lon);
    node.ref_node = osmid;
    this->nodes[osmid] = node;
    add_osm_housenumber(osmid, tags);
}

void Visitor::way_callback(uint64_t osmid, const CanalTP::Tags &tags, const std::vector<uint64_t> &refs){
    total_ways++;

    OSMWay w;
    w.refs = refs;
    w.properties = parse_way_tags(tags);
    // Est-ce que au moins une propriété fait que la rue est empruntable (voiture, vélo, piéton)
    // Alors on le garde comme way navitia
    if(w.properties.any()){
        georef::Way gr_way;
        gr_way.idx = geo_ref.ways.size();
        gr_way.uri = std::to_string(osmid);
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

void Visitor::relation_callback(uint64_t osmid, const CanalTP::Tags & tags, const CanalTP::References & refs){
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


void Visitor::add_osm_housenumber(uint64_t osmid, const CanalTP::Tags & tags){
    if(tags.find("addr:housenumber") != tags.end()){
        OSMHouseNumber osm_hn;
        osm_hn.number = str_to_int(tags.at("addr:housenumber"));
        if (osm_hn.number > 0 ){
            this->housenumbers[osmid] = osm_hn;
        }
    }
}

void Visitor::count_nodes_uses() {
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

void Visitor::edges(){
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


void Visitor::HouseNumbers(){
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

type::GeographicalCoord Visitor::admin_centre_coord(const CanalTP::References & refs){
    type::GeographicalCoord best;
    for(CanalTP::Reference ref : refs){
        if (ref.member_type == OSMPBF::Relation_MemberType_NODE){
            try{
                Node n = nodes[ref.member_id];
                best.set_lon(n.coord.lon());
                best.set_lat(n.coord.lat());
                break;
            }catch(...){
                std::cout << "AttenReferencestion, le noued  : [" << ref.member_id<< " est introuvable].  " << std::endl;;
            }
        }
    }
    return best;
}


void Visitor::manage_admin_boundary(const CanalTP::References & refs, navitia::georef::Admin& admin){
    std::vector<uint64_t> vec_id =  nodes_of_relation(refs);

    // Fermer le polygon ci ce n'est pas le cas:
    if (vec_id.front() != vec_id.back()){
        vec_id.push_back(vec_id.front());
    }

    for (auto osm_node_id : vec_id){
        // Dans certaines données, un way peut faire référence à un Node qui n'est pas dans les données
        try{
            const Node & node = this->nodes.at(osm_node_id);
            //Il ne faut pas ajouter le Node dont le coord est vide.
            if (node.coord.lat() != 0 || node.coord.lon() != 0){
                boost::geometry::append(admin.boundary, node.coord);
            }

        } catch(...) {
        }
    }
}

/// à partir des références d'une relation, reconstruit la liste des identifiants OSM ordonnés
std::vector<uint64_t> Visitor::nodes_of_relation(const CanalTP::References & refs){
    std::vector<uint64_t> result;

    //Le vector pour tracer l'index du ref qui a été ajouté
    std::vector<bool> added(refs.size(), false);

    for(size_t i = 0; i < refs.size(); ++i) {//Il faut itérer au n² fois pour être sur d'avoir vu tous les cas
        for(size_t j = 0; j < refs.size(); ++j) {
            if(!added[j]) {
                CanalTP::Reference ref = refs[j];

                // étape 1 :  on récupère la liste des id OSM des nœuds de la référence
                std::vector<uint64_t> current_nodes;
                if (ref.member_type == OSMPBF::Relation_MemberType_WAY){
                    auto osmway_it = ways.find(ref.member_id);
                    if(osmway_it == ways.end()){
                        std::cout << "Rue introuvable :"+ std::to_string(ref.member_id) << std::endl;
                    } else {
                        current_nodes = osmway_it->second.refs;
                    }
                }
                else if (ref.member_type == OSMPBF::Relation_MemberType_RELATION && ref.role != "subarea"){
                    current_nodes = nodes_of_relation(references.at(ref.member_id));
                }

                if (!current_nodes.empty()){
                    // étape 2 : est-ce les nœuds courants "suivent" les nœuds existants
                    // si result est vide, on ajoute tout
                    if(result.empty()) {
                        result = current_nodes;
                        added[j] = true;
                    } else if (result.back() == current_nodes.front()) { // Si les current sont dans le bon sens
                        result.insert(result.end(), current_nodes.begin() + 1, current_nodes.end()); // +1 pour pas avoir de doublons
                        added[j] = true;
                    } else if(result.back() == current_nodes.back()) { // Current est inversé
                        result.insert(result.end(), current_nodes.rbegin() + 1, current_nodes.rend());
                        added[j] = true;
                    } // Sinon c'est current nodes doit être inséré à un autre moment
                }
            }
        }
    }
    return result;
}


void Visitor::AdminRef(){
    for(auto ar : OSMAdminRefs){
        navitia::georef::Admin admin;
        admin.insee = ar.second.insee;
        admin.idx = geo_ref.admins.size();
        admin.post_code = ar.second.postcode;
        admin.name = ar.second.name;
        admin.level = boost::lexical_cast<int>( ar.second.level);
        admin.coord = admin_centre_coord(ar.second.refs);
        manage_admin_boundary(ar.second.refs, admin);
        geo_ref.admins.push_back(admin);
    }
}

void Visitor::set_admin_of_ways(){
    for(navitia::georef::Way& way : geo_ref.ways){
        navitia::type::GeographicalCoord coord = way.barycentre(geo_ref.graph);
        std::vector<navitia::type::idx_t> vect_idx = geo_ref.find_admins(coord);
        for(navitia::type::idx_t id : vect_idx){
            way.admins.push_back(id);
        }
    }
}



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
    geo_ref_to_fill.build_rtree();
    v.set_admin_of_ways();
    std::cout << "On a : " << v.total_house_number << " adresses" << std::endl;
    std::cout << "On a : " << v.geo_ref.admins.size() << " données administratives" << std::endl;

}

}}



