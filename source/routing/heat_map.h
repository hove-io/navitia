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

#include <utility>

#include "isochrone.h"
#include "raptor.h"

namespace navitia {
namespace routing {

template <typename F, typename R>
void separated_by_coma(std::stringstream& os, F f, const R& range) {
    auto it = std::begin(range);
    const auto end = std::end(range);
    if (it != end) {
        f(os, *it);
        ++it;
    }
    for (; it != end; ++it) {
        os << ",";
        f(os, *it);
    }
}

struct SingleCoord {
    double min_coord;
    double step;
    SingleCoord(const double min_coord, const double step) : min_coord(min_coord), step(step) {}
};

struct BoundBox {
    type::GeographicalCoord max = type::GeographicalCoord(-180, -90);
    type::GeographicalCoord min = type::GeographicalCoord(180, 90);

    void set_box(const type::GeographicalCoord& coord, const double offset) {
        const auto lon_max = std::max(coord.lon() + offset, this->max.lon());
        const auto lat_max = std::max(coord.lat() + offset, this->max.lat());
        const auto lon_min = std::min(coord.lon() - offset, this->min.lon());
        const auto lat_min = std::min(coord.lat() - offset, this->min.lat());
        this->max = type::GeographicalCoord(lon_max, lat_max);
        this->min = type::GeographicalCoord(lon_min, lat_min);
    }

    bool contains(const type::GeographicalCoord& coord) {
        return this->max.lon() >= coord.lon() && this->max.lat() >= coord.lat() && this->min.lon() <= coord.lon()
               && this->min.lat() <= coord.lat();
    }
};

struct HeatMap {
    std::vector<SingleCoord> header;
    std::vector<std::pair<SingleCoord, std::vector<navitia::time_duration>>> body;
    HeatMap(std::vector<SingleCoord> header,
            std::vector<std::pair<SingleCoord, std::vector<navitia::time_duration>>> body)
        : header(std::move(header)), body(std::move(body)) {}

    HeatMap(const uint step, const BoundBox& box, const double height_step, const double width_step) {
        for (uint i = 0; i < step; i++) {
            auto lon = SingleCoord(box.min.lon() + i * width_step, width_step);
            for (uint j = 0; j < step; j++) {
                if (i == 0) {
                    header.emplace_back((box.min.lat() + j * height_step), height_step);
                }
            }
            body.push_back({lon, std::vector<navitia::time_duration>{step, bt::pos_infin}});
        }
    }
};

constexpr static double N_DEG_TO_DISTANCE =
    type::GeographicalCoord::N_DEG_TO_RAD * type::GeographicalCoord::EARTH_RADIUS_IN_METERS;

std::vector<navitia::time_duration> init_distance(const georef::GeoRef& worker,
                                                  const std::vector<type::StopPoint*>& stop_points,
                                                  const DateTime& init_dt,
                                                  const RAPTOR& raptor,
                                                  const type::Mode_e& mode,
                                                  const type::GeographicalCoord& coord_origin,
                                                  const bool clockwise,
                                                  const DateTime& bound,
                                                  const double speed);

HeatMap fill_heat_map(const BoundBox& box,
                      const double height_step,
                      const double width_step,
                      const georef::GeoRef& worker,
                      const double min_dist,
                      const double max_duration,
                      const double speed,
                      const std::vector<navitia::time_duration>& distances,
                      const size_t step);

std::string print_grid(const HeatMap& heat_map);

std::string build_raster_isochrone(const georef::GeoRef& worker,
                                   const double& speed,
                                   const type::Mode_e& mode,
                                   const DateTime init_dt,
                                   RAPTOR& raptor,
                                   const type::GeographicalCoord& coord_origin,
                                   const DateTime duration,
                                   const bool clockwise,
                                   const DateTime bound,
                                   const uint resolution);

}  // namespace routing
}  // namespace navitia
