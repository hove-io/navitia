#pragma once
#include "fare/fare.h"

namespace ed { namespace connectors {


/// Exception levée si on utilise une clef inconnue
struct invalid_key : navitia::exception{invalid_key(const std::string& s): navitia::exception(s) {}};

/// Exception levée si on n'arrive pas à parser une condition
struct invalid_condition : navitia::exception {invalid_condition(const std::string& s): navitia::exception(s) {}};

navitia::fare::Condition parse_condition(const std::string & condition_str);

std::vector<navitia::fare::Condition> parse_conditions(const std::string & conditions);

navitia::fare::State parse_state(const std::string & state_str);


void add_in_state_str(std::string& str, std::string key, const std::string& val);


std::string to_string(navitia::fare::State state);


std::string to_string(navitia::fare::OD_key::od_type type);


navitia::fare::OD_key::od_type to_od_type(const std::string& key);

std::string to_string(navitia::fare::Transition::GlobalCondition cond);

navitia::fare::Transition::GlobalCondition to_global_condition(const std::string& str);
}
}
