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
#include "isochron.h"
#include "raptor.h"

#include <set>
#include <assert.h>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
 #include <boost/geometry/geometry.hpp>
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
        direction_rad = 2 * M_PI - direction_rad;
    }
    double center_lat_deg = center.lat();
    double center_lon_deg = center.lon();
    type::GeographicalCoord center_right_interval = type::GeographicalCoord(center_lon_deg, center_lat_deg);
    double center_lat_rad = center_right_interval.lat() * GeographicalCoord::N_DEG_TO_RAD;
    double center_lon_rad = center_right_interval.lon() * GeographicalCoord::N_DEG_TO_RAD;
    double lat;
    double lon;
    // To avoid rounding error we use ° instread of rad.
    if (fabs(center_right_interval.lat()) == 90) {
        lon = direction_rad - M_PI;
        lat = M_PI_2 - alpha;
    } else {
        double delta_lat;
        double delta_lon;
        if (fmod(direction, 180) == 0) {
            delta_lat = pow(-1, fmod(direction / 180, 2)) * alpha;
            lat = center_lat_rad + delta_lat;
            lon = center_lon_rad;
        } else {
            // asin and acos give results in rad.
            double projection = asin(sin(alpha) * sin(direction_rad));
            delta_lat = acos(cos(alpha) / cos(projection));
            if (direction_rad > M_PI_2 && direction_rad < 3 * M_PI_2) {
                delta_lat = -1 * delta_lat;
            }
            lat = center_lat_rad + delta_lat;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if (lat == M_PI_2) {
                delta_lon = (direction == 90 ? 1 : - 1) * alpha;
#pragma GCC diagnostic pop
            } else {
                double b = (cos(alpha) - sin(lat) * sin(center_lat_rad)) / (cos(lat) * cos(center_lat_rad));
                if (fabs(b) >= 1) {
                    b = (std::signbit(b) ? fmod(b - 1, 2) + 1 : fmod(b + 1, 2) - 1);
                }
                delta_lon = acos(b);
                if (direction_rad > M_PI) {
                    delta_lon = -1 * delta_lon;
                }
            }
            lon = center_lon_rad + delta_lon;
        }
    }
    // Lon and lat are in °
    double lon_deg = lon * N_RAD_TO_DEG;
    double lat_deg = lat * N_RAD_TO_DEG;
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

static type::MultiPolygon merge_poly(const type::MultiPolygon& multi_poly, type::Polygon poly) {
    type::MultiPolygon multi_polys_merged;
    for(const type::Polygon& p: multi_poly) {
        if (boost::geometry::intersects(p, poly)) {
           type::MultiPolygon poly_union;
           boost::geometry::union_(poly, p, poly_union);
           poly = poly_union[0];
        } else {
            multi_polys_merged.push_back(p);
        }
    }
    multi_polys_merged.push_back(poly);
    return multi_polys_merged;
}

template<typename T>
static bool in_bound(const T & begin, const T & end, bool clockwise) {
    return (clockwise && begin < end) ||
            (!clockwise && begin > end);
}

type::MultiPolygon build_isochron(RAPTOR& raptor,
                                const std::vector<type::StopPoint*>& stop_points,
                                const bool clockwise,
                                const DateTime& bound,
                                const map_stop_point_duration& origin,
                                const double& speed,
                                const int& max_duration) {
    type::MultiPolygon circles;
    const auto& data_departure = raptor.data.pt_data->stop_points;
    for (auto it = origin.begin(); it != origin.end(); ++it){
        if (it->second.total_seconds() < max_duration) {
            int duration_max = max_duration - int(it->second.total_seconds());
            //Delete the circles too small to avoid rounding error
            if (duration_max * speed < 5) {continue;}
            const auto& center = data_departure[it->first.val]->coord;
            type::Polygon circle_to_add = circle(center , duration_max * speed);
            circles = merge_poly(circles, circle_to_add);
        }
    }
    for(const type::StopPoint* sp: stop_points) {
        SpIdx sp_idx(*sp);
        const auto best_lbl = raptor.best_labels_pts[sp_idx];
        if (in_bound(best_lbl, bound, clockwise)) {
            int round = raptor.best_round(sp_idx);
            if (round == -1 || ! raptor.labels[round].pt_is_initialized(sp_idx)
                    || ! raptor.labels[round].transfer_is_initialized(sp_idx)) {
                continue;
            }
            int duration = abs(int(best_lbl) - int(bound));
            //Delete the circles too small to avoid rounding error
            if (duration * speed < 5 ) {continue;}
            type::Polygon circle_to_add = circle(sp->coord, duration * speed);
            circles = merge_poly(circles, circle_to_add);
        }
    }
    return circles;
}

}}
