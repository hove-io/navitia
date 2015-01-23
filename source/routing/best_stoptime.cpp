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

static const type::JourneyPatternPoint*
get_jpp(JpIdx jp_idx, uint16_t jpp_order, const type::Data& data) {
    return data.pt_data->journey_patterns[jp_idx.val]->journey_pattern_point_list[jpp_order];
}

std::pair<const type::StopTime*, uint32_t>
best_stop_time(const type::JourneyPatternPoint* jpp,
               const DateTime dt,
               const type::VehicleProperties& vehicle_properties,
               const bool clockwise,
               bool disruption_active,
               const type::Data &data,
               bool reconstructing_path) {
    if(clockwise)
        return earliest_stop_time(jpp, dt, data, disruption_active,
                                  reconstructing_path, vehicle_properties);
    else
        return tardiest_stop_time(jpp, dt, data, disruption_active,
                                  reconstructing_path, vehicle_properties);
}

/** Which is the first valid stop_time in this range ?
 *  Returns invalid_idx is none is
 */
static std::pair<const type::StopTime*, DateTime>
next_valid_discrete_pick_up(const JpIdx jp_idx,
                            uint16_t jpp_order,
                            const DateTime dt,
                            const dataRAPTOR& dataRaptor,
                            bool reconstructing_path,
                            const type::VehicleProperties& required_vehicle_properties,
                            bool disruption_active) {

    if (dataRaptor.departure_times.empty()) {
        return {nullptr, DateTimeUtils::inf};
    }
    const auto begin = dataRaptor.departure_times.begin() +
        dataRaptor.first_stop_time[jp_idx] +
        jpp_order * dataRaptor.nb_trips[jp_idx];
    const auto end_it = begin + dataRaptor.nb_trips[jp_idx];
    const auto it = std::lower_bound(begin, end_it, DateTimeUtils::hour(dt),
                               bound_predicate_earliest);

    type::idx_t idx = it - dataRaptor.departure_times.begin();
    const type::idx_t end = (begin - dataRaptor.departure_times.begin()) +
                           dataRaptor.nb_trips[jp_idx];


    auto date = DateTimeUtils::date(dt);
    for(; idx < end; ++idx) {
        const type::StopTime* st = dataRaptor.st_forward[idx];
        BOOST_ASSERT(JpIdx(*st->journey_pattern_point->journey_pattern) == jp_idx);
        BOOST_ASSERT(st->journey_pattern_point->order == jpp_order);
        if (is_valid(st, date, false, disruption_active, reconstructing_path, required_vehicle_properties)) {
                return {st, DateTimeUtils::set(date, st->departure_time % DateTimeUtils::SECONDS_PER_DAY)};
        }
    }

    //if none was found, we try again the next day
    idx = begin - dataRaptor.departure_times.begin();
    date++;
    for(; idx < end; ++idx) {
        const type::StopTime* st = dataRaptor.st_forward[idx];
        BOOST_ASSERT(JpIdx(*st->journey_pattern_point->journey_pattern) == jp_idx);
        BOOST_ASSERT(st->journey_pattern_point->order == jpp_order);
        if (is_valid(st, date, false, disruption_active, reconstructing_path, required_vehicle_properties)) {
                return {st, DateTimeUtils::set(date, st->departure_time % DateTimeUtils::SECONDS_PER_DAY)};
        }
    }

    // if nothing found, return max
    return {nullptr, DateTimeUtils::inf};
}

