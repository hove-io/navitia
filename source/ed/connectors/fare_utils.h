#pragma once
#include "utils/csv.h"
#include "fare/fare.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

/**
 * All kind of elemental utilities to parse fares
 */

namespace fa = navitia::fare;

BOOST_FUSION_ADAPT_STRUCT(
    fa::Condition,
    (std::string, key)
    (fa::Comp_e, comparaison)
    (std::string, value)
)

namespace ed { namespace connectors {

namespace qi = ::boost::spirit::qi;
namespace ph = ::boost::phoenix;

/// Exception levée si on utilise une clef inconnue
struct invalid_key : std::exception{};

/// Exception levée si on n'arrive pas à parser une condition
struct invalid_condition : std::exception {};

inline fa::Condition parse_condition(const std::string & condition_str) {
    std::string str = boost::algorithm::to_lower_copy(condition_str);
    boost::algorithm::replace_all(str, " ", "");
    fa::Condition cond;

    if(str.empty() || str == "true")
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

inline std::vector<fa::Condition> parse_conditions(const std::string & conditions){
    std::vector<fa::Condition> ret;
    std::vector<std::string> string_vec;
    boost::algorithm::split(string_vec, conditions, boost::algorithm::is_any_of("&"));
    for (const std::string & cond_str : string_vec) {
        ret.push_back(parse_condition(cond_str));
    }
    return ret;
}

inline fa::State parse_state(const std::string & state_str){
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

inline void add_in_state_str(std::string& str, std::string key, const std::string& val) {
    if (val.empty())
        return;
    if (! str.empty())
        str = "&";
    str += key + fa::comp_to_string(fa::Comp_e::EQ) + val;
}

inline std::string to_string(fa::State state) {
    std::string str;

    add_in_state_str(str, "line", state.line);
    add_in_state_str(str, "zone", state.zone);
    add_in_state_str(str, "mode", state.mode);
    add_in_state_str(str, "stoparea", state.stop_area);
    add_in_state_str(str, "network", state.network);
    add_in_state_str(str, "ticket", state.ticket);

    if (str.empty()) {
        str = "*";
    }
    return str;
}

inline std::string to_string(fa::OD_key::od_type type) {
    switch (type) {
    case fa::OD_key::od_type::Zone:
        return "Zone";
    case fa::OD_key::od_type::StopArea:
        return "StopArea";
    case fa::OD_key::od_type::Mode:
        return "Mode";
    default:
        throw navitia::exception("unhandled od_type case");
    }
}

inline fa::OD_key::od_type to_od_type(const std::string& key) {
    std::string lower_key = boost::algorithm::to_lower_copy(key);
    if (lower_key == "mode")
        return fa::OD_key::Mode;
    if (lower_key == "zone")
        return fa::OD_key::Zone;
    if (lower_key == "stop" || lower_key == "stoparea")
        return fa::OD_key::StopArea;
    throw navitia::exception("Unable to parse " + key + " as OD_Key");
}


}
}
