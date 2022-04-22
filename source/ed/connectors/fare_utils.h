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
#include "fare/fare.h"

namespace ed {
namespace connectors {

/// Exception levée si on utilise une clef inconnue
struct invalid_key : navitia::exception {
    invalid_key(const std::string& s) : navitia::exception(s) {}
    invalid_key(const invalid_key&) = default;
    invalid_key& operator=(const invalid_key&) = default;
    ~invalid_key() noexcept override;
};

/// Exception levée si on n'arrive pas à parser une condition
struct invalid_condition : navitia::exception {
    invalid_condition(const std::string& s) : navitia::exception(s) {}
    invalid_condition(const invalid_condition&) = default;
    invalid_condition& operator=(const invalid_condition&) = default;
    ~invalid_condition() noexcept override;
};

navitia::fare::Condition parse_condition(const std::string& condition_str);

std::vector<navitia::fare::Condition> parse_conditions(const std::string& conditions);

navitia::fare::State parse_state(const std::string& state_str);

void add_in_state_str(std::string& str, std::string key, const std::string& val);

std::string to_string(navitia::fare::State state);

std::string to_string(navitia::fare::OD_key::od_type type);

navitia::fare::OD_key::od_type to_od_type(const std::string& key);

std::string to_string(navitia::fare::Transition::GlobalCondition cond);

navitia::fare::Transition::GlobalCondition to_global_condition(const std::string& str);
}  // namespace connectors
}  // namespace ed
