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
#include "routing/stop_event.h"
#include "routing/routing.h"
#include "type/data.h"

#include <queue>

namespace navitia {
namespace routing {
using datetime_stop_time = std::pair<DateTime, const type::StopTime*>;

/**
 * @brief get_stop_times: Return all departures leaving from the journey_pattern points
 * @param journey_pattern_points: list of the journey_pattern_point we want to start from
 * @param dt: departure date time
 * @param calendar_id: id of the calendar (optional)
 * @param max_dt: maximal Datetime
 * @param nb_departures: max number of departure
 * @param data: data container
 * @param accessibilite_params: accebility criteria to restrict the stop times
 * @return: a list of pair <datetime, departure st.idx>. The list is sorted on the datetimes.
 */
std::vector<datetime_stop_time> get_stop_times(
    const routing::StopEvent stop_event,
    const std::vector<routing::JppIdx>& journey_pattern_points,
    const DateTime& dt,
    const DateTime& max_dt,
    const size_t max_departures,
    const type::Data& data,
    const type::RTLevel rt_level,
    const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams());

std::vector<datetime_stop_time> get_calendar_stop_times(
    const std::vector<routing::JppIdx>& journey_pattern_points,
    const uint32_t begining_time,
    const uint32_t max_time,
    const type::Data& data,
    const std::string& calendar_id,
    const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams());

/// Return a list of all {time in the day, stoptime} leaving the jpp for the given calendar
std::vector<std::pair<uint32_t, const type::StopTime*>> get_all_calendar_stop_times(
    const routing::JourneyPattern& jp,
    const routing::JourneyPatternPoint& jpp,
    const std::string& calendar_id,
    const type::VehicleProperties& vehicle_properties = type::VehicleProperties());

struct JppSt {
    routing::JppIdx jpp;
    const type::StopTime* st;
    DateTime dt;
};

/*
 * Comparator used in the heap
 * for clockwise, we want the smallest first,
 * for anticlockwise, we want the greatest first
 */
struct BestDTComp {
    bool operator()(const JppSt& j1, const JppSt& j2) {
        if (clockwise) {
            return j1.dt > j2.dt;
        }
        return j1.dt < j2.dt;
    }
    const bool clockwise;
};

using JppStQueue = std::priority_queue<JppSt, std::vector<JppSt>, BestDTComp>;

/*
 * for schedule with calendar, we want to sort the result a quite a strange way
 * we want the first stop time to start from a given dt and loop through the day after
 * */
struct CalendarScheduleSort {
    CalendarScheduleSort(DateTime d) : pivot(d) {}
    DateTime pivot;
    bool operator()(datetime_stop_time dst1, datetime_stop_time dst2) const {
        auto is_before_start1 = (DateTimeUtils::hour(dst1.first) < DateTimeUtils::hour(pivot));
        auto is_before_start2 = (DateTimeUtils::hour(dst2.first) < DateTimeUtils::hour(pivot));

        if (is_before_start1 != is_before_start2) {
            // if one is before and one is after, we want the one after first
            return !is_before_start1;
        }
        return DateTimeUtils::hour(dst1.first) < DateTimeUtils::hour(dst2.first);
    }
};

}  // namespace routing
}  // namespace navitia
