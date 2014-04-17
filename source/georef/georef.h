#pragma once
#include "autocomplete/autocomplete.h"
#include "proximity_list/proximity_list.h"
#include "third_party/RTree/RTree.h"
#include "adminref.h"
#include "utils/exception.h"
#include "utils/flat_enum_map.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <map>


namespace nt = navitia::type;
namespace nf = navitia::autocomplete;

namespace navitia { namespace georef {

/// default speed (in m/s) by transportation mode, defined at compile time
const flat_enum_map<nt::Mode_e, float> default_speed {
                                                    {{
                                                        1.12f, //nt::Mode_e::Walking
                                                        4.1f, //nt::Mode_e::Bike
                                                        16.8f, //nt::Mode_e::Car
                                                        4.1f //nt::Mode_e::Vls
                                                    }}
                                                    };

const boost::posix_time::seconds default_time_bss_pickup(120);
const boost::posix_time::seconds default_time_bss_putback(60);

/** Propriétés Nœud (intersection entre deux routes) */
struct Vertex {
    nt::GeographicalCoord coord;

    Vertex(){}
    Vertex(double x, double y, bool is_meters = false) : coord(x, y){
        if(is_meters) {
            coord.set_xy(x, y);
        }
    }

    Vertex(const nt::GeographicalCoord& other) : coord(other.lon(), other.lat()){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & coord;
    }
};

/** Propriétés des arcs : (anciennement "segment")*/

struct Edge {
    nt::idx_t way_idx = nt::invalid_idx; //< indexe vers le nom de rue
    boost::posix_time::time_duration duration = {}; // duration of the edge

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & way_idx & duration;
    }
    Edge(nt::idx_t wid, boost::posix_time::time_duration dur) : way_idx(wid), duration(dur) {}
    Edge() {}
};

// Plein de typedefs pour nous simpfilier un peu la vie

/** Définit le type de graph que l'on va utiliser
  *
  * Les arcs sortants et la liste des nœuds sont représentés par des vecteurs
  * les arcs sont orientés
  * les propriétés des nœuds et arcs sont les classes définies précédemment
  */
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Vertex, Edge> Graph;

/// Représentation d'un nœud dans le g,raphe
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;

/// Représentation d'un arc dans le graphe
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

/// Pour parcourir les segements du graphe
typedef boost::graph_traits<Graph>::edge_iterator edge_iterator;


/** le numéro de la maison :
    il représente un point dans la rue, voie */
struct HouseNumber{
    nt::GeographicalCoord coord;
    int number;

    HouseNumber(): number(-1){}

    bool operator<(const HouseNumber & other) const{
        return this->number < other.number;
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & coord & number;
    }
};




/** Nommage d'une voie (anciennement "adresse").
    Typiquement le nom de rue **/
struct Way :public nt::Nameable, nt::Header{
public:
    std::string way_type;
    // liste des admins
    std::vector<Admin*> admin_list;

    std::vector< HouseNumber > house_number_left;
    std::vector< HouseNumber > house_number_right;
    std::vector< std::pair<vertex_t, vertex_t> > edges;

    void add_house_number(const HouseNumber&);
    nt::GeographicalCoord nearest_coord(const int, const Graph&);
    int nearest_number(const nt::GeographicalCoord& );
    nt::GeographicalCoord barycentre(const Graph& );
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
      ar & idx & name & comment & uri & way_type & admin_list & house_number_left & house_number_right & edges;
    }

private:
      nt::GeographicalCoord get_geographical_coord(const std::vector< HouseNumber>&, const int);
      nt::GeographicalCoord extrapol_geographical_coord(int);
};


/** Un bout d'itinéraire :
        un nom de voie et une liste de segments */
struct PathItem {
    nt::idx_t way_idx = nt::invalid_idx; //< Way of this path item
    boost::posix_time::time_duration duration = {}; //< Length of the journey on this item
    std::deque<nt::GeographicalCoord> coordinates;//< path item coordinates
    int angle = 0; //< Angle with the next PathItem (needed to give direction)

    enum class TransportCaracteristic {
        Walk,
        Bike,
        Car,
        BssTake, //when a bike is taken
        BssPutBack //when a bike is put back
    };
    TransportCaracteristic transportation = TransportCaracteristic::Walk;

    double get_length() const {
        switch (transportation) {
        case TransportCaracteristic::BssPutBack:
        case TransportCaracteristic::BssTake:
            return 0;
        case TransportCaracteristic::Walk:
            //milliseconds to reduce rounding
            return duration.total_milliseconds() * (default_speed[type::Mode_e::Walking]) / 1000;
        case TransportCaracteristic::Bike:
            return duration.total_milliseconds() * (default_speed[type::Mode_e::Bike]) / 1000;
        case TransportCaracteristic::Car:
            return duration.total_milliseconds() * (default_speed[type::Mode_e::Car]) / 1000;
        default:
            throw navitia::exception("unhandled transportation case");
        }
    }
};

