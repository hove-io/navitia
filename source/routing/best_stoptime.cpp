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

#include "best_stoptime.h"

namespace navitia { namespace routing {

std::pair<const type::StopTime*, uint32_t>
best_stop_time(const type::JourneyPatternPoint* jpp,
               const DateTime dt,
               const type::VehicleProperties& vehicle_properties,
               const bool clockwise, bool disruption_active, const type::Data &data, bool reconstructing_path) {
    if(clockwise)
        return earliest_stop_time(jpp, dt, data, disruption_active, reconstructing_path, vehicle_properties);
    else
        return tardiest_stop_time(jpp, dt, data, disruption_active, reconstructing_path, vehicle_properties);
}

/** Which is the first valid stop_time in this range ?
 *  Returns invalid_idx is none is
 */
const type::StopTime* next_valid_pick_up(type::idx_t idx, const type::idx_t end, const DateTime dt,
        const type::Data &data, bool reconstructing_path,
        const type::VehicleProperties &required_vehicle_properties,
        bool disruption_active){
    const auto date = DateTimeUtils::date(dt);
    const auto hour = DateTimeUtils::hour(dt);
    for(; idx < end; ++idx) {
        const type::StopTime* st = data.dataRaptor->st_idx_forward[idx];
        if (st->valid_end(reconstructing_path) && st->valid_hour(hour, true) &&
            st->is_valid_day(date, false, disruption_active)
            && st->vehicle_journey->accessible(required_vehicle_properties) ){
                return st;
        }
    }
    return nullptr;
}


/**
 * the valid_pick_up funciton can be called 2 differents ways:
 * - with a date time => we look for the new valid stop time valid the the day of the date time and after the hour of the date time.
 *      Note: if nothing found, we also look for a stop time the day after
 * - with a calendar and a hour => we look for the next valid stop time valid for the calendar and after the hour of the date time.
 */
std::pair<const type::StopTime*, DateTime>
valid_pick_up(const std::vector<uint32_t>::const_iterator begin, type::idx_t idx, const type::idx_t end, const DateTime dt,
        const type::Data &data, bool reconstructing_path,
        const type::VehicleProperties &vehicle_properties,
        bool disruption_active) {
    const type::StopTime* first_st = next_valid_pick_up(idx, end,
            dt, data, reconstructing_path, vehicle_properties, disruption_active);
    // If no trip was found, we look for one the day after
    if(first_st != nullptr) {
        return {first_st, dt};
    }
    idx = begin - data.dataRaptor->departure_times.begin();
    auto working_dt = DateTimeUtils::set(DateTimeUtils::date(dt)+1, 0);
    first_st = next_valid_pick_up(idx, end, working_dt,
        data, reconstructing_path, vehicle_properties,disruption_active);

    return {first_st, working_dt};
}

const type::StopTime* valid_drop_off(type::idx_t idx, const type::idx_t end, const DateTime dt,
               const type::Data &data, bool reconstructing_path,
               const type::VehicleProperties &required_vehicle_properties,
               bool disruption_active){
    const auto date = DateTimeUtils::date(dt);
    const auto hour = DateTimeUtils::hour(dt);
    for(; idx < end; ++idx) {
        const type::StopTime* st = data.dataRaptor->st_idx_backward[idx];
        if (st->valid_end(!reconstructing_path) && st->valid_hour(hour, false) &&
            st->is_valid_day(date, true, disruption_active)
            && st->vehicle_journey->accessible(required_vehicle_properties) ){
                return st;
        }
    }
    return nullptr;
}


std::pair<const type::StopTime*, uint32_t>
earliest_stop_time(const type::JourneyPatternPoint* jpp,
                   const DateTime dt, const type::Data &data,
                   bool disruption_active,
                   bool reconstructing_path,
                   const type::VehicleProperties& vehicle_properties) {
    //We look for the earliest stop time of the journey_pattern >= dt.hour()
    auto begin = data.dataRaptor->departure_times.begin() +
            data.dataRaptor->first_stop_time[jpp->journey_pattern->idx] +
            jpp->order * data.dataRaptor->nb_trips[jpp->journey_pattern->idx];
    auto end = begin + data.dataRaptor->nb_trips[jpp->journey_pattern->idx];
    auto it = std::lower_bound(begin, end, DateTimeUtils::hour(dt),
                               bound_predicate_earliest);

    type::idx_t idx = it - data.dataRaptor->departure_times.begin();
    type::idx_t end_idx = (begin - data.dataRaptor->departure_times.begin()) +
                           data.dataRaptor->nb_trips[jpp->journey_pattern->idx];

    //Return the first valid trip
    std::pair<const type::StopTime*, DateTime> first_st =
            valid_pick_up(begin, idx, end_idx, dt, data, reconstructing_path, vehicle_properties, disruption_active);

    if(first_st.first != nullptr) {
        if(!first_st.first->is_frequency()) {
            DateTimeUtils::update(first_st.second, first_st.first->departure_time);
        } else {
            first_st.second = dt;
            const DateTime tmp_dt = f_departure_time(DateTimeUtils::hour(first_st.second), first_st.first);
            DateTimeUtils::update(first_st.second, DateTimeUtils::hour(tmp_dt));
        }

        return first_st;
    }
    return {nullptr, 0};
}

std::pair<const type::StopTime*, uint32_t>
earliest_stop_time(const type::JourneyPatternPoint* jpp,
                   const uint32_t time, const type::Data &/*data*/,
                   const std::string calendar_id,
                   const type::VehicleProperties& vehicle_properties) {
    // earliest stop time for calendar is different than for a datetime
    // we have to consider only the first theoric vj of all meta vj for the given jpp
    // for all those vj, we select the one associated to the calendar,
    // and we loop through all stop times for the stop point,
    // and we select the earliest one
    std::set<const type::MetaVehicleJourney*> meta_vjs;
    for (auto vj: jpp->journey_pattern->vehicle_journey_list) {
        if (! vj->meta_vj) {
            throw navitia::exception("vj " + vj->uri + " has been ill constructed, it has no meta vj");
        }
        meta_vjs.insert(vj->meta_vj);
    }
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
        return {nullptr, 0};
    }

