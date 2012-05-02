#pragma once
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#define BOOST_GRAPH_USE_NEW_CSR_INTERFACE
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include "type.h"
#include <stdint.h>

typedef unsigned int idx_t;

struct etiquette {
    //public :
    uint32_t temps;
    uint32_t arrivee;
    bool commence;
    idx_t cible;


    bool operator==(etiquette e) {return (e.arrivee == this->arrivee) & (e.temps == this->temps) & (e.commence == this->commence);}

    static etiquette max() {
        etiquette etiquettemax;
        etiquettemax.arrivee = 3600*24+1;
        etiquettemax.temps = 3600*24+1;
        etiquettemax.commence = true;

        return etiquettemax;
    }

    bool operator<(etiquette e) {
        if(this->arrivee == e.arrivee)
            return this->temps < e.temps;
        else
            return this->arrivee < e.arrivee;
    }
};


struct etiquette2 {
    //public :
    uint32_t temps, heure_arrivee;
    uint16_t date_arrivee;
    bool commence;


    bool operator==(etiquette2 e) {return (e.date_arrivee == this->date_arrivee) & (e.heure_arrivee == this->heure_arrivee) & (e.temps == this->temps) & (e.commence == this->commence);}

    bool operator !=(etiquette2 e) { return !((*this )== e);}

    static etiquette2 max() {
        etiquette2 etiquettemax;
        etiquettemax.date_arrivee = std::numeric_limits<uint16_t>::max();
        etiquettemax.heure_arrivee = std::numeric_limits<uint32_t>::max();
        etiquettemax.temps = std::numeric_limits<uint32_t>::max();
        etiquettemax.commence = true;
        return etiquettemax;
    }

    bool operator<(etiquette2 e) {
        if(this->date_arrivee == e.date_arrivee) {
            if(this->heure_arrivee == e.heure_arrivee) {
                return this->temps < e.temps;
            }
            else
                return this->heure_arrivee < e.heure_arrivee;
        } else return this->date_arrivee < e.date_arrivee;
    }
};



namespace network {
enum node_type {
    sa,
    sp,
    rp,
    ta,
    td,
    tc,
    descente,
    arrivee,
    ho,
    voidt
};

enum edge_type {
    tatd, //0
    tdta, //1
    tatc, //2
    tctd, //3
    sptc, //4
    tcsp, //5
    spsa, //6
    sasp, //7
    spho, //8
    hosp, //9
    hoho, //10
    hoi,  //11
    spta, //12
    tdtd, //13
    tatdc,//14
    sprp, //15
    rpta, //16
    autre //17
};

class VertexDesc {
public:
    VertexDesc(unsigned int idx_,unsigned int vpidx_,  node_type nt_) : idx(idx_), saidx(vpidx_), nt(nt_){}
    VertexDesc(unsigned int idx_, node_type nt_) : idx(idx_), saidx(0), nt(nt_){}
    VertexDesc() : idx(0), saidx(0), nt(voidt){}

    idx_t idx, saidx;
    node_type nt;


    bool operator==(VertexDesc vx) { return (vx.idx == idx )& (vx.nt == nt);}

    std::ostream &operator<<( std::ostream &out) {
        out << nt << " " << idx;
        return out;
    }

    bool operator<(VertexDesc vx) { return this->idx < vx.idx;}

};

class EdgeDesc {
public:
    idx_t idx;
    uint32_t debut;
    uint32_t fin;
    edge_type e_type;
    idx_t cible;

