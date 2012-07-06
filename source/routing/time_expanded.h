#pragma once
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include "routing.h"
#include "astar.h"

namespace navitia { namespace routing { namespace timeexpanded {



/// Etiquette servant pour l'algorithme de Dijkstra
struct etiquette {
    //public :
    uint32_t temps;         /// Temps de parcours
    uint32_t heure_arrivee; /// Heure d'arrivée
    uint16_t date_arrivee;  /// Date d'arrivée
    uint8_t correspondances;

    etiquette() : temps(std::numeric_limits<uint32_t>::max()), heure_arrivee(std::numeric_limits<uint32_t>::max()), date_arrivee(std::numeric_limits<uint16_t>::max()), correspondances(std::numeric_limits<uint8_t>::max()) {}

    bool operator==(etiquette e) {return (e.date_arrivee == this->date_arrivee) & (e.heure_arrivee == this->heure_arrivee) & (e.temps == this->temps);}

    bool operator !=(etiquette e) { return !((*this )== e);}

    /// Etiquette maximum
    static etiquette max() {
        etiquette etiquettemax;
        etiquettemax.date_arrivee = std::numeric_limits<uint16_t>::max();
        etiquettemax.heure_arrivee = std::numeric_limits<uint32_t>::max();
        etiquettemax.temps = std::numeric_limits<uint32_t>::max();
        etiquettemax.correspondances = std::numeric_limits<uint8_t>::max();
        return etiquettemax;
    }

