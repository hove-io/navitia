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

#include "autocomplete/utils.h"

#include "type/type_interfaces.h"

namespace navitia {
namespace autocomplete {

int get_type_e_order(const type::Type_e& type) {
    switch (type) {
        case type::Type_e::Network:
            return 1;
        case type::Type_e::CommercialMode:
            return 2;
        case type::Type_e::Admin:
            return 3;
        case type::Type_e::StopArea:
            return 4;
        case type::Type_e::POI:
            return 5;
        case type::Type_e::Address:
            return 6;
        case type::Type_e::Line:
            return 7;
        case type::Type_e::Route:
            return 8;
        default:
            return 9;
    }
}

bool compare_type_e(const type::Type_e& a, const type::Type_e& b) {
    const auto a_order = get_type_e_order(a);
    const auto b_order = get_type_e_order(b);
    return a_order < b_order;
}

std::vector<std::vector<type::Type_e>> build_type_groups(std::vector<type::Type_e> filter) {
    std::vector<std::vector<type::Type_e>> result;
    std::sort(filter.begin(), filter.end(), compare_type_e);
    int current_order = -1;
    for (const auto& f : filter) {
        auto order = get_type_e_order(f);
        if (order != current_order) {
            current_order = order;
            result.emplace_back();
        }
        result.back().push_back(f);
    }
    return result;
}

}  // namespace autocomplete
}  // namespace navitia
