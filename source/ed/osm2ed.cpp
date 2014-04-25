#include "osm2ed.h"
#include <stdio.h>

#include <iostream>

#include "ed/connectors/osm_tags_reader.h"
#include "utils/functions.h"
#include "utils/init.h"

#include "config.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace pt = boost::posix_time;

namespace ed { namespace connectors {

void Visitor::node_callback(uint64_t osmid, double lon, double lat, const CanalTP::Tags & tags){
    Node node;
    node.set_coord(lon,lat);
    this->nodes[osmid] = node;
    add_osm_housenumber(osmid, tags);
    fill_pois(osmid, tags);
}

void Visitor::way_callback(uint64_t osmid, const CanalTP::Tags &tags, const std::vector<uint64_t> &refs){
    total_ways++;

    OSMWay w;
    // On ignore les ref vers les nœuds qui ne sont pas dans les données
    for(auto ref : refs){
        if(this->nodes.find(ref) != this->nodes.end())
            w.refs.push_back(ref);
    }
    w.properties = parse_way_tags(tags);
    // Est-ce que au moins une propriété fait que la rue est empruntable (voiture, vélo, piéton)
    // Alors on le garde comme way navitia
    if(w.properties.any()){
        std::string name;
        if(tags.find("name") != tags.end())
            name = tags.at("name");
        this->persistor.lotus.insert({std::to_string(osmid), name, std::to_string(osmid)});
    }else{
        // Dans le cas où la maison est dessinée par un way et non pas juste par un node
        add_osm_housenumber(refs.front(), tags);
        fill_pois(refs.front(), tags);
    }
    ways[osmid] = w;
}

void Visitor::relation_callback(uint64_t osmid, const CanalTP::Tags & tags, const CanalTP::References & refs){
    references[osmid] = refs;
    const auto tmp_admin_level = tags.find("admin_level") ;
    if(tmp_admin_level != tags.end()){
        OSMAdminRef inf;
        inf.level = tmp_admin_level->second;

        std::vector<std::string> accepted_levels{"8", "9", "10"};
        if(std::find(accepted_levels.begin(), accepted_levels.end(), inf.level) != accepted_levels.end()){
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
    } else {
        LOG4CPLUS_INFO(logger, "Unable to find tag admin_level");
    }
}


void Visitor::add_osm_housenumber(uint64_t osmid, const CanalTP::Tags & tags){
    if(tags.find("addr:housenumber") != tags.end()){
        ed::types::HouseNumber osm_hn;
        osm_hn.number = tags.at("addr:housenumber");
        this->housenumbers[osmid] = osm_hn;

    }
}


void Visitor::insert_if_needed(uint64_t ref) {
    const auto tmp_node = nodes.find(ref);
    if(tmp_node != nodes.end()) {
        Node & n = tmp_node->second;
        if(n.increment_use(this->node_idx)){
            this->node_idx++;
            std::string line = "POINT(" + std::to_string(n.lon()) + " " + std::to_string(n.lat()) + ")";
            this->persistor.lotus.insert({std::to_string(ref), line});
        }
    } else {
        LOG4CPLUS_INFO(logger, "Unable to find node " << ref);
    }
}


void Visitor::count_nodes_uses() {
    this->persistor.lotus.prepare_bulk_insert("georef.node", {"id", "coord"});

    for(const auto & w : ways){
        if(w.second.properties.any()) {
            for(uint64_t ref : w.second.refs){
                this->insert_if_needed(ref);
            }

            if(w.second.refs.size() > 0) {
                // make sure that the last node is considered as an extremity
                this->insert_if_needed(w.second.refs.front());
                this->insert_if_needed(w.second.refs.back());
            }
        }

    }

    persistor.lotus.finish_bulk_insert();
}

void Visitor::insert_edges(){
    this->persistor.lotus.prepare_bulk_insert("georef.edge", {"source_node_id", "target_node_id", "way_id", "the_geog", "pedestrian_allowed", "cycles_allowed", "cars_allowed"});
    std::stringstream geog;
    geog << std::cout.precision(10);

    for(const auto & w : ways){
        if(w.second.properties.any()){
            uint64_t source = 0;
            geog.str("");
            geog << "LINESTRING(";

            for(size_t i = 0; i < w.second.refs.size(); ++i){
                uint64_t current_ref = w.second.refs[i];
                const auto tmp_node = nodes.find(current_ref);
                if(tmp_node != nodes.end()) {
                    Node current_node = tmp_node->second;

                    if(i > 0) geog << ", ";
                    geog << current_node.lon() << " " << current_node.lat();

                    if(i == 0) {
                        source = current_ref;
                    }
                    // If a node is used more than once, it is an intersection, hence it's a node of the street network graph
                    else if(current_node.uses > 1){
                        uint64_t target = current_ref;
                        geog << ")";
                        this->persistor.lotus.insert({std::to_string(source), std::to_string(target), std::to_string(w.first), geog.str(),
                                           std::to_string(w.second.properties[FOOT_FWD]), std::to_string(w.second.properties[CYCLE_FWD]), std::to_string(w.second.properties[CAR_FWD])});
                        this->persistor.lotus.insert({std::to_string(target), std::to_string(source), std::to_string(w.first), geog.str(),
                                           std::to_string(w.second.properties[FOOT_BWD]), std::to_string(w.second.properties[CYCLE_BWD]), std::to_string(w.second.properties[CAR_BWD])});
                        source = target;
                        geog.str("");
                        geog << "LINESTRING(" << current_node.lon() << " " << current_node.lat();
                    }
                } else {
                    LOG4CPLUS_INFO(logger, "Unable to find node " << current_ref);
                }
            }
        }
    }
    persistor.lotus.finish_bulk_insert();
}


void Visitor::insert_house_numbers(){
    this->persistor.lotus.prepare_bulk_insert("georef.house_number", {"coord", "number", "left_side"});
    for(auto hn : housenumbers) {
        const auto tmp_node = nodes.find(hn.first);
        if(tmp_node != nodes.end()) {
            Node n = tmp_node->second;
            std::string point = "POINT(" + std::to_string(n.lon()) + " " + std::to_string(n.lat()) + ")";
            this->persistor.lotus.insert({point, hn.second.number, std::to_string((str_to_int(hn.second.number) % 2) == 0)});
        } else {
            LOG4CPLUS_INFO(logger, "Unable to find node " << hn.first);
        }
    }
    persistor.lotus.finish_bulk_insert();
}

navitia::type::GeographicalCoord Visitor::admin_centre_coord(const CanalTP::References & refs){
    navitia::type::GeographicalCoord best;
    for(CanalTP::Reference ref : refs){
        if (ref.member_type == OSMPBF::Relation_MemberType_NODE){
            const auto tmp_node = nodes.find(ref.member_id);
            if(tmp_node != nodes.end()) {
                Node n = tmp_node->second;
                best.set_lon(n.lon());
                best.set_lat(n.lat());
                break;
            }else{
                LOG4CPLUS_INFO(logger, "Unable to find node " << ref.member_id);
            }
        }
    }
    return best;
}


std::string Visitor::geometry_of_admin(const CanalTP::References & refs) const{
    std::vector<uint64_t> vec_id = nodes_of_relation(refs);
    if(vec_id.size() < 3)
        return "";

    // Fermer le polygon si ce n'est pas le cas:
    if (vec_id.front() != vec_id.back()){
        vec_id.push_back(vec_id.front());
    }

    std::string geom("MULTIPOLYGON(((");
    std::string sep = "";

    for (auto osm_node_id : vec_id){

        const Node & node = this->nodes.at(osm_node_id);
        geom += sep + std::to_string(node.lon()) + " " + std::to_string(node.lat());
        sep = ",";
    }
    geom += ")))";
    return geom;
}

/// à partir des références d'une relation, reconstruit la liste des identifiants OSM ordonnés
std::vector<uint64_t> Visitor::nodes_of_relation(const CanalTP::References & refs) const {
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
                    } else {
                        current_nodes = osmway_it->second.refs;
                    }
                }
                else if (ref.member_type == OSMPBF::Relation_MemberType_RELATION && ref.role != "subarea"){
                    const auto tmp_ref = references.find(ref.member_id);
                    if(tmp_ref != references.end()) {
                        current_nodes = nodes_of_relation(tmp_ref->second);
                    } else {
                        LOG4CPLUS_INFO(logger, "Unable to find reference ");
                    }
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


void Visitor::insert_admin(){
    this->persistor.lotus.prepare_bulk_insert("georef.admin", {"id", "name", "post_code", "insee", "level", "coord", "boundary", "uri"});
    int ignored = 0;
    for(auto ar : OSMAdminRefs){
        auto admin = ar.second;
        std::string multipolygon = this->geometry_of_admin(admin.refs);
        if(!multipolygon.empty()){
            std::string coord = "POINT(" + std::to_string(admin.coord.lon()) + " " + std::to_string(admin.coord.lat()) + ")";
            this->persistor.lotus.insert({std::to_string(ar.first), admin.name, admin.postcode, admin.insee, admin.level, coord,multipolygon , std::to_string(ar.first)});
        } else {
            ignored++;
        }
    }
    this->persistor.lotus.finish_bulk_insert();
    LOG4CPLUS_INFO(logger, "Admin ignorés : " << ignored << " sur " << OSMAdminRefs.size());
}

void Visitor::insert_poitypes(){
    this->persistor.lotus.prepare_bulk_insert("georef.poi_type", {"id", "uri", "name"});
    for(auto pt : poi_types){
        this->persistor.lotus.insert({std::to_string(pt.second.id), "poi_type:" + pt.first, pt.second.name});
    }
    persistor.lotus.finish_bulk_insert();
}

void Visitor::insert_pois(){
    this->persistor.lotus.prepare_bulk_insert("georef.poi", {"id","weight","coord", "name", "uri", "poi_type_id", "address_name", "address_number"});
    for(auto poi : pois){
        Node n;
        try{
            n = nodes.at(poi.first);
        }catch(const std::out_of_range& e){
            LOG4CPLUS_INFO(logger, "Unable to find node [" << poi.first<<"] "<<e.what());
            continue;
        }
        std::string point = "POINT(" + std::to_string(n.lon()) + " " + std::to_string(n.lat()) + ")";
        this->persistor.lotus.insert({std::to_string(poi.second.id),std::to_string(poi.second.weight),
                                    point, poi.second.name, "poi:" + std::to_string(poi.first),
                                    std::to_string(poi.second.poi_type->id), poi.second.address_name,
                                    poi.second.address_number});

    }
    persistor.lotus.finish_bulk_insert();
}

void Visitor::insert_properties(){
    this->persistor.lotus.prepare_bulk_insert("georef.poi_properties", {"poi_id","key","value"});
    for(const auto& poi : pois){
        try{
            nodes.at(poi.first);
        }catch(const std::out_of_range& e){
            LOG4CPLUS_INFO(logger, "Unable to find node [" << poi.first<<"] "<<e.what());
            continue;
        }
        for(const auto& property : poi.second.properties){
            this->persistor.lotus.insert({std::to_string(poi.second.id),property.first, property.second});
        }
    }
    persistor.lotus.finish_bulk_insert();
}

void Visitor::build_relation(){
    PQclear(this->persistor.lotus.exec("SELECT georef.match_way_to_admin();", "", PGRES_TUPLES_OK));
}

void Visitor::fill_PoiTypes(){
    poi_types["college"] = ed::types::PoiType(0,  "école");
    poi_types["university"] = ed::types::PoiType(1, "université");
    poi_types["theatre"] = ed::types::PoiType(2, "théâtre");
    poi_types["hospital"] = ed::types::PoiType(3, "hôpital");
    poi_types["post_office"] = ed::types::PoiType(4, "bureau de poste");
    poi_types["bicycle_rental"] = ed::types::PoiType(5, "station vls");
    poi_types["bicycle_parking"] = ed::types::PoiType(6, "Parking vélo");
    poi_types["parking"] = ed::types::PoiType(7, "Parking");
    poi_types["park_ride"] = ed::types::PoiType(8, "Parc-relais");
    poi_types["police"] = ed::types::PoiType(9, "Police, Gendarmerie");
    poi_types["townhall"] = ed::types::PoiType(10, "Mairie");
    poi_types["garden"] = ed::types::PoiType(11, "Jardin");
    poi_types["park"] = ed::types::PoiType(12, "Zone Parc. Zone verte ouverte, pour déambuler. habituellement municipale");
}

bool Visitor::is_property_to_ignore(const std::string& tag){
    for(const std::string& st : properties_to_ignore){
        if(st == tag){
            return true;
        }
    }
    return false;
}

void Visitor::fill_properties_to_ignore(){
    properties_to_ignore.push_back("name");
    properties_to_ignore.push_back("amenity");
    properties_to_ignore.push_back("leisure");
    properties_to_ignore.push_back("addr:housenumber");
    properties_to_ignore.push_back("addr:street");
    properties_to_ignore.push_back("addr:city");
    properties_to_ignore.push_back("addr:postcode");
    properties_to_ignore.push_back("addr:country");
}

void Visitor::fill_pois(const uint64_t osmid, const CanalTP::Tags & tags){
    std::string ref_tag;
    if(tags.find("amenity") != tags.end()){
            ref_tag = "amenity";
    }
    if(tags.find("leisure") != tags.end()){
            ref_tag = "leisure";
    }
    if(!ref_tag.empty()){
        auto poi_it = pois.find(osmid);
        if (poi_it != pois.end()){
            return;
        }
        std::string value = tags.at(ref_tag);
        auto it = poi_types.find(value);
        if(it != poi_types.end()){
            ed::types::Poi poi;
            if((it->first == "parking") && (tags.find("park_ride") != tags.end())){
                std::string park_ride = tags.at("park_ride");
                boost::to_lower(park_ride);
                if(park_ride == "yes"){
                    it = poi_types.find("park_ride");
                }
            }
            poi.poi_type = &it->second;
            if(tags.find("name") != tags.end()){
                poi.name = tags.at("name");
            }
            for(auto st : tags){
                if(is_property_to_ignore(st.first)){
                    if(st.first == "addr:housenumber"){
                        poi.address_number = st.second;
                    }
                    if(st.first == "addr:street"){
                        poi.address_name = st.second;
                    }
                }else{
                    poi.properties[st.first] = st.second;
                }
            }
            poi.id = pois.size();
            pois[osmid] = poi;
        }
    }
}
void Visitor::set_coord_admin(){
    for(auto& adm : OSMAdminRefs ){
        adm.second.coord = admin_centre_coord(adm.second.refs);
    }
}

}
}

int main(int argc, char** argv) {
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");
    pt::ptime start;
    std::string input, connection_string;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("version,v", "Affiche la version")
        ("help,h", "Affiche l'aide")
        ("input,i", po::value<std::string>(&input)->required(), "Fichier OSM à utiliser")
        ("connection-string", po::value<std::string>(&connection_string)->required(), "parametres de connexion à la base de données: host=localhost user=navitia dbname=navitia password=navitia");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        LOG4CPLUS_INFO(logger, argv[0] << " V" << KRAKEN_VERSION << " " << NAVITIA_BUILD_TYPE );
        return 0;
    }

