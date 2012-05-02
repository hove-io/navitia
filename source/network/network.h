#pragma once
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include "type/type.h"
#include "type/data.h"
typedef unsigned int idx_t;

namespace network {


struct etiquette {
    //public :
    uint32_t temps, heure_arrivee;
    uint16_t date_arrivee;
    bool commence;


    bool operator==(etiquette e) {return (e.date_arrivee == this->date_arrivee) & (e.heure_arrivee == this->heure_arrivee) & (e.temps == this->temps) & (e.commence == this->commence);}

    bool operator !=(etiquette e) { return !((*this )== e);}

    static etiquette max() {
        etiquette etiquettemax;
        etiquettemax.date_arrivee = std::numeric_limits<uint16_t>::max();
        etiquettemax.heure_arrivee = std::numeric_limits<uint32_t>::max();
        etiquettemax.temps = std::numeric_limits<uint32_t>::max();
        etiquettemax.commence = true;
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

enum edge_type {
    geo, //geographique
    cor, //correspondance
    tra //transport
};

enum node_type {
    SA,
    SP,
    RP,
    TA,
    TD,
    VOIDN
};

class VertexDesc {
public:
    VertexDesc(unsigned int idx_) : idx(idx_){}
    VertexDesc() : idx(0) {}

    idx_t idx;

    bool operator==(VertexDesc vx) { return vx.idx == idx ;}

    std::ostream &operator<<( std::ostream &out) {
        out << " " << idx;
        return out;
    }

    bool operator<(VertexDesc vx) { return this->idx < vx.idx;}

};


class EdgeDesc {
public:
    uint32_t debut, fin;
    uint16_t validity_pattern;
    bool transport;

    EdgeDesc() : debut(0), fin(0), validity_pattern(0), transport(false){}
    EdgeDesc(idx_t debut, idx_t fin, uint16_t validity_pattern, bool transport) : debut(debut), fin(fin), validity_pattern(validity_pattern), transport(transport){}

    std::ostream &operator<<( std::ostream &out) {
        out << "edge : " << " " << debut << " " << fin;
        return out;
    }
};


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


unsigned int get_sp_idx(unsigned int spid, navitia::type::Data &data);
unsigned int get_rp_idx(unsigned int rpid, navitia::type::Data &data);
unsigned int get_ta_idx(unsigned int stid, navitia::type::Data &data);
unsigned int get_td_idx(unsigned int stid, navitia::type::Data &data);

idx_t get_idx(unsigned int idx, navitia::type::Data &data);
uint32_t get_saidx(unsigned int idx, navitia::type::Data &data);
node_type get_n_type(unsigned int idx, navitia::type::Data &data);
bool is_passe_minuit(uint32_t debut, uint32_t fin, navitia::type::Data &data);
int get_validity_pattern_idx(navitia::type::ValidityPattern vp, navitia::type::Data &data);
navitia::type::ValidityPattern* decalage_pam(navitia::type::ValidityPattern &vp);


void charger_graph(navitia::type::Data &data, NW &g);

class combine2 {
private:
    navitia::type::Data &data;
    NW &g;
    idx_t cible;
    uint16_t jour_debut;
public:
    combine2(navitia::type::Data &data, NW &g, idx_t cible, uint16_t jour_debut) : data(data), g(g), cible(cible), jour_debut(jour_debut) {}

    etiquette operator()(etiquette debut, EdgeDesc ed) const;
};


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

//    template <class Edge, class Graph>
//    void edge_not_relaxed(Edge e, Graph &g)
//    {
//        if((get_n_type(target(e, g), data) == TA) & (get_saidx(target(e, g), data) == m_goal)){
//            throw found_goal(target(e, g));
//        }
//    }

//    template <class Graph>
//    void finish_vertex(unsigned int u, Graph /*&g*/) {
//        if((get_n_type(u, data) == TA) & (get_saidx(u, data) == m_goal)){
//                        throw found_goal(u);
//        }
//    }


private:
    unsigned int m_goal;
    navitia::type::Data &data;
    unsigned int prev;
};

class etape {
public:
    idx_t ligne;
    idx_t descente;
    etape(idx_t ligne, idx_t depart) : ligne(ligne), descente(depart){}

    bool operator==(etape e2);
    bool operator!=(etape e2);

};

class parcours {
public:
    std::list<etape> etapes;

    parcours() {
        etapes = std::list<etape>();
    }

    void ajouter_etape(idx_t ligne, idx_t descente);
    bool operator==(parcours i2);
    bool operator!=(parcours i2);

};

class itineraire {
public :
    uint32_t parcours;
    std::list<idx_t> stop_times;

    itineraire(uint32_t parcours, std::list<idx_t> stop_times) : parcours(parcours), stop_times(stop_times){}
};
}

namespace std {
template <>
class numeric_limits<network::etiquette> {
public:
    static network::etiquette max() {
        network::etiquette e;
        e.commence = network::etiquette::max().commence;
        e.date_arrivee = network::etiquette::max().date_arrivee;
        e.heure_arrivee = network::etiquette::max().heure_arrivee;
        e.temps = network::etiquette::max().temps;
        return e;
    }
};
}
