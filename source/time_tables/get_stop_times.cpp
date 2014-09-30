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
#include "routing/best_stoptime.h"
#include "type/pb_converter.h"
namespace navitia { namespace timetables {

std::vector<datetime_stop_time> get_stop_times(const std::vector<type::idx_t>& journey_pattern_points, 
                                               const DateTime& dt,
                                               const DateTime& max_dt,
                                               const size_t max_departures,
                                               const type::Data& data, 
                                               bool disruption_active,
                                               const type::AccessibiliteParams& accessibilite_params) {
    std::vector<datetime_stop_time> result;
    auto test_add = true;

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
            auto st = routing::earliest_stop_time(jpp,
                                                  next_requested_datetime[jpp_idx],
                                                  data,
                                                  disruption_active,
                                                  false,
                                                  accessibilite_params.vehicle_properties);

            if(st.first != nullptr) {
                DateTime dt_temp = st.second;
                if(dt_temp <= max_dt) {
                    result.push_back(std::make_pair(dt_temp, st.first));
                    test_add = true;
                    // The next stop time must be at least one second after
                    if(st.first->is_frequency()) {
                        DateTimeUtils::update(dt_temp, st.first->vehicle_journey->end_time+st.first->departure_time);
                    }
                    next_requested_datetime[jpp_idx] = dt_temp + 1;
                    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"), "jpp " << jpp_idx << " datetime " << next_requested_datetime[jpp_idx]
                                    << " str: " << str(next_requested_datetime[jpp_idx])
                                    );
                }
            }
        }
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"), "we continue, we got " << result.size() << " / " << max_departures);
     }
    std::sort(result.begin(), result.end(),[](datetime_stop_time dst1, datetime_stop_time dst2) {return dst1.first < dst2.first;});
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
        auto st = routing::get_all_stop_times(jpp, calendar_id, accessibilite_params.vehicle_properties);

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

}} // namespace navitia::timetables
