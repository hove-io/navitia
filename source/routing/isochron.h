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

#pragma once

#include "type/geographical_coord.h"
#include "utils/exception.h"
#include "raptor.h"
#include <set>

namespace navitia { namespace routing {

struct IsochronException: public recoverable_exception {
    IsochronException(const std::string& msg): recoverable_exception(msg) {}
    IsochronException() = default;
    IsochronException(const IsochronException&) = default;
    IsochronException& operator=(const IsochronException&) = default;
    virtual ~IsochronException() noexcept = default;
};

constexpr static double N_RAD_TO_DEG = 57.295779513;

//Delete the circles too small to avoid rounding error in meter
constexpr static double MIN_RADIUS = 5;

type::GeographicalCoord project_in_direction(const type::GeographicalCoord& center,
                                             const double& direction,
                                             const double& radius);

//Create a circle
type::Polygon circle(const type::GeographicalCoord& center,
                     const double& radius);


//Create a multi polygon with circles around all the stop points in the isochrone
type::MultiPolygon build_single_isochron(RAPTOR& raptor,
                                const std::vector<type::StopPoint*>& stop_points,
                                const bool clockwise,
                                const DateTime& bound,
                                const map_stop_point_duration &origine,
                                const double& speed,
                                const int& duration);

type::MultiPolygon build_isochrons(RAPTOR& raptor,
                                   const bool clockwise,
                                   const DateTime& bound_max,
                                   const DateTime& bound_min,
                                   const map_stop_point_duration& origin,
                                   const double& speed,
                                   const int& max_duration,
                                   const int& min_duration);
}}