static std::pair<const type::StopTime*, DateTime>
next_valid_frequency_pick_up(const type::JourneyPatternPoint* jpp, const DateTime dt,
                             bool reconstructing_path,
                             const type::VehicleProperties &vehicle_properties,
                             bool disruption_active) {
    // to find the next frequency VJ, for the moment we loop through all frequency VJ of the JP
    // and for each jp, get compute the datetime on the jpp
    std::pair<const type::StopTime*, DateTime> best = {nullptr, DateTimeUtils::inf};
    for (const auto& freq_vj: jpp->journey_pattern->frequency_vehicle_journey_list) {
        //we get stop time corresponding to the jpp

        const auto& st = freq_vj->stop_time_list[jpp->order];

        if (! st.valid_end(reconstructing_path) ||
            ! freq_vj->accessible(vehicle_properties)) {
            continue;
        }

        const auto next_dt = get_next_departure(dt, *freq_vj, st, disruption_active);

        if (next_dt < best.second) {
            best = {&st, next_dt};
        }
    }
    //TODO we could do the next day checkup in only one round if needed (but with an even more complicated code :/)
    if (best.first == nullptr) {
        const auto next_date = DateTimeUtils::set(DateTimeUtils::date(dt) + 1, 0);
        for (const auto& freq_vj: jpp->journey_pattern->frequency_vehicle_journey_list) {
            //we get stop time corresponding to the jpp

            const auto& st = freq_vj->stop_time_list[jpp->order];

            if (! st.valid_end(reconstructing_path) ||
                ! freq_vj->accessible(vehicle_properties)) {
                continue;
            }

            const auto next_dt = get_next_departure(next_date, *freq_vj, st, disruption_active);

            if (next_dt < best.second) {
                best = {&st, next_dt};
            }
        }
    }
    return best;
}

static std::pair<const type::StopTime*, DateTime>
previous_valid_frequency_drop_off(const type::JourneyPatternPoint* jpp, const DateTime dt,
                             bool reconstructing_path,
                             const type::VehicleProperties &vehicle_properties,
                             bool disruption_active) {
    std::pair<const type::StopTime*, DateTime> best = {nullptr, DateTimeUtils::not_valid};
    for (const auto& freq_vj: jpp->journey_pattern->frequency_vehicle_journey_list) {
        //we get stop time corresponding to the jpp
        const auto& st = freq_vj->stop_time_list[jpp->order];

        if (! st.valid_end(!reconstructing_path) ||
            ! freq_vj->accessible(vehicle_properties)) {
            continue;
        }

        const auto previous_dt = get_previous_arrival(dt, *freq_vj, st, disruption_active);

        if (previous_dt == DateTimeUtils::not_valid) {
            continue;
        }
        if (best.second == DateTimeUtils::not_valid || previous_dt > best.second) {
            best = {&st, previous_dt};
        }
    }

    if (best.first == nullptr) {
        auto date = DateTimeUtils::date(dt);
        if (date == 0) {
            return best;
        }
        const auto previous_date = DateTimeUtils::set(date - 1, DateTimeUtils::SECONDS_PER_DAY - 1);
        for (const auto& freq_vj: jpp->journey_pattern->frequency_vehicle_journey_list) {
            //we get stop time corresponding to the jpp
            const auto& st = freq_vj->stop_time_list[jpp->order];

            if (! st.valid_end(!reconstructing_path) ||
                ! freq_vj->accessible(vehicle_properties)) {
                continue;
            }

            const auto previous_dt = get_previous_arrival(previous_date, *freq_vj, st, disruption_active);

            if (previous_dt == DateTimeUtils::not_valid) {
                continue;
            }
            if (best.second == DateTimeUtils::not_valid || previous_dt > best.second) {
                best = {&st, previous_dt};
            }
        }
    }
    return best;
}

