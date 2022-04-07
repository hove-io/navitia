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

#pragma once

#include "type/geographical_coord.h"
#include "utils/exception.h"
#include "raptor.h"

#include <set>

namespace navitia {
namespace routing {

struct IsochroneException : public recoverable_exception {
    IsochroneException(const std::string& msg) : recoverable_exception(msg) {}
    IsochroneException() = default;
    IsochroneException(const IsochroneException&) = default;
    IsochroneException& operator=(const IsochroneException&) = default;
    ~IsochroneException() noexcept override;
};

constexpr static double N_RAD_TO_DEG = 57.295779513;

// Delete the circles too small to avoid rounding error in meter
constexpr static double MIN_RADIUS = 5;

type::GeographicalCoord project_in_direction(const type::GeographicalCoord& center,
                                             const double& direction,
                                             const double& radius);

// Create a circle
type::Polygon circle(const type::GeographicalCoord& center, const double& radius);

template <typename T>
bool in_bound(const T& begin, const T& end, bool clockwise) {
    return (clockwise && begin < end) || (!clockwise && begin > end);
}

DateTime build_bound(const bool clockwise, const DateTime duration, const DateTime init_dt);

// Create a multi polygon with circles around all the stop points in the isochrone
type::MultiPolygon build_single_isochrone(RAPTOR& raptor,
                                          const std::vector<type::StopPoint*>& stop_points,
                                          const bool clockwise,
                                          const type::GeographicalCoord& coord_origin,
                                          const DateTime& bound,
                                          const map_stop_point_duration& origin,
                                          const double& speed,
                                          const int& duration);

struct Isochrone {
    type::MultiPolygon shape;
    DateTime min_duration;
    DateTime max_duration;
    Isochrone(type::MultiPolygon shape, DateTime min_duration, DateTime max_duration)
        : shape(std::move(shape)), min_duration(min_duration), max_duration(max_duration) {}
};

std::vector<Isochrone> build_isochrones(RAPTOR& raptor,
                                        const bool clockwise,
                                        const type::GeographicalCoord& coord_origin,
                                        const map_stop_point_duration& origin,
                                        const double& speed,
                                        const std::vector<DateTime>& boundary_duration,
                                        const DateTime init_dt);

}  // namespace routing
}  // namespace navitia
