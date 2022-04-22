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

#include "fare_parser.h"
#include "utils/csv.h"
#include "utils/base64_encode.h"
#include "utils/functions.h"
#include "ed/data.h"
#include "fare_utils.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace fa = navitia::fare;

namespace greg = boost::gregorian;

namespace ed {
namespace connectors {

void fare_parser::load() {
    load_prices();

    load_transitions();

    load_od();
}

void fare_parser::load_transitions() {
    CsvReader reader(state_transition_filename);
    std::vector<std::string> row;
    reader.next();  // header

    for (row = reader.next(); !reader.eof(); row = reader.next()) {
        bool symetric = false;

        if (row.size() != 6) {
            LOG4CPLUS_ERROR(logger, "Wrongly formated line " << row.size() << " columns, we skip the line");
            continue;
        }

        fa::State start = parse_state(row.at(0));
        fa::State end = parse_state(row.at(1));

        is_valid(start);
        is_valid(end);

        fa::Transition transition;
        transition.start_conditions = parse_conditions(row.at(2));
        transition.end_conditions = parse_conditions(row.at(3));
        std::vector<std::string> global_conditions;
        std::string str_condition = boost::algorithm::trim_copy(row.at(4));
        boost::algorithm::split(global_conditions, str_condition, boost::algorithm::is_any_of("&"));

        for (std::string cond : global_conditions) {
            if (cond == "symetric") {
                symetric = true;
            } else {
                transition.global_condition = ed::connectors::to_global_condition(cond);
            }
        }
        transition.ticket_key = boost::algorithm::trim_copy(row[5]);

        // coherence check
        if (!transition.ticket_key.empty() && data.fare_map.find(transition.ticket_key) == data.fare_map.end()) {
            LOG4CPLUS_WARN(logger, "impossible to find ticket " << transition.ticket_key << ", transition skipped");
            continue;
        }

        for (const navitia::fare::Condition& condition : transition.start_conditions) {
            is_valid(condition);
        }
        for (const navitia::fare::Condition& condition : transition.end_conditions) {
            is_valid(condition);
        }

        data.transitions.emplace_back(start, end, transition);

        if (symetric) {
            fa::Transition sym_transition = transition;
            sym_transition.start_conditions = transition.end_conditions;
            sym_transition.end_conditions = transition.start_conditions;
            data.transitions.emplace_back(start, end, sym_transition);
        }
    }
}

bool fare_parser::is_valid(const navitia::fare::State& state) {
    if (!state.mode.empty()) {
        bool found =
            navitia::contains_if(data.physical_modes, [&](const auto& mode) { return mode->uri == state.mode; });
        if (!found) {
            LOG4CPLUS_WARN(logger, "A transition is valid only for the mode "
                                       << state.mode << " but this mode does not appears in the data.");
            return false;
        }
    }

    if (!state.stop_area.empty()) {
        bool found = navitia::contains_if(data.stop_areas,
                                          [&](const auto& stop_area) { return stop_area->uri == state.stop_area; });
        if (!found) {
            LOG4CPLUS_WARN(logger, "A transition is valid only for the stop_area "
                                       << state.stop_area << " but this stop_area does not appears in the data.");
            return false;
        }
    }

    if (!state.line.empty()) {
        bool found = navitia::contains_if(data.lines, [&](const auto& line) { return line->uri == state.line; });
        if (!found) {
            LOG4CPLUS_WARN(logger, "A transition is valid only for the line "
                                       << state.line << " but this line does not appears in the data.");
            return false;
        }
    }

    if (!state.network.empty()) {
        bool found =
            navitia::contains_if(data.networks, [&](const auto& network) { return network->uri == state.network; });
        if (!found) {
            LOG4CPLUS_WARN(logger, "A transition is valid only for the network "
                                       << state.network << " but this network does not appears in the data.");
            return false;
        }
    }

    if (!state.zone.empty()) {
        bool found = navitia::contains_if(data.stop_points,
                                          [&](const auto& stop_point) { return stop_point->fare_zone == state.zone; });
        if (!found) {
            LOG4CPLUS_WARN(logger, "A transition is valid only for the zone "
                                       << state.zone << " but this zone does not appears in the data.");
            return false;
        }
    }

    return true;
}

bool fare_parser::is_valid(const navitia::fare::Condition& condition) {
    // a condition with no key corresponds to an "always true" condition
    if (condition.key.empty()) {
        return true;
    }
    if (condition.key != "zone" && condition.key != "stoparea" && condition.key != "duration"
        && condition.key != "nb_changes" && condition.key != "ticket" && condition.key != "line") {
        LOG4CPLUS_WARN(logger, "A transition has a condition with an invalid key : \"" << condition.key << "\"");
        return false;
    }

    if (condition.key == "zone") {
        bool found = navitia::contains_if(
            data.stop_points, [&](const auto& stop_point) { return stop_point->fare_zone == condition.value; });
        if (!found) {
            LOG4CPLUS_WARN(logger, "A transition has a condition with the zone "
                                       << condition.value << " but this zone does not appears in the data.");
            return false;
        }
    }
    if (condition.key == "stoparea") {
        bool found = navitia::contains_if(data.stop_areas,
                                          [&](const auto& stop_area) { return stop_area->uri == condition.value; });
        if (!found) {
            LOG4CPLUS_WARN(logger, "A transition has a condition with the stop_area "
                                       << condition.value << " but this stop_area does not appears in the data.");
            return false;
        }
    }

    if (condition.key == "duration") {
        try {
            boost::lexical_cast<int>(condition.value);
        } catch (const boost::bad_lexical_cast&) {
            LOG4CPLUS_WARN(logger, "A transition has a condition with a duration "
                                       << condition.value << " but this string is not parsable as an integer.");
            return false;
        }
    }
    if (condition.key == "nb_changes") {
        if (condition.comparaison != navitia::fare::Comp_e::LT || condition.comparaison != navitia::fare::Comp_e::LTE) {
            LOG4CPLUS_WARN(logger, "A transition has a condition on nb_changes  which is not a < or <= condition. "
                                       << condition.to_string());
            return false;
        }
        try {
            int nb_changes = boost::lexical_cast<int>(condition.value);
            if (nb_changes <= -1) {
                LOG4CPLUS_WARN(logger, "A transition has a condition with a nb_changes equals to "
                                           << condition.value << " which is <= -1 .");
            }
        } catch (const boost::bad_lexical_cast&) {
            LOG4CPLUS_WARN(logger, "A transition has a condition with a nb_changes "
                                       << condition.value << " but this string is not parsable as an integer.");
            return false;
        }
    }

    if (condition.key == "ticket") {
        if (data.fare_map.find(condition.value) == data.fare_map.end()) {
            LOG4CPLUS_WARN(logger, "A transition has a condition with a ticket id "
                                       << condition.value << " but this ticket id does not appear in data.");
            return false;
        }
    }
    if (condition.key == "line") {
        bool found = navitia::contains_if(data.lines, [&](const auto& line) { return line->uri == condition.value; });
        if (!found) {
            LOG4CPLUS_WARN(logger, "A transition has a condition with the line "
                                       << condition.value << " but this line does not appears in the data.");
            return false;
        }
    }
    return true;
}

void fare_parser::load_prices() {
    CsvReader reader(prices_filename);
    for (std::vector<std::string> row = reader.next(); !reader.eof(); row = reader.next()) {
        // csv structure is:
        // key; begin; end; price; name; unknown field :); comment; currency(optional)
        greg::date start(greg::from_undelimited_string(row.at(1)));
        greg::date end(greg::from_undelimited_string(row.at(2)));

        fa::Ticket dated_ticket(row.at(0), row.at(4), boost::lexical_cast<int>(row.at(3)), row.at(6));
        if (row.size() > 7) {
            dated_ticket.currency = row[7];
        }
        data.fare_map[row.at(0)].add(start, end, dated_ticket);
    }
}

void fare_parser::load_od() {
    CsvReader reader(od_filename);
    std::vector<std::string> row;
    reader.next();  // header

    // file format is :
    // Origin ID, Origin name, Origin mode, Destination ID, Destination name, Destination mode, ticket_id, ticket id,
    // .... (with any number of ticket)

    int count = 0;
    std::locale locale;
    for (row = reader.next(); !reader.eof(); row = reader.next()) {
        if (row.size() < 7) {
            LOG4CPLUS_WARN(logger, "wrongly formated OD line : " << boost::algorithm::join(row, ", "));
            continue;
        }
        std::string start_saec = boost::algorithm::trim_copy(row[0]);
        std::string dest_saec = boost::algorithm::trim_copy(row[3]);
        // col 1 and 4 are the human readable name of the start/end, and are not used

        std::string start_mode = boost::algorithm::trim_copy(row[2]);
        std::string dest_mode = boost::algorithm::trim_copy(row[5]);

        std::vector<std::string> price_keys;
        for (size_t i = 6; i < row.size(); ++i) {
            std::string price_key = boost::algorithm::trim_copy(row[i]);

            if (price_key.empty())
                continue;

            // coherence check
            if (data.fare_map.find(price_key) == data.fare_map.end()) {
                LOG4CPLUS_WARN(logger, "impossible to find ticket " << price_key << ", od ticket skipped");
                continue;  // do we have to skip the entire OD ?
            }

            price_keys.push_back(price_key);
        }
        if (price_keys.empty()) {
            LOG4CPLUS_WARN(logger, "no tickets in od, " << start_saec << "->" << dest_saec << ", od ticket skipped");
            continue;
        }

        fa::OD_key start(to_od_type(start_mode), start_saec), dest(to_od_type(dest_mode), dest_saec);

        // zones are not encoded
        if (start.type != fa::OD_key::Zone) {
            start.value = navitia::encode_uri(start.value);
        }

        if (dest.type != fa::OD_key::Zone) {
            dest.value = navitia::encode_uri(dest.value);
        }
        data.od_tickets[start][dest] = price_keys;

        count++;
    }
    LOG4CPLUS_INFO(logger, "Nb OD fares: " << count);
}

}  // namespace connectors
}  // namespace ed
