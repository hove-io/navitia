#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>

namespace bg = boost::graph;
namespace navitia { namespace streetnetwork {

/** Propriétés Nœud (intersection entre deux routes) */
struct Vertex {
    double lon;
    double lat;

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & lon & lat;
    }
};

/** Propriétés des arcs */
struct Edge {
    unsigned int way_idx; //< indexe vers le nom de rue
    float length; //< longeur en mètres de l'arc
    bool cyclable; //< est-ce que le segment est accessible à vélo ?
    int start_number; //< numéro de rue au début du segment
    int end_number; //< numéro de rue en fin de segment

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & way_idx & length & cyclable & start_number & end_number;
    }
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
    std::vector< std::pair<vertex_t, vertex_t> > edges;


    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & idx & name & city & edges;
    }
};

struct StreetNetwork {
    std::vector<Way> ways;
    Graph graph;

    void load_bdtopo(std::string filename);

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & ways & graph;
    }


    void save(const std::string & filename);
    void load(const std::string & filename);

    void save_bin(const std::string & filename);
    void load_bin(const std::string & filename);

    void load_flz(const std::string & filename);
    void save_flz(const std::string & filename);

};

}} //namespace navitia::streetnetwork
