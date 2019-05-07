/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

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

#include "type/connection.h"
#include "type/type.h"
#include "type/indexes.h"
#include "type/serialization.h"

namespace navitia {
namespace type {

template <class Archive>
void StopPointConnection::save(Archive& ar, const unsigned int) const {
    ar& idx& uri& departure& destination& display_duration& duration& max_duration& connection_type& _properties;
}
template <class Archive>
void StopPointConnection::load(Archive& ar, const unsigned int) {
    ar& idx& uri& departure& destination& display_duration& duration& max_duration& connection_type& _properties;

    // loading manage StopPoint::stop_point_connection_list
    departure->stop_point_connection_list.push_back(this);
    destination->stop_point_connection_list.push_back(this);
}
SPLIT_SERIALIZABLE(StopPointConnection)

Indexes StopPointConnection::get(Type_e type, const PT_Data&) const {
    Indexes result;
    switch (type) {
        case Type_e::StopPoint:
            result.insert(this->departure->idx);
            result.insert(this->destination->idx);
            break;
        default:
            break;
    }
    return result;
}
bool StopPointConnection::operator<(const StopPointConnection& other) const {
    if (this->departure != other.departure) {
        return *this->departure < *other.departure;
    }
    return *this->destination < *other.destination;
}

}  // namespace type
}  // namespace navitia