    EdgeDesc() : idx(0), e_type(autre), cible(0){}
    EdgeDesc(idx_t idx_) : idx(idx_), e_type(autre) {}
    EdgeDesc(idx_t idx_,  edge_type e_type) : idx(idx_), debut(0), fin(0), e_type(e_type), cible(0){}
    EdgeDesc(idx_t idx_, uint32_t debut_, uint32_t fin_) :  idx(idx_), debut(debut_), fin(fin_),e_type(tatd), cible(0){}
    EdgeDesc(idx_t idx_, uint32_t debut_, uint32_t fin_, edge_type e_type) :  idx(idx_), debut(debut_), fin(fin_),e_type(e_type), cible(0){}
    EdgeDesc(idx_t idx_, uint32_t debut_, uint32_t fin_, edge_type e_type, idx_t cible) :  idx(idx_), debut(debut_), fin(fin_), e_type(e_type),cible(cible){}
    std::ostream &operator<<( std::ostream &out) {
        out << "edge : " << e_type << " " << idx;
        return out;
    }
};

struct edge_less{
    bool operator()(etiquette a, etiquette b) const {
        return a < b;
    }
    bool operator ()(EdgeDesc, etiquette) const{return false;}
};

enum edge_type2 {
    geo, //geographique
    cor, //correspondance
    tra //transport
};

enum node_type2 {
    SA,
    SP,
    RP,
    TA,
    TD,
    VOIDN
};

class VertexDesc2 {
public:
    VertexDesc2(unsigned int idx_) : idx(idx_){}
    VertexDesc2() : idx(0) {}

    idx_t idx;

    bool operator==(VertexDesc2 vx) { return vx.idx == idx ;}

    std::ostream &operator<<( std::ostream &out) {
        out << " " << idx;
        return out;
    }

    bool operator<(VertexDesc2 vx) { return this->idx < vx.idx;}

};


class EdgeDesc2 {
public:
    uint32_t debut, fin;
    uint16_t validity_pattern;
    bool transport;

    EdgeDesc2() : debut(0), fin(0), validity_pattern(0), transport(false){}
    EdgeDesc2(idx_t debut, idx_t fin, uint16_t validity_pattern, bool transport) : debut(debut), fin(fin), validity_pattern(validity_pattern), transport(transport){}

    std::ostream &operator<<( std::ostream &out) {
        out << "edge : " << " " << debut << " " << fin;
        return out;
    }
};


struct edge_less2{
    bool operator()(etiquette2 a, etiquette2 b) const {
        return a < b;
    }
    bool operator ()(EdgeDesc2, etiquette2) const{return false;}
};




typedef boost::property<boost::edge_weight_t, EdgeDesc> DistanceProperty;

typedef boost::property<boost::vertex_index_t, uint32_t> vertex_32;


typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
VertexDesc,
EdgeDesc, vertex_32> NW;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
VertexDesc2,
EdgeDesc2, vertex_32> NW2;

typedef boost::graph_traits<NW>::vertex_descriptor vertex_t;
typedef boost::graph_traits<NW2>::vertex_descriptor vertex_t2;

typedef boost::graph_traits<NW>::edge_descriptor edge_t;
typedef boost::graph_traits<NW2>::edge_descriptor edge_t2;


typedef boost::property_map<NW, idx_t EdgeDesc::*>::type idxp;
typedef boost::property_map<NW, idx_t VertexDesc::*>::type idxv;




typedef boost::compressed_sparse_row_graph<
boost::directedS,
VertexDesc,
EdgeDesc
> NWCSR;
typedef boost::graph_traits < NWCSR >::vertex_descriptor vertex_descriptorcsr;




unsigned int get_sp_idx(unsigned int spid, navitia::type::Data &data);
unsigned int get_rp_idx(unsigned int rpid, navitia::type::Data &data);
unsigned int get_ta_idx(unsigned int stid, navitia::type::Data &data);
unsigned int get_td_idx(unsigned int stid, navitia::type::Data &data);
unsigned int get_tc_idx(unsigned int stid, navitia::type::Data &data);
unsigned int get_ho_idx(unsigned int stid, navitia::type::Data &data);
idx_t get_idx(unsigned int idx, navitia::type::Data &data);
uint32_t get_saidx(unsigned int idx, navitia::type::Data &data);
node_type2 get_n_type(unsigned int idx, navitia::type::Data &data);
bool is_passe_minuit(uint32_t debut, uint32_t fin, navitia::type::Data &data);
int get_validity_pattern_idx(navitia::type::ValidityPattern vp, navitia::type::Data &data);
navitia::type::ValidityPattern* decalage_pam(navitia::type::ValidityPattern &vp);

void charger_graph4(navitia::type::Data &data, NW &g);
void charger_graph5(navitia::type::Data &data, NW2 &g);

etiquette calc_fin3(etiquette debut, EdgeDesc ed);

etiquette calc_fin4(etiquette debut, EdgeDesc ed);


class combine2 {
private:
    navitia::type::Data &data;
    NW2 &g;
    idx_t cible;
    uint16_t jour_debut;
public:
    combine2(navitia::type::Data &data, NW2 &g, idx_t cible, uint16_t jour_debut) : data(data), g(g), cible(cible), jour_debut(jour_debut) {}

