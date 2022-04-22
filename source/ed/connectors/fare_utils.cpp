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

#include "fare_utils.h"
#include "utils/csv.h"
#include "utils/base64_encode.h"
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

/// Wrapper pour pouvoir parser une condition en une seule fois avec boost::spirit::qi
BOOST_FUSION_ADAPT_STRUCT(navitia::fare::Condition,
                          (std::string, key)(navitia::fare::Comp_e, comparaison)(std::string, value))

namespace ed {
namespace connectors {

invalid_key::~invalid_key() noexcept = default;
invalid_condition::~invalid_condition() noexcept = default;

namespace qi = ::boost::spirit::qi;
namespace ph = ::boost::phoenix;

navitia::fare::Condition parse_condition(const std::string& condition_str) {
    std::string str(condition_str);
    boost::algorithm::replace_all(str, " ", "");
    navitia::fare::Condition cond;

    if (str.empty() || boost::algorithm::to_lower_copy(str) == "true") {
        return cond;
    }

    // Match du texte : du alphanumérique et quelques chars spéciaux
    qi::rule<std::string::iterator, std::string()> txt = +(qi::alnum | qi::char_("_:-"));

    // Tous les opérateurs que l'on veut matcher et leur valeur associée
    using cond_rule = qi::rule<std::string::iterator, navitia::fare::Comp_e()>;
    cond_rule operator_r = qi::string("<=")[qi::_val = navitia::fare::Comp_e::LTE]
                           | qi::string(">=")[qi::_val = navitia::fare::Comp_e::GTE]
                           | qi::string("!=")[qi::_val = navitia::fare::Comp_e::NEQ]
                           | qi::string("<")[qi::_val = navitia::fare::Comp_e::LT]
                           | qi::string(">")[qi::_val = navitia::fare::Comp_e::GT]
                           | qi::string("=")[qi::_val = navitia::fare::Comp_e::EQ];

    // Une condition est de la forme "txt op txt"
    qi::rule<std::string::iterator, navitia::fare::Condition()> condition_r = txt >> operator_r >> txt;

    std::string::iterator begin = str.begin();
    std::string::iterator end = str.end();

    // Si on n'arrive pas à tout parser
    if (!qi::phrase_parse(begin, end, condition_r, boost::spirit::ascii::space, cond) || begin != end) {
        throw invalid_condition("impossible to parse condition " + condition_str);
    }

    std::vector<std::string> to_encode = {"line", "mode", "stoparea", "network"};
    if (std::find(to_encode.begin(), to_encode.end(), cond.key) != to_encode.end()) {
        cond.value = navitia::encode_uri(cond.value);
    }

    return cond;
}

std::vector<navitia::fare::Condition> parse_conditions(const std::string& conditions) {
    std::vector<navitia::fare::Condition> ret;
    std::vector<std::string> string_vec;
    boost::algorithm::split(string_vec, conditions, boost::algorithm::is_any_of("&"));
    for (const std::string& cond_str : string_vec) {
        ret.push_back(parse_condition(cond_str));
    }
    return ret;
}

navitia::fare::State parse_state(const std::string& state_str) {
    navitia::fare::State state;
    if (state_str == "" || state_str == "*")
        return state;
    for (navitia::fare::Condition cond : parse_conditions(state_str)) {
        if (cond.comparaison != navitia::fare::Comp_e::EQ)
            throw invalid_key("invalid key, comparator has to be equal and is "
                              + navitia::fare::comp_to_string(cond.comparaison));
        if (cond.key == "line") {
            if (state.line != "")
                throw invalid_key("line already filled");
            state.line = cond.value;
        } else if (cond.key == "zone") {
            if (state.zone != "")
                throw invalid_key("zone already filled");
            state.zone = cond.value;
        } else if (cond.key == "mode") {
            if (state.mode != "")
                throw invalid_key("mode already filled");
            state.mode = cond.value;
        } else if (cond.key == "stoparea") {
            if (state.stop_area != "")
                throw invalid_key("stoparea already filled");
            state.stop_area = cond.value;
        } else if (cond.key == "network") {
            if (state.network != "")
                throw invalid_key("network already filled");
            state.network = cond.value;
        } else if (cond.key == "ticket") {
            if (state.ticket != "")
                throw invalid_key("ticket already filled");
            state.ticket = cond.value;
        } else {
            throw invalid_key("unhandled condition case");
        }
    }

    return state;
}

void add_in_state_str(std::string& str, std::string key, const std::string& val) {
    if (val.empty())
        return;
    if (!str.empty())
        str = "&";
    str += key + navitia::fare::comp_to_string(navitia::fare::Comp_e::EQ) + val;
}

std::string to_string(navitia::fare::State state) {
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

std::string to_string(navitia::fare::OD_key::od_type type) {
    switch (type) {
        case navitia::fare::OD_key::od_type::Zone:
            return "Zone";
        case navitia::fare::OD_key::od_type::StopArea:
            return "StopArea";
        case navitia::fare::OD_key::od_type::Mode:
            return "Mode";
        default:
            throw navitia::exception("unhandled od_type case");
    }
}

navitia::fare::OD_key::od_type to_od_type(const std::string& key) {
    std::string lower_key = boost::algorithm::to_lower_copy(key);
    if (lower_key == "mode")
        return navitia::fare::OD_key::Mode;
    if (lower_key == "zone")
        return navitia::fare::OD_key::Zone;
    if (lower_key == "stop" || lower_key == "stoparea")
        return navitia::fare::OD_key::StopArea;
    throw navitia::exception("Unable to parse " + key + " as OD_Key");
}

std::string to_string(navitia::fare::Transition::GlobalCondition cond) {
    switch (cond) {
        case navitia::fare::Transition::GlobalCondition::nothing:
            return "nothing";
        case navitia::fare::Transition::GlobalCondition::exclusive:
            return "exclusive";
        case navitia::fare::Transition::GlobalCondition::with_changes:
            return "with_changes";
        default:
            throw navitia::exception("unhandled GlobalCondition case");
    }
}

navitia::fare::Transition::GlobalCondition to_global_condition(const std::string& str) {
    std::string lower_key = boost::algorithm::to_lower_copy(str);
    if (lower_key == "nothing" || lower_key == "")
        return navitia::fare::Transition::GlobalCondition::nothing;
    if (lower_key == "exclusive")
        return navitia::fare::Transition::GlobalCondition::exclusive;
    if (lower_key == "with_changes" || lower_key == "stoparea")
        return navitia::fare::Transition::GlobalCondition::with_changes;
    throw navitia::exception("Unable to parse " + str + " as global condition");
}

}  // namespace connectors
}  // namespace ed
