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

#include "type/dataset.h"
#include "type/route.h"
#include "type/indexes.h"
#include "type/line.h"
#include "type/pt_data.h"
#include "type/serialization.h"
#include "type/stop_area.h"
#include "type/stop_point.h"
#include "type/vehicle_journey.h"

#include <boost/serialization/set.hpp>

namespace navitia {
namespace type {

template <class Archive>
void Route::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& idx& name& uri& line& destination& discrete_vehicle_journey_list& frequency_vehicle_journey_list& impacts&
        shape& direction_type& dataset_list;
}
SERIALIZABLE(Route)

bool Route::operator<(const Route& other) const {
    if (this->line != other.line) {
        return *this->line < *other.line;
    }
    if (this->name != other.name) {
        return this->name < other.name;
    }
    return this->uri < other.uri;
}

std::string Route::get_label() const {
    // RouteLabel = LineLabel + (RouteName)
    std::stringstream s;
    if (line) {
        s << line->get_label();
    }
    if (!name.empty()) {
        s << " (" << name << ")";
    }
    return s.str();
}

Indexes Route::get(Type_e type, const PT_Data& data) const {
    Indexes result;
    switch (type) {
        case Type_e::Line:
            result.insert(line->idx);
            break;
        case Type_e::VehicleJourney:
            for_each_vehicle_journey([&](const VehicleJourney& vj) {
                result.insert(vj.idx);  // TODO use bulk insert ?
                return true;
            });
            break;
        case Type_e::Impact:
            return data.get_impacts_idx(get_impacts());
        case Type_e::Dataset:
            return indexes(dataset_list);
        case Type_e::StopPoint:
            return indexes(stop_point_list);
        case Type_e::StopArea:
            return indexes(stop_area_list);
        default:
            break;
    }
    return result;
}

type::hasOdtProperties Route::get_odt_properties() const {
    type::hasOdtProperties result;
    for_each_vehicle_journey([&](const VehicleJourney& vj) {
        type::hasOdtProperties cur;
        cur.set_estimated(vj.has_datetime_estimated());
        cur.set_zonal(vj.has_zonal_stop_point());
        result.odt_properties |= cur.odt_properties;
        return true;
    });
    return result;
}
}  // namespace type
}  // namespace navitia