/** Itinéraire complet */
struct Path {
    boost::posix_time::time_duration duration = {}; //< Longueur totale du parcours
    std::deque<PathItem> path_items = {}; //< Liste des voies parcourues
};

class ProjectionData;

/** Rectangle utilisé par RTree pour indexer spatialement les communes */
struct Rect{
    double min[2];
    double max[2];
    Rect() : min{0,0}, max{0,0} {}

    Rect(const type::GeographicalCoord & coord) {
        min[0] = coord.lon();
        min[1] = coord.lat();
        max[0] = coord.lon();
        max[1] = coord.lat();
    }

    Rect(double a_minX, double a_minY, double a_maxX, double a_maxY){
        min[0] = a_minX;
        min[1] = a_minY;

        max[0] = a_maxX;
        max[1] = a_maxY;
    }
};

//Forward declarations
struct POI;
struct POIType;
/** Structure contenant tout ce qu'il faut savoir sur le référentiel de voirie */
struct GeoRef {

    ///Liste des POIType et POI
    std::vector<POIType*> poitypes;
    std::map<std::string, nt::idx_t> poitype_map;
    std::vector<POI*> pois;
    std::map<std::string, nt::idx_t> poi_map;
    proximitylist::ProximityList<type::idx_t> poi_proximity_list;
    /// Liste des voiries
    std::vector<Way*> ways;

    std::map<std::string, nt::idx_t> way_map;
    /// données administratives
    std::map<std::string, nt::idx_t> admin_map;
    std::vector<Admin*> admins;

    RTree<nt::idx_t, double, 2> rtree;

    /// Indexe sur les noms de voirie
    autocomplete::Autocomplete<unsigned int> fl_admin;

    /// Indexe sur les noms de voirie
    autocomplete::Autocomplete<unsigned int> fl_way;

    /// Indexe sur les pois
    autocomplete::Autocomplete<unsigned int> fl_poi;

    /// Indexe tous les nœuds
    proximitylist::ProximityList<vertex_t> pl;

    /// for all stop_point, we store it's projection on each graph
    typedef flat_enum_map<nt::Mode_e, ProjectionData> ProjectionByMode;
    std::vector<ProjectionByMode> projected_stop_points = {};

    /// Graphe pour effectuer le calcul d'itinéraire
    Graph graph;

    /*
     * We have 3 graphs :
     *  1/ for walking
     *  2/ for biking
     *  3/ for driving
     *
     *  But 4 transportation mode cf explanation in init()
     */
    flat_enum_map<nt::Mode_e, nt::idx_t> offsets;

    /// number of vertex by transportation mode
    nt::idx_t nb_vertex_by_mode;

    /// Liste des alias
    std::map<std::string, std::string> alias;
    std::map<std::string, std::string> synonymes;
    int word_weight = 5; //Pas serialisé : lu dans le fichier ini

    void init();

    template<class Archive> void save(Archive & ar, const unsigned int) const {
        ar & ways & way_map & graph & offsets & fl_admin & fl_way & pl & projected_stop_points
                & admins & admin_map &  pois & fl_poi & poitypes &poitype_map & poi_map & alias & synonymes & poi_proximity_list
                & nb_vertex_by_mode;
    }

