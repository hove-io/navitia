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

#include "type/type_utils.h"
#include "type/datetime.h"
#include "type/message.h"
#include "type/meta_data.h"
#include "utils/logger.h"

namespace nt = navitia::type;
namespace bt = boost::posix_time;
namespace navitia {

/**
 * Compute base passage from amended passage, knowing amended and base stop-times
 */
bt::ptime get_date_time(const routing::StopEvent stop_event,
                        const nt::StopTime* st_orig,
                        const nt::StopTime* st_base,
                        const bt::ptime& dt_orig,
                        bool is_departure) {
    if (st_orig == nullptr || st_base == nullptr) {
        return bt::not_a_date_time;
    }

    const uint32_t hour = (stop_event == routing::StopEvent::pick_up) ? st_orig->departure_time : st_orig->arrival_time;

    uint32_t shift_seconds =
        (st_orig == st_base) ? 0 : st_orig->vehicle_journey->shift * DateTimeUtils::SECONDS_PER_DAY;

    if (is_departure) {
        return (dt_orig - bt::seconds(shift_seconds) - bt::seconds(hour)) + bt::seconds(st_base->departure_time);
    }
    return (dt_orig - bt::seconds(shift_seconds) - bt::seconds(hour)) + bt::seconds(st_base->arrival_time);
}

const nt::StopTime& earliest_stop_time(const std::vector<nt::StopTime>& sts) {
    assert(!sts.empty());
    const auto& min_st = std::min_element(sts.begin(), sts.end(), [](const nt::StopTime& st1, const nt::StopTime& st2) {
        return std::min(st1.boarding_time, st1.arrival_time) < std::min(st2.boarding_time, st2.arrival_time);
    });
    return *min_st;
}

pbnavitia::RTLevel to_pb_realtime_level(const navitia::type::RTLevel realtime_level) {
    switch (realtime_level) {
        case nt::RTLevel::Base:
            return pbnavitia::BASE_SCHEDULE;
        case nt::RTLevel::Adapted:
            return pbnavitia::ADAPTED_SCHEDULE;
        case nt::RTLevel::RealTime:
            return pbnavitia::REALTIME;
        default:
            throw navitia::exception("realtime level case not handled");
    }
}

pbnavitia::ActiveStatus to_pb_active_status(navitia::type::disruption::ActiveStatus active_status) {
    switch (active_status) {
        case nt::disruption::ActiveStatus::past:
            return pbnavitia::past;
        case nt::disruption::ActiveStatus::active:
            return pbnavitia::active;
        case nt::disruption::ActiveStatus::future:
            return pbnavitia::future;
        default:
            throw navitia::recoverable_exception("unhandled disruption active status");
    }
}

}  // namespace navitia
