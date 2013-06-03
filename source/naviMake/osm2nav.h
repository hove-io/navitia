#pragma once
#include "type/data.h"
#include "third_party/osmpbfreader/osmpbfreader.h"
#include "georef/pois.h"

namespace navitia { namespace georef {
class GeoRef;

/** Remplit l'objet georef à partir du fichier openstreetmap */
void fill_from_osm(GeoRef & geo_ref_to_fill, const std::string & osm_pbf_filename);

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

struct OSMAdminRef{
    std::string level;
    std::string insee;
    std::string name;
    std::string postcode;
    type::GeographicalCoord coord;
    CanalTP::References refs;
};

struct OSMAPoi{
    std::string key;
    std::string name;
    type::GeographicalCoord coord;
};

/** Structure appelée par parseur OSM PBF */
struct Visitor{
    log4cplus::Logger logger;
    std::unordered_map<uint64_t,CanalTP::References> references;
    std::unordered_map<uint64_t, Node> nodes;
    std::unordered_map<uint64_t, OSMHouseNumber> housenumbers;
    std::unordered_map<uint64_t, OSMAdminRef> OSMAdminRefs;
    std::unordered_map<uint64_t, OSMAPoi> OSMAPois;
    int total_ways;
    int total_vls_stations;
    int total_house_number;

    std::unordered_map<uint64_t, OSMWay> ways;
    georef::GeoRef & geo_ref;
    //Pour charger les données administratives
    navitia::adminref::Levels levellist;
    // Pour charger les pois
    navitia::georef::Pois poilist;
    std::map<std::string,std::string>::iterator iter;

    Visitor(GeoRef & to_fill) : total_ways(0), total_vls_stations(0), total_house_number(0), geo_ref(to_fill){
        init_logger();
        logger = log4cplus::Logger::getInstance("log");
    }

    void node_callback(uint64_t osmid, double lon, double lat, const CanalTP::Tags & tags);
    void way_callback(uint64_t osmid, const CanalTP::Tags &tags, const std::vector<uint64_t> &refs);
    void relation_callback(uint64_t osmid, const CanalTP::Tags & tags, const CanalTP::References & refs);

    void add_osm_housenumber(uint64_t osmid, const CanalTP::Tags & tags);

    void add_osm_poi(const navitia::type::GeographicalCoord& coord, const CanalTP::Tags & tags);
    /// Once all the ways and nodes are read, we count how many times a node is used to detect intersections
    void count_nodes_uses();

    /// Calcule la source et cible des edges
    void edges();

    /// Calcule la source et cible des edges pour le vls
    void build_vls_edges();

    /// Chargement des adresses
    void HouseNumbers();

    /// récupération des coordonnées du noued "admin_centre"
    type::GeographicalCoord admin_centre_coord(const CanalTP::References & refs);

    /// à partir des références d'une relation, reconstruit la liste des identifiants OSM ordonnés
    std::vector<uint64_t> nodes_of_relation(const CanalTP::References & refs);

    /// gestion des limites des communes : création des polygons
    void manage_admin_boundary(const CanalTP::References & refs, navitia::adminref::Admin& admin);

    /// construction des informations administratives
    void AdminRef();

    /// Associe des communes des rues
    void set_admin_of_ways();

    /// cahrgement des PoiType
    void fillPoiType();
};

}}