    template<class Archive> void load(Archive & ar, const unsigned int) {
        // La désérialisation d'une boost adjacency list ne vide pas le graphe
        // On avait donc une fuite de mémoire
        graph.clear();
        ar & ways & way_map & graph & offsets & fl_admin & fl_way & pl & projected_stop_points
                & admins & admin_map & pois & fl_poi & poitypes &poitype_map & poi_map & alias & synonymes & poi_proximity_list
                & nb_vertex_by_mode;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    /// Récupération de la liste des admins à partir des coordonnées
    std::vector<navitia::type::idx_t> find_admins(const type::GeographicalCoord &);

    /** Construit l'indexe spatial */
    void build_proximity_list();

    ///  Construit l'indexe autocomplete à partir des rues
    void build_autocomplete_list();

    /// Normalisation des codes externes
    void normalize_extcode_way();
    /// Normalisation des codes externes des admins
    void build_admin_map();

    /// Chargement de la liste map code externe idx sur poitype et poi
    void build_poitypes_map();
    void build_pois_map();

    /// Construit l’indexe spatial permettant de retrouver plus vite la commune à une coordonnées
    void build_rtree();

    /// Recherche d'une adresse avec un numéro en utilisant Autocomplete
    std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> find_ways(const std::string & str, const int nbmax, const int search_type,std::function<bool(nt::idx_t)> keep_element) const;


    /**
     * Project each stop_point on the georef network
     */
    void project_stop_points(const std::vector<type::StopPoint*> & stop_points);
    void build_admins_stop_points(std::vector<type::StopPoint*> & stop_points);
    void build_admins_pois();
    /** project the stop point on all transportation mode
      * return a pair with :
      * - the projected array
      * - a boolean corresponding to the fact that at least one projection has been found
    */
    std::pair<ProjectionByMode, bool> project_stop_point(const type::StopPoint* stop_point) const;

    /** Retourne l'arc (segment) le plus proche
      *
      * Pour le trouver, on cherche le nœud le plus proche, puis pour chaque arc adjacent, on garde le plus proche
      * Ce n'est donc pas optimal, mais pour améliorer ça, il faudrait indexer des segments, ou ratisser plus large
     */

    vertex_t nearest_vertex(const type::GeographicalCoord & coordinates, const proximitylist::ProximityList<vertex_t> &prox) const;
    edge_t nearest_edge(const type::GeographicalCoord &coordinates) const;
    edge_t nearest_edge(const type::GeographicalCoord &coordinates, const proximitylist::ProximityList<vertex_t> &prox) const;
    edge_t nearest_edge(const type::GeographicalCoord &coordinates, type::idx_t offset, const proximitylist::ProximityList<vertex_t>& prox) const;

    edge_t nearest_edge(const type::GeographicalCoord & coordinates, const vertex_t & u) const;

    edge_t nearest_edge(const type::GeographicalCoord & coordinates, type::Mode_e mode, const proximitylist::ProximityList<vertex_t>& prox) const {
        return nearest_edge(coordinates, offsets[mode], prox);
    }

    void add_way(const Way& w);

    ///Add the projected start and end to the path
    void add_projections(Path& p, const ProjectionData& start, const ProjectionData& end) const;

    ///get the transportation mode of the vertex
    type::Mode_e get_mode(vertex_t vertex) const;
    PathItem::TransportCaracteristic get_caracteristic(edge_t edge) const;
    ~GeoRef();
};

/** When given a coordinate, we have to associate it with the street network.
  *
  * This structure handle this.
  *
  * It contains
  *   - 2 possible nodes (each end of the edge where the coordinate has been projected)
  *   - the coordinate of the projection
  *   - the 2 distances between the projected point and the ends (NOTE, this is not the distance between the coordinate and the ends)
  *
  */
struct ProjectionData {

    /// enum used to acces the nodes and the distances
    enum class Direction {
        Source = 0,
        Target,
        size
    };
    /// 2 possible nodes (each end of the edge where the coordinate has been projected)
    flat_enum_map<Direction, vertex_t> vertices;

    /// has the projection been successful?
    bool found = false;

    /// The coordinate projected on the edge
    type::GeographicalCoord projected;

    /// Distance between the projected point and the ends
    flat_enum_map<Direction, double> distances {{{-1, -1}}};

    ProjectionData() {}
    /// Project the coordinate on the graph
    ProjectionData(const type::GeographicalCoord & coord, const GeoRef &sn, const proximitylist::ProximityList<vertex_t> &prox);
    /// Project the coordinate on the graph corresponding to the transportation mode of the offset
    ProjectionData(const type::GeographicalCoord & coord, const GeoRef &sn, type::idx_t offset, const proximitylist::ProximityList<vertex_t> &prox);

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & vertices & projected & distances & found;
    }

    void init(const type::GeographicalCoord & coord, const GeoRef & sn, edge_t nearest_edge);

    /// syntaxic sugar
    vertex_t operator[] (Direction d) const { return vertices[d]; }
};

/** Nommage d'un POI (point of interest). **/
struct POIType : public nt::Nameable, nt::Header{
    const static type::Type_e type = type::Type_e::POIType;
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar &idx &uri &name;
    }

    std::vector<type::idx_t> get(type::Type_e type, const GeoRef & data) const;

    private:
};


/** Nommage d'un POI (point of interest). **/
struct POI : public nt::Nameable, nt::Header{
    const static type::Type_e type = type::Type_e::POI;
    int weight;
    nt::GeographicalCoord coord;
    std::vector<Admin*> admin_list;
    std::map<std::string, std::string> properties;
    nt::idx_t poitype_idx;
    int address_number;
    std::string address_name;

    POI(): weight(0), poitype_idx(type::invalid_idx), address_number(-1){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar &idx & uri &name & weight & coord & admin_list & properties & poitype_idx & visible & address_number & address_name;
    }

    std::vector<type::idx_t> get(type::Type_e type, const GeoRef &) const;

    private:
};

int compute_directions(const navitia::georef::Path& path, const nt::GeographicalCoord& c_coord);

}} //namespace navitia::georef
