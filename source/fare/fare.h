/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "conf.h"
#include "routing/routing.h"
#include "utils/serialization_vector.h"
#include "utils/logger.h"
#include "type/request.pb.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/serialization/utility.hpp>
#include <utility>

namespace navitia {

namespace type {
struct StopPoint;
struct Data;
}  // namespace type

class PbCreator;

namespace fare {

/**
 * Structure to model a possible null cost (boost::optional was not meeting the requirement)
 */
struct Cost {
    int value = 0;
    bool undefined = false;  // with at least one undefined ticket in the label, its cost become undefined

    Cost(int v) : value(v) {}
    Cost() : undefined(true) {}
    Cost(const Cost& c) = default;
    Cost& operator=(const Cost& c) = default;

    Cost operator+(Cost c) const { return c += *this; }

    Cost& operator+=(Cost c) {
        if (c.undefined)
            undefined = true;
        // we continue to update the value even if the cost is undefined (it will be the cost on all defined subcost)
        value += c.value;
        return *this;
    }

    // an undefined cost is always greater than a defined one
    bool operator<(Cost c) const {
        if (undefined)
            return false;
        if (c.undefined)
            return true;
        return value < c.value;
    }
    bool operator==(Cost c) const {
        if (undefined)
            return c.undefined;
        if (c.undefined)
            return false;
        return c.value == value;
    }
    bool operator!=(Cost c) const { return !(*this == c); }
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& value& undefined;
    }
};

inline std::ostream& operator<<(std::ostream& s, const Cost& c) {
    c.undefined ? s << "undef (" << c.value << ")" : s << c.value;
    return s;
}

/// Defines a ticket : name (caption) and price (value)
struct SectionKey;
struct Ticket {
    enum ticket_type { FlatFare, GraduatedFare, ODFare, None };
    std::string key;
    std::string caption;
    std::string currency = "euro";
    Cost value = {0};
    std::string comment;
    ticket_type type;
    std::vector<SectionKey> sections;
    bool is_default_ticket() const { return value.undefined; }

    Ticket() : value(0), type(None) {}
    Ticket(std::string key, std::string caption, int value, std::string comment, ticket_type type = FlatFare)
        : key(std::move(key)), caption(std::move(caption)), value(value), comment(std::move(comment)), type(type) {}

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& key& caption& value& comment& type& currency;
    }
};

std::ostream& operator<<(std::ostream& ss, const Ticket& t);

inline Ticket make_default_ticket() {
    Ticket default_t("unknown_ticket", "unknown ticket", 0, "unknown ticket");
    default_t.value = Cost();  // undefined cost
    return default_t;
}

/// Defines a ticket for a given period
struct PeriodTicket {
    PeriodTicket() = default;
    PeriodTicket(boost::gregorian::date_period p, Ticket t) : validity_period(p), ticket(std::move(t)) {}

    boost::gregorian::date_period validity_period =
        boost::gregorian::date_period(boost::gregorian::date(boost::gregorian::not_a_date_time),
                                      boost::gregorian::date_duration(1));
    Ticket ticket;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& validity_period& ticket;
    }
};

/// Contains a set of fares for every validity dates
struct DateTicket {
    std::vector<PeriodTicket> tickets;

    /// Returns fare for a given date
    Ticket get_fare(boost::gregorian::date date) const;

    /// Add a new period to a ticket
    void add(boost::gregorian::date begin, boost::gregorian::date end, const Ticket& ticket);

    /// Sum of two DateTicket
    /// This funciton assumes that there's the same number of tickets and that their validity period are the same
    DateTicket operator+(const DateTicket& other) const;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& tickets;
    }
};

struct no_ticket {};

/// Defines the current state
struct State {
    /// Last used mode
    std::string mode;

    /// Last used zone
    std::string zone;

    /// Last area of purchase
    std::string stop_area;

    /// Last used line
    std::string line;

    /// Network used
    std::string network;

    /// Ticket used
    std::string ticket;

    State() = default;

    bool operator==(const State& other) const { return this->concat() == other.concat(); }

    bool operator<(const State& other) const { return this->concat() < other.concat(); }

    std::string concat() const {
        std::stringstream res;
        res << mode << "&" << zone << "&" << stop_area << "&" << line << "&" << network << "&" << ticket;
        return res.str();
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& mode& zone& stop_area& line& network& ticket;
    }
};

std::ostream& operator<<(std::ostream& ss, const State& k);

/// Type de comparaison possible entre un arc et une valeur
enum class Comp_e { EQ, NEQ, LT, GT, LTE, GTE, True };

std::string comp_to_string(const Comp_e comp);

/// Define an edge and the condition to take it
struct Condition {
    /// Value to respect
    std::string key;

    /// ticket to buy to take the edge
    /// if empty, it's free
    std::string ticket;

    /// comparison operator
    Comp_e comparaison = Comp_e::True;

    /// Value to compare with
    std::string value;

    std::string to_string() const { return key + comp_to_string(comparaison) + value; }

    Condition() = default;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& key& ticket& comparaison& value;
    }
};

std::ostream& operator<<(std::ostream& ss, const Condition& k);

