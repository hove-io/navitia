#pragma once

#include "routing.h"
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

    DateTime() : date(std::numeric_limits<int>::max()), hour(std::numeric_limits<int>::max()){}

    bool operator<(DateTime other) const {
        if(this->date == other.date)
            return hour < other.hour;
        else
            return this->date < other.date;
    }

    static DateTime infinity() {
        return DateTime();
    }

    void normalize(){
        if(date > 300) std::cout << "on normalise l'infini..." << std::endl;
        this->date += this->hour / (24*3600);
        this->hour = hour % (24*3600);
    }

    bool operator==(DateTime other) {
        return this->hour == other.hour && this->date == other.date;
    }
};

std::ostream & operator<<(std::ostream & os, const DateTime & dt);

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

    ValidityPatternTime() {}
    ValidityPatternTime(int vp_idx, int hour) : vp_idx(vp_idx), hour(hour){}
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
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, boost::property<boost::edge_weight_t, uint> > GraphAStar;

/// Représentation d'un nœud dans le graphe
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;

/// Représentation d'un arc dans le graphe
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;
typedef boost::graph_traits<GraphAStar>::edge_descriptor as_edge_t;

/// Type Itérateur sur les nœuds du graphe
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iterator;

/// Type itérateur sur les arcs du graphe
typedef boost::graph_traits<Graph>::edge_iterator edge_iterator;


bool operator==(const PathItem & a, const PathItem & b);
std::ostream & operator<<(std::ostream & os, const PathItem & b);
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
    GraphAStar astar_graph;

    size_t stop_area_offset;
    size_t stop_point_offset;

    std::vector<vertex_t> preds;
    std::vector<DateTime> distance;
    std::vector<uint> min_time;
    std::vector<DateTime> astar_dist;

    TimeDependent(const type::PT_Data & data);

    /// Génère le graphe sur le quel sera fait le calcul
    void build_graph();

    /** Calcule un itinéraire entre deux stop area
     *
     * hour correspond à
     * day correspond au jour de circulation au départ
     */
    Path compute(type::idx_t dep, type::idx_t arr, int hour, int day);
    std::vector<PathItem> compute_astar(const type::StopArea & departure, const type::StopArea & arr, int hour, int day);

    /** Calcule le temps minimal pour atteindre un nœud
     *
     *  Sert pour la heuristique A*
     */
    void build_heuristic(vertex_t destination);
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