static std::pair<const type::StopTime*, DateTime>
previous_valid_discrete_drop_off(const JpIdx jp_idx,
                                 uint16_t jpp_order,
                                 const DateTime dt,
                                 const dataRAPTOR& dataRaptor,
                                 bool reconstructing_path,
                                 const type::VehicleProperties& required_vehicle_properties,
                                 bool disruption_active) {
    //On cherche le plus grand stop time de la journey_pattern <= dt.hour()
    if (dataRaptor.arrival_times.empty()) {
        return {nullptr, DateTimeUtils::inf};
    }
    const auto begin = dataRaptor.arrival_times.begin() +
                       dataRaptor.first_stop_time[jp_idx] +
                       jpp_order * dataRaptor.nb_trips[jp_idx];
    const auto end_it = begin + dataRaptor.nb_trips[jp_idx];
    const auto it = std::lower_bound(begin, end_it, DateTimeUtils::hour(dt), bound_predicate_tardiest);

    type::idx_t idx = it - dataRaptor.arrival_times.begin();
    const type::idx_t end = (begin - dataRaptor.arrival_times.begin()) + dataRaptor.nb_trips[jp_idx];

    auto date = DateTimeUtils::date(dt);
    for(; idx < end; ++idx) {
        const type::StopTime* st = dataRaptor.st_backward[idx];
        BOOST_ASSERT(JpIdx(*st->journey_pattern_point->journey_pattern) == jp_idx);
        BOOST_ASSERT(st->journey_pattern_point->order == jpp_order);
        if (is_valid(st, date, true, disruption_active, !reconstructing_path, required_vehicle_properties)) {
                return {st, DateTimeUtils::set(date, st->arrival_time % DateTimeUtils::SECONDS_PER_DAY)};
        }
    }

    if (date == 0) {
        return {nullptr, DateTimeUtils::not_valid};
    }
    idx = begin - dataRaptor.arrival_times.begin();

    --date;
    for(; idx < end; ++idx) {
        const type::StopTime* st = dataRaptor.st_backward[idx];
        BOOST_ASSERT(JpIdx(*st->journey_pattern_point->journey_pattern) == jp_idx);
        BOOST_ASSERT(st->journey_pattern_point->order == jpp_order);
        if (is_valid(st, date, true, disruption_active, !reconstructing_path, required_vehicle_properties)) {
            return {st, DateTimeUtils::set(date, st->arrival_time % DateTimeUtils::SECONDS_PER_DAY)};
        }
    }

    return {nullptr, DateTimeUtils::not_valid};
}

/**
 * earliest_stop_time function:
 * we look for the next valid stop time valid the day of the date time and after the hour of the date time.
 *
 * 2 lookup are done,
 *  one on the ordered vector of departures (for the non frequency VJ),
 *  and one on the frenquency VJ (for the moment we loop through all frequency VJ, no clever data structure, we'll see if it is worth adding it)
 *
 *      Note: if nothing found, we do the same lookup the day after
 */
static std::pair<const type::StopTime*, DateTime>
earliest_stop_time(JpIdx jp_idx,
                   uint16_t jpp_order,
                   const DateTime dt,
                   const type::Data &data,
                   bool disruption_active,
                   bool reconstructing_path,
                   const type::VehicleProperties& vehicle_properties,
                   bool check_freq = true) {
    //We look for the earliest stop time of the journey_pattern >= dt.hour()

    //Return the first valid trip
    const auto first_discrete_st_pair =
        next_valid_discrete_pick_up(jp_idx, jpp_order, dt, *data.dataRaptor, reconstructing_path,
                                    vehicle_properties, disruption_active);

    if (check_freq) {
        //TODO use first_discrete as a LB ?
        const auto first_frequency_st_pair =
            next_valid_frequency_pick_up(get_jpp(jp_idx, jpp_order, data), dt, reconstructing_path,
                                         vehicle_properties, disruption_active);

        //we need to find the earliest between the frequency one and the 'normal' one
        if (first_frequency_st_pair.second < first_discrete_st_pair.second) {
            return first_frequency_st_pair;
        }
    }

    return first_discrete_st_pair;
}

std::pair<const type::StopTime*, DateTime>
earliest_stop_time(const type::JourneyPatternPoint* jpp,
                   const DateTime dt, const type::Data &data,
                   bool disruption_active,
                   bool reconstructing_path,
                   const type::VehicleProperties& vehicle_properties) {
    return earliest_stop_time(JpIdx(*jpp->journey_pattern),
                              jpp->order,
                              dt,
                              data,
                              disruption_active,
                              reconstructing_path,
                              vehicle_properties);
}

