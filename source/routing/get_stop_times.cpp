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

#include "get_stop_times.h"

#include "routing/dataraptor.h"
#include "routing/next_stop_time.h"
#include "type/pb_converter.h"

#include <functional>

namespace navitia {
namespace routing {

std::vector<datetime_stop_time> get_stop_times(const routing::StopEvent stop_event,
                                               const std::vector<routing::JppIdx>& journey_pattern_points,
                                               const DateTime& dt,
                                               const DateTime& max_dt,
                                               const size_t max_departures,
                                               const type::Data& data,
                                               const type::RTLevel rt_level,
                                               const type::AccessibiliteParams& accessibilite_params) {
    const bool clockwise(max_dt >= dt);
    std::vector<datetime_stop_time> result;
    routing::NextStopTime next_st = routing::NextStopTime(data);

    // Next departure for the next stop: we store it to have the next departure for each jpp
    // We init it with the next_stop_time for each jpp
    JppStQueue next_requested_dt({clockwise});
    for (const auto& jpp_idx : journey_pattern_points) {
        const routing::JourneyPatternPoint& jpp = data.dataRaptor->jp_container.get(jpp_idx);
        if (!data.pt_data->stop_points[jpp.sp_idx.val]->accessible(accessibilite_params.properties)) {
            // we do not push them in the queue at all
            continue;
        }
        auto st = next_st.next_stop_time(stop_event, jpp_idx, dt, clockwise, rt_level,
                                         accessibilite_params.vehicle_properties, true, max_dt);
        if (st.first) {
            next_requested_dt.push({jpp_idx, st.first, st.second});
        }
    }

    while (!next_requested_dt.empty() && result.size() < max_departures) {
        const auto best_jpp_dt = next_requested_dt.top();  // copy
        next_requested_dt.pop();
        if ((clockwise && best_jpp_dt.dt > max_dt) || (!clockwise && best_jpp_dt.dt < max_dt)) {
            // the best elt of the queue is after the limit, we can stop
            break;
        }

        auto result_dt = best_jpp_dt.dt;
        if (stop_event == StopEvent::pick_up) {
            result_dt += best_jpp_dt.st->get_boarding_duration();
        } else {
            result_dt -= best_jpp_dt.st->get_alighting_duration();
        }
        result.emplace_back(result_dt, best_jpp_dt.st);

        // we insert the next stop time in the queue (it must be at least one second after/before)
        auto next_dt = best_jpp_dt.dt + (clockwise ? 1 : -1);
        auto st = next_st.next_stop_time(stop_event, best_jpp_dt.jpp, next_dt, clockwise, rt_level,
                                         accessibilite_params.vehicle_properties, true, max_dt);
        if (st.first) {
            next_requested_dt.push({best_jpp_dt.jpp, st.first, st.second});
        }
    }

    return result;
}

std::vector<datetime_stop_time> get_calendar_stop_times(const std::vector<routing::JppIdx>& journey_pattern_points,
                                                        const uint32_t begining_time,
                                                        const uint32_t max_time,
                                                        const type::Data& data,
                                                        const std::string& calendar_id,
                                                        const type::AccessibiliteParams& accessibilite_params) {
    std::vector<datetime_stop_time> result;

    for (auto jpp_idx : journey_pattern_points) {
        const routing::JourneyPatternPoint& jpp = data.dataRaptor->jp_container.get(jpp_idx);
        if (!data.pt_data->stop_points[jpp.sp_idx.val]->accessible(accessibilite_params.properties)) {
            continue;
        }
        auto st = get_all_calendar_stop_times(data.dataRaptor->jp_container.get(jpp.jp_idx), jpp, calendar_id,
                                              accessibilite_params.vehicle_properties);

        // afterward we filter the datetime not in [dt, max_dt]
        // the difficult part comes from the fact that, for calendar, 'dt' and 'max_dt' are not really datetime,
        // there are time but max_dt can be the day after like [today 4:00, tomorow 3:00]
        for (auto& res : st) {
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
std::vector<std::pair<uint32_t, const type::StopTime*>> get_all_calendar_stop_times(
    const routing::JourneyPattern& jp,
    const routing::JourneyPatternPoint& jpp,
    const std::string& calendar_id,
    const type::VehicleProperties& vehicle_properties) {
    std::set<const type::MetaVehicleJourney*> meta_vjs;
    auto insert_meta_vj = [&](const nt::VehicleJourney& vj) {
        assert(vj.meta_vj);
        meta_vjs.insert(vj.meta_vj);
    };
    for (const auto* vj : jp.discrete_vjs) {
        if (vj->is_base_schedule()) {
            insert_meta_vj(*vj);
        }
    }
    for (const auto* vj : jp.freq_vjs) {
        insert_meta_vj(*vj);
    }
    std::vector<const type::VehicleJourney*> vjs;
    for (const auto* meta_vj : meta_vjs) {
        if (meta_vj->associated_calendars.find(calendar_id) == meta_vj->associated_calendars.end()) {
            // meta vj not associated with the calender, we skip
            continue;
        }
        // we can get only the first theoric one, because BY CONSTRUCTION all theoric vj have the same local times
        vjs.push_back(meta_vj->get_base_vj().front().get());
    }
    if (vjs.empty()) {
        return {};
    }

    std::vector<std::pair<DateTime, const type::StopTime*>> res;
    for (const auto vj : vjs) {
        // loop through stop times for stop jpp->stop_point
        const auto& st = get_corresponding_stop_time(*vj, jpp.order);
        if (!st.vehicle_journey->accessible(vehicle_properties)) {
            continue;  // the stop time must be accessible
        }
        if (st.is_frequency()) {
            // if it is a frequency, we got to expand the timetable

            // Note: end can be lower than start, so we have to cycle through the day
            const auto freq_vj = dynamic_cast<const type::FrequencyVehicleJourney*>(vj);
            bool is_looping = (freq_vj->start_time > freq_vj->end_time);
            auto stop_loop = [freq_vj, is_looping, st](u_int32_t t) {
                if (!is_looping) {
                    return t <= freq_vj->end_time + st.departure_time;
                }
                return t > freq_vj->end_time + st.departure_time;
            };
            for (auto time = freq_vj->start_time + st.departure_time; stop_loop(time); time += freq_vj->headway_secs) {
                if (is_looping && time > DateTimeUtils::SECONDS_PER_DAY) {
                    time -= DateTimeUtils::SECONDS_PER_DAY;
                }

                // we need to convert this to local there since we do not have a precise date (just a period)
                res.emplace_back(time + freq_vj->utc_to_local_offset(), &st);
            }
        } else {
            // same utc tranformation
            res.emplace_back(st.departure_time + vj->utc_to_local_offset(), &st);
        }
    }

    return res;
}

}  // namespace routing
}  // namespace navitia
