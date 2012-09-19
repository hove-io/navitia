#pragma once

#include "routing.h"
#include "type/data.h"
#include <boost/graph/adjacency_list.hpp>

namespace navitia { namespace routing {  namespace timedependent {


struct TimeTableElement {
    ValidityPatternTime first;
    ValidityPatternTime second;
    type::idx_t vj;
    TimeTableElement(ValidityPatternTime departure, ValidityPatternTime arrival, type::idx_t vj) : first(departure), second(arrival), vj(vj){}
    bool operator<(const TimeTableElement & other) const {
        return first < other.first;
    }
};

/// Propriété des arcs : ils contiennent une grille horaire ou un horaire constant
struct TimeTable {
    /// Parfois on a des durées constantes : correspondance, réseau en fréquence; vaut -1 s'il faut utiliser les horaires
    int constant_duration;

    /// Horaires (départ, arrivée) sur l'arc entre deux routePoints
    std::vector<TimeTableElement> time_table;

    /// Ajoute un nouvel horaire à l'arc
    void add(ValidityPatternTime departure, ValidityPatternTime arrival, type::idx_t vj){
        time_table.push_back(TimeTableElement(departure, arrival, vj));
    }

    /** Évalue la prochaine arrivée possible étant donnée une heure d'arrivée
     *
     * Prend en compte le passe-minuit
     */
    DateTime eval(const DateTime &departure, const type::PT_Data & data) const;

    /** Retourne le premier départ possible à une heure donnée */
    std::pair<DateTime, type::idx_t> first_departure(DateTime departure, const type::PT_Data & data) const;

    TimeTable() : constant_duration(-1){}
    TimeTable(int constant_duration) : constant_duration(constant_duration){}
};

struct Edge {
    TimeTable t;
    Edge() : t(-1) {}
    Edge(int duration) : t(duration) {}
};

// Plein de typedefs pour nous simpfilier un peu la vie

/** Définit le type de graph que l'on va utiliser
  *
  * Les arcs sortants et la liste des nœuds sont représentés par des vecteurs
  * les arcs sont orientés
  * les propriétés des arcs sont les classes définies précédemment
  * Les nœuds n'ont pas de propriété
  */
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, Edge> Graph;

/// Représentation d'un nœud dans le graphe
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;

/// Représentation d'un arc dans le graphe
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

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

    size_t stop_point_offset;
    size_t route_point_offset;

    std::vector<vertex_t> preds;
    std::vector<DateTime> distance;
    TimeDependent(const type::Data & global_data);

    /// Génère le graphe sur le quel sera fait le calcul
    void build_graph();

    /** Calcule un itinéraire entre deux stop area
     *
     * hour correspond à
     * day correspond au jour de circulation au départ
     */
    std::vector<Path> compute(type::idx_t dep, type::idx_t arr, int hour, int day, senscompute );

    /** Construit le chemin à retourner à partir du résultat du Dijkstra */
    Path makePath(type::idx_t arr);

    bool is_stop_area(vertex_t vertex) const {return vertex < stop_point_offset;}
    bool is_stop_point(vertex_t vertex) const {return vertex >= stop_point_offset && vertex < route_point_offset;}
    bool is_route_point(vertex_t vertex) const {return vertex >= route_point_offset;}
};

}}}
