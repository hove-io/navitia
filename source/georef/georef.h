#pragma once

#include "autocomplete/autocomplete.h"
#include "proximity_list/proximity_list.h"


#include "third_party/RTree/RTree.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <map>
#include "adminref.h"


namespace nt = navitia::type;
namespace nf = navitia::autocomplete;

namespace navitia { namespace georef {

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
    nt::idx_t way_idx; //< indexe vers le nom de rue
    float length; //< longeur en mètres de l'arc
    float time; /// temps en second utilisé pour le VLS

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & way_idx & length & time;
    }
    Edge(nt::idx_t wid, float len) : way_idx(wid), length(len), time(std::numeric_limits<int>::max()){}
    Edge() : way_idx(0), length(0),time(std::numeric_limits<int>::max()){}
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
struct PathItem{
    nt::idx_t way_idx; //< Voie sur laquel porte le bout du trajet
    float length; //< Longueur du trajet effectué sur cette voie
    std::vector<edge_t> segments; //< Segments traversés
    PathItem() : length(0){}
};

/** Itinéraire complet */
struct Path {
    float length; //< Longueur totale du parcours
    std::vector<PathItem> path_items; //< Liste des voies parcourues
    std::deque<nt::GeographicalCoord> coordinates; //< Coordonnées du parcours
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

    /// Pour chaque stop_point, on associe la projection sur le filaire
    std::vector<ProjectionData> projected_stop_points;

    /// Graphe pour effectuer le calcul d'itinéraire
    Graph graph;

    /*
    Nous avons 4 graphes :
        1) pour la gestion de la MAP
        2) pour la gestion de VLS
        3) pour la gestion du vélo
        4) pour la gestion de la voiture
    */
    nt::idx_t vls_offset; // VLS
    nt::idx_t bike_offset; // Vélo
    nt::idx_t car_offset; // voiture

    /// Liste des alias
    std::map<std::string, std::string> alias;
    std::map<std::string, std::string> synonymes;
    int word_weight; //Pas serialisé : lu dans le fichier ini

    GeoRef(): vls_offset(0), bike_offset(0), car_offset(0), word_weight(0){}

    void init_offset(nt::idx_t);

    template<class Archive> void save(Archive & ar, const unsigned int) const {
        ar & ways & way_map & graph & vls_offset & bike_offset & car_offset & fl_admin & fl_way & pl & projected_stop_points & admins &  pois & fl_poi & poitypes &poitype_map & poi_map & alias & synonymes & poi_proximity_list;
    }

