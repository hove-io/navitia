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

#include "type/line.h"

#include "type/calendar.h"
#include "type/commercial_mode.h"
#include "type/company.h"
#include "type/indexes.h"
#include "type/network.h"
#include "type/physical_mode.h"
#include "type/pt_data.h"
#include "type/route.h"
#include "type/serialization.h"

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

namespace navitia {
namespace type {

template <class Archive>
void Line::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& idx& name& uri& code& forward_name& backward_name& additional_data& color& text_color& sort& commercial_mode&
        company_list& network& route_list& physical_mode_list& impacts& calendar_list& shape& closing_time&
            opening_time& properties& line_group_list;
}
SERIALIZABLE(Line)

template <class Archive>
void LineGroup::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& idx& name& uri& main_line& line_list;
}
SERIALIZABLE(LineGroup)

Indexes Line::get(Type_e type, const PT_Data& data) const {
    Indexes result;
    switch (type) {
        case Type_e::CommercialMode:
            if (this->commercial_mode) {
                result.insert(commercial_mode->idx);
            }
            break;
        case Type_e::PhysicalMode:
            return indexes(physical_mode_list);
        case Type_e::Company:
            return indexes(company_list);
        case Type_e::Network:
            result.insert(network->idx);
            break;
        case Type_e::Route:
            return indexes(route_list);
        case Type_e::Calendar:
            return indexes(calendar_list);
        case Type_e::LineGroup:
            return indexes(line_group_list);
        case Type_e::Impact:
            return data.get_impacts_idx(get_impacts());
        default:
            break;
    }
    return result;
}

bool Line::operator<(const Line& other) const {
    if (this->network != other.network) {
        return *this->network < *other.network;
    }
    if (this->sort != other.sort) {
        return this->sort < other.sort;
    }
    if (this->code != other.code) {
        return navitia::pseudo_natural_sort()(this->code, other.code);
    }
    if (this->name != other.name) {
        return this->name < other.name;
    }
    return this->uri < other.uri;
}

type::hasOdtProperties Line::get_odt_properties() const {
    type::hasOdtProperties result;
    if (!this->route_list.empty()) {
        for (const auto route : this->route_list) {
            result |= route->get_odt_properties();
        }
    }
    return result;
}

std::string Line::get_label() const {
    // LineLabel = NetworkName + ModeName + LineCode
    //            \__ if different__/      '-> LineName if empty
    std::stringstream s;
    if (commercial_mode) {
        if (network && !boost::iequals(network->name, commercial_mode->name)) {
            s << network->name << " ";
        }
        s << commercial_mode->name << " ";
    } else if (network) {
        s << network->name << " ";
    }
    if (!code.empty()) {
        s << code;
    } else if (!name.empty()) {
        s << name;
    }
    return s.str();
}

Indexes LineGroup::get(Type_e type, const PT_Data& /*unused*/) const {
    Indexes result;
    switch (type) {
        case Type_e::Line:
            return indexes(line_list);
        default:
            break;
    }
    return result;
}
}  // namespace type
}  // namespace navitia
