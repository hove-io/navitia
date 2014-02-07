#include "fare_parser.h"
#include "utils/csv.h"
#include "ed/data.h"
#include "fare_utils.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

/// Wrapper pour pouvoir parser une condition en une seule fois avec boost::spirit::qi
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
    reader.next(); //en-tête

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
        if (data.fare_map.find(transition.ticket_key) == data.fare_map.end()) {
            LOG4CPLUS_WARN(logger, "impossible to find ticket " << transition.ticket_key << ", transition skipped");
//            continue;
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
        // La structure du csv est : clef;date_debut;date_fin;prix;libellé
        greg::date start(greg::from_undelimited_string(row.at(1)));
        greg::date end(greg::from_undelimited_string(row.at(2)));
        data.fare_map[row.at(0)].add(start, end,
                             fa::Ticket(row.at(0), row.at(4), boost::lexical_cast<int>(row.at(3)), row.at(5)) );
    }
}

void fare_parser::load_od() {
    CsvReader reader(od_filename);
    std::vector<std::string> row;
    reader.next(); //en-tête

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
        //col 1 and 3 are the human readable name of the start/end, and are not used

        std::string start_mode = boost::algorithm::trim_copy(row[2]);
        std::string dest_mode = boost::algorithm::trim_copy(row[5]);

        std::vector<std::string> price_keys;
        for (int i = 6; i < row.size(); ++i) {
            std::string price_key = boost::algorithm::trim_copy(row[i]);

            if (price_key.empty())
                continue;

            //coherence check
            if (data.fare_map.find(price_key) == data.fare_map.end()) {
                LOG4CPLUS_WARN(logger, "impossible to find ticket " << price_key << ", od ticket skipped");
//                continue; //do we have to skip the entire OD ?
            }

            price_keys.push_back(price_key);
        }

        fa::OD_key start(to_od_type(start_mode), start_saec), dest(to_od_type(dest_mode), dest_saec);
        data.od_tickets[start][dest] = price_keys;

        count++;
    }
    LOG4CPLUS_INFO(logger, "Nombre de tarifs OD Île-de-France : " << count);
}

}
}