    etiquette2 operator()(etiquette2 debut, EdgeDesc2 ed) const  {
        if(debut == etiquette2::max()) {
            return etiquette2::max();
        }
        else {
            etiquette2 retour;
            uint32_t debut_temps = 0, fin_temps = 0;


            if(get_n_type(ed.debut, data) == TA) {
                debut_temps = data.pt_data.stop_times.at(get_idx(ed.debut, data)).arrival_time % 86400;
                fin_temps = data.pt_data.stop_times.at(get_idx(ed.fin, data)).departure_time % 86400;
            } else if(get_n_type(ed.debut, data) == TD) {
                debut_temps = data.pt_data.stop_times.at(get_idx(ed.debut, data)).departure_time % 86400;
                if(get_n_type(ed.fin, data) == TD) {
                    fin_temps = data.pt_data.stop_times.at(get_idx(ed.fin, data)).departure_time % 86400;
                }
                else {
                    fin_temps = data.pt_data.stop_times.at(get_idx(ed.fin, data)).arrival_time % 86400;
                }
            }

            if(!debut.commence) {
                if(!ed.transport) {
                    if((get_n_type(ed.debut, data) == TD) & (get_n_type(ed.fin, data) == TD)) {
                        return etiquette2::max();
                    } else {
                        retour.date_arrivee = debut.date_arrivee;
                        retour.heure_arrivee = debut.heure_arrivee;
                        retour.commence = false;
                        retour.temps = 0;
                        return retour;
                    }
                } else {
                    if(debut_temps > fin_temps) {
                        retour.date_arrivee = debut.date_arrivee + 1;
                        retour.temps = 86400 - debut.heure_arrivee + fin_temps;
                        retour.heure_arrivee = 0;
                    } else {
                        retour.date_arrivee = debut.date_arrivee;
                        retour.temps = debut.temps + fin_temps - debut_temps;
                        retour.heure_arrivee = debut.heure_arrivee;
                    }
                    if(data.pt_data.validity_patterns.at(ed.validity_pattern).check(retour.date_arrivee)
                            & (retour.heure_arrivee < debut_temps)) {
                        retour.heure_arrivee = fin_temps;
                        retour.commence = true;
                        return retour;
                    } else {
                        return etiquette2::max();
                    }
                }
            } else {
                //On verifie le validity pattern
                //On ne veut pas "trainer" dans la zone d'arret finale
               if(debut_temps > fin_temps) { //Passe minuit
                    retour.date_arrivee = debut.date_arrivee + 1;
                    retour.temps = 86400 - debut.heure_arrivee + fin_temps;
                    retour.heure_arrivee = 0;
                } else {
                    retour.date_arrivee = debut.date_arrivee;
                    retour.temps = debut.temps + fin_temps - debut_temps;
                }
                if((data.pt_data.validity_patterns.at(ed.validity_pattern).check(retour.date_arrivee) || !ed.transport)
                        & (retour.heure_arrivee < debut_temps)) {
                    retour.heure_arrivee = fin_temps;
                    retour.commence = true;
                    return retour;
                } else {
                    return etiquette2::max();
                }
            }
        }
    }
};



class sort_edges {
private:
    NW &g;
    navitia::type::Data &data;
public:
    sort_edges (NW &g, navitia::type::Data &data) : g(g), data(data) { }
    bool operator() (const vertex_t& v1, const vertex_t& v2) const {

        uint32_t t1 = 0, t2 = 0;

        if(g[v1].nt == ta)
            t1 = data.pt_data.stop_times.at(g[v1].idx).arrival_time;
        if(g[v1].nt == td)
            t1 = data.pt_data.stop_times.at(g[v1].idx).departure_time;


        if(g[v2].nt == ta)
            t2 = data.pt_data.stop_times.at(g[v2].idx).arrival_time;
        if(g[v2].nt == td)
            t2 = data.pt_data.stop_times.at(g[v2].idx).departure_time;


        return (t1 < t2) || ((t1 == t2) & (g[v1].idx < g[v2].idx));
    }

};


class sort_edges2 {
private:
    NW2 &g;
    navitia::type::Data &data;
public:
    sort_edges2 (NW2 &g, navitia::type::Data &data) : g(g), data(data) { }
    bool operator() (const vertex_t2& v1, const vertex_t2& v2) const {

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




class dijkstra_goal_visitor2 : public boost::default_dijkstra_visitor
{
public:

    dijkstra_goal_visitor2(vertex_t goal, navitia::type::Data &data) : m_goal(goal), data(data)
    {
    }

    template <class Graph>
    void finish_vertex(unsigned int u, Graph& g)
    {
        if((g[u].nt == ta) & (g[u].saidx == m_goal)){
            throw found_goal(u);
        }
    }

    //    template <class Edge, class Graph>
    //    void examine_edge(Edge e, Graph &g) {
    //        if(g[boost::target(e, g)].nt == ta) {
    //            if(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(g[boost::target(e, g)].idx).route_point_idx).stop_point_idx).stop_area_idx == m_goal) {
    //                std::cout <<"Je suis ici " << data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(g[boost::target(e, g)].idx).route_point_idx).stop_point_idx).stop_area_idx;
    //                std::cout <<" type : " << g[e].e_type;
    //                std::cout << " et je viens de : " << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(g[boost::source(e, g)].idx).route_point_idx).stop_point_idx).stop_area_idx).name <<std::endl;
    //                prev = boost::source(e, g);
    //            }
    //        }

    //    }


private:
    unsigned int m_goal;
    navitia::type::Data &data;
    unsigned int prev;
};


class dijkstra_goal_visitor3 : public boost::default_dijkstra_visitor
{
public:

    dijkstra_goal_visitor3(vertex_t goal, navitia::type::Data &data) : m_goal(goal), data(data)
    {
    }

    template <class Graph>
    void finish_vertex(unsigned int u, Graph& g)
    {
        if((g[u].nt == ta) & (g[u].saidx == m_goal)){
            std::cout << "Touche ! " << std::flush;
        }
    }

private:
    unsigned int m_goal;
    navitia::type::Data &data;
    unsigned int prev;
};


class dijkstra_goal_visitor4 : public boost::default_dijkstra_visitor
{
public:

    dijkstra_goal_visitor4(vertex_t2 goal, navitia::type::Data &data) : m_goal(goal), data(data)
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


}//end namespace network


namespace std {
template <>
class numeric_limits<etiquette> {
public:
    static etiquette max() {
        etiquette retour ;
        retour.temps = 3600*24+1;
        retour.arrivee = 3600*24+1;
        retour.commence = true;
        return retour;
    }
};
template <>
class numeric_limits<etiquette2> {
public:
    static etiquette2 max() {
        etiquette2 e;
        e.commence = etiquette2::max().commence;
        e.date_arrivee = etiquette2::max().date_arrivee;
        e.heure_arrivee = etiquette2::max().heure_arrivee;
        e.temps = etiquette2::max().temps;
        return e;
    }
};

}
