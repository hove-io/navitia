#pragma once

#include "routing.h"
#include "astar.h"
#include "type/pt_data.h"
#include <boost/graph/adjacency_list.hpp>

namespace navitia { namespace routing {  namespace timedependent {


/// Un nœud représente un RoutePoint
struct Vertex {
};

/// Propriété des arcs : ils contiennent une grille horaire ou un horaire constant
struct TimeTable {
    /// Parfois on a des durées constantes : correspondance, réseau en fréquence; vaut -1 s'il faut utiliser les horaires
    int constant_duration;

    /// Horaires (départ, arrivée) sur l'arc entre deux routePoints
    std::vector< std::pair<ValidityPatternTime, ValidityPatternTime> > time_table;

    /// Ajoute un nouvel horaire à l'arc
    void add(ValidityPatternTime departure, ValidityPatternTime arrival){
        time_table.push_back(std::make_pair(departure, arrival));
    }

    /** Évalue la prochaine arrivée possible étant donnée une heure d'arrivée
     *
     * Prend en compte le passe-minuit
     */
    DateTime eval(DateTime departure, const type::PT_Data & data) const;

    /** Retourne le premier départ possible à une heure donnée */
    DateTime first_departure(DateTime departure, const type::PT_Data & data) const;

    /** Plus courte durée possible de faire */
    int min_duration() const;

    TimeTable() : constant_duration(-1){}
    TimeTable(int constant_duration) : constant_duration(constant_duration){}
};

struct Edge {
    /// Correspond à la meilleure durée possible. On s'en sert pour avoir une borne inférieure de temps
    uint min_duration;

    TimeTable t;
    Edge() : t(-1) {}
    Edge(int duration) : t(duration){}
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

/** Représentation du réseau de transport en commun de type « type-dependent »
 *
 *C'est un modèle où les nœuds représentent les arrêtes (des RoutePoint pour être précis) et les arcs tous les horaires
 *entre ces deux RoutePoints
 *
 *Le calcul d'itinéaire se fait avec un Dijkstra modifié de manière à prendre des fonctions qui calculent l'arrivée au plus tôt
 *comme poids des arcs
 */
struct TimeDependent : public AbstractRouter{

    const type::PT_Data & data;
    Graph graph;
    navitia::routing::astar::Astar astar_graph;

    size_t stop_point_offset;
    size_t route_point_offset;

    std::vector<vertex_t> preds;
    std::vector<DateTime> distance;
    std::vector<DateTime> astar_dist;

    TimeDependent(const type::PT_Data & data);

    /// Génère le graphe sur le quel sera fait le calcul
    void build_graph();

    /// Génère le graphe astar
    void build_heuristic(uint destination);


    /** Calcule un itinéraire entre deux stop area
     *
     * hour correspond à
     * day correspond au jour de circulation au départ
     */
    Path compute(type::idx_t dep, type::idx_t arr, int hour, int day);

    Path compute_astar(type::idx_t dep, type::idx_t arr, int hour, int day);

    Path makePath(type::idx_t arr);

    bool is_stop_area(vertex_t vertex) const;
    bool is_stop_point(vertex_t vertex) const;
    bool is_route_point(vertex_t vertex) const;
};

}}}
