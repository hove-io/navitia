/* Copyright © 2001-2019, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/
#include "type/stop_point.h"
#include "type/stop_area.h"
#include "type/dataset.h"
#include "type/connection.h"
#include "type/pt_data.h"

#include "type/serialization.h"
#include <boost/serialization/weak_ptr.hpp>
#include "type/indexes.h"

namespace navitia {
namespace type {

template <class Archive>
void StopPoint::serialize(Archive& ar, const unsigned int) {
    // The *_list are not serialized here to avoid stack abuse
    // during serialization and deserialization.
    //
    // stop_point_connection_list is managed by StopPointConnection
    ar& uri& label& name& stop_area& coord& fare_zone& is_zonal& idx& platform_code& admin_list& _properties& impacts&
        dataset_list;
}
SERIALIZABLE(StopPoint)

bool StopPoint::operator<(const StopPoint& other) const {
    if (this->stop_area != other.stop_area) {
        return *this->stop_area < *other.stop_area;
    }
    if (this->name != other.name) {
        return this->name < other.name;
    }
    return this->uri < other.uri;
}

Indexes StopPoint::get(Type_e type, const PT_Data& data) const {
    Indexes result;
    switch (type) {
        case Type_e::StopArea:
            result.insert(stop_area->idx);
            break;
        case Type_e::Connection:
        case Type_e::StopPointConnection:
            for (const StopPointConnection* stop_cnx : stop_point_connection_list)
                result.insert(stop_cnx->idx);  // TODO use bulk insert ?
            break;
        case Type_e::Impact:
            return data.get_impacts_idx(get_impacts());
        case Type_e::Dataset:
            return indexes(dataset_list);
        case Type_e::Route:
            return indexes(route_list);
        default:
            break;
    }
    return result;
}

}  // namespace type
}  // namespace navitia
