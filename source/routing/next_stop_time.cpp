/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

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

#include "next_stop_time.h"

#include "dataraptor.h"
#include "get_stop_times.h"
#include "type/data.h"
#include "type/pt_data.h"
#include "type/meta_data.h"

#include <boost/range/algorithm/sort.hpp>

namespace navitia { namespace routing {

DateTime NextStopTimeData::Departure::get_time(const type::StopTime& st) const {
    return st.departure_time;
}
bool NextStopTimeData::Departure::is_valid(const type::StopTime& st) const {
    return st.valid_begin(true);
}
DateTime NextStopTimeData::Arrival::get_time(const type::StopTime& st) const {
    return st.arrival_time;
}
bool NextStopTimeData::Arrival::is_valid(const type::StopTime& st) const {
    return st.valid_begin(false);
}

template<typename Getter>
void NextStopTimeData::TimesStopTimes<Getter>::init(const JourneyPattern& jp,
        const JourneyPatternPoint& jpp)
        {
    // collect the stop times at the given jpp
    const size_t jpp_order = jpp.order;
    stop_times.reserve(jp.discrete_vjs.size());
    for(const auto& vj: jp.discrete_vjs) {
        const auto& st = vj->stop_time_list[jpp_order];
        if (! getter.is_valid(st)) { continue; }
        stop_times.push_back(&st);
    }

    // sort the stop times in ascending order
    boost::sort(stop_times, [&](const type::StopTime* st1, const type::StopTime* st2) {
        const auto time1 = DateTimeUtils::hour(getter.get_time(*st1));
        const auto time2 = DateTimeUtils::hour(getter.get_time(*st2));
        if (time1 != time2) { return time1 < time2; }
        const auto& st1_first = st1->vehicle_journey->stop_time_list.front();
        const auto& st2_first = st2->vehicle_journey->stop_time_list.front();
        if (getter.get_time(st1_first) != getter.get_time(st2_first)) {
            return getter.get_time(st1_first) < getter.get_time(st2_first);
        }
        return st1_first.vehicle_journey->idx < st2_first.vehicle_journey->idx;
    });

    // collect the corresponding times
    times.reserve(stop_times.size());
    for (const auto* st: stop_times) {
        times.push_back(DateTimeUtils::hour(getter.get_time(*st)));
    }
}

void NextStopTimeData::load(const JourneyPatternContainer& jp_container) {
    departure.assign(jp_container.get_jpps_values());
    arrival.assign(jp_container.get_jpps_values());

    for (const auto& jp: jp_container.get_jps()) {
        for (const auto& jpp_idx: jp.second.jpps) {
            const auto& jpp = jp_container.get(jpp_idx);
            departure[jpp_idx].init(jp.second, jpp);
            arrival[jpp_idx].init(jp.second, jpp);
        }
    }
}

inline static bool
is_valid(const type::StopTime* st,
        const DateTime date,
        const bool clockwise,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props)
{
    return st->is_valid_day(date, !clockwise, rt_level) &&
            st->vehicle_journey->accessible(vehicle_props);
}

/** Which is the first valid stop_time in this range ?
 *  Returns invalid_idx is none is
 */
static std::pair<const type::StopTime*, DateTime>
next_valid_discrete(const StopEvent stop_event,
        const dataRAPTOR& dataRaptor,
        const JppIdx jpp_idx,
        const DateTime dt,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props,
        const DateTime bound) {
    auto date = DateTimeUtils::date(dt);
    for (const auto* st: dataRaptor.next_stop_time_data.stop_time_range_after(jpp_idx, dt, stop_event)) {
        assert(dataRaptor.jp_container.get_jpp(*st) == jpp_idx);
        const uint32_t hour = (stop_event == StopEvent::pick_up) ? st->departure_time : st->arrival_time;
        const DateTime cur_dt = DateTimeUtils::set(date, DateTimeUtils::hour(hour));
        if (bound < cur_dt) { return {nullptr, DateTimeUtils::inf}; }
        if (is_valid(st, date, true, rt_level, vehicle_props)) {
            return {st, cur_dt};
        }
    }

    //if none was found, we try again the next day
    date++;
    for (const auto* st: dataRaptor.next_stop_time_data.stop_time_range_forward(jpp_idx, stop_event)) {
        assert(dataRaptor.jp_container.get_jpp(*st) == jpp_idx);
        const uint32_t hour = (stop_event == StopEvent::pick_up) ? st->departure_time : st->arrival_time;
        const DateTime cur_dt = DateTimeUtils::set(date, DateTimeUtils::hour(hour));
        if (bound < cur_dt) { return {nullptr, DateTimeUtils::inf}; }
        if (is_valid(st, date, true, rt_level, vehicle_props)) {
            return {st, cur_dt};
        }
    }

    // if nothing found, return max
    return {nullptr, DateTimeUtils::inf};
}

static std::pair<const type::StopTime*, DateTime>
next_valid_frequency(const StopEvent stop_event,
        const dataRAPTOR& dataRaptor,
        const JppIdx jpp_idx,
        const DateTime dt,
        const type::RTLevel rt_level,
        const type::VehicleProperties &vehicle_props) {
    // to find the next frequency VJ, for the moment we loop through all frequency VJ of the JP
    // and for each jp, get compute the datetime on the jpp
    const auto& jpp = dataRaptor.jp_container.get(jpp_idx);
    const auto& jp = dataRaptor.jp_container.get(jpp.jp_idx);
    std::pair<const type::StopTime*, DateTime> best = {nullptr, DateTimeUtils::inf};
    for (const auto& freq_vj: jp.freq_vjs) {
        const auto& st = freq_vj->stop_time_list[jpp.order];

        if (! freq_vj->accessible(vehicle_props)) { continue; }
        if (stop_event == StopEvent::pick_up && ! st.pick_up_allowed()) { continue; }
        if (stop_event == StopEvent::drop_off && ! st.drop_off_allowed()) { continue; }

        const auto next_dt = get_next_stop_time(stop_event, dt, *freq_vj, st, rt_level);

        if (next_dt < best.second) {
            best = {&st, next_dt};
        }
    }

    if (best.first == nullptr) {
        const auto next_date = DateTimeUtils::set(DateTimeUtils::date(dt) + 1, 0);
        for (const auto& freq_vj: jp.freq_vjs) {
            const auto& st = freq_vj->stop_time_list[jpp.order];

            if (! freq_vj->accessible(vehicle_props)) { continue; }
            if (stop_event == StopEvent::pick_up && ! st.pick_up_allowed()) { continue; }
            if (stop_event == StopEvent::drop_off && ! st.drop_off_allowed()) { continue; }

            const auto next_dt = get_next_stop_time(stop_event, next_date, *freq_vj, st, rt_level);

            if (next_dt < best.second) {
                best = {&st, next_dt};
            }
        }
    }
    return best;
}

static std::pair<const type::StopTime*, DateTime>
previous_valid_frequency(const StopEvent stop_event,
        const dataRAPTOR& dataRaptor,
        const JppIdx jpp_idx,
        const DateTime dt,
        const type::RTLevel rt_level,
        const type::VehicleProperties &vehicle_props) {
    const auto& jpp = dataRaptor.jp_container.get(jpp_idx);
    const auto& jp = dataRaptor.jp_container.get(jpp.jp_idx);
    std::pair<const type::StopTime*, DateTime> best = {nullptr, DateTimeUtils::not_valid};
    for (const auto& freq_vj: jp.freq_vjs) {
        const auto& st = freq_vj->stop_time_list[jpp.order];

        if (! freq_vj->accessible(vehicle_props)) { continue; }
        if (stop_event == StopEvent::pick_up && ! st.pick_up_allowed()) { continue; }
        if (stop_event == StopEvent::drop_off && ! st.drop_off_allowed()) { continue; }

        const auto previous_dt = get_previous_stop_time(stop_event, dt, *freq_vj, st, rt_level);

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
        for (const auto& freq_vj: jp.freq_vjs) {
            const auto& st = freq_vj->stop_time_list[jpp.order];

            if (! freq_vj->accessible(vehicle_props)) { continue; }
            if (stop_event == StopEvent::pick_up && ! st.pick_up_allowed()) { continue; }
            if (stop_event == StopEvent::drop_off && ! st.drop_off_allowed()) { continue; }

            const auto previous_dt = get_previous_stop_time(stop_event, previous_date, *freq_vj,
                    st, rt_level);

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
previous_valid_discrete(const StopEvent stop_event,
        const dataRAPTOR& dataRaptor,
        const JppIdx jpp_idx,
        const DateTime dt,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props,
        const DateTime bound) {
    auto date = DateTimeUtils::date(dt);
    for (const auto* st: dataRaptor.next_stop_time_data.stop_time_range_before(jpp_idx, dt, stop_event)) {
        assert(dataRaptor.jp_container.get_jpp(*st) == jpp_idx);
        const uint32_t hour = (stop_event == StopEvent::pick_up) ? st->departure_time : st->arrival_time;
        const DateTime cur_dt = DateTimeUtils::set(date, DateTimeUtils::hour(hour));
        if (bound > cur_dt) { return {nullptr, DateTimeUtils::not_valid}; }
        if (is_valid(st, date, false, rt_level, vehicle_props)) {
            return {st, cur_dt};
        }
    }

    if (date == 0) {
        return {nullptr, DateTimeUtils::not_valid};
    }

    --date;
    for (const auto* st: dataRaptor.next_stop_time_data.stop_time_range_backward(jpp_idx, stop_event)) {
        assert(dataRaptor.jp_container.get_jpp(*st) == jpp_idx);
        const uint32_t hour = (stop_event == StopEvent::pick_up) ? st->departure_time : st->arrival_time;
        const DateTime cur_dt = DateTimeUtils::set(date, DateTimeUtils::hour(hour));
        if (bound > cur_dt) { return {nullptr, DateTimeUtils::not_valid}; }
        if (is_valid(st, date, false, rt_level, vehicle_props)) {
            return {st, cur_dt};
        }
    }

    return {nullptr, DateTimeUtils::not_valid};
}

std::pair<const type::StopTime*, DateTime>
NextStopTime::earliest_stop_time(const StopEvent stop_event,
        const JppIdx jpp_idx,
        const DateTime dt,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props,
        const bool check_freq,
        const DateTime bound) const
{
    const auto first_discrete_st_pair =
            next_valid_discrete(stop_event, *data.dataRaptor, jpp_idx, dt, rt_level, vehicle_props, bound);

    if (check_freq) {
        const auto first_frequency_st_pair =
                next_valid_frequency(stop_event, *data.dataRaptor, jpp_idx, dt, rt_level, vehicle_props);

        if (first_frequency_st_pair.second < first_discrete_st_pair.second) {
            return first_frequency_st_pair;
        }
    }

    return first_discrete_st_pair;
}

std::pair<const type::StopTime*, DateTime>
NextStopTime::tardiest_stop_time(const StopEvent stop_event,
        const JppIdx jpp_idx,
        const DateTime dt,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props,
        const bool check_freq,
        const DateTime bound) const
{
    const auto first_discrete_st_pair =
            previous_valid_discrete(stop_event, *data.dataRaptor, jpp_idx, dt, rt_level, vehicle_props, bound);

    if (check_freq) {
        const auto first_frequency_st_pair =
                previous_valid_frequency(stop_event, *data.dataRaptor, jpp_idx, dt, rt_level, vehicle_props);

        // since the default value is DateTimeUtils::not_valid (== DateTimeUtils::max)
        // we need to check first that they are
        if (first_discrete_st_pair.second == DateTimeUtils::not_valid) {
            return first_frequency_st_pair;
        }
        if (first_frequency_st_pair.second == DateTimeUtils::not_valid) {
            return first_discrete_st_pair;
        }

        if (first_frequency_st_pair.second < first_discrete_st_pair.second) {
            return first_frequency_st_pair;
        }
    }

    return first_discrete_st_pair;
}


/*
 * Discrete VJs and Frequency VJs are looped differently when computing the next stop time
 *
 * For Frequency VJs, we need to loop over its active period since several vjs are instantiated actually
 *
 * For Discrete VJs, there is no need to do that, because we get only one VJ.
 *
 * */
template<typename F>
static void vj_loop(const nt::DiscreteVehicleJourney*, F f) {
    f(0);
}

template<typename F>
static void vj_loop(const nt::FrequencyVehicleJourney* vj, F f) {
    for (auto freq_shift = vj->start_time; freq_shift <= vj->end_time; freq_shift+= vj->headway_secs) {
        f(freq_shift);
    }
}


template<typename VJ_T>
static void fill_cache(const DateTime from,
        const DateTime to,
        const type::RTLevel rt_level,
        const type::AccessibiliteParams& accessibilite_params,
        const JourneyPattern& jp,
        const std::vector<const VJ_T*>& vjs,
        IdxMap<JourneyPatternPoint, std::vector<CachedNextStopTime::DtSt>>& arrival_cache,
        IdxMap<JourneyPatternPoint, std::vector<CachedNextStopTime::DtSt>>& departure_cache){

    const int to_int = static_cast<int>(DateTimeUtils::date(to));
    // In case of Vj that passes midnight, we should compute one day before "from"
    const int from_int = std::max(static_cast<int>(DateTimeUtils::date(from)) - 1, 0);

    for (const auto* vj : vjs) {
        if (! vj->accessible(accessibilite_params.vehicle_properties)) {
            continue;
        }
        // test validity pattern of vj
        const auto* vp = vj->validity_patterns[rt_level];
        for (int day = from_int; day <= to_int ; ++day) {
            if (! vp->check(day)) {
                continue;
            }
            const auto shift = navitia::DateTimeUtils::SECONDS_PER_DAY * day;
            size_t i = 0;
            for (const auto& st : vj->stop_time_list) {
                auto jpp_idx = jp.jpps[i];
                auto loop_impl = [&](DateTime freq_shift){
                    if (st.drop_off_allowed()) {
                        auto arrival_time = st.arrival_time + shift + freq_shift;
                        if (from <= arrival_time && arrival_time <= to) {
                            arrival_cache[jpp_idx].emplace_back(arrival_time, &st);
                        }
                    }
                    if (st.pick_up_allowed()) {
                        auto departure_time = st.departure_time + shift + freq_shift;
                        if (departure_time <= to && from <= departure_time ) {
                            departure_cache[jpp_idx].emplace_back(departure_time, &st);
                        }
                    }
                };
                // loop is done differently according to the type of Vehicle Journey
                vj_loop(vj, loop_impl);
                ++i;
            }
        }
    }
}

bool CachedNextStopTimeKey::operator<(const CachedNextStopTimeKey& other) const {
    if (from != other.from) {
        return from < other.from;
    } else if (rt_level != other.rt_level) {
        return rt_level < other.rt_level;
    }
    return accessibilite_params < other.accessibilite_params;
}

CachedNextStopTime CachedNextStopTimeManager::Fun::operator()(const CachedNextStopTimeKey& key) const {
    CachedNextStopTime result;
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_INFO(logger, "Cache miss : " << ++cache_miss);
    const auto& jp_container = data.dataRaptor->jp_container;

    result.departure.assign(jp_container.get_jpps_values());
    result.arrival.assign(jp_container.get_jpps_values());
    DateTime dt_from = DateTimeUtils::set(key.from, 0);
    DateTime dt_to = DateTimeUtils::set(key.from + 2, 0);

    for( const auto& jp : jp_container.get_jps_values() ) {
        fill_cache(dt_from, dt_to, key.rt_level, key.accessibilite_params, jp,
                jp.discrete_vjs, result.arrival, result.departure);
        fill_cache(dt_from, dt_to, key.rt_level, key.accessibilite_params, jp,
                jp.freq_vjs, result.arrival, result.departure);
    }
    auto compare = [](const CachedNextStopTime::DtSt& lhs, const CachedNextStopTime::DtSt& rhs) noexcept{
        return lhs.first < rhs.first;
    };
    for (const auto& jpp_dtst : result.arrival) {
        boost::sort(jpp_dtst.second, compare);
    }
    for (const auto& jpp_dtst : result.departure) {
        boost::sort(jpp_dtst.second, compare);
    }
    return result;
}

void CachedNextStopTimeManager::load(const DateTime from,
                                     const type::RTLevel rt_level,
                                     const type::AccessibiliteParams& accessibilite_params) {
    CachedNextStopTimeKey key(DateTimeUtils::date(from), rt_level, accessibilite_params);
    cache = &lru(key);
}

std::pair<const type::StopTime*, DateTime>
CachedNextStopTimeManager::next_stop_time(const StopEvent stop_event,
        const JppIdx jpp_idx,
        const DateTime dt,
        const bool clockwise) const {
    const auto& v = stop_event == StopEvent::pick_up ? cache->departure[jpp_idx] : cache->arrival[jpp_idx];
    const type::StopTime* null_st = nullptr;
    decltype(v.begin()) search;
    auto cmp = [](const CachedNextStopTime::DtSt& a, const CachedNextStopTime::DtSt& b) noexcept {
        return a.first < b.first;
    };
    if (clockwise) {
        search = boost::lower_bound(v, std::make_pair(dt, null_st), cmp);
    } else {
        search = boost::upper_bound(v, std::make_pair(dt, null_st), cmp);
        if (search == v.begin()) {
            search = v.end();
        } else if (!v.empty()) {
            --search;
        }
    }
    if (search != v.end()) {
        return {search->second, search->first};
    }
    return {nullptr, 0};
}



inline static bool within(u_int32_t val, std::pair<u_int32_t, u_int32_t> bound) {
    return val >= bound.first && val <= bound.second;
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
DateTime get_next_stop_time(const StopEvent stop_event,
        DateTime dt,
        const type::FrequencyVehicleJourney& freq_vj,
        const type::StopTime& st,
        const type::RTLevel rt_level) {
    const u_int32_t considered_time = (stop_event == StopEvent::pick_up) ? st.departure_time : st.arrival_time;
    const u_int32_t lower_bound = DateTimeUtils::hour(freq_vj.start_time + considered_time);
    const u_int32_t upper_bound = DateTimeUtils::hour(freq_vj.end_time + considered_time);

    auto hour = DateTimeUtils::hour(dt);
    auto date = DateTimeUtils::date(dt);

    const bool normal_case = lower_bound <= upper_bound;

    //in the 'normal' case, hour should be in [lower, higher]
    // but in case of midnight overrun, hour should be in [higher, lower]
    if (normal_case) {
        if (hour <= lower_bound) {
            if (freq_vj.is_valid(date, rt_level)) {
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
        if (! freq_vj.is_valid(date, rt_level)) {
            return DateTimeUtils::inf;
        }

        double diff = hour - lower_bound;
        const uint32_t x = std::ceil(diff / double(freq_vj.headway_secs));
        return DateTimeUtils::set(date, lower_bound + x * freq_vj.headway_secs);
    }

    // overnight case and hour > midnight
    if (hour > upper_bound) {
        if (freq_vj.is_valid(date, rt_level)) {
            return DateTimeUtils::set(date, lower_bound);
        }
        return DateTimeUtils::inf;
    }
    //we need to see if the vj was valid the day before
    if (! freq_vj.is_valid(date - 1, rt_level)) {
        //the vj was not valid, next departure is lower
        return DateTimeUtils::set(date, lower_bound);
    }

    double diff = hour - lower_bound + DateTimeUtils::SECONDS_PER_DAY;
    const int32_t x = std::ceil(diff / double(freq_vj.headway_secs));

    return DateTimeUtils::set(date, DateTimeUtils::hour(lower_bound + x * freq_vj.headway_secs));
}

DateTime get_previous_stop_time(const StopEvent stop_event,
        DateTime dt,
        const type::FrequencyVehicleJourney& freq_vj,
        const type::StopTime& st,
        const type::RTLevel rt_level) {
    const u_int32_t considered_time = (stop_event == StopEvent::pick_up) ?
            st.departure_time : st.arrival_time;
    const u_int32_t lower_bound = DateTimeUtils::hour(freq_vj.start_time + considered_time);
    const u_int32_t upper_bound = DateTimeUtils::hour(freq_vj.end_time + considered_time);

    auto hour = DateTimeUtils::hour(dt);
    auto date = DateTimeUtils::date(dt);

    const bool normal_case = lower_bound <= upper_bound;

    // in the 'normal' case, hour should be in [lower, higher]
    // but in case of midnight overrun, hour should be in [higher, lower]
    if (normal_case) {
        if (hour >= upper_bound) {
            if (freq_vj.is_valid(date, rt_level)) {
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
        if (! freq_vj.is_valid(date, rt_level)) {
            return DateTimeUtils::not_valid;
        }

        double diff = hour - lower_bound;
        const uint32_t x = std::floor(diff / double(freq_vj.headway_secs));
        return DateTimeUtils::set(date, lower_bound + x * freq_vj.headway_secs);
    }

    // overnight case and hour > midnight
    if (hour > upper_bound) {
        //case with hour in [upper_bound, lower_boud]
        if (freq_vj.is_valid(date, rt_level)) {
            return DateTimeUtils::set(date, upper_bound);
        }
        return DateTimeUtils::not_valid;
    }
    //we need to see if the vj was valid the day before
    if (! freq_vj.is_valid(date - 1, rt_level)) {
        //the vj was not valid, next departure is lower
        return DateTimeUtils::not_valid;
    }

    const double diff = hour - lower_bound + DateTimeUtils::SECONDS_PER_DAY;
    const int32_t x = std::floor(diff / double(freq_vj.headway_secs));

    return DateTimeUtils::set(date - 1, lower_bound + x * freq_vj.headway_secs);

}

}} // namespace navitia::routing
