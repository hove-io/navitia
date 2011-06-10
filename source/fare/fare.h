#pragma once

#include "utils/csv.h"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

/// Définit un billet : libellé et tarif
typedef std::pair<std::string, int> ticket_t;

/// Définit un billet pour une période données
typedef std::pair<boost::gregorian::date_period, ticket_t> date_ticket_t;

/// Contient un ensemble de tarif pour toutes les dates de validités
struct DateTicket {
    std::vector<date_ticket_t> tickets;

    /// Retourne le tarif à une date données
    ticket_t get_fare(boost::gregorian::date date);

    /// Ajoute une nouvelle période
    void add(std::string begin_date, std::string end_date, ticket_t ticket);
};

struct no_ticket{};


/// Définit l'état courant
struct State {
    /// Dernier mode utilisé
    std::string mode;

    /// Dernière zone utilisée
    std::string zone;

    /// Dernier endroit où à eu lieu l'achat
    std::string stop_area;

    /// Dernière ligne utilisée
    std::string line;

    /// Réseau utilisé
    std::string network;

    State() {}

    bool operator==(const State & other) const {
        return this->concat() == other.concat();
    }


    bool operator<(const State & other) const {
        return this->concat() < other.concat();
    }

    std::string concat() const {
        return mode + zone + stop_area + line + network;
    }
};


/// Type de comparaison possible entre un arc et une valeur
enum Comp_e { EQ, NEQ, LT, GT, LTE, GTE, True, Exclusive};

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

    Condition() : comparaison(True) {}
};


/// Représente un transition possible et l'achat éventuel d'un billet
struct Transition {
    Condition cond; //< condition pour emprunter l'arc
    std::string ticket_key; //< clef vers le tarif correspondant
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
struct Label;
typedef boost::shared_ptr<Label> Label_ptr;
struct Label {
    int cost; //< Coût cummulé
    Label_ptr pred; //< pointeur vers le prédécesseur pour reconstituer le chemin
    int duration;//< durée jusqu'à présent du trajet
    int nb_changes;//< nombre de changement effectués

    std::vector<ticket_t> tickets; //< Ensemble de billets à acheter pour arriver à cette étiquette
    ///Constructeur par défaut
    Label() : cost(0), duration(0), nb_changes(0) {}
};


/// Contient les données retournées par navitia
struct SectionKey {
    std::string network;
    std::string start_stop_area;
    std::string dest_stop_area;
    std::string line;
    int start_time;
    int dest_time;
    std::string start_zone;
    std::string dest_zone;
    std::string mode;
    boost::gregorian::date date;

    SectionKey(const std::string & key);
    int duration() const;
};

int parse_time(const std::string & time_str);
boost::gregorian::date parse_nav_date(const std::string & date_str);

/// Contient l'ensemble du système tarifaire
struct Fare {
    /// Map qui associe les clefs de tarifs aux tarifs
    std::map<std::string, DateTicket> fare_map;

    /// Charge le fichier
    Fare(const std::string & filename, const std::string & prices_filename);

    /// Charge les tarifs
    void load_fares(const std::string & filename);

    /// Contient le graph des transitions
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, State, Transition > Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;
    Graph g;

    /// Effectue la recherche du meilleur tarif
    /// Retourne une liste de billets à acheter
    std::vector<ticket_t> compute(const std::vector<std::string> & section_keys);

};

/// Retourne vrai s'il est possible d'emprunter un tel arc avec une telle étiquette
bool valid_transition(const Transition & transition, Label_ptr label);

/// Retourne vrai s'il est possible d'atteindre l'état par la section key
bool valid_dest(const State & state, SectionKey section_key);
bool valid_start(const State & state, SectionKey section_key);

/// Wrapper pour pouvoir parser une condition en une seule fois avec boost::spirit::qi
BOOST_FUSION_ADAPT_STRUCT(
    Condition,
    (std::string, key)
    (Comp_e, comparaison)
    (std::string, value)
)