    bool operator<(etiquette e) {
        if(this->date_arrivee == e.date_arrivee) {
            if(this->heure_arrivee == e.heure_arrivee) {
                if(this->temps == e.temps)
                    return this->correspondances < e.correspondances;
                else
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
    TC
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
    uint16_t validity_pattern;      /// Validity Pattern de l'arête
    uint16_t temps;                 /// Temps sur l'arc
    bool is_pam;                    /// Vrai si l'arête est un passe minuit, faux sinon
    EdgeDesc() :validity_pattern(0), temps(0), is_pam(false){}
    EdgeDesc(uint16_t validity_pattern) : validity_pattern(validity_pattern), temps(0), is_pam(false){}
    EdgeDesc(uint16_t validity_pattern, bool is_pam) : validity_pattern(validity_pattern), temps(0), is_pam(is_pam){}
    EdgeDesc(uint16_t validity_pattern, uint16_t temps) : validity_pattern(validity_pattern), temps(temps), is_pam(false){}
    EdgeDesc(uint16_t validity_pattern, uint16_t temps, bool is_pam) : validity_pattern(validity_pattern), temps(temps), is_pam(is_pam){}


    std::ostream &operator<<( std::ostream &out) {
        out << "edge : " << " " ;
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




typedef unsigned int idx_t;
typedef boost::property<boost::vertex_index_t, uint32_t> vertex_32;
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                              boost::no_property, EdgeDesc, vertex_32> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;
typedef std::map<idx_t, idx_t> map_tc_t;


/// Fonction retournant  l'idx du sa auquel appartient le noeud
uint32_t get_saidx(unsigned int idx, type::PT_Data&data, Graph & g, map_tc_t &map_tc);






/// Fonction calculant la différence entre le deuxieme et le premier argument
uint16_t calc_diff(uint16_t debut, uint16_t fin);




typedef std::list<idx_t> idx_t_list;
typedef std::map<idx_t, idx_t_list> map_ar_t;
/// Calcul les lignes aller retours
void calculer_ar(type::PT_Data&data, map_ar_t &map_ar);

typedef std::list<idx_t> list_idx;
typedef std::map<idx_t, list_idx> map_routes_t;
/// Trouve les routes A/R
void calculer_AR(type::PT_Data&data, Graph &g, map_routes_t & map_routes);


/// Verifie si une correspondance est valide
bool correspondance_valide(idx_t tav, idx_t tdv, bool pam, Graph &g, type::PT_Data&data, map_routes_t & map_routes, map_tc_t map_tc);


/// Détermine si une arête est une arête de transport
bool est_transport(edge_t e, type::PT_Data&data, Graph & g);

/// Sert pour le calcul du plus court chemin

class combine {
private:
    type::PT_Data&data;
    Graph &g;
    map_tc_t &map_tc;
    idx_t sa_depart, cible;
    uint16_t jour_debut;
public:
    combine(type::PT_Data&data, Graph &g, map_tc_t &map_tc, idx_t sa_depart, idx_t cible, uint16_t jour_debut) : data(data), g(g), map_tc(map_tc), sa_depart(sa_depart), cible(cible), jour_debut(jour_debut) {}


    etiquette operator()(etiquette debut, edge_t e) const;
};



struct found_goal
{
    vertex_t v;
    found_goal(vertex_t v) : v(v) {}
    found_goal() {}
}; // exception for termination



struct TimeExpanded : public AbstractRouter {
    const type::PT_Data & data;
    Graph graph;
    navitia::routing::astar::Astar astar_graph;

    std::vector<type::ValidityPattern> validityPatterns;

    std::vector<vertex_t> predecessors;
    std::vector<etiquette> distances;

    size_t stop_area_offset;
    size_t stop_point_offset;
    size_t route_point_offset;
    size_t ta_offset;
    size_t td_offset;
    size_t tc_offset;


    TimeExpanded(const type::PT_Data &data);


    /// Génère le graphe sur le quel sera fait le calcul
    void build_graph();


    /// Génère le graphe astar
    void build_heuristic(uint stop_area);



    /** Calcule un itinéraire entre deux stop area
     *
     * hour correspond à
     * day correspond au jour de circulation au départ
     */
    Path compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day);


    uint32_t get_saidx(const vertex_t &v) const;

    /** Retourne le temps associé au noeud passé
        Renvoie -1 s'il s'agit d'un SA, SP, RP */
    int32_t get_time(const vertex_t& v) const;

    /// Fonction retournant le type du nœud
    node_type get_n_type(const vertex_t &v) const;

    /// Fonction helper retournant l'idx d'un vertex
    idx_t get_idx(const vertex_t& v) const;


private:

    /// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un sp
    unsigned int get_sp_idx(unsigned int spid) {
        return stop_point_offset + spid;
    }

    /// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un rp
    unsigned int get_rp_idx(unsigned int rpid) {
        return route_point_offset + rpid;
    }

    /// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un ta
    unsigned int get_ta_idx(unsigned int stid) {
        return ta_offset + stid;
    }

    /// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un td
    unsigned int get_td_idx(unsigned int stid) {
        return td_offset + stid;
    }

    /// Fonction helper servant à récupérer l'id dans le tableau des vertices d'un td
    unsigned int get_tc_idx(unsigned int stid) {
        return tc_offset + stid;
    }


    /// Fonction retournant l'idx du validity pattern, si le validity pattern n'existe pas il est ajouté
    int get_validity_pattern_idx(navitia::type::ValidityPattern vp);


    /// Fonction déterminant si une arête est un passe minuit ou non
    bool is_passe_minuit(int32_t debut_t, int32_t fin_t) const;

    /// Fonction renvoyant un validity pattern décalé d'une journée
    navitia::type::ValidityPattern* decalage_pam(const navitia::type::ValidityPattern &vp);



    uint16_t  calc_diff(uint16_t debut, uint16_t fin) {
      int diff = fin - debut;
      return (diff>0) ? diff : 86400 + diff;
    }

     int trouver_premier_tc(idx_t saidx, int depart);


     /// Sert à couper l'algorithme de Dijkstra lorsque le stop area cible est atteint
     class dijkstra_goal_visitor : public boost::default_dijkstra_visitor
     {
     private:
         unsigned int m_goal;
         unsigned int prev;
         const TimeExpanded & te;
     public:

         dijkstra_goal_visitor(vertex_t goal, const TimeExpanded & te) : m_goal(goal), prev(0), te(te)
         {
         }

         template <class Graph>
         void examine_vertex(unsigned int u, Graph &/*g*/)
         {
             if(te.get_n_type(u) == TA)
                 if (te.get_saidx(u) == m_goal){
                 throw found_goal(u);
             }
         }
     };





};

class combine_simple {
private:
    const navitia::type::PT_Data & data;
    const std::vector<type::ValidityPattern> & validityPatterns;

public:
    combine_simple(const navitia::type::PT_Data & data, const std::vector<type::ValidityPattern> & validityPatterns) : data(data), validityPatterns(validityPatterns) {}


    etiquette operator()(etiquette debut, EdgeDesc ed) const;
};


}}}


namespace std {
template <>
class numeric_limits<navitia::routing::timeexpanded::etiquette> {
public:
    static navitia::routing::timeexpanded::etiquette max() {
        navitia::routing::timeexpanded::etiquette e;
        e.date_arrivee = navitia::routing::timeexpanded::etiquette::max().date_arrivee;
        e.heure_arrivee = navitia::routing::timeexpanded::etiquette::max().heure_arrivee;
        e.temps = navitia::routing::timeexpanded::etiquette::max().temps;
        return e;
    }
};
}
