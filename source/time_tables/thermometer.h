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
#include "type/fwd_type.h"
#include "type/type_interfaces.h"

#include <boost/functional/hash.hpp>
namespace navitia {
namespace timetables {

using vector_idx = std::vector<idx_t>;
using vector_size = std::vector<uint16_t>;

struct Thermometer {
    void generate_thermometer(const std::vector<vector_idx>& sps);
    void generate_thermometer(const type::Route* route);
    vector_idx get_thermometer() const;

    // res[stop_time.order()] correspond to the index of the
    // thermometer for a stop time of the given vj
    std::vector<uint32_t> stop_times_order(const type::VehicleJourney&) const;
    std::vector<uint32_t> stop_times_order_helper(const vector_idx& stop_point_list) const;

    struct cant_match {
        type::idx_t jpp_idx;
        cant_match() : jpp_idx(type::invalid_idx) {}
        cant_match(type::idx_t rp_idx) : jpp_idx(rp_idx) {}
    };

    struct upper {};

private:
    vector_idx thermometer;
    std::string filter;
    int nb_branches = 0;

    bool generate_topological_thermometer(const std::vector<vector_idx>& stop_point_lists);

    std::vector<uint32_t> untail(std::vector<vector_idx>& journey_patterns,
                                 type::idx_t spidx,
                                 std::vector<vector_size>& pre_computed_lb);
    void retail(std::vector<vector_idx>& journey_patterns,
                type::idx_t spidx,
                const std::vector<uint32_t>& to_retail,
                std::vector<vector_size>& pre_computed_lb);
    vector_idx generate_possibilities(const std::vector<vector_idx>& journey_patterns,
                                      std::vector<vector_size>& pre_computed_lb);
    std::pair<vector_idx, bool> recc(std::vector<vector_idx>& journey_patterns,
                                     std::vector<vector_size>& pre_computed_lb,
                                     const uint32_t lower_bound_,
                                     type::idx_t max_sp,
                                     const uint32_t upper_bound_ = std::numeric_limits<uint32_t>::max(),
                                     int depth = 0);
};
uint32_t get_lower_bound(std::vector<vector_size>& pre_computed_lb, vector_size mins, type::idx_t max_sp);

}  // namespace timetables
}  // namespace navitia