    template<class Archive> void load(Archive & ar, const unsigned int) {
        // La désérialisation d'une boost adjacency list ne vide pas le graphe
        // On avait donc une fuite de mémoire
        graph.clear();
        ar & ways & way_map & graph & vls_offset & bike_offset & car_offset & fl_admin &fl_way & pl & projected_stop_points & admins &  pois & fl_poi & poitypes &poitype_map & poi_map & alias & synonymes & poi_proximity_list;
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
    void normalize_extcode_admin();

    /// Chargement de la liste map code externe idx sur poitype et poi
    void build_poitypes();
    void build_pois();

    /// Construit l’indexe spatial permettant de retrouver plus vite la commune à une coordonnées
    void build_rtree();

    /// Recherche d'une adresse avec un numéro en utilisant Autocomplete
    std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> find_ways(const std::string & str, const int nbmax, const int search_type,std::function<bool(nt::idx_t)> keep_element) const;


    /** Projete chaque stop_point sur le filaire de voirie

        Retourne le nombre de stop_points effectivement accrochés
    */
    int project_stop_points(const std::vector<type::StopPoint*> & stop_points);

    /** Calcule le meilleur itinéraire entre deux listes de nœuds
     *
     * Le paramètre zeros indique la distances (en mètres) de chaque nœud de départ. Il faut qu'il y ait autant d'éléments que dans starts
     * Si la taille ne correspond pas, on considère une distance de 0
     */
    Path compute(std::vector<vertex_t> starts, std::vector<vertex_t> destinations, std::vector<double> start_zeros = std::vector<double>(), std::vector<double> dest_zeros = std::vector<double>()) const;

    /// Calcule le meilleur itinéraire entre deux coordonnées
    Path compute(const type::GeographicalCoord & start_coord, const type::GeographicalCoord & dest_coord) const;

    /** Retourne l'arc (segment) le plus proche
      *
      * Pour le trouver, on cherche le nœud le plus proche, puis pour chaque arc adjacent, on garde le plus proche
      * Ce n'est donc pas optimal, mais pour améliorer ça, il faudrait indexer des segments, ou ratisser plus large
     */

    edge_t nearest_edge(const type::GeographicalCoord &coordinates) const;
    edge_t nearest_edge(const type::GeographicalCoord &coordinates, const proximitylist::ProximityList<vertex_t> &prox) const;
    vertex_t nearest_vertex(const type::GeographicalCoord & coordinates, const proximitylist::ProximityList<vertex_t> &prox) const;

    edge_t nearest_edge(const type::GeographicalCoord & coordinates, const vertex_t & u) const;
    /** Initialise les structures nécessaires à dijkstra
     *
     * Attention !!! Modifie distances et predecessors
     **/
    void init(std::vector<float> & distances, std::vector<vertex_t> & predecessors) const;

    /** Lance un calcul de dijkstra sans initaliser de structure de données
     *
     * Attention !!! Modifie distances et predecessors
     **/
    template<class Visitor>
    void dijkstra(vertex_t start, std::vector<float> & distances, std::vector<vertex_t> & predecessors, Visitor visitor) const{
        predecessors[start] = start;
        boost::two_bit_color_map<> color(boost::num_vertices(this->graph));
        boost::dijkstra_shortest_paths_no_init(this->graph, start, &predecessors[0], &distances[0],

                                               boost::get(&Edge::length, this->graph), // weigth map
                                               boost::identity_property_map(),
                                               std::less<float>(), boost::closed_plus<float>(),
                                               0,
                                               visitor,
                                               color
                                               );

    }

    /// Reconstruit un itinéraire à partir de la destination et la liste des prédécesseurs
    Path build_path(vertex_t best_destination, std::vector<vertex_t> preds) const;
    void add_way(const Way& w);

    ~GeoRef();
};

// Exception levée dès que l'on trouve une destination
struct DestinationFound{};

// Visiteur qui lève une exception dès qu'une des cibles souhaitées est atteinte
struct target_visitor : public boost::dijkstra_visitor<> {
    const std::vector<vertex_t> & destinations;
    target_visitor(const std::vector<vertex_t> & destinations) : destinations(destinations){}
    void finish_vertex(vertex_t u, const Graph&){
        if(std::find(destinations.begin(), destinations.end(), u) != destinations.end())
            throw DestinationFound();
    }
};


/** Lorsqu'on a une coordonnée, il faut l'accrocher au filaire. Cette structure contient l'accroche
  *
  * Elle consiste en deux nœuds possibles (les extrémités du segment où a été projeté la coordonnée)
  * La coordonnée projetée
  * Les distances entre le projeté les les nœuds
  */
struct ProjectionData {
    /// deux nœuds possibles (les extrémités du segment où a été projeté la coordonnée)
    vertex_t source;
    vertex_t target;

    /// Est-ce que la projection a réussi ?
    bool found;

    /// La coordonnée projetée sur le segment
    type::GeographicalCoord projected;

    /// Distance entre le projeté et les nœuds du segment
    double source_distance;
    double target_distance;

    ProjectionData() : found(false), source_distance(-1), target_distance(-1){}
    /// Initialise la structure à partir d'une coordonnée et d'un graphe sur lequel on projette
    ProjectionData(const type::GeographicalCoord & coord, const GeoRef &sn, const proximitylist::ProximityList<vertex_t> &prox);
    /// Incrémentation des noeuds suivant le mode de transport au début et à la fin : marche, vélo ou voiture
    void inc_vertex(const vertex_t);    
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & source & target & projected & source_distance & target_distance & found;
    }
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
    nt::idx_t poitype_idx;

    POI(): weight(0), poitype_idx(type::invalid_idx){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar &idx & uri &name & weight & coord & admin_list & poitype_idx;
    }

    std::vector<type::idx_t> get(type::Type_e type, const GeoRef &) const;

    private:
};

}} //namespace navitia::streetnetwork
