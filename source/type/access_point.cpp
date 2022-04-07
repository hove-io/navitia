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
https://groups.google.com/d/forum/navitia
www.navitia.io
*/
#include "type/access_point.h"

#include "type/stop_area.h"
#include "type/serialization.h"
#include "georef/georef.h"

#include <boost/serialization/weak_ptr.hpp>

namespace navitia {
namespace type {
template <class Archive>
void AccessPoint::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& uri& name& stop_code& is_entrance& is_exit& pathway_mode& length& traversal_time& stair_count& max_slope&
        min_width& signposted_as& reversed_signposted_as& parent_station& coord& idx;
}
SERIALIZABLE(AccessPoint)

bool AccessPoint::operator<(const AccessPoint& other) const {
    if (this->parent_station != other.parent_station) {
        return *this->parent_station < *other.parent_station;
    }
    std::string lower_name = strip_accents_and_lower(this->name);
    std::string lower_other_name = strip_accents_and_lower(other.name);
    if (lower_name != lower_other_name) {
        return lower_name < lower_other_name;
    }
    return this->uri < other.uri;
}

}  // namespace type
}  // namespace navitia
