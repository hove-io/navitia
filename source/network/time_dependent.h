#pragma once

#include "type/pt_data.h"
#include <boost/graph/adjacency_list.hpp>

namespace navitia { namespace routing {

/** On se crée une structure qui représente une date et heure
 *
 * Date : sous la forme d'un numéro de jour à chercher dans le validity pattern
 * Heure : entier en nombre de secondes depuis minuit. S'il dépasse minuit, on fait modulo 24h et on incrémente la date
 *
 * On utilise cette structure pendant le calcul d'itinéaire
 */
struct DateTime {
    // TODO : on pourrait optimiser la conso mémoire en utilisant 8 bits pour la date, et 24 pour l'heure ;)
    int date;
    int hour;

    DateTime() {
        *this = DateTime::infinity();
    }

    bool operator<(DateTime other) const {
        if(other.date == other.date)
            return hour < other.hour;
        else
            return other.date < other.date;
    }

    static DateTime infinity() {
        DateTime dt;
        dt.date = std::numeric_limits<int>::max();
        dt.hour = std::numeric_limits<int>::max();
        return dt;
    }

    void normalize(){
        this->date += this->date / (24*3600);
        this->hour = hour % (24*3600);
    }

    bool operator==(DateTime other) {
        return this->hour == other.hour && this->date == other.date;
    }
};

DateTime operator+(DateTime dt, int seconds);

/** Représente un horaire associé à un validity pattern
 *
 * Il s'agit donc des horaires théoriques
 */
struct ValidityPatternTime {
    type::idx_t vp_idx;
    int hour;

    template<class T>
    bool operator<(T other) const {
        return hour < other.hour;
    }

    void normalize(){
        this->vp_idx += this->vp_idx / 24*3600;
        this->hour = hour % 24*3600;
    }

    ValidityPatternTime() {}
    ValidityPatternTime(int vp_idx, int hour) : vp_idx(vp_idx), hour(hour){
        this->normalize();
    }
};

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

    TimeTable() : constant_duration(-1){}
    TimeTable(int constant_duration) : constant_duration(constant_duration){}
};

struct Edge {
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
struct TimeDependent {

    const type::PT_Data & data;
    Graph graph;

    int stop_area_offset_dep;
    int stop_area_offset_arr;
    int stop_point_offset_dep;
    int stop_point_offset_arr;

    TimeDependent(const type::PT_Data & data);

    void build_graph();

    std::vector<std::string> compute(const type::StopArea & departure, const type::StopArea & arrival);
};

}}

namespace std {
template <>
class numeric_limits<navitia::routing::DateTime> {
public:
    static navitia::routing::DateTime max() {
        return navitia::routing::DateTime::infinity();
    }
};
}
