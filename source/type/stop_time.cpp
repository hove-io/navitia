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

#include "type/stop_time.h"

#include "type/indexes.h"
#include "type/serialization.h"
#include "type/stop_point.h"
#include "type/stop_area.h"
#include "type/vehicle_journey.h"
#include "utils/logger.h"

namespace navitia {
namespace type {

StopTime StopTime::clone() const {
    StopTime ret{arrival_time, departure_time, stop_point};
    ret.alighting_time = alighting_time;
    ret.boarding_time = boarding_time;
    ret.properties = properties;
    ret.local_traffic_zone = local_traffic_zone;
    ret.vehicle_journey = nullptr;
    ret.shape_from_prev = shape_from_prev;
    return ret;
}

bool StopTime::is_valid_day(u_int32_t day, const bool is_arrival, const RTLevel rt_level) const {
    if ((is_arrival && alighting_time >= DateTimeUtils::SECONDS_PER_DAY)
        || (!is_arrival && boarding_time >= DateTimeUtils::SECONDS_PER_DAY)) {
        if (day == 0) {
            return false;
        }
        --day;
    }
    return vehicle_journey->validity_patterns[rt_level]->check(day);
}

const StopTime* StopTime::get_base_stop_time() const {
    if (vehicle_journey == nullptr) {
        return nullptr;
    }

    auto base_vj = vehicle_journey->get_corresponding_base();

    if (base_vj == nullptr) {
        return nullptr;
    }

    if (vehicle_journey == base_vj) {
        return this;
    }

    // Partially handle lollipop
    //
    // We get the rank of the stop_stime in the current VJ, and
    // return the one with same rank in the base VJ.
    // There are limitations if:
    //   * the lollipop's node (stop_point B) is added before
    //          ie. Base VJ:     A - B - C - B - D
    //              RT VJ:   B - A - B - C - B - D
    //   * the first stop time of the node is deleted
    //          ie. Base VJ:     A - B - C - B - D
    //              RT VJ:       A - - - C - B - D
    // TODO - Use Levenshtein (edit) distance to handle those cases properly
    size_t rank_current_vj = 0;
    for (const auto& st : vehicle_journey->stop_time_list) {
        if (st.is_similar(*this)) {
            break;
        }
        if (stop_point == st.stop_point) {
            rank_current_vj++;
        }
    }

    for (const auto& base_st : base_vj->stop_time_list) {
        if (stop_point->idx == base_st.stop_point->idx) {
            if (rank_current_vj == 0) {
                return &base_st;
            }
            rank_current_vj--;
        }
    }

    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_DEBUG(logger, "Ignored stop_time " << stop_point->uri << ":" << departure_time
                                                 << ": impossible to match exactly one base stop_time");
    return nullptr;
}

bool StopTime::is_similar(const StopTime& st) const {
    return arrival_time == st.arrival_time && departure_time == st.departure_time
           && stop_point->idx == st.stop_point->idx;
}

bool StopTime::is_in_stop_area(const std::string& stop_area_uri) const {
    if (stop_point == nullptr) {
        return false;
    }
    if (stop_point->stop_area == nullptr) {
        return false;
    }
    return stop_point->stop_area->uri == stop_area_uri;
}

}  // namespace type
}  // namespace navitia