static std::pair<const type::StopTime*, DateTime>
tardiest_stop_time(JpIdx jp_idx,
                   uint16_t jpp_order,
                   const DateTime dt,
                   const type::Data &data,
                   bool disruption_active,
                   bool reconstructing_path,
                   const type::VehicleProperties& vehicle_properties,
                   bool check_freq = true) {

    const auto first_discrete_st_pair =
        previous_valid_discrete_drop_off(jp_idx, jpp_order, dt, *data.dataRaptor, reconstructing_path,
                                         vehicle_properties, disruption_active);

    if (check_freq) {
        //TODO use first_discrete as a LB ?
        const auto first_frequency_st_pair =
            previous_valid_frequency_drop_off(get_jpp(jp_idx, jpp_order, data), dt, reconstructing_path,
                                              vehicle_properties, disruption_active);

        // since the default value is DateTimeUtils::not_valid (== DateTimeUtils::max)
        // we need to check first that they are
        if (first_discrete_st_pair.second == DateTimeUtils::not_valid) {
            return first_frequency_st_pair;
        }
        if (first_frequency_st_pair.second == DateTimeUtils::not_valid) {
            return first_discrete_st_pair;
        }
        //we need to find the tardiest between the frequency one and the 'normal' one
        if (first_frequency_st_pair.second < first_discrete_st_pair.second) {
            return first_frequency_st_pair;
        }
    }

    return first_discrete_st_pair;
}

std::pair<const type::StopTime*, DateTime>
tardiest_stop_time(const type::JourneyPatternPoint* jpp,
                   const DateTime dt,
                   const type::Data &data,
                   bool disruption_active,
                   bool reconstructing_path,
                   const type::VehicleProperties& vehicle_properties) {
    return tardiest_stop_time(JpIdx(*jpp->journey_pattern),
                              jpp->order,
                              dt,
                              data,
                              disruption_active,
                              reconstructing_path,
                              vehicle_properties);
}

