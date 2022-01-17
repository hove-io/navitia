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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "type/type_interfaces.h"
#include "type/geographical_coord.h"
#include "type/fwd_type.h"
#include "type/access_point.h"

#include <boost/container/flat_set.hpp>

#include <vector>
#include <set>

namespace navitia {
namespace type {

struct StopPoint : public Header, Nameable, hasProperties, HasMessages {
    const static Type_e type = Type_e::StopPoint;
    GeographicalCoord coord;
    std::string fare_zone;
    bool is_zonal = false;
    std::string platform_code;
    std::string label;
    std::string address_id;

    StopArea* stop_area;
    std::vector<navitia::georef::Admin*> admin_list;
    std::set<navitia::type::AccessPoint> access_points;
    Network* network;
    std::vector<StopPointConnection*> stop_point_connection_list;
    std::set<Dataset*> dataset_list;
    boost::container::flat_set<Route*> route_list;
    navitia::georef::Address* address = nullptr;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);

    StopPoint() : fare_zone(), stop_area(nullptr), network(nullptr) {}

    Indexes get(Type_e type, const PT_Data& data) const;
    bool operator<(const StopPoint& other) const;
};

}  // namespace type
}  // namespace navitia