    std::pair<const type::StopTime*, uint32_t> best_st = {nullptr, 0};
    for (const auto vj: vjs) {
        //loop through stop times for stop jpp->stop_point
        auto st = *(vj->stop_time_list.begin() + jpp->order);
        if (! st->valid_hour(time, true)) {
            continue; //the stop must be after the given hour
        }
        if (! st->vehicle_journey->accessible(vehicle_properties)) {
            continue; //the stop time must be accessible
        }
        uint32_t departure_time = st->is_frequency() ?
                    //if it is a frequency, we got the hour of the next departure after the wanted hour
                    DateTimeUtils::hour(f_departure_time(time, st)) :
                    //else we only got the departure of the stop time
                    st->departure_time;

        //we need to convert this to local there since we do not have a precise date (just a period)
        departure_time += vj->utc_to_local_offset;

        if (departure_time < time) {
            continue;
        }
        if (best_st.first == nullptr || departure_time < best_st.second) {
            best_st = {st, departure_time};
        }
    }


    return best_st;
}


std::pair<const type::StopTime*, uint32_t>
tardiest_stop_time(const type::JourneyPatternPoint* jpp,
                   const DateTime dt, const type::Data &data, bool disruption_active,
                   bool reconstructing_path,
                   const type::VehicleProperties& vehicle_properties) {
    //On cherche le plus grand stop time de la journey_pattern <= dt.hour()
    const auto begin = data.dataRaptor->arrival_times.begin() +
                       data.dataRaptor->first_stop_time[jpp->journey_pattern->idx] +
                       jpp->order * data.dataRaptor->nb_trips[jpp->journey_pattern->idx];
    const auto end = begin + data.dataRaptor->nb_trips[jpp->journey_pattern->idx];
    auto it = std::lower_bound(begin, end, DateTimeUtils::hour(dt), bound_predicate_tardiest);

    type::idx_t idx = it - data.dataRaptor->arrival_times.begin();
    type::idx_t end_idx = (begin - data.dataRaptor->arrival_times.begin()) +
                           data.dataRaptor->nb_trips[jpp->journey_pattern->idx];

    const type::StopTime* first_st = valid_drop_off(idx, end_idx,
            dt, data,
            reconstructing_path, vehicle_properties, disruption_active);

    auto working_dt = dt;
    // If no trip was found, we look for one the day before
    if(first_st == nullptr && DateTimeUtils::date(dt) > 0){
        idx = begin - data.dataRaptor->arrival_times.begin();
        working_dt = DateTimeUtils::set(DateTimeUtils::date(working_dt) - 1,
                                        DateTimeUtils::SECONDS_PER_DAY - 1);
        first_st = valid_drop_off(idx, end_idx, working_dt, data, reconstructing_path,
                vehicle_properties, disruption_active);
    }

    if(first_st != nullptr){
        if(!first_st->is_frequency()) {
            DateTimeUtils::update(working_dt, DateTimeUtils::hour(first_st->arrival_time), false);
        } else {
            working_dt = dt;
            const DateTime tmp_dt = f_arrival_time(DateTimeUtils::hour(working_dt), first_st);
            DateTimeUtils::update(working_dt, DateTimeUtils::hour(tmp_dt), false);
        }
        return std::make_pair(first_st, working_dt);
    }

    //Cette journey_pattern ne comporte aucun trip compatible
    return std::make_pair(nullptr, 0);
}
}}

