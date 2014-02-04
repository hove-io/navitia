#include "fare_parser.h"
#include "utils/csv.h"
#include "ed/data.h"

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
BOOST_FUSION_ADAPT_STRUCT(
    fa::Condition,
    (std::string, key)
    (fa::Comp_e, comparaison)
    (std::string, value)
)

namespace greg = boost::gregorian;
namespace qi = boost::spirit::qi;
namespace ph = boost::phoenix;

namespace ed { namespace connectors {

void fare_parser::load() {
    load_transitions();

    load_prices();

    load_od();
}

void fare_parser::parse_files(Data& data){
    parse_prices(data);
    parse_trasitions(data);
    parse_origin_destinations(data);
}

std::vector<fa::Condition> parse_conditions(const std::string & conditions){
    std::vector<fa::Condition> ret;
    std::vector<std::string> string_vec;
    boost::algorithm::split(string_vec, conditions, boost::algorithm::is_any_of("&"));
    for (const std::string & cond_str : string_vec) {
        ret.push_back(parse_condition(cond_str));
    }
    return ret;
}

fa::State parse_state(const std::string & state_str){
    fa::State state;
    if (state_str == "" || state_str == "*")
        return state;
    for (fa::Condition cond : parse_conditions(state_str)) {
        if(cond.comparaison != fa::Comp_e::EQ) throw invalid_key();
        if(cond.key == "line"){
            if(state.line != "") throw invalid_key();
            state.line = cond.value;
        }
        else if(cond.key == "zone"){
            if(state.zone != "") throw invalid_key();
            state.zone = cond.value;
        }
        else if(cond.key == "mode"){
            if(state.mode != "") throw invalid_key();
            state.mode = cond.value;
        }
        else if(cond.key == "stoparea"){
            if(state.stop_area != "") throw invalid_key();
            state.stop_area = cond.value;
        }
        else if(cond.key == "network"){
            if(state.network != "") throw invalid_key();
            state.network = cond.value;
        }
        else if(cond.key == "ticket"){
            if(state.ticket != "") throw invalid_key();
            state.ticket = cond.value;
        }
        else{
            throw invalid_key();
        }
    }

    return state;
}

fa::Condition parse_condition(const std::string & condition_str) {
    std::string str = boost::algorithm::to_lower_copy(condition_str);
    boost::algorithm::replace_all(str, " ", "");
    fa::Condition cond;

    if(str.empty())
        return cond;

    // Match du texte : du alphanumérique et quelques chars spéciaux
    qi::rule<std::string::iterator, std::string()> txt = +(qi::alnum|qi::char_("_:-"));

    // Tous les opérateurs que l'on veut matcher et leur valeur associée
    qi::rule<std::string::iterator, fa::Comp_e()> operator_r = qi::string("<=")[qi::_val = fa::Comp_e::LTE]
                                                         | qi::string(">=")[qi::_val = fa::Comp_e::GTE]
                                                         | qi::string("!=")[qi::_val = fa::Comp_e::NEQ]
                                                         | qi::string("<") [qi::_val = fa::Comp_e::LT]
                                                         | qi::string(">") [qi::_val = fa::Comp_e::GT]
                                                         | qi::string("=")[qi::_val = fa::Comp_e::EQ];

    // Une condition est de la forme "txt op txt"
    qi::rule<std::string::iterator, fa::Condition()> condition_r = txt >> operator_r >> txt ;

    std::string::iterator begin = str.begin();
    std::string::iterator end = str.end();

    // Si on n'arrive pas à tout parser
    if(!qi::phrase_parse(begin, end, condition_r, boost::spirit::ascii::space, cond) || begin != end) {
        std::cout << "impossible de parser la condition " << condition_str << std::endl;
        throw invalid_condition();
    }
    return cond;
}


void fare_parser::load_transitions() {
    CsvReader reader(state_transition_filename);
    std::vector<std::string> row;

    // Associe un état à un nœud du graph
    std::map<fa::State, fa::Fare::vertex_t> state_map;
    fa::State begin; // Le début est un nœud vide
    fa::Fare::vertex_t begin_v = boost::add_vertex(begin, data.g);
    state_map[begin] = begin_v;
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
               transition.global_condition = cond;
           }
        }
        transition.ticket_key = boost::algorithm::trim_copy(row[5]);

