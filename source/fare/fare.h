#pragma once

#include "utils/csv.h"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/graph/adjacency_list.hpp>

/// Définit l'état courant
struct State {
    /// Dernier ticket utilisé
    std::string ticket;

    /// Durée depuis le dernier ticket acheté (en secondes)
    int duration;

    /// Dernier mode utilisé
    std::string mode;

    /// Dernière zone utilisée
    std::string zone;

    /// Dernier endroit où à eu lieu l'achat
    std::string stop_area;

    std::string line;

    /// Nombre de changements effectués
    int changes;

    /// Réseau utilisé
    std::string network;

    State() : duration(0), changes(0) {}

    bool operator==(const State & other) const {
        return ticket==other.ticket && duration==other.duration && mode == other.mode
                && zone==other.zone && stop_area == other.stop_area && line == other.line
                && changes == other.changes && network == other.network;
    }


    bool operator<(const State & other) const {
        return ticket<other.ticket && duration<other.duration && mode < other.mode
                && zone<other.zone && stop_area<other.stop_area && line<other.line
                && changes<other.changes && network<other.network;
    }
};


/// Type de comparaison possible entre un arc et une valeur
enum Comp_e { EQ, NEQ, LT, GT, LTE, GTE};

/// Définit un arc et les conditions pour l'emprunter
/// Les conditions peuvent être : prendre u
struct Condition {
    /// Valeur à que doit respecter la condition
    std::string key;

    /// Ticket à acheter pour prendre cet arc
    /// Chaîne vide si rien à faire
    std::string ticket;
    
    /// Opérateur de comparaison
    /// Nil s'il n'y a pas de restriction
    Comp_e comparaison;

    /// Valeur à comparer
    std::string value;
};

BOOST_FUSION_ADAPT_STRUCT(
    Condition,
    (std::string, key)
    (Comp_e, comparaison)
    (std::string, value)
)

/// Représente un transition possible et l'achat éventuel d'un billet
struct Transition {
    Condition cond;
    std::string ticket;
    float value;
};

/// Exception levée si on utilise une clef inconnue
struct invalid_key : std::exception{};

/// Parse un état
State parse_state(const std::string & state);

/// Parse une condition de passage
Condition parse_condition(const std::string & condition);

/// Exception levée si on n'arrive pas à parser une condition
struct invalid_condition : std::exception {};

/// Parse une liste de conditions séparés par des &
std::vector<Condition> parse_conditions(const std::string & conditions);

/// Parse une ligne complète
Transition parse_transition(const std::string & transition);

/// Structure représentant une étiquette
struct Label {

};

/// Contient l'ensemble du système tarifaire
struct Fare {
    /// Charge le fichier
    Fare(const std::string & filename);

    /// Contient le graph des transitions
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, State, Transition > Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;
    Graph g;
};

/// Contient les données retournées par navitia
struct SectionKey {
    std::string network;
    std::string start_stop_area;
    std::string dest_stop_area;
    std::string line;
    float start_time;
    float dest_time;
    std::string start_zone;
    std::string dest_zone;

    SectionKey(const std::string & key);
    float duration() const;
};

int parse_time(const std::string & time_str);
