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

#include "isochrone.h"
#include "raptor.h"

namespace navitia { namespace routing {

template<typename F, typename R>
void separated_by_coma(std::stringstream& os, F f, const R& range) {
    auto it = std::begin(range);
    const auto end = std::end(range);
    if (it != end) { f(os, *it); ++it; }
    for (; it != end; ++it) {
        os << ",";
        f(os, *it);
    }
}

struct SingleCoord {
    double min_coord;
    double step;
    SingleCoord(const double min_coord,
                const double step): min_coord(min_coord), step(step){}
};

struct HeatMap {
    std::vector <SingleCoord> header;
    std::vector<std::pair <SingleCoord, std::vector<navitia::time_duration>>> body;
    HeatMap(const std::vector <SingleCoord>& header,
            const std::vector<std::pair <SingleCoord, std::vector<navitia::time_duration>>>& body):
        header(header), body(body) {}
};

constexpr static double N_DEG_TO_DISTANCE = type::GeographicalCoord::N_DEG_TO_RAD *
        type::GeographicalCoord::EARTH_RADIUS_IN_METERS;

std::string print_grid(const HeatMap& heat_map);

std::string build_raster_isochrone(const georef::GeoRef& worker,
                                   const double& speed,
                                   const type::Mode_e& mode,
                                   const DateTime init_dt,
                                   RAPTOR& raptor,
                                   const type::GeographicalCoord& coord_origin,
                                   const DateTime duration,
                                   const bool clockwise,
                                   const DateTime bound);

}} //namespace navitia::routing
