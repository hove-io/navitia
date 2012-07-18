#pragma once

#include "first_letter/first_letter.h"
#include "proximity_list/proximity_list.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <map>

namespace nt = navitia::type;
namespace navitia { namespace streetnetwork {

/** Propriétés Nœud (intersection entre deux routes) */
struct Vertex {
    type::GeographicalCoord coord;

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & coord;
    }
};

/** Propriétés des arcs */
struct Edge {
    nt::idx_t way_idx; //< indexe vers le nom de rue
    float length; //< longeur en mètres de l'arc
    bool cyclable; //< est-ce que le segment est accessible à vélo ?
    int start_number; //< numéro de rue au début du segment
    int end_number; //< numéro de rue en fin de segment

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & way_idx & length & cyclable & start_number & end_number;
    }

    Edge() : way_idx(0), length(0), cyclable(false), start_number(-1), end_number(-1){}
};

// Plein de typedefs pour nous simpfilier un peu la vie

/** Définit le type de graph que l'on va utiliser
  *
  * Les arcs sortants et la liste des nœuds sont représentés par des vecteurs
  * les arcs sont orientés
  * les propriétés des nœuds et arcs sont les classes définies précédemment
  */
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Vertex, Edge> Graph;

/// Représentation d'un nœud dans le graphe
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;

/// Représentation d'un arc dans le graphe
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

/// Type Itérateur sur les nœuds du graphe
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iterator;

/// Type itérateur sur les arcs du graphe
typedef boost::graph_traits<Graph>::edge_iterator edge_iterator;

/** Nommage d'une voie (anciennement "adresse"). Typiquement le nom de rue **/
struct Way{
    nt::idx_t idx;
    std::string name;
    std::string city;
    nt::idx_t city_idx;
    std::vector< std::pair<vertex_t, vertex_t> > edges;


    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & idx & name & city & edges;
    }
};

/** Un bout d'itinéraire : un nom de voie et une liste de segments */
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
    std::vector<nt::GeographicalCoord> coordinates; //< Coordonnées du parcours
};

/** Structure contenant tout ce qu'il faut savoir sur le référentiel de voirie */
struct StreetNetwork {
    /// Liste des voiries
    std::vector<Way> ways;

    /// Indexe sur les noms de voirie
    FirstLetter<unsigned int> fl;

    /// Indexe
    ProximityList<vertex_t> pl;

    /// Graphe pour effectuer le calcul d'itinéraire
    Graph graph;

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & ways & graph & fl & pl;
    }

    /** Construit l'indexe spatial */
    void build_proximity_list();

    void save(const std::string & filename);
    void load(const std::string & filename);

    void load_flz(const std::string & filename);
    void save_flz(const std::string & filename);

    /** Calcule le meilleur itinéraire entre deux listes de nœuds
     *
     * Le paramètre zeros indique la distances (en mètres) de chaque nœud de départ. Il faut qu'il y ait autant d'éléments que dans starts
     * Si la taille ne correspond pas, on considère une distance de 0
     */
    Path compute(std::vector<vertex_t> starts, std::vector<vertex_t> destinations, std::vector<double> start_zeros = std::vector<double>(), std::vector<double> dest_zeros = std::vector<double>());

    /// Calcule le meilleur itinéraire entre deux coordonnées
    Path compute(const type::GeographicalCoord & start_coord, const type::GeographicalCoord & dest_coord);

    /** Retourne l'arc (segment) le plus proche
      *
      * Pour le trouver, on cherche le nœud le plus proche, puis pour chaque arc adjacent, on garde le plus proche
      * Ce n'est donc pas optimal, mais pour améliorer ça, il faudrait indexer des segments, ou ratisser plus large
      */
    edge_t nearest_edge(const type::GeographicalCoord & coordinates) const;

    /** On définit les coordonnées de départ, un proximitylist et un rayon
     *
     * Retourne tous les idx atteignables dans ce rayon, ainsi que la distance en suivant le filaire de voirie
     **/
    std::vector< std::pair<type::idx_t, double> > find_nearest(const type::GeographicalCoord & start_coord, const ProximityList<type::idx_t> & pl, double radius);

private :
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
    void dijkstra(vertex_t start, std::vector<float> & distances, std::vector<vertex_t> & predecessors, Visitor visitor){
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
};


/** Lorsqu'on a une coordonnée, il faut l'accrocher au filaire. Cette structure contient l'accroche
  *
  * Elle consiste en deux nœuds possibles (les extrémités du segment où a été projeté la coordonnée)
  * La coordonnée projetée
  * Les distances entre le projeté les les nœuds
  */
struct ProjectionData {
    /// deux nœuds possibles (les extrémités du segment où a été projeté la coordonnée)
    std::vector<vertex_t> vertices;

    /// Segment sur lequel a été projeté la coordonnée
    edge_t edge;

    /// La coordonnée projetée sur le segment
    type::GeographicalCoord projected;

    /// Distance entre le projeté et les nœuds du segment
    std::vector<double> distances;

    /// Initialise la structure à partir d'une coordonnée et d'un graphe sur lequel on projette
    ProjectionData(const type::GeographicalCoord & coord, const StreetNetwork &sn);
};

/** Permet de construire un graphe de manière simple

    C'est essentiellement destiné aux tests unitaires
  */
struct GraphBuilder{
    /// Graphe que l'on veut construire
    StreetNetwork & street_network;

    /// Associe une chaine de caractères à un nœud
    std::map<std::string, vertex_t> vertex_map;

    /// Le constructeur : on précise sur quel graphe on va construire
    GraphBuilder(StreetNetwork & street_network) : street_network(street_network){}

    /// Ajoute un nœud, s'il existe déjà, les informations sont mises à jour
    GraphBuilder & add_vertex(std::string node_name, float x, float y);

    /// Ajoute un arc. Si un nœud n'existe pas, il est créé automatiquement
    /// Si la longueur n'est pas précisée, il s'agit de la longueur à vol d'oiseau
    GraphBuilder & add_edge(std::string source_name, std::string target_name, float length = -1);

    /// Surchage de la création de nœud pour plus de confort
    GraphBuilder & operator()(std::string node_name, float x, float y){ return add_vertex(node_name, x, y);}

    /// Surchage de la création de nœud pour plus de confort
    GraphBuilder & operator()(std::string node_name, navitia::type::GeographicalCoord geo){ return add_vertex(node_name, geo.x, geo.y);}

    /// Surchage de la création d'arc pour plus de confort
    GraphBuilder & operator()(std::string source_name, std::string target_name, float length = -1){ return add_edge(source_name, target_name, length);}

    /// Retourne le nœud demandé, jette une exception si on ne trouve pas
    vertex_t get(const std::string & node_name);

    /// Retourne l'arc demandé, jette une exception si on ne trouve pas
    edge_t get(const std::string & source_name, const std::string & target_name);
};

/** Projette un point sur un segment

   Retourne les coordonnées projetées et la distance au segment

   Si le point projeté tombe en dehors du segment, alors ça tombe sur le nœud le plus proche

   http://paulbourke.net/geometry/pointline/
   */
std::pair<type::GeographicalCoord, float> project(type::GeographicalCoord point, type::GeographicalCoord segment_start, type::GeographicalCoord segment_end);

}} //namespace navitia::streetnetwork
