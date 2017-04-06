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

#include "type/datetime.h"
#include "type/message.h"
#include "type/meta_data.h"
#include "type/type_utils.h"
#include "utils/logger.h"

namespace nt = navitia::type;
namespace bt = boost::posix_time;
namespace navitia {

const nt::StopTime* get_base_stop_time(const nt::StopTime* st_orig) {
    const nt::StopTime* st_base = nullptr;
    size_t nb_st_found = 0;
    if (st_orig == nullptr || st_orig->vehicle_journey == nullptr) { return nullptr; }
    auto base_vj = st_orig->vehicle_journey->get_corresponding_base();
    if (base_vj == nullptr) { return nullptr; }

    if (st_orig->vehicle_journey == base_vj) { return st_orig; }
    for (const auto& st_it : base_vj->stop_time_list) {
        if (st_orig->stop_point == st_it.stop_point) {
            st_base = &st_it;
            ++nb_st_found;
        }
    }
    //TODO handle lolipop vj, bob-commerce-commerce-bobito vj...
    if (nb_st_found != 1) {
        auto logger = log4cplus::Logger::getInstance("log");
        LOG4CPLUS_DEBUG(logger, "Ignored stop_time finding: impossible to match exactly one base stop_time");
        return nullptr;
    }
    return st_base;
}

/**
 * Compute base passage from amended passage, knowing amended and base stop-times
 */
bt::ptime get_date_time(const routing::StopEvent stop_event,
                        const nt::StopTime* st_orig, const nt::StopTime* st_base,
                        const bt::ptime& dt_orig, bool is_departure) {
    if (st_orig == nullptr || st_base == nullptr) { return bt::not_a_date_time; }

    const uint32_t hour = (stop_event == routing::StopEvent::pick_up) ? st_orig->departure_time : st_orig->arrival_time;
    if (is_departure) {
        return (dt_orig - bt::seconds(hour)) + bt::seconds(st_base->departure_time);
    } else {
        return (dt_orig - bt::seconds(hour)) + bt::seconds(st_base->arrival_time);
    }
}

const nt::StopTime& earliest_stop_time(const std::vector<nt::StopTime>& sts) {
    assert(!sts.empty());
    const auto& min_st = std::min_element(sts.begin(), sts.end(),
                                          [](const nt::StopTime& st1, const nt::StopTime& st2) {
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
}
