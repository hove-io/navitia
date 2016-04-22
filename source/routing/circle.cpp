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

#include "type/geographical_coord.h"
#include "circle.h"

#include <set>
#include <assert.h>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#define PI 3.14159265

namespace navitia { namespace routing {


type::GeographicalCoord project_in_direction(const type::GeographicalCoord& center,
                                             const double& direction,
                                             const double& radius){
    assert(direction >= 0);
    assert(direction <= 360);
    assert(radius>0);
    static const double EARTH_RADIUS_IN_METERS = 6372797.560856;
    double alpha=radius/EARTH_RADIUS_IN_METERS;
    double direction_rad = direction*N_DEG_TO_RAD;
    double center_lat_rad = center.lat()*N_DEG_TO_RAD;
    double center_lon_rad = center.lon()*N_DEG_TO_RAD;
    double projection=asin(sin(alpha)*sin(direction_rad));
    double delta_lat = acos(cos(alpha)/cos(projection));
    if (direction_rad > PI/2 && direction_rad < 3*PI/2){
        delta_lat = -1*delta_lat;
    }
    double lat = center_lat_rad + delta_lat;
    double delta_lon = acos((cos(alpha)-sin(lat)*sin(center_lat_rad))/(cos(lat)*cos(center_lat_rad)));
    if(direction_rad > PI){
        delta_lon = -1*delta_lon;
    }
    double lon = center_lon_rad + delta_lon;
    return type::GeographicalCoord(lon*N_RAD_TO_DEG,lat*N_RAD_TO_DEG);
}

type::Polygon circle(const type::GeographicalCoord& center,
                     const double& radius){
    type::Polygon points;
    for (double i=0; i<360; i++){
        points.outer().push_back(project_in_direction(center,i,radius));
    }
    points.outer().push_back(project_in_direction(center,0,radius));
    return points;
}

}}
