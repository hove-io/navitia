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
#include "type/type_interfaces.h"
#include "type/fwd_type.h"
#include "type/rt_level.h"
#include "type/vehicle_journey.h"
#include "type/meta_vehicle_journey.h"

namespace navitia {
namespace type {

template <typename F>
bool concerns_base_at_period(const VehicleJourney& vj,
                             const RTLevel rt_level,
                             const std::vector<boost::posix_time::time_period>& periods,
                             const F& fun,
                             bool check_past_midnight = true) {
    bool intersect = false;

    ValidityPattern concerned_vp = *vj.validity_patterns[rt_level];
    // We shift the vlidity pattern to get the base one (from adapted or realtime)
    concerned_vp.days >>= vj.shift;

    for (const auto& period : periods) {
        // we can impact a vj with a departure the day before who past midnight
        namespace bg = boost::gregorian;
        bg::day_iterator titr(period.begin().date() - bg::days(check_past_midnight ? 1 : 0));
        for (; titr <= period.end().date(); ++titr) {
            // check that day is not out of concerned_vp bound, then if day is concerned by concerned_vp
            if (*titr < concerned_vp.beginning_date) {
                continue;
            }
            size_t day((*titr - concerned_vp.beginning_date).days());
            if (day >= concerned_vp.days.size() || !concerned_vp.check(day)) {
                continue;
            }
            // we check on the execution period of base-schedule vj, as impact are targeted on this period
            const auto base_vj = vj.meta_vj->get_base_vj_circulating_at_date(*titr);
            if (!base_vj) {
                continue;
            }
            if (period.intersects(base_vj->execution_period(*titr))) {
                intersect = true;
                // calling fun() on concerned day of base vj
                if (!fun(day)) {
                    return intersect;
                }
            }
        }
    }
    return intersect;
}
}  // namespace type
}  // namespace navitia
