/* Copyright © 2001-2016, Canal TP and/or its affiliates. All rights reserved.

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

#include "type/geographical_coord.h"
#include "circle.h"
#include "raptor.h"

#include <set>
#include <assert.h>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <cmath>

namespace navitia { namespace routing {


type::GeographicalCoord project_in_direction(const type::GeographicalCoord& center,
                                             const double& direction,
                                             const double& radius){
    using navitia::type::GeographicalCoord;
    assert(radius > 0);
    double alpha = radius / GeographicalCoord::EARTH_RADIUS_IN_METERS;
    double direction_rad = fmod(direction, 360) * GeographicalCoord::N_DEG_TO_RAD;
    if (direction < 0) {
        direction_rad = 2 * PI - direction_rad;
    }
    double center_lat_rad = center.lat() * GeographicalCoord::N_DEG_TO_RAD;
    double center_lon_rad = center.lon() * GeographicalCoord::N_DEG_TO_RAD;
    double delta_lat;
    double delta_lon;
    double lat;
    double lon;
    if (fmod(direction, 180) == 0) {
        delta_lat = pow(-1, fmod(direction / 180, 2)) * alpha;
        lat = center_lat_rad + delta_lat;
        lon = center_lon_rad;
    } else {
        double projection = asin(sin(alpha) * sin(direction_rad));
        delta_lat = acos(cos(alpha) / cos(projection));
        if (direction_rad > PI / 2 && direction_rad < 3 * PI / 2) {
            delta_lat = -1 * delta_lat;
        }
        lat = center_lat_rad + delta_lat;
        delta_lon = acos((cos(alpha) - sin(lat) * sin(center_lat_rad)) / (cos(lat) * cos(center_lat_rad)));
        if (direction_rad > PI) {
            delta_lon = -1 * delta_lon;
        }
        lon = center_lon_rad + delta_lon;
    }
    double lon_deg = lon * N_RAD_TO_DEG;
    double lat_deg = lat * N_RAD_TO_DEG;
    //If latitude is not in  [-90,90] or longitude is not in [-180,180]
    if (lat_deg < -90 || lat_deg > 90) {
        lat_deg = (lat_deg / abs(lat_deg)) * 90 + fmod(lat_deg, 90);
    }
    if (lon_deg < -180 || lon_deg > 180) {
        lon_deg = (lon_deg / abs(lon_deg)) * 180 + fmod(lon_deg, 180);
    }
    return type::GeographicalCoord(lon_deg, lat_deg);
}


type::Polygon circle(const type::GeographicalCoord& center,
                     const double& radius) {
    type::Polygon points;
    auto& points_out = points.outer();
    points_out.reserve(360);
    for (double i = 0; i < 360; i++) {
        points_out.push_back(project_in_direction(center, i, radius));
    }
    points_out.push_back(project_in_direction(center, 0, radius));
    return points;
}

}}
