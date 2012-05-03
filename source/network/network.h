#pragma once
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include "type/type.h"
#include "type/data.h"
typedef unsigned int idx_t;

namespace network {


/// Etiquette servant pour l'algorithme de Dijkstra
struct etiquette {
    //public :
    uint32_t temps;         /// Temps de parcours
    uint32_t heure_arrivee; /// Heure d'arrivée
    uint16_t date_arrivee;  /// Date d'arrivée


    bool operator==(etiquette e) {return (e.date_arrivee == this->date_arrivee) & (e.heure_arrivee == this->heure_arrivee) & (e.temps == this->temps);}

    bool operator !=(etiquette e) { return !((*this )== e);}

    /// Etiquette maximum
    static etiquette max() {
        etiquette etiquettemax;
        etiquettemax.date_arrivee = std::numeric_limits<uint16_t>::max();
        etiquettemax.heure_arrivee = std::numeric_limits<uint32_t>::max();
        etiquettemax.temps = std::numeric_limits<uint32_t>::max();
        return etiquettemax;
    }

    bool operator<(etiquette e) {
        if(this->date_arrivee == e.date_arrivee) {
            if(this->heure_arrivee == e.heure_arrivee) {
                return this->temps < e.temps;
            }
            else
                return this->heure_arrivee < e.heure_arrivee;
        } else return this->date_arrivee < e.date_arrivee;
    }
};


/// Les différents types de nœuds disponibles
enum node_type {
    SA,
    SP,
    RP,
    TA,
    TD,
    VOIDN
};

/// Descripteur de Vertex
class VertexDesc {
public:
    VertexDesc(unsigned int idx_) : idx(idx_){}
    VertexDesc() : idx(0) {}

    idx_t idx; /// Idx du vertex, peut être supprimé

    bool operator==(VertexDesc vx) { return vx.idx == idx ;}

    std::ostream &operator<<( std::ostream &out) {
        out << " " << idx;
        return out;
    }

    bool operator<(VertexDesc vx) { return this->idx < vx.idx;}

};


/// Descripteur d'arête
class EdgeDesc {
public:
    uint32_t debut;                 /// Id ( dans le tableau des vertices ) du noeud de départ de l'arête
    uint32_t fin;                   /// Id ( dans le tableau des vertices ) du noeud de fin de l'arête
    uint16_t validity_pattern;      /// Validity Pattern de l'arête

    EdgeDesc() : debut(0), fin(0), validity_pattern(0){}
    EdgeDesc(idx_t debut, idx_t fin, uint16_t validity_pattern) : debut(debut), fin(fin), validity_pattern(validity_pattern){}

    std::ostream &operator<<( std::ostream &out) {
        out << "edge : " << " " << debut << " " << fin;
        return out;
    }
};


/// Sert à comparer deux étiquettes
struct edge_less{
    bool operator()(etiquette a, etiquette b) const {
        return a < b;
    }
    bool operator ()(EdgeDesc, etiquette) const{return false;}
};


typedef boost::property<boost::edge_weight_t, EdgeDesc> DistanceProperty;
typedef boost::property<boost::vertex_index_t, uint32_t> vertex_32;
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
VertexDesc,
EdgeDesc, vertex_32> NW;

typedef boost::graph_traits<NW>::vertex_descriptor vertex_t;
typedef boost::graph_traits<NW>::edge_descriptor edge_t;

/// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un sp
unsigned int get_sp_idx(unsigned int spid, navitia::type::Data &data);

/// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un rp
unsigned int get_rp_idx(unsigned int rpid, navitia::type::Data &data);

/// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un ta
unsigned int get_ta_idx(unsigned int stid, navitia::type::Data &data);

/// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un td
unsigned int get_td_idx(unsigned int stid, navitia::type::Data &data);

/// Fonction helper retournant l'idx d'un vertex
idx_t get_idx(unsigned int idx, navitia::type::Data &data);

/// Fonction retournant  l'idx du sa auquel appartient le noeud
uint32_t get_saidx(unsigned int idx, navitia::type::Data &data);

/// Fonction retournant le type du nœud
node_type get_n_type(unsigned int idx, navitia::type::Data &data);

/// Fonction déterminant si une arête est un passe minuit ou non
bool is_passe_minuit(uint32_t debut, uint32_t fin, navitia::type::Data &data);

/// Fonction retournant l'idx du validity pattern, si le validity pattern n'existe pas il est ajouté
int get_validity_pattern_idx(navitia::type::ValidityPattern vp, navitia::type::Data &data);

/// Fonction renvoyant un validity pattern décalé d'une journée
navitia::type::ValidityPattern* decalage_pam(navitia::type::ValidityPattern &vp);

/// Remplit le graph passé en paramètre avec les données passées
void charger_graph(navitia::type::Data &data, NW &g);

/// Détermine si une arête est une arête de transport
bool est_transport(EdgeDesc e, navitia::type::Data &data);

/// Sert pour le calcul du plus court chemin
class combine2 {
private:
    navitia::type::Data &data;
    NW &g;
    idx_t sa_depart, cible;
    uint16_t jour_debut;
public:
    combine2(navitia::type::Data &data, NW &g, idx_t sa_depart, idx_t cible, uint16_t jour_debut) : data(data), g(g), sa_depart(sa_depart), cible(cible), jour_debut(jour_debut) {}


    etiquette operator()(etiquette debut, EdgeDesc ed) const;
};

/// Compare deux arêtes entre elles
class sort_edges {
private:
    NW &g;
    navitia::type::Data &data;
public:
    sort_edges (NW &g, navitia::type::Data &data) : g(g), data(data) { }
    bool operator() (const vertex_t& v1, const vertex_t& v2) const {

        uint32_t t1 = 0, t2 = 0;

        if(get_n_type(v1, data) == TA)
            t1 = data.pt_data.stop_times.at(g[v1].idx).arrival_time % 86400;
        if(get_n_type(v1, data) == TD)
            t1 = data.pt_data.stop_times.at(g[v1].idx).departure_time % 86400;


        if(get_n_type(v2, data) == TA)
            t2 = data.pt_data.stop_times.at(g[v2].idx).arrival_time % 86400;
        if(get_n_type(v2, data) == TD)
            t2 = data.pt_data.stop_times.at(g[v2].idx).departure_time % 86400;

        return (t1 < t2) || ((t1 == t2) & (g[v1].idx < g[v2].idx));
    }

};

struct found_goal
{
    vertex_t v;
    found_goal(vertex_t v) : v(v) {}
    found_goal() {}
}; // exception for termination



/// Sert à couper l'algorithme de Dijkstra lorsque le stop area cible est atteint
class dijkstra_goal_visitor : public boost::default_dijkstra_visitor
{
public:

    dijkstra_goal_visitor(vertex_t goal, navitia::type::Data &data) : m_goal(goal), data(data)
    {
    }

    template <class Edge, class Graph>
    void edge_relaxed(Edge e, Graph &g)
    {
        if((get_n_type(target(e, g), data) == TA) & (get_saidx(target(e, g), data) == m_goal)){
            throw found_goal(target(e, g));
        }
    }

private:
    unsigned int m_goal;
    navitia::type::Data &data;
    unsigned int prev;
};


}

/// Sert à pour couper l'algorithme de Dijkstra
namespace std {
template <>
class numeric_limits<network::etiquette> {
public:
    static network::etiquette max() {
        network::etiquette e;
        e.date_arrivee = network::etiquette::max().date_arrivee;
        e.heure_arrivee = network::etiquette::max().heure_arrivee;
        e.temps = network::etiquette::max().temps;
        return e;
    }
};
}