std::pair<const type::StopTime*, uint32_t>
best_stop_time(const JpIdx jp_idx,
               const uint16_t jpp_order,
               const DateTime dt,
               const type::VehicleProperties& vehicle_properties,
               const bool clockwise,
               bool disruption_active,
               const type::Data &data,
               bool reconstructing_path,
               bool check_freq) {
    if(clockwise)
        return earliest_stop_time(jp_idx, jpp_order, dt, data, disruption_active,
                                  reconstructing_path, vehicle_properties, check_freq);
    else
        return tardiest_stop_time(jp_idx, jpp_order, dt, data, disruption_active,
                                  reconstructing_path, vehicle_properties, check_freq);
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


/**
* Here we want the next dt in the period of the frequency.
* If none is found, we return DateTimeUtils::inf
*
*  In normal case we have something like:
* 0-------------------------------------86400(midnight)
*     start-------end
*
* in this case, it's easy, hour should be in [start; end]
* if before, next departure is start, if after, no next departure
*
*
* If end_time, is after midnight, so end_time%86400 will be < start_time
*
* So we will have something like:
* 0-------------------------------------86400----------------------------86400*2
*                             start----------------end
*
* by check that on only one day (modulating by 86400), we have:
* 0-------------------------------------86400(midnight)
*  -------end                 start--------
*
* Note: If hour in [0, end] we have to check the previous day's validity pattern
**/
DateTime get_next_departure(DateTime dt, const type::FrequencyVehicleJourney& freq_vj, const type::StopTime& st, const bool adapted) {
    const u_int32_t lower_bound = (freq_vj.start_time + st.departure_time) % DateTimeUtils::SECONDS_PER_DAY;
    const u_int32_t upper_bound = (freq_vj.end_time + st.departure_time) % DateTimeUtils::SECONDS_PER_DAY;

    auto hour = DateTimeUtils::hour(dt);
    auto date = DateTimeUtils::date(dt);

    const bool normal_case = lower_bound <= upper_bound;

    //in the 'normal' case, hour should be in [lower, higher]
    // but in case of midnight overrun, hour should be in [higher, lower]
    if (normal_case) {
        if (hour <= lower_bound) {
            if (freq_vj.is_valid(date, adapted)) {
                return DateTimeUtils::set(date, lower_bound);
            }
            return DateTimeUtils::inf;
        }
        if (hour > upper_bound) {
            //no solution on the day
            return DateTimeUtils::inf;
        }
    }

    if (normal_case || //in classic case, we must be in [start, end]
            (! normal_case && within(hour, {lower_bound, DateTimeUtils::SECONDS_PER_DAY}))) {
        //we need to check if the vj is valid for our day
        if (! freq_vj.is_valid(date, adapted)) {
            return DateTimeUtils::inf;
        }

         double diff = hour - lower_bound;
         const uint32_t x = std::ceil(diff / double(freq_vj.headway_secs));
         return DateTimeUtils::set(date, lower_bound + x * freq_vj.headway_secs);
    }

    // overnight case and hour > midnight
    if (hour > upper_bound) {
        if (freq_vj.is_valid(date, adapted)) {
            return DateTimeUtils::set(date, lower_bound);
        }
        return DateTimeUtils::inf;
    }
    //we need to see if the vj was valid the day before
    if (! freq_vj.is_valid(date - 1, adapted)) {
        //the vj was not valid, next departure is lower
        return DateTimeUtils::set(date, lower_bound);
    }

    double diff = hour - lower_bound + DateTimeUtils::SECONDS_PER_DAY;
    const int32_t x = std::ceil(diff / double(freq_vj.headway_secs));

    return DateTimeUtils::set(date, (lower_bound + x * freq_vj.headway_secs) % DateTimeUtils::SECONDS_PER_DAY);
}

DateTime get_previous_arrival(DateTime dt, const type::FrequencyVehicleJourney& freq_vj, const type::StopTime& st, const bool adapted) {
    const u_int32_t lower_bound = (freq_vj.start_time + st.arrival_time) % DateTimeUtils::SECONDS_PER_DAY;
    const u_int32_t upper_bound = (freq_vj.end_time + st.arrival_time) % DateTimeUtils::SECONDS_PER_DAY;

    auto hour = DateTimeUtils::hour(dt);
    auto date = DateTimeUtils::date(dt);

    const bool normal_case = lower_bound <= upper_bound;

    // in the 'normal' case, hour should be in [lower, higher]
    // but in case of midnight overrun, hour should be in [higher, lower]
    if (normal_case) {
        if (hour >= upper_bound) {
            if (freq_vj.is_valid(date, adapted)) {
                return DateTimeUtils::set(date, upper_bound);
            }
            return DateTimeUtils::not_valid;
        }
        if (hour < lower_bound) {
            //no solution on the day
            return DateTimeUtils::not_valid;
        }
    }

    if (normal_case || //in classic case, we must be in [start, end]
            (! normal_case && within(hour, {lower_bound, DateTimeUtils::SECONDS_PER_DAY}))) {
        //we need to check if the vj is valid for our day
        if (! freq_vj.is_valid(date, adapted)) {
            return DateTimeUtils::not_valid;
        }

         double diff = hour - lower_bound;
         const uint32_t x = std::floor(diff / double(freq_vj.headway_secs));
         return DateTimeUtils::set(date, lower_bound + x * freq_vj.headway_secs);
    }

    // overnight case and hour > midnight
    if (hour > upper_bound) {
        //case with hour in [upper_bound, lower_boud]
        if (freq_vj.is_valid(date, adapted)) {
            return DateTimeUtils::set(date, upper_bound);
        }
        return DateTimeUtils::not_valid;
    }
    //we need to see if the vj was valid the day before
    if (! freq_vj.is_valid(date - 1, adapted)) {
        //the vj was not valid, next departure is lower
        return DateTimeUtils::not_valid;
    }

    const double diff = hour - lower_bound + DateTimeUtils::SECONDS_PER_DAY;
    const int32_t x = std::floor(diff / double(freq_vj.headway_secs));

    return DateTimeUtils::set(date - 1, lower_bound + x * freq_vj.headway_secs);

}
}}