        fa::Fare::vertex_t start_v, end_v;
        if(state_map.find(start) == state_map.end()){
            start_v = boost::add_vertex(start, data.g);
            state_map[start] = start_v;
        }
        else start_v = state_map[start];

        if(state_map.find(end) == state_map.end()) {
            end_v = boost::add_vertex(end, data.g);
            state_map[end] = end_v;
        }
        else end_v = state_map[end];

        boost::add_edge(start_v, end_v, transition, data.g);
        if (symetric) {
           fa::Transition sym_transition = transition;
           sym_transition.start_conditions = transition.end_conditions;
           sym_transition.end_conditions = transition.start_conditions;
           boost::add_edge(start_v, end_v, sym_transition, data.g);
        }
    }
}

void fare_parser::load_prices() {
    CsvReader reader(prices_filename);
    for (std::vector<std::string> row = reader.next() ; ! reader.eof() ; row = reader.next()) {
        // La structure du csv est : clef;date_debut;date_fin;prix;libellé
        data.fare_map[row.at(0)].add(row.at(1), row.at(2),
                             fa::Ticket(row.at(0), row.at(4), boost::lexical_cast<int>(row.at(3)), row.at(5)) );
    }
}

void fare_parser::load_od() {
    if (use_stif_format)
        load_od_stif(); //TODO delete that, only one format, this has to be done in fusio
    else
        load_od_classic();
}

void fare_parser::load_od_classic() {
    CsvReader reader(od_filename);
    reader.next(); //en-tête

    int count = 0;
    for (auto row = reader.next(); !reader.eof(); row = reader.next()) {
        std::string start_saec = boost::algorithm::trim_copy(row[0]);
        boost::algorithm::to_lower(start_saec);
        std::string dest_saec = boost::algorithm::trim_copy(row[2]);
        boost::algorithm::to_lower(dest_saec);

        std::vector<std::string> price_keys;
        std::string price_key = boost::algorithm::trim_copy(row[4]);
        price_keys.push_back(price_key);

        fa::OD_key start = fa::OD_key(fa::OD_key::StopArea, start_saec);
        fa::OD_key dest = fa::OD_key(fa::OD_key::StopArea, dest_saec);
        data.od_tickets[start][dest] = price_keys;
        count++;
    }
    LOG4CPLUS_INFO(logger, "Nombre de tarifs OD : " << count);
}

void fare_parser::load_od_stif() {
    CsvReader reader(od_filename);
    std::vector<std::string> row;
    reader.next(); //en-tête

    int count = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()) {
        std::string start_saec = boost::algorithm::trim_copy(row[0]);
        std::string dest_saec = boost::algorithm::trim_copy(row[2]);


        std::vector<std::string> price_keys;
        std::string price_key = boost::algorithm::trim_copy(row[4]);
        price_keys.push_back(price_key);

        price_key = boost::algorithm::trim_copy(row[5]);
        if(price_key != "")
            price_keys.push_back(price_key);

        price_key = boost::algorithm::trim_copy(row[6]);
        if(price_key != "")
            price_keys.push_back(price_key);

        price_key = boost::algorithm::trim_copy(row[7]);
        if(price_key != "")
            price_keys.push_back(price_key);

        fa::OD_key start, dest;
        if(start_saec != "8775890")
        {
            start = fa::OD_key(fa::OD_key::StopArea, start_saec);
            if(dest_saec != "8775890")
            {
                data.od_tickets[start][fa::OD_key(fa::OD_key::StopArea, dest_saec)] = price_keys;
            }
            else
            {
                data.od_tickets[start][fa::OD_key(fa::OD_key::Mode, "metro")] = price_keys;
                data.od_tickets[start][fa::OD_key(fa::OD_key::Zone, "1")] = price_keys;
            }
        }
        else
        {
            dest = fa::OD_key(fa::OD_key::StopArea, dest_saec);
            if(start_saec != "8775890")
            {
                data.od_tickets[fa::OD_key(fa::OD_key::StopArea, start_saec)][dest] = price_keys;
            }
            else
            {
                data.od_tickets[fa::OD_key(fa::OD_key::Mode, "metro")][dest] = price_keys;
                data.od_tickets[fa::OD_key(fa::OD_key::Zone, "1")][dest] = price_keys;
            }
        }

        count++;
    }
    LOG4CPLUS_INFO(logger, "Nombre de tarifs OD Île-de-France : " << count);
}

