#include <boost/graph/adjacency_list.hpp>

namespace bg = boost::graph;
namespace navitia { namespace streetnetwork {

/** Propriétés Nœud (intersection entre deux routes) */
struct Vertex {
    double lon;
    double lat;
};

/** Propriétés des arcs */
struct Edge {
    unsigned int way_idx; //< indexe vers le nom de rue
    float length; //< longeur en mètres de l'arc
    bool cyclable; //< est-ce que le segment est accessible à vélo ?
    int start_number; //< numéro de rue au début du segment
    int end_number; //< numéro de rue en fin de segment
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


struct Way{
    unsigned int idx;
    std::string name;
    std::string city;
    std::vector<edge_t> edges;
};

struct StreetNetwork {
    std::vector<Way> ways;
    Graph graph;

    void load_bdtopo(std::string filename);

};

}} //namespace navitia::streetnetwork
