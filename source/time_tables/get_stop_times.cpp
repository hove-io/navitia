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

#include "get_stop_times.h"
#include "routing/next_stop_time.h"
#include "type/pb_converter.h"
#include <functional>

namespace navitia { namespace timetables {

std::vector<datetime_stop_time> get_stop_times(const std::vector<type::idx_t>& journey_pattern_points, 
                                               const DateTime& dt,
                                               const DateTime& max_dt,
                                               const size_t max_departures,
                                               const type::Data& data, 
                                               bool disruption_active,
                                               const type::AccessibiliteParams& accessibilite_params,
                                               const bool clockwise) {
    std::vector<datetime_stop_time> result;
    auto test_add = true;
    routing::NextStopTime next_st = routing::NextStopTime(data);

    // Next departure for the next stop: we use it not to have twice the same stop_time
    // We init it with the wanted date time
    std::map<type::idx_t, DateTime> next_requested_datetime;
    for(auto jpp_idx : journey_pattern_points){
        next_requested_datetime[jpp_idx] = dt;
    }

    while(test_add && result.size() < max_departures) {
        test_add = false;
        for(auto jpp_idx : journey_pattern_points) {
            const type::JourneyPatternPoint* jpp = data.pt_data->journey_pattern_points[jpp_idx];
            if(!jpp->stop_point->accessible(accessibilite_params.properties)) {
                continue;
            }
            auto st = next_st.next_stop_time(routing::JppIdx(*jpp), next_requested_datetime[jpp_idx],
                                             clockwise, disruption_active,
                                             accessibilite_params.vehicle_properties, true);

            if (st.first != nullptr) {
                DateTime dt_temp = st.second;
                if ( (clockwise && dt_temp <= max_dt) || (!clockwise && dt_temp >= max_dt)) {
                    result.push_back(std::make_pair(dt_temp, st.first));
                    test_add = true;
                    // The next stop time must be at least one second after
                    next_requested_datetime[jpp_idx] = dt_temp + (clockwise? 1 : -1);
                }
            }
        }
    }
    std::sort(result.begin(), result.end(),
            [&clockwise](datetime_stop_time dst1, datetime_stop_time dst2) {
            if (clockwise) {
                return dst1.first < dst2.first;
            }
            return dst1.first > dst2.first;
    });
    if (result.size() > max_departures) {
        result.resize(max_departures);
    }

    return result;
}


std::vector<datetime_stop_time> get_stop_times(const std::vector<type::idx_t>& journey_pattern_points,
                                               const uint32_t begining_time,
                                               const uint32_t max_time,
                                               const type::Data& data,
                                               const std::string calendar_id,
                                               const type::AccessibiliteParams& accessibilite_params) {
    std::vector<datetime_stop_time> result;

    for(auto jpp_idx : journey_pattern_points) {
        const type::JourneyPatternPoint* jpp = data.pt_data->journey_pattern_points[jpp_idx];
        if(!jpp->stop_point->accessible(accessibilite_params.properties)) {
            continue;
        }
        auto st = get_all_stop_times(jpp, calendar_id, accessibilite_params.vehicle_properties);

        //afterward we filter the datetime not in [dt, max_dt]
        //the difficult part comes from the fact that for calendar dt are max_dt are not really datetime,
        //there are time but max_dt can be the day after like [today 4:00, tomorow 3:00]
        for (auto& res: st) {
            auto time = DateTimeUtils::hour(res.first);
            if (max_time > begining_time) {
                // we keep the st in [dt, max_dt]
                if (time >= begining_time && time <= max_time) {
                    result.push_back(res);
                }
            } else {
                // we filter the st in ]max_dt, dt[
                if (time >= begining_time || time <= max_time) {
                    result.push_back(res);
                }
            }
        }
    }

    return result;
}

/** get all stop times for a given jpp and a given calendar
 *
 * earliest stop time for calendar is different than for a datetime
 * we have to consider only the first theoric vj of all meta vj for the given jpp
 * for all those vj, we select the one associated to the calendar,
 * and we loop through all stop times for the jpp
*/
std::vector<std::pair<uint32_t, const type::StopTime*>>
get_all_stop_times(const type::JourneyPatternPoint* jpp,
                   const std::string calendar_id,
                   const type::VehicleProperties& vehicle_properties) {

    std::set<const type::MetaVehicleJourney*> meta_vjs;
    jpp->journey_pattern->for_each_vehicle_journey([&](const nt::VehicleJourney& vj ) {
        assert(vj.meta_vj);
        meta_vjs.insert(vj.meta_vj);
        return true;
    });
    std::vector<const type::VehicleJourney*> vjs;
    for (const auto meta_vj: meta_vjs) {
        if (meta_vj->associated_calendars.find(calendar_id) == meta_vj->associated_calendars.end()) {
            //meta vj not associated with the calender, we skip
            continue;
        }
        //we can get only the first theoric one, because BY CONSTRUCTION all theoric vj have the same local times
        vjs.push_back(meta_vj->theoric_vj.front());
    }
    if (vjs.empty()) {
        return {};
    }

    std::vector<std::pair<DateTime, const type::StopTime*>> res;
    for (const auto vj: vjs) {
        //loop through stop times for stop jpp->stop_point
        const auto& st = *(vj->stop_time_list.begin() + jpp->order);
        if (! st.vehicle_journey->accessible(vehicle_properties)) {
            continue; //the stop time must be accessible
        }
        if (st.is_frequency()) {
            //if it is a frequency, we got to expand the timetable

            //Note: end can be lower than start, so we have to cycle through the day
            const auto freq_vj = static_cast<const type::FrequencyVehicleJourney*>(vj);
            bool is_looping = (freq_vj->start_time > freq_vj->end_time);
            auto stop_loop = [freq_vj, is_looping](u_int32_t t) {
                if (! is_looping)
                    return t <= freq_vj->end_time;
                return t > freq_vj->end_time;
            };
            for (auto time = freq_vj->start_time; stop_loop(time); time += freq_vj->headway_secs) {
                if (is_looping && time > DateTimeUtils::SECONDS_PER_DAY) {
                    time -= DateTimeUtils::SECONDS_PER_DAY;
                }

                //we need to convert this to local there since we do not have a precise date (just a period)
                res.push_back({time + freq_vj->utc_to_local_offset, &st});
            }
        } else {
            //same utc tranformation
            res.push_back({st.departure_time + vj->utc_to_local_offset, &st});
        }
    }

    return res;
}

}} // namespace navitia::timetables
