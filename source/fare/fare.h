#pragma once

#include "config.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include "routing/routing.h"

namespace navitia { namespace fare {

/// Définit un billet : libellé et tarif
struct SectionKey;
struct Ticket {
    enum ticket_type {FlatFare, GraduatedFare, ODFare, None};
    std::string key;
    std::string caption;
    int value;
    std::string comment;
    ticket_type type;
    std::vector<SectionKey> sections;

    Ticket() : value(0), type(None) {}
    Ticket(const std::string & key, const std::string & caption, int value, const std::string & comment, ticket_type type = FlatFare) :
        key(key), caption(caption), value(value), comment(comment), type(type){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & key & caption & value & comment & type /*& sections*/; //CHECK, il me semble qu'on a pas besoin de serializer les sections car le ticket est forcement vide dans le graph
    }
};

/// Définit un billet pour une période données
typedef std::pair<boost::gregorian::date_period, Ticket> date_ticket_t;

/// Contient un ensemble de tarif pour toutes les dates de validités
struct DateTicket {
    std::vector<date_ticket_t> tickets;

    /// Retourne le tarif à une date données
    Ticket get_fare(boost::gregorian::date date) const;

    /// Ajoute une nouvelle période
    void add(std::string begin_date, std::string end_date, Ticket ticket);

    /// Somme deux tickets, suppose qu'il y a le même nombre de billet et que les dates sont compatibles
    DateTicket operator+(const DateTicket & other) const;

    template<class Archive> void serialize(Archive & /*ar*/, const unsigned int) {
//        ar & tickets; TODO,pas le temps de regarder pourquoi les boost::period ralent
    }
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

    /// Ticket utilisé
    std::string ticket;

    State() {}

    bool operator==(const State & other) const {
        return this->concat() == other.concat();
    }

    bool operator<(const State & other) const {
        return this->concat() < other.concat();
    }

    std::string concat() const {
        return mode + zone + stop_area + line + network + ticket;
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & mode & zone & stop_area & line & network & ticket;
    }
};


/// Type de comparaison possible entre un arc et une valeur
enum class Comp_e { EQ, NEQ, LT, GT, LTE, GTE, True};

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

    Condition() : comparaison(Comp_e::True) {}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & key & ticket & comparaison & value;
    }
};


/// Structure représentant une étiquette
struct Label {
    int cost; //< Coût cummulé
    int start_time; //< Heure de compostage du billet
    //int duration;//< durée jusqu'à présent du trajet depuis le dernier ticket
    int nb_changes;//< nombre de changement effectués depuis le dernier ticket
    std::string stop_area; //< stop_area d'achat du billet
   // std::string dest_stop_area; //< on est obligé de descendre à ce stop_area
    std::string zone;
    std::string mode;
    std::string line;
    std::string network;

    Ticket::ticket_type current_type;

    std::vector<Ticket> tickets; //< Ensemble de billets à acheter pour arriver à cette étiquette
    ///Constructeur par défaut
    Label() : cost(0), start_time(0), nb_changes(0), current_type(Ticket::FlatFare) {}
    bool operator==(const Label & l) const {
        return cost==l.cost && start_time==l.start_time && nb_changes==l.nb_changes &&
                stop_area==l.stop_area && zone==l.zone && mode == l.mode &&
                line == l.line && network == l.network;
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & cost & start_time & nb_changes & stop_area & zone & mode & line & network & current_type & tickets;
    }
};

/// Contient les données retournées par navitia
struct SectionKey {
    std::string network;
    std::string start_stop_area;
    std::string dest_stop_area;
    std::string line;
    uint32_t start_time;
    uint32_t dest_time;
    std::string start_zone;
    std::string dest_zone;
    std::string mode;
    boost::gregorian::date date;

    SectionKey(const routing::PathItem& path_item);
    int duration_at_begin(int ticket_start_time) const;
    int duration_at_end(int ticket_start_time) const;
};

/// Représente un transition possible et l'achat éventuel d'un billet
struct Transition {
    std::vector<Condition> start_conditions; //< condition pour emprunter l'arc
    std::vector<Condition> end_conditions; //< condition à la sortie de l'arc
    std::string ticket_key; //< clef vers le tarif correspondant
    std::string global_condition; //< condition telle que exclusivité ou OD

    std::string dump() const {
        return ticket_key + "_" + global_condition;
    }
    bool valid(const SectionKey & section, const Label & label) const;

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & start_conditions & end_conditions & ticket_key & global_condition;
    }
};


struct OD_key{
    enum od_type {Zone, StopArea, Mode};
    od_type type;
    std::string value;
    OD_key() {}
    OD_key(od_type type, std::string value) : type(type), value(value){}

    bool operator<(const OD_key & other) const {
        if (value != other.value)
            return value < other.value ;
        else
            return type < other.type;
    }
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & type & value;
    }
};

struct results {
    std::vector<Ticket> tickets;
};

/// Contient l'ensemble du système tarifaire
struct Fare {
    /// Map qui associe les clefs de tarifs aux tarifs
    std::map<std::string, DateTicket> fare_map;

    std::map< OD_key, std::map<OD_key, std::vector<std::string> > > od_tickets;

    /// Contient le graph des transitions
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, State, Transition > Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;
    Graph g;

    /// Effectue la recherche du meilleur tarif
    /// Retourne une liste de billets à acheter
    results compute_fare(const routing::Path& path) const;

    template<class Archive> void save(Archive & ar, const unsigned int) const {
        ar & fare_map & od_tickets & g;
    }

    template<class Archive> void load(Archive & ar, const unsigned int) {
        // boost adjacency load does not seems to empty the graph, hence there was a memory leak
        g.clear();
        ar & fare_map & od_tickets & g;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
private:
    /// Retourne le ticket OD qui va bien ou lève une exception no_ticket si on ne trouve pas
    DateTicket get_od(Label label, SectionKey section) const;

    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
};


}
}
