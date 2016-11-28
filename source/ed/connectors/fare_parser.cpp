/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "fare_parser.h"
#include "utils/csv.h"
#include "utils/base64_encode.h"
#include "ed/data.h"
#include "fare_utils.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

namespace fa = navitia::fare;

namespace greg = boost::gregorian;

namespace ed { namespace connectors {

void fare_parser::load() {
    load_prices();

    load_transitions();

    load_od();
}

void fare_parser::load_transitions() {
    CsvReader reader(state_transition_filename);
    std::vector<std::string> row;
    reader.next(); //header

    for (row = reader.next(); !reader.eof(); row = reader.next()) {
        bool symetric = false;

        if (row.size() != 6) {
            LOG4CPLUS_ERROR(logger, "Wrongly formated line " << row.size() << " columns, we skip the line");
            continue;
        }

        fa::State start = parse_state(row.at(0));
        fa::State end = parse_state(row.at(1));

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

        //coherence check
        if (! transition.ticket_key.empty() && data.fare_map.find(transition.ticket_key) == data.fare_map.end()) {
            LOG4CPLUS_WARN(logger, "impossible to find ticket " << transition.ticket_key << ", transition skipped");
            continue;
        }

        data.transitions.push_back(std::make_tuple(start, end, transition));

        if (symetric) {
            fa::Transition sym_transition = transition;
            sym_transition.start_conditions = transition.end_conditions;
            sym_transition.end_conditions = transition.start_conditions;
            data.transitions.push_back(std::make_tuple(start, end, sym_transition));
        }
    }

}


void fare_parser::load_prices() {
    CsvReader reader(prices_filename);
    for (std::vector<std::string> row = reader.next() ; ! reader.eof() ; row = reader.next()) {
        // csv structure is:
        //key; begin; end; price; name; unknown field :); comment; currency(optional)
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
    reader.next(); //header

    // file format is :
    // Origin ID, Origin name, Origin mode, Destination ID, Destination name, Destination mode, ticket_id, ticket id, .... (with any number of ticket)

    int count = 0;
    for (row=reader.next(); !reader.eof(); row = reader.next()) {

        if (row.size() < 7) {
            LOG4CPLUS_WARN(logger, "wrongly formated OD line : " << boost::algorithm::join(row, ", "));
            continue;
        }
        std::string start_saec = boost::algorithm::trim_copy(row[0]);
        std::string dest_saec = boost::algorithm::trim_copy(row[3]);
        //col 1 and 4 are the human readable name of the start/end, and are not used

        std::string start_mode = boost::algorithm::trim_copy(row[2]);
        std::string dest_mode = boost::algorithm::trim_copy(row[5]);

        std::vector<std::string> price_keys;
        for (size_t i = 6; i < row.size(); ++i) {
            std::string price_key = boost::algorithm::trim_copy(row[i]);

            if (price_key.empty())
                continue;

            //coherence check
            if (data.fare_map.find(price_key) == data.fare_map.end()) {
                LOG4CPLUS_WARN(logger, "impossible to find ticket " << price_key << ", od ticket skipped");
                continue; //do we have to skip the entire OD ?
            }

            price_keys.push_back(price_key);
        }
        if (price_keys.empty()) {
            LOG4CPLUS_WARN(logger, "no tickets in od, " << start_saec << "->" << dest_saec << ", od ticket skipped");
            continue;
        }

        fa::OD_key start(to_od_type(start_mode), start_saec), dest(to_od_type(dest_mode), dest_saec);

        //zones are not encoded
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

}
}
