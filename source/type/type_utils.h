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
#include "type/meta_data.h"
#include "type/type.pb.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "type/message.h"

namespace nt = navitia::type;
namespace bt = boost::posix_time;
namespace navitia {

static const nt::StopTime* get_base_stop_time(const nt::StopTime* st_orig) {
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
static bt::ptime get_base_dt(const nt::StopTime* st_orig, const nt::StopTime* st_base,
                             const bt::ptime& dt_orig, bool is_departure) {
    if (st_orig == nullptr || st_base == nullptr) { return bt::not_a_date_time; }
    // compute the "base validity_pattern" day of dt_orig:
    // * shift back if amended-vj is shifted compared to base-vj
    // * shift back if the hour in stop-time of amended-vj is >24h
    auto validity_pattern_dt_day = dt_orig.date();
    validity_pattern_dt_day -= boost::gregorian::days(st_orig->vehicle_journey->shift);
    auto hour_of_day_orig = (is_departure ? st_orig->departure_time : st_orig->arrival_time);
    validity_pattern_dt_day -= boost::gregorian::days(hour_of_day_orig / DateTimeUtils::SECONDS_PER_DAY);
    // from the "base validity_pattern" day, we simply have to apply stop_time from base_vj (st_base)
    auto hour_of_day_base = (is_departure ? st_base->departure_time : st_base->arrival_time);
    return bt::ptime(validity_pattern_dt_day, boost::posix_time::seconds(hour_of_day_base));
}

static pbnavitia::RTLevel to_pb_realtime_level(const navitia::type::RTLevel realtime_level) {
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
