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
#include "type/stop_area.h"

#include "georef/adminref.h"
#include "type/indexes.h"
#include "type/pt_data.h"
#include "type/route.h"
#include "type/serialization.h"
#include "type/stop_point.h"

namespace navitia {
namespace type {

template <class Archive>
void StopArea::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& idx& label& uri& name& coord& stop_point_list& admin_list& _properties& wheelchair_boarding& impacts& visible&
        timezone;
}
SERIALIZABLE(StopArea)

bool StopArea::operator<(const StopArea& other) const {
    std::string lower_name = strip_accents_and_lower(name);
    std::string lower_other_name = strip_accents_and_lower(other.name);
    if (lower_name != lower_other_name) {
        return lower_name < lower_other_name;
    }
    return uri < other.uri;
}

Indexes StopArea::get(Type_e type, const PT_Data& data) const {
    Indexes result;
    switch (type) {
        case Type_e::StopPoint:
            return indexes(stop_point_list);
        case Type_e::Route:
            return indexes(route_list);
        case Type_e::Impact:
            return data.get_impacts_idx(get_impacts());

        default:
            break;
    }
    return result;
}

}  // namespace type
}  // namespace navitia
