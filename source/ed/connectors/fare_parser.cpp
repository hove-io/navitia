#include "fare_parser.h"
#include "utils/csv.h"

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
    if(state_str == "" || state_str == "*")
        return state;
    BOOST_FOREACH(fa::Condition cond, parse_conditions(state_str)){
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

    for(row=reader.next(); !reader.eof(); row = reader.next()) {
        bool symetric = false;

        if(row.size() != 6) std::cout << "Ligne sans le bon nombre d'items : " << row.size() << std::endl;

        fa::State start = parse_state(row.at(0));
        fa::State end = parse_state(row.at(1));

        fa::Transition transition;
        transition.csv_string = boost::algorithm::join(row, "; ");
        transition.start_conditions = parse_conditions(row.at(2));
        transition.end_conditions = parse_conditions(row.at(3));
        std::vector<std::string> global_conditions;
        std::string str_condition = boost::algorithm::trim_copy(row.at(4));
        boost::algorithm::split(global_conditions, str_condition, boost::algorithm::is_any_of("&"));

        BOOST_FOREACH(std::string cond, global_conditions){
           if(cond == "symetric"){
               symetric = true;
           }else{
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
        if(symetric){
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
        load_od_stif();
    else
        load_od_classic();
}

void fare_parser::load_od_classic() {
    CsvReader reader(od_filename);
    std::vector<std::string> row;
    reader.next(); //en-tête

    int count = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()) {
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
  //  std::cout << "Nombre de tarifs OD Île-de-France : " << count << std::endl;
    LOG4CPLUS_INFO(logger, "Nombre de tarifs OD Île-de-France : " << count);
}

/*



boost::gregorian::date parse_nav_date(const std::string & date_str){
     std::vector< std::string > res;
   boost::algorithm::split(res, date_str, boost::algorithm::is_any_of("|"));
   if(res.size() != 3)
       throw std::string("Date dans un format non parsable : " + date_str);
   boost::gregorian::date date;
   try{
       date = boost::gregorian::date(boost::lexical_cast<int>(res.at(0)),
                                     boost::lexical_cast<int>(res.at(1)),
                                     boost::lexical_cast<int>(res.at(2)));
   } catch (boost::bad_lexical_cast e){
       throw std::string("Conversion des chiffres dans la date impossible " + date_str);
   }
   return date;
}
void Fare::load_fares(const std::string & filename){
    CsvReader reader(filename);
    std::vector<std::string> row;
    for(row=reader.next();  !reader.eof(); row = reader.next()) {
        // La structure du csv est : clef;date_debut;date_fin;prix;libellé
        fare_map[row.at(0)].add(row.at(1), row.at(2),
                             Ticket(row.at(0), row.at(4), boost::lexical_cast<int>(row.at(3)), row.at(5)) );
    }
}

*/
}
}
