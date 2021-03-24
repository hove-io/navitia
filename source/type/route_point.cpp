/* Copyright © 2001-2021, Canal TP and/or its affiliates. All rights reserved.

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

#include "type/route_point.h"
#include "type/stop_point.h"

#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/insert.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/type_erased.hpp>

namespace navitia {
namespace type {

namespace ba = boost::adaptors;

RoutePointRefs route_points_from(const StopPointRange& sps) {
    auto to_route_point_list = [](const StopPoint& sp) { return ba::values(sp.route_point_list); };
    auto to_route_point_list_size = [](const StopPoint& sp) { return sp.route_point_list.size(); };

    size_t rp_size = boost::accumulate(sps | ba::transformed(to_route_point_list_size), 0);

    RoutePointRefs rps;
    rps.reserve(rp_size);

    for (const auto& e : sps | ba::transformed(to_route_point_list)) {
        boost::insert(rps, rps.end(), e);
    }

    return rps;
}

RoutePointRefs route_points_from(const std::vector<StopPoint*>& sps) {
    return route_points_from(sps | ba::indirected);
}

}  // namespace type
}  // namespace navitia