void fare_parser::parse_trasitions(Data& data){
    CsvReader reader(state_transition_filename);
    std::vector<std::string> row;
    int count = 0;
    std::string cle_ticket;

    reader.next(); //en-tête

    for (row = reader.next(); !reader.eof(); row = reader.next()) {

        if (row.size() != 6) {
            LOG4CPLUS_ERROR(logger, "Wrongly formated line " << row.size() << " columns, we skip the line");
            continue;
        }

        ed::types::Transition* trans = new ed::types::Transition();
        count++;
        trans->idx = count;
        trans->before_change = row.at(0);
        trans->after_change = row.at(1);
        trans->start_trip = row.at(2);
        trans->end_trip = row.at(3);
        trans->global_condition = row.at(4);

        cle_ticket = row.at(5);
        if (!cle_ticket.empty()){
            auto result = data.price_map.find(cle_ticket);
            if (result != data.price_map.end()){
                trans->price_idx = result->second;
            }
        }
        data.transitions.push_back(trans);
    }
    LOG4CPLUS_INFO(logger, "Nombre de lignes dans transition : " << count);
}

void fare_parser::parse_prices(Data& data){
    CsvReader reader(prices_filename);
    std::vector<std::string> row;
    int count = 0;


    for (row = reader.next() ; ! reader.eof() ; row = reader.next()) {
        ed::types::Price* price = new ed::types::Price();
        count++;
        price->idx = count;
        price->cle_ticket = row.at(0);
        price->valid_from = greg::date(greg::from_undelimited_string(row.at(1)));
        price->valid_to = greg::date(greg::from_undelimited_string(row.at(2)));
        price->ticket_price = boost::lexical_cast<int>(row.at(3));
        price->ticket_title = row.at(4);
        data.prices.push_back(price);
        data.price_map[price->cle_ticket] = price->idx;
    }
    LOG4CPLUS_INFO(logger, "Nombre de lignes dans prix.esv : " << count);
}

void fare_parser::parse_origin_destinations(Data& data){
    CsvReader reader(od_filename);
    std::vector<std::string> row;
    reader.next(); //en-tête
    std::string price_number;

    int count = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()) {
        ed::types::Origin_Destination* od = new ed::types::Origin_Destination();
        count++;
        od->idx = count;
        od->code_uic_depart = row.at(0);
        od->gare_depart = row.at(1);
        od->code_uic_arrival = row.at(2);
        od->gare_arrival = row.at(3);

        price_number = row.at(4);
        if (!price_number.empty()){
            auto result = data.price_map.find(price_number);
            if (result != data.price_map.end()){
                od->price_idx1 = result->second;
            }
        }
        price_number = row.at(5);
        if (!price_number.empty()){
            auto result = data.price_map.find(price_number);
            if (result != data.price_map.end()){
                od->price_idx2 = result->second;
            }
        }
        price_number = row.at(6);
        if (!price_number.empty()){
            auto result = data.price_map.find(price_number);
            if (result != data.price_map.end()){
                od->price_idx3 = result->second;
            }
        }
        price_number = row.at(7);
        if (!price_number.empty()){
            auto result = data.price_map.find(price_number);
            if (result != data.price_map.end()){
                od->price_idx4 = result->second;
            }
        }
        od->delta_zone = row.at(8);
        data.origin_destinations.push_back(od);
    }
    LOG4CPLUS_INFO(logger, "Nombre de tarifs OD Île-de-France : " << count);

}


}
}
