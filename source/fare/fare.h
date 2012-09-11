#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

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
};

/// Définit un billet pour une période données
typedef std::pair<boost::gregorian::date_period, Ticket> date_ticket_t;

/// Contient un ensemble de tarif pour toutes les dates de validités
struct DateTicket {
    std::vector<date_ticket_t> tickets;

    /// Retourne le tarif à une date données
    Ticket get_fare(boost::gregorian::date date);

    /// Ajoute une nouvelle période
    void add(std::string begin_date, std::string end_date, Ticket ticket);

    /// Somme deux tickets, suppose qu'il y a le même nombre de billet et que les dates sont compatibles
    DateTicket operator+(DateTicket & other);
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
};


/// Type de comparaison possible entre un arc et une valeur
enum Comp_e { EQ, NEQ, LT, GT, LTE, GTE, True};

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
    std::string section;

    SectionKey(const std::string & key);
    int duration_at_begin(int ticket_start_time) const;
    int duration_at_end(int ticket_start_time) const;
};

/// Représente un transition possible et l'achat éventuel d'un billet
struct Transition {
    std::vector<Condition> start_conditions; //< condition pour emprunter l'arc
    std::vector<Condition> end_conditions; //< condition à la sortie de l'arc
    std::string ticket_key; //< clef vers le tarif correspondant
    std::string global_condition; //< condition telle que exclusivité ou OD

    std::string csv_string; //< Ligne brute dans le CSV
    bool valid(const SectionKey & section, const Label & label) const;
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

/// Parse l'heure au format hh|mm
int parse_time(const std::string & time_str);

/// Parse la date au format AAAA|MM|JJ
boost::gregorian::date parse_nav_date(const std::string & date_str);

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
};

/// Contient l'ensemble du système tarifaire
struct Fare {
    /// Map qui associe les clefs de tarifs aux tarifs
    std::map<std::string, DateTicket> fare_map;

    std::map< OD_key, std::map<OD_key, std::vector<std::string> > > od_tickets;

    /// Charge les deux fichiers obligatoires
    void init(const std::string & filename, const std::string & prices_filename);

    /// Charge les tarifs
    void load_fares(const std::string & filename);

    /// Contient le graph des transitions
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, State, Transition > Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;
    Graph g;

    /// Effectue la recherche du meilleur tarif
    /// Retourne une liste de billets à acheter
    std::vector<Ticket> compute(const std::vector<std::string> & section_keys);

    /// Charge le fichier d'OD générique
    void load_od(const std::string & filename);

    /// Charge le fichier d'OD du stif
    void load_od_stif(const std::string & filename);

    /// Retourne le ticket OD qui va bien ou lève une exception no_ticket si on ne trouve pas
    DateTicket get_od(Label label, SectionKey section);
};


