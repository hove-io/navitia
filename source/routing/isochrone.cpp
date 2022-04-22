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

#include "type/geographical_coord.h"
#include "isochrone.h"
#include "raptor.h"
#include "raptor_api.h"
#include "utils/exception.h"
#include "utils/logger.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/range/algorithm.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <set>
#include <string>
#include <vector>

namespace navitia {
namespace routing {

IsochroneException::~IsochroneException() noexcept = default;

type::GeographicalCoord project_in_direction(const type::GeographicalCoord& center,
                                             const double& direction,
                                             const double& radius) {
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
        if (fabs(fmod(direction, 180)) < GeographicalCoord::coord_epsilon) {
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
                delta_lon = (direction == 90 ? 1 : -1) * alpha;
#pragma GCC diagnostic pop
            } else {
                double b = (cos(alpha) - sin(lat) * sin(center_lat_rad)) / (cos(lat) * cos(center_lat_rad));
                if (fabs(b) > 1) {
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

static type::GeographicalCoord build_sym(const type::GeographicalCoord& point,
                                         const type::GeographicalCoord& center,
                                         const unsigned int& i) {
    if (i % 180 == 0) {
        return type::GeographicalCoord(2 * center.lon() - point.lon(), point.lat());
    }
    return type::GeographicalCoord(point.lon(), 2 * center.lat() - point.lat());
}

type::Polygon circle(const type::GeographicalCoord& center, const double& radius) {
    type::Polygon points;
    auto& points_out = points.outer();
    points_out.reserve(180);
    for (unsigned int i = 0; i <= 90; i += 2) {
        points_out.push_back(project_in_direction(center, i, radius));
    }
    for (unsigned int j = 90; j <= 270; j += 90) {
        for (unsigned int k = 1; k <= 45; k++) {
            type::GeographicalCoord point;
            point = points_out[j / 2 - k];
            points_out.push_back(build_sym(point, center, j));
        }
    }
    points_out.push_back(project_in_direction(center, 0, radius));
    return points;
}

static type::MultiPolygon merge_poly(const type::MultiPolygon& multi_poly, const type::Polygon& poly) {
    type::MultiPolygon poly_union;
    try {
        boost::geometry::union_(poly, multi_poly, poly_union);
    } catch (const boost::geometry::exception& e) {
        // We don't merge the polygons
        log4cplus::Logger logger = log4cplus::Logger::getInstance("logger");
        LOG4CPLUS_WARN(logger, "impossible to merge polygon: " << e.what());
    }
    return poly_union;
}

struct InfoCircle {
    type::GeographicalCoord center;
    int duration_left;
    InfoCircle(const type::GeographicalCoord& center, const int& duration_left)
        : center(center), duration_left(duration_left) {}
};

static bool within_info_circle(const InfoCircle& circle,
                               const std::vector<InfoCircle>& multi_poly,
                               const double speed) {
    auto begin = multi_poly.begin();
    const auto end = multi_poly.end();
    double circle_radius = circle.duration_left * speed;
    double coslat = cos(circle.center.lat() * type::GeographicalCoord::N_DEG_TO_RAD);
    return any_of(begin, end, [&](const InfoCircle& it) {
        double it_radius = it.duration_left * speed;
        return sqrt(circle.center.approx_sqr_distance(it.center, coslat)) + circle_radius < it_radius;
    });
}

static std::vector<InfoCircle> delete_useless_circle(std::vector<InfoCircle> circles, const double speed) {
    std::vector<InfoCircle> useful_circles;
    boost::sort(circles, [](const InfoCircle& a, const InfoCircle& b) { return a.duration_left > b.duration_left; });
    for (auto& circle : circles) {
        if (!within_info_circle(circle, useful_circles, speed)) {
            useful_circles.push_back(circle);
        }
    }
    return useful_circles;
}

DateTime build_bound(const bool clockwise, const DateTime duration, const DateTime init_dt) {
    return clockwise ? init_dt + duration : init_dt - duration;
}

type::MultiPolygon build_single_isochrone(RAPTOR& raptor,
                                          const std::vector<type::StopPoint*>& stop_points,
                                          const bool clockwise,
                                          const type::GeographicalCoord& coord_origin,
                                          const DateTime& bound,
                                          const map_stop_point_duration& origin,
                                          const double& speed,
                                          const int& duration) {
    std::vector<InfoCircle> circles_classed;
    type::MultiPolygon circles;
    circles_classed.emplace_back(coord_origin, duration);
    const auto& data_departure = raptor.data.pt_data->stop_points;
    for (const auto& it : origin) {
        if (it.second.total_seconds() < duration) {
            int duration_left = duration - int(it.second.total_seconds());
            if (duration_left * speed < MIN_RADIUS) {
                continue;
            }
            const auto& center = data_departure[it.first.val]->coord;
            circles_classed.emplace_back(center, duration_left);
        }
    }
    for (const type::StopPoint* sp : stop_points) {
        SpIdx sp_idx(*sp);
        const auto best_lbl = raptor.best_labels[sp_idx].dt_pt;
        if (in_bound(best_lbl, bound, clockwise)) {
            uint duration_left = abs(int(best_lbl) - int(bound));
            if (duration_left * speed < MIN_RADIUS) {
                continue;
            }
            const auto& center = sp->coord;
            circles_classed.emplace_back(center, duration_left);
        }
    }
    std::vector<InfoCircle> circles_check = delete_useless_circle(std::move(circles_classed), speed);

    for (const auto& c : circles_check) {
        type::Polygon circle_to_add = circle(c.center, c.duration_left * speed);
        circles = merge_poly(circles, circle_to_add);
    }
    return circles;
}

std::vector<Isochrone> build_isochrones(RAPTOR& raptor,
                                        const bool clockwise,
                                        const type::GeographicalCoord& coord_origin,
                                        const map_stop_point_duration& origin,
                                        const double& speed,
                                        const std::vector<DateTime>& boundary_duration,
                                        const DateTime init_dt) {
    std::vector<Isochrone> isochrone;
    if (!boundary_duration.empty()) {
        type::MultiPolygon max_isochrone = build_single_isochrone(
            raptor, raptor.data.pt_data->stop_points, clockwise, coord_origin,
            build_bound(clockwise, boundary_duration[0], init_dt), origin, speed, boundary_duration[0]);
        for (size_t i = 1; i < boundary_duration.size(); i++) {
            type::MultiPolygon output;
            if (boundary_duration[i] > 0) {
                type::MultiPolygon min_isochrone = build_single_isochrone(
                    raptor, raptor.data.pt_data->stop_points, clockwise, coord_origin,
                    build_bound(clockwise, boundary_duration[i], init_dt), origin, speed, boundary_duration[i]);
                boost::geometry::difference(max_isochrone, min_isochrone, output);
                max_isochrone = std::move(min_isochrone);
            } else {
                output = max_isochrone;
            }
            isochrone.emplace_back(std::move(output), boundary_duration[i], boundary_duration[i - 1]);
        }
    }
    std::reverse(isochrone.begin(), isochrone.end());
    return isochrone;
}

}  // namespace routing
}  // namespace navitia