    if(vm.count("help")) {
        LOG4CPLUS_INFO(logger, desc );
        return 1;
    }

    start = pt::microsec_clock::local_time();
    po::notify(vm);
    ed::connectors::Visitor v(connection_string);
    v.persistor.lotus.start_transaction();

    v.persistor.clean_georef();
    v.persistor.clean_poi();
    v.fill_PoiTypes();
    v.fill_properties_to_ignore();
    v.persistor.lotus.prepare_bulk_insert("georef.way", {"id", "name", "uri"});
    CanalTP::read_osm_pbf(input, v);
    v.set_coord_admin();
    v.persistor.lotus.finish_bulk_insert();

    LOG4CPLUS_INFO(logger, v.nodes.size() << " nodes, " << v.ways.size() << " ways/" << v.total_ways );
    LOG4CPLUS_INFO(logger, v.poi_types.size()<<" poitype/ "<<v.pois.size()<< " poi");
    v.count_nodes_uses();
    v.insert_edges();

    LOG4CPLUS_INFO(logger, "add House_numbers data");
    v.insert_house_numbers();
    LOG4CPLUS_INFO(logger, "add poi_types data");
    v.insert_poitypes();
    LOG4CPLUS_INFO(logger, "add pois data");
    v.insert_pois();
    LOG4CPLUS_INFO(logger, "add pois properties data");
    v.insert_properties();
    LOG4CPLUS_INFO(logger, "add admins data");
    v.insert_admin();

    v.build_relation();

    LOG4CPLUS_INFO(logger, "Fusion ways");
    v.persistor.build_ways();

    v.persistor.lotus.commit();

    LOG4CPLUS_INFO(logger, "Integration time OSM Data :"<<to_simple_string(pt::microsec_clock::local_time() - start));
    return 0;
}