/// Structure représentant une étiquette
struct Label {
    Cost cost = 0;  //< Coût cummulé
    size_t nb_undefined_sub_cost = 0;
    int start_time = 0;  //< Heure de compostage du billet
    // int duration;//< durée jusqu'à présent du trajet depuis le dernier ticket
    int nb_changes = 0;     //< nombre de changement effectués depuis le dernier ticket
    std::string stop_area;  //< stop_area d'achat du billet
                            // std::string dest_stop_area; //< on est obligé de descendre à ce stop_area
    std::string zone;
    std::string mode;
    std::string line;
    std::string network;

    Ticket::ticket_type current_type = Ticket::FlatFare;

    std::vector<Ticket> tickets;  //< Ensemble de billets à acheter pour arriver à cette étiquette
    /// Constructeur par défaut
    Label() = default;
    bool operator==(const Label& l) const {
        return cost == l.cost && start_time == l.start_time && nb_changes == l.nb_changes && stop_area == l.stop_area
               && zone == l.zone && mode == l.mode && line == l.line && network == l.network;
    }

    bool operator<(const Label& l) const {
        if (nb_undefined_sub_cost != l.nb_undefined_sub_cost)
            return nb_undefined_sub_cost < l.nb_undefined_sub_cost;
        if (cost.value != l.cost.value)
            return cost.value < l.cost.value;
        if (tickets.size() != l.tickets.size()) {
            return tickets.size() < l.tickets.size();
        }

        return nb_changes < l.nb_changes;
    }
};

std::ostream& operator<<(std::ostream& ss, const Label& l);

/// Contient les données retournées par navitia
struct SectionKey {
    std::string network = {};
    std::string start_stop_area = {};
    std::string dest_stop_area = {};
    std::string line = {};
    uint32_t start_time = {};
    uint32_t dest_time = {};
    std::string start_zone = {};
    std::string dest_zone = {};
    std::string mode = {};
    boost::gregorian::date date = {};
    size_t path_item_idx = 0;

    boost::optional<std::string> section_id;

    SectionKey(const routing::PathItem& path_item, const size_t idx);
    SectionKey(const pbnavitia::PtFaresRequest::PtSection& section,
               const type::StopPoint& first_stop_point,
               const type::StopPoint& last_stop_point);

    int duration_at_begin(int ticket_start_time) const;
    int duration_at_end(int ticket_start_time) const;
};

std::ostream& operator<<(std::ostream& ss, const SectionKey& k);

/// Représente un transition possible et l'achat éventuel d'un billet
struct Transition {
    enum class GlobalCondition { nothing, exclusive, with_changes };

    std::vector<Condition> start_conditions;                      //< condition pour emprunter l'arc
    std::vector<Condition> end_conditions;                        //< condition à la sortie de l'arc
    std::string ticket_key;                                       //< clef vers le tarif correspondant
    GlobalCondition global_condition = GlobalCondition::nothing;  //< condition telle que exclusivité ou OD

    bool valid(const SectionKey& section, const Label& label) const;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& start_conditions& end_conditions& ticket_key& global_condition;
    }
};

std::ostream& operator<<(std::ostream& ss, const Transition& k);

struct OD_key {
    enum od_type { Zone, StopArea, Mode };  // NOTE: don't forget to change the bdd enum if this change
    od_type type;
    std::string value;
    OD_key() = default;
    OD_key(od_type type, std::string value) : type(type), value(std::move(value)) {}

    bool operator<(const OD_key& other) const {
        if (value != other.value)
            return value < other.value;
        else
            return type < other.type;
    }
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& type& value;
    }
};

struct results {
    std::vector<Ticket> tickets;
    Cost total;
    bool not_found = true;
};

/// Contient l'ensemble du système tarifaire
struct Fare {
    /// Map qui associe les clefs de tarifs aux tarifs
    std::map<std::string, DateTicket> fare_map;

    std::map<OD_key, std::map<OD_key, std::vector<std::string>>> od_tickets;

    /// Contient le graph des transitions
    using Graph = boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, State, Transition>;
    using vertex_t = boost::graph_traits<Graph>::vertex_descriptor;
    using edge_t = boost::graph_traits<Graph>::edge_descriptor;
    Graph g;
    Fare::vertex_t begin_v{};  // begin vertex descriptor

    Fare();

    /// Effectue la recherche du meilleur tarif
    /// Retourne une liste de billets à acheter
    results compute_fare(const routing::Path& path) const;
    results compute_fare(const pbnavitia::PtFaresRequest::PtJourney& fares, const type::Data& data) const;

    template <class Archive>
    void save(Archive& ar, const unsigned int) const {
        ar& fare_map& od_tickets& g;
    }

    template <class Archive>
    void load(Archive& ar, const unsigned int) {
        // boost adjacency load does not seems to empty the graph, hence there was a memory leak
        g.clear();
        ar& fare_map& od_tickets& g;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    size_t nb_transitions() const;

private:
    /// Retourne le ticket OD qui va bien ou lève une exception no_ticket si on ne trouve pas
    DateTicket get_od(const Label& label, const SectionKey& section) const;

    void add_default_ticket();

    log4cplus::Logger logger = log4cplus::Logger::getInstance("fare");
};

void fill_fares(PbCreator& pb_creator, const pbnavitia::PtFaresRequest& fares);

}  // namespace fare
}  // namespace navitia
