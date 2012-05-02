#pragma once
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/depth_first_search.hpp>

#include "network.h"
#include "data.h"

using namespace network;


typedef boost::graph_traits<NW>::edge_descriptor edge_t;
typedef boost::graph_traits<NWCSR>::vertex_descriptor vertex_t2;
typedef boost::graph_traits<NW2>::edge_descriptor edge_t2;
typedef boost::graph_traits<NW>::out_edge_iterator iterator_t;


typedef boost::property_map<NW, idx_t EdgeDesc::*>::type idxp;
typedef boost::property_map<NW, edge_type EdgeDesc::*>::type ett;
typedef boost::property_map<NW, uint32_t EdgeDesc::*>::type distp;
typedef boost::property_map<NW, uint32_t EdgeDesc::*>::type intp;
typedef boost::property_map<NWCSR, int EdgeDesc::*>::type intp2;



typedef boost::property_map<NW, idx_t VertexDesc::*>::type idxv;
typedef boost::property_map<NW, std::string EdgeDesc::*>::type desce;
typedef boost::property_map<NWCSR, idx_t VertexDesc::*>::type idxv2;

typedef boost::property_map<NW, node_type VertexDesc::*>::type ntt;

void charger_graph(navitia::type::Data &d, NW &g_, std::vector<vertex_t> &vertices);
void charger_graph2(navitia::type::Data &d, NW &g);
void charger_graph3(navitia::type::Data &data, NW &g);

void graph_test(NW &g, std::vector<vertex_t> &vertices);
void debug1(vertex_t &v, NW &g, navitia::type::Data &data);
void debug2(NW &g);
void debug_prochains_horaires(NW &g, vertex_t v, unsigned int heure) ;

void afficher_chemin(vertex_t debut, vertex_t fin, std::vector<vertex_t>& predecessors, std::vector<etiquette> & distances, NW &g);
void afficher_chemin(vertex_t2 debut, vertex_t2 fin, std::vector<vertex_t2>& predecessors, std::vector<etiquette2> & distances, NW2 &g);

//void afficher_chemin2(vertex_t debut, vertex_t fin, std::vector<vertex_t>& predecessors, std::vector<etiquette> & distances, NWCSR &g);


vertex_t accroche_depart(std::list<vertex_t> departs, NW &g);
vertex_t accroche_arrivee(std::list<vertex_t> arrivees, NW &g);



etiquette calc_fin(etiquette debut, EdgeDesc ed);
etiquette calc_fin2(etiquette debut, EdgeDesc ed);


void afficher_horaire(int tps_secs);

bool check_parcours(vertex_t v1, vertex_t v2, std::vector<vertex_t> &predecessors,  std::vector<double> distances/*,navitia::type::Data &data, NW &g*/);






struct label_name {
    NW g;
    NW2 g2;
    navitia::type::Data &data;
    bool type;

    label_name(NW &g, navitia::type::Data &data): g(g), data(data), type(true) {}
    label_name(NW2 &g, navitia::type::Data &data): g2(g), data(data), type(false) {}

    void operator()(std::ostream& oss,vertex_t v) {
        if(type) {
            oss << "[label=\"";
            switch(g[v].nt) {
            case sa : oss << "SA"; break;
            case sp : oss << "SP"; break;
            case rp : oss << "RP"; break;
            case ta : oss << "TA"; break;
            case td : oss << "TD"; break;
            case tc : oss << "TC"; break;
            case descente : oss << "Depart"; break;
            case arrivee : oss << "Arrivee"; break;
            case ho : oss << "Horaire"; break;
            case voidt : oss << "void"; break;
            }

            oss << g[v].idx << "\"]";
        } else {
            oss << "[label=\"";
            switch(get_n_type(v, data)) {
            case SA : oss << "SA "<< data.pt_data.stop_areas.at(v).name; break;
            case SP : oss << "SP"; break;
            case RP : oss << "RP"; break;
            case TA : oss << "TA"<< get_idx(v, data) <<" " << data.pt_data.stop_times.at(get_idx(v, data)).arrival_time; break;
            case TD : oss << "TD"<< get_idx(v, data) <<" " << data.pt_data.stop_times.at(get_idx(v, data)).departure_time; break;

            case VOIDN : oss << "void"; break;
            }

            oss << "\"]";
        }
        }


};


struct edge_namer {
    NW g;
    NW2 g2;
    bool type;

    edge_namer(NW &g): g(g), type(true) {}
    edge_namer(NW2 &g): g2(g), type(false) {}

    void operator()(std::ostream& oss, edge_t2 e) {
        oss << "[label=\"";
        if(g2[e].transport)
            oss << "tra";
        else
            oss << "cor";


        oss <<"\"]";
    }


};



// visitor that terminates when we find the goal

class dijkstra_goal_visitor : public boost::default_dijkstra_visitor
{
public:

    dijkstra_goal_visitor(vertex_t goal) : m_goal(goal)
    {
    }

    template <class Graph>
    void finish_vertex(unsigned int u, Graph&)
    {
        if(u == m_goal)
            throw found_goal();
    }

//    template <class Edge, class Graph>
//    void examine_edge(Edge e, Graph &g) {
//        if(boost::target(e, g) == m_goal) {
//            std::cout <<"Je suis ici et je viens de : " << boost::source(e, g) <<std::endl;
//        }
//    }
private:
    unsigned int m_goal;
};





struct detect_loops : public boost::dfs_visitor<>
{
    detect_loops(navitia::type::Data &data) : data(data) {}
    template <class Edge, class Graph>
    void back_edge(Edge e, const Graph& g) {
        std::cout << g[e].e_type << " " << source(e, g) << "(" << g[source(e, g)].nt << ", " << data.pt_data.route_points.at(data.pt_data.stop_times.at(g[source(e, g)].idx).route_point_idx).stop_point_idx << ")"
                  << " -- "
                  << target(e, g) << "(" << g[target(e, g)].nt << ", " << data.pt_data.route_points.at(data.pt_data.stop_times.at(g[target(e, g)].idx).route_point_idx).stop_point_idx<< ")"<< "\n";
    }
private :
    navitia::type::Data &data;
};

void init_distances_predecessors_map(etiquette debut, std::vector<etiquette> &distances, std::vector<vertex_t> &predecessors,std::vector<vertex_t> &indexes);

void detecter_troncs_communs(std::vector<etiquette> &distances, std::vector<vertex_t> &predecessors);
