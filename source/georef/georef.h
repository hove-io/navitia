#pragma once

#include "autocomplete/autocomplete.h"
#include "proximity_list/proximity_list.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <map>

namespace nt = navitia::type;
namespace nf = navitia::autocomplete;

namespace navitia { namespace georef {

/** Propriétés Nœud (intersection entre deux routes) */
struct Vertex {
    type::GeographicalCoord coord;

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & coord;
    }
};

/** Propriétés des arcs : (anciennement "segment")*/

struct Edge {
    nt::idx_t way_idx; //< indexe vers le nom de rue
    float length; //< longeur en mètres de l'arc
    bool cyclable; //< est-ce que le segment est accessible à vélo ?

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & way_idx & length & cyclable;
    }

    Edge() : way_idx(0), length(0), cyclable(false){}
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

/// Type Itérateur sur les nœuds du graphe
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iterator;

/// Type itérateur sur les arcs du graphe
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

/** Nommage d'un POI (point of interest). **/
struct POIType : public nt::Nameable, nt::NavitiaHeader{
public:
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar &idx &uri &name;
    }
private:
};


/** Nommage d'un POI (point of interest). **/
struct POI : public nt::Nameable, nt::NavitiaHeader{
public:
    int weight;
    nt::GeographicalCoord coord;
    std::string city;
    nt::idx_t city_idx;
    std::string poitype;
    nt::idx_t poitype_idx;

    POI(): weight(0), city(""), city_idx(-1), poitype(""), poitype_idx(-1){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar &idx & uri & idx &name & weight & coord & city & city_idx & poitype & poitype_idx;
    }

private:
};


/** Nommage d'une voie (anciennement "adresse").
    Typiquement le nom de rue **/
struct Way :public nt::Nameable, nt::NavitiaHeader{
public:
    std::string way_type;
    std::string city;
    nt::idx_t city_idx;
    std::vector< HouseNumber > house_number_left;
    std::vector< HouseNumber > house_number_right;
    std::vector< std::pair<vertex_t, vertex_t> > edges;

    void add_house_number(const HouseNumber&);
    nt::GeographicalCoord nearest_coord(const int, const Graph&);
    int nearest_number(const nt::GeographicalCoord& );
    nt::GeographicalCoord barycentre(const Graph& );
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
      ar & idx & name & comment & uri & way_type & city & city_idx & house_number_left & house_number_right & edges;
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

/** Structure contenant tout ce qu'il faut savoir sur le référentiel de voirie */
struct GeoRef {

    ///Liste des POIType et POI
    std::vector<POIType> poitypes;
    std::map<std::string, nt::idx_t> poitype_map;
    std::vector<POI> pois;
    std::map<std::string, nt::idx_t> poi_map;

    /// Liste des voiries
    std::vector<Way> ways;
    std::map<std::string, nt::idx_t> way_map;

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

    /// Liste des alias
    std::map<std::string, std::string> alias;
    std::map<std::string, std::string> synonymes;
    int word_weight;//(Pas serialisé)


    template<class Archive> void save(Archive & ar, const unsigned int) const {
        ar & ways & way_map & graph & fl_way & pl & projected_stop_points & pois & fl_poi & poitypes &poitype_map & poi_map & alias & synonymes;
    }

    template<class Archive> void load(Archive & ar, const unsigned int) {
        // La désérialisation d'une boost adjacency list ne vide pas le graphe
        // On avait donc une fuite de mémoire
        graph.clear();
        ar & ways & way_map & graph & fl_way & pl & projected_stop_points & pois & fl_poi & poitypes &poitype_map & poi_map & alias & synonymes;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()


    /** Construit l'indexe spatial */
    void build_proximity_list();

    ///  Construit l'indexe autocomplete à partir des rues
    void build_autocomplete_list();

    /// Normalisation des codes externes
    void normalize_extcode_way();
    /// Chargement de la liste map code externe idx
    void build_ways();
    ///Chargement de la liste map code externe idx sur poitype et poi
    void build_poitypes();
    void build_pois();

    /// Recherche d'une adresse avec un numéro en utilisant Autocomplete
    std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> find_ways(const std::string & str, const int nbmax) const;


    /** Projete chaque stop_point sur le filaire de voirie */
    void project_stop_points(const std::vector<type::StopPoint> & stop_points);

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

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & source & target & projected & source_distance & target_distance & found;
    }
};

}} //namespace navitia::streetnetwork
