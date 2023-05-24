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

#include "next_stop_time.h"

#include "dataraptor.h"
#include "type/data.h"
#include "type/meta_data.h"
#include "type/pt_data.h"
#include "type/type_utils.h"
#include "type/vehicle_journey.h"
#include "utils/logger.h"

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

namespace nt = navitia::type;

namespace navitia {
namespace routing {

DateTime NextStopTimeData::Departure::get_time(const type::StopTime& st) const {
    return st.boarding_time;
}
bool NextStopTimeData::Departure::is_valid(const type::StopTime& st) const {
    return st.valid_begin(true);
}
DateTime NextStopTimeData::Arrival::get_time(const type::StopTime& st) const {
    return st.alighting_time;
}
bool NextStopTimeData::Arrival::is_valid(const type::StopTime& st) const {
    return st.valid_begin(false);
}

template <typename Getter>
void NextStopTimeData::TimesStopTimes<Getter>::init(const JourneyPattern& jp, const JourneyPatternPoint& jpp) {
    using StopTimePair = std::pair<const type::StopTime*, const type::StopTime*>;
    // collect the stop times at the given jpp
    const auto jpp_order = jpp.order;
    std::vector<StopTimePair> stop_times_and_earliest_stop_time;
    stop_times_and_earliest_stop_time.reserve(jp.discrete_vjs.size());
    for (const auto& vj : jp.discrete_vjs) {
        const type::StopTime& st = get_corresponding_stop_time(*vj, jpp_order);
        if (!getter.is_valid(st)) {
            continue;
        }
        const type::StopTime& earliest_stop_time = navitia::earliest_stop_time(st.vehicle_journey->stop_time_list);
        StopTimePair pair = StopTimePair(&st, &earliest_stop_time);
        stop_times_and_earliest_stop_time.push_back(pair);
    }

    // sort the stop times in ascending order
    boost::sort(stop_times_and_earliest_stop_time, [&](StopTimePair pair1, StopTimePair pair2) {
        const type::StopTime* st1 = pair1.first;
        const type::StopTime* st2 = pair2.first;
        const auto time1 = DateTimeUtils::hour(getter.get_time(*st1));
        const auto time2 = DateTimeUtils::hour(getter.get_time(*st2));
        if (time1 != time2) {
            return time1 < time2;
        }
        const type::StopTime* st1_first = pair1.second;
        const type::StopTime* st2_first = pair2.second;
        auto st1_first_time = getter.get_time(*st1_first);
        auto st2_first_time = getter.get_time(*st2_first);
        if (st1_first_time != st2_first_time) {
            return st1_first_time < st2_first_time;
        }
        return st1_first->vehicle_journey->idx < st2_first->vehicle_journey->idx;
    });
    stop_times.reserve(jp.discrete_vjs.size());
    for (const auto& pair : stop_times_and_earliest_stop_time) {
        stop_times.push_back(pair.first);
    }

    // collect the corresponding times
    times.reserve(stop_times.size());
    for (const auto* st : stop_times) {
        times.push_back(DateTimeUtils::hour(getter.get_time(*st)));
    }
}

void NextStopTimeData::load(const JourneyPatternContainer& jp_container) {
    departure.assign(jp_container.get_jpps_values());
    arrival.assign(jp_container.get_jpps_values());

    for (const auto jp : jp_container.get_jps()) {
        for (const auto& jpp_idx : jp.second.jpps) {
            const auto& jpp = jp_container.get(jpp_idx);
            departure[jpp_idx].init(jp.second, jpp);
            arrival[jpp_idx].init(jp.second, jpp);
        }
    }
}

inline static bool is_valid(const type::StopTime* st,
                            const DateTime date,
                            const bool clockwise,
                            const type::RTLevel rt_level,
                            const type::VehicleProperties& vehicle_props) {
    return st->is_valid_day(date, !clockwise, rt_level) && st->vehicle_journey->accessible(vehicle_props);
}

/** Which is the first valid stop_time in this range ?
 *  Returns invalid_idx is none is
 */
static std::pair<const type::StopTime*, DateTime> next_valid_discrete(const StopEvent stop_event,
                                                                      const dataRAPTOR& dataRaptor,
                                                                      const JppIdx jpp_idx,
                                                                      const DateTime dt,
                                                                      const type::RTLevel rt_level,
                                                                      const type::VehicleProperties& vehicle_props,
                                                                      const DateTime bound) {
    auto date = DateTimeUtils::date(dt);
    // On the first iteration we only check the stop_times after dt, we init the range with this
    auto st_range = dataRaptor.next_stop_time_data.stop_time_range_after(jpp_idx, dt, stop_event);

    while (DateTimeUtils::date(bound) >= date) {
        for (const auto* st : st_range) {
            assert(dataRaptor.jp_container.get_jpp(*st) == jpp_idx);
            const uint32_t hour = (stop_event == StopEvent::pick_up) ? st->boarding_time : st->alighting_time;
            const DateTime cur_dt = DateTimeUtils::set(date, DateTimeUtils::hour(hour));
            if (bound < cur_dt) {
                return {nullptr, DateTimeUtils::inf};
            }
            if (is_valid(st, date, true, rt_level, vehicle_props)) {
                return {st, cur_dt};
            }
        }
        // If this is the first day (first iteration) change the range to all the stop_times
        if (date == DateTimeUtils::date(dt)) {
            st_range = dataRaptor.next_stop_time_data.stop_time_range_forward(jpp_idx, stop_event);
        }
        date++;
    }

    // if nothing found, return max
    return {nullptr, DateTimeUtils::inf};
}

static std::pair<const type::StopTime*, DateTime> next_valid_frequency(const StopEvent stop_event,
                                                                       const dataRAPTOR& dataRaptor,
                                                                       const JppIdx jpp_idx,
                                                                       const DateTime dt,
                                                                       const type::RTLevel rt_level,
                                                                       const type::VehicleProperties& vehicle_props,
                                                                       const DateTime bound) {
    // to find the next frequency VJ, for the moment we loop through all frequency VJ of the JP
    // and for each jp, get compute the datetime on the jpp
    const auto& jpp = dataRaptor.jp_container.get(jpp_idx);
    const auto& jp = dataRaptor.jp_container.get(jpp.jp_idx);
    std::pair<const type::StopTime*, DateTime> best = {nullptr, DateTimeUtils::inf};
    auto base_dt = dt;

    while (best.first == nullptr && base_dt <= bound) {
        for (const auto& freq_vj : jp.freq_vjs) {
            const auto& st = get_corresponding_stop_time(*freq_vj, jpp.order);

            if (!freq_vj->accessible(vehicle_props)) {
                continue;
            }
            if (stop_event == StopEvent::pick_up && !st.pick_up_allowed()) {
                continue;
            }
            if (stop_event == StopEvent::drop_off && !st.drop_off_allowed()) {
                continue;
            }

            const auto next_dt = get_next_stop_time(stop_event, base_dt, *freq_vj, st, rt_level);
            if (next_dt < best.second && next_dt <= bound) {
                best = {&st, next_dt};
            }
        }
        base_dt = DateTimeUtils::set(DateTimeUtils::date(base_dt) + 1, 0);
    }

    return best;
}

static std::pair<const type::StopTime*, DateTime> previous_valid_frequency(const StopEvent stop_event,
                                                                           const dataRAPTOR& dataRaptor,
                                                                           const JppIdx jpp_idx,
                                                                           const DateTime dt,
                                                                           const type::RTLevel rt_level,
                                                                           const type::VehicleProperties& vehicle_props,
                                                                           const DateTime bound) {
    const auto& jpp = dataRaptor.jp_container.get(jpp_idx);
    const auto& jp = dataRaptor.jp_container.get(jpp.jp_idx);
    std::pair<const type::StopTime*, DateTime> best = {nullptr, DateTimeUtils::not_valid};
    auto base_dt = dt;

    while (best.first == nullptr && base_dt >= bound) {
        for (const auto& freq_vj : jp.freq_vjs) {
            const auto& st = get_corresponding_stop_time(*freq_vj, jpp.order);

            if (!freq_vj->accessible(vehicle_props)) {
                continue;
            }
            if (stop_event == StopEvent::pick_up && !st.pick_up_allowed()) {
                continue;
            }
            if (stop_event == StopEvent::drop_off && !st.drop_off_allowed()) {
                continue;
            }

            const auto previous_dt = get_previous_stop_time(stop_event, base_dt, *freq_vj, st, rt_level);

            if (previous_dt == DateTimeUtils::not_valid || previous_dt < bound) {
                continue;
            }
            if (best.second == DateTimeUtils::not_valid || previous_dt > best.second) {
                best = {&st, previous_dt};
            }
        }
        auto date = DateTimeUtils::date(base_dt);
        if (date == 0) {
            return best;
        }
        base_dt = DateTimeUtils::set(date - 1, DateTimeUtils::SECONDS_PER_DAY - 1);
    }

    return best;
}

static std::pair<const type::StopTime*, DateTime> previous_valid_discrete(const StopEvent stop_event,
                                                                          const dataRAPTOR& dataRaptor,
                                                                          const JppIdx jpp_idx,
                                                                          const DateTime dt,
                                                                          const type::RTLevel rt_level,
                                                                          const type::VehicleProperties& vehicle_props,
                                                                          const DateTime bound) {
    auto date = DateTimeUtils::date(dt);
    // Init the range with stop_times before dt
    auto st_range = dataRaptor.next_stop_time_data.stop_time_range_before(jpp_idx, dt, stop_event);

    while (DateTimeUtils::date(bound) <= date) {
        for (const auto* st : st_range) {
            assert(dataRaptor.jp_container.get_jpp(*st) == jpp_idx);
            const uint32_t hour = (stop_event == StopEvent::pick_up) ? st->boarding_time : st->alighting_time;
            const DateTime cur_dt = DateTimeUtils::set(date, DateTimeUtils::hour(hour));
            if (bound > cur_dt) {
                return {nullptr, DateTimeUtils::not_valid};
            }
            if (is_valid(st, date, false, rt_level, vehicle_props)) {
                return {st, cur_dt};
            }
        }
        if (date == 0) {
            return {nullptr, DateTimeUtils::not_valid};
        }
        // If this is the first iteration change the range to all the stop_times
        if (DateTimeUtils::date(dt) == date) {
            st_range = dataRaptor.next_stop_time_data.stop_time_range_backward(jpp_idx, stop_event);
        }
        date--;
    }

    return {nullptr, DateTimeUtils::not_valid};
}

std::pair<const type::StopTime*, DateTime> NextStopTime::earliest_stop_time(
    const StopEvent stop_event,
    const JppIdx jpp_idx,
    const DateTime dt,
    const type::RTLevel rt_level,
    const type::VehicleProperties& vehicle_props,
    const bool check_freq,
    const boost::optional<DateTime>& bound) const {
    // The default bound is the end of next day
    DateTime next_dep_bound = bound ? *bound : DateTimeUtils::set(DateTimeUtils::date(dt) + 2, 0) - 1;
    // Limit the next bound to the end production date
    next_dep_bound = std::min(next_dep_bound, DateTimeUtils::set(data.meta->production_date.length().days(), 0));
    const auto first_discrete_st_pair =
        next_valid_discrete(stop_event, *data.dataRaptor, jpp_idx, dt, rt_level, vehicle_props, next_dep_bound);

    if (check_freq) {
        const auto first_frequency_st_pair =
            next_valid_frequency(stop_event, *data.dataRaptor, jpp_idx, dt, rt_level, vehicle_props, next_dep_bound);

        if (first_frequency_st_pair.second < first_discrete_st_pair.second) {
            return first_frequency_st_pair;
        }
    }

    return first_discrete_st_pair;
}

std::pair<const type::StopTime*, DateTime> NextStopTime::tardiest_stop_time(
    const StopEvent stop_event,
    const JppIdx jpp_idx,
    const DateTime dt,
    const type::RTLevel rt_level,
    const type::VehicleProperties& vehicle_props,
    const bool check_freq,
    const boost::optional<DateTime>& bound) const {
    auto cur_date = DateTimeUtils::date(dt);
    // Default bound is the previous day at midnight
    const DateTime prev_dep_bound = bound ? *bound : (cur_date < 2 ? 0 : DateTimeUtils::set(cur_date - 1, 0));
    const auto first_discrete_st_pair =
        previous_valid_discrete(stop_event, *data.dataRaptor, jpp_idx, dt, rt_level, vehicle_props, prev_dep_bound);

    if (check_freq) {
        const auto first_frequency_st_pair = previous_valid_frequency(stop_event, *data.dataRaptor, jpp_idx, dt,
                                                                      rt_level, vehicle_props, prev_dep_bound);
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
template <typename F>
static void vj_loop(const nt::DiscreteVehicleJourney* /*unused*/, F f) {
    f(0);
}

template <typename F>
static void vj_loop(const nt::FrequencyVehicleJourney* vj, F f) {
    int start_time = vj->start_time;
    int end_time = vj->end_time;
    // start date is relative to the production begin date
    // end_time may be smaller than start_time because of the UTC conversion
    if (vj->start_time > vj->end_time) {
        // In this case, the vj passes midnight
        end_time += static_cast<int>(DateTimeUtils::SECONDS_PER_DAY);
    }
    for (auto freq_shift = start_time; freq_shift <= end_time; freq_shift += vj->headway_secs) {
        f(freq_shift);
    }
}

template <typename VJ_T>
static void fill_cache(const DateTime from,
                       const DateTime to,
                       const type::RTLevel rt_level,
                       const type::AccessibiliteParams& accessibilite_params,
                       const JourneyPattern& jp,
                       const std::vector<const VJ_T*>& vjs,
                       IdxMap<JourneyPatternPoint, std::vector<CachedNextStopTime::DtSt>>& arrival_cache,
                       IdxMap<JourneyPatternPoint, std::vector<CachedNextStopTime::DtSt>>& departure_cache) {
    const auto to_int = static_cast<int>(DateTimeUtils::date(to));
    // In case of Vj that passes midnight, we should compute one day before "from"
    const int from_int = std::max(static_cast<int>(DateTimeUtils::date(from)) - 1, 0);
    for (const auto* vj : vjs) {
        if (!vj->accessible(accessibilite_params.vehicle_properties)) {
            continue;
        }
        // test validity pattern of vj
        const auto* vp = vj->validity_patterns[rt_level];
        for (int day = from_int; day <= to_int; ++day) {
            if (!vp->check(day)) {
                continue;
            }
            const auto shift = navitia::DateTimeUtils::SECONDS_PER_DAY * day;
            RankJourneyPatternPoint i{0};
            for (const auto& st : vj->stop_time_list) {
                auto jpp_idx = jp.get_jpp_idx(i);
                auto loop_impl = [&](long freq_shift) {
                    if (st.drop_off_allowed()) {
                        auto arrival_time = st.alighting_time + shift + freq_shift;
                        if (from <= arrival_time && arrival_time <= to) {
                            arrival_cache[jpp_idx].emplace_back(arrival_time, &st);
                        }
                    }
                    if (st.pick_up_allowed()) {
                        auto departure_time = st.boarding_time + shift + freq_shift;
                        if (departure_time <= to && from <= departure_time) {
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
    }
    if (rt_level != other.rt_level) {
        return rt_level < other.rt_level;
    }
    return accessibilite_params < other.accessibilite_params;
}

CachedNextStopTime CachedNextStopTimeManager::CacheCreator::operator()(const CachedNextStopTimeKey& key) const {
    CachedNextStopTime::vDtStByJpp departure, arrival;
    const auto& jp_container = dataRaptor.jp_container;

    departure.assign(jp_container.get_jpps_values());
    arrival.assign(jp_container.get_jpps_values());
    DateTime dt_from = DateTimeUtils::set(key.from, 0);
    DateTime dt_to = DateTimeUtils::set(key.from + 2, 0);  // cache window is 2-days wide (journeys : 24h max)

    for (const auto& jp : jp_container.get_jps_values()) {
        fill_cache(dt_from, dt_to, key.rt_level, key.accessibilite_params, jp, jp.discrete_vjs, arrival, departure);
        fill_cache(dt_from, dt_to, key.rt_level, key.accessibilite_params, jp, jp.freq_vjs, arrival, departure);
    }
    auto compare = [](const CachedNextStopTime::DtSt& lhs, const CachedNextStopTime::DtSt& rhs) noexcept {
        return lhs.first < rhs.first;
    };
    for (const auto jpp_dtst : arrival) {
        boost::sort(jpp_dtst.second, compare);
    }
    for (const auto jpp_dtst : departure) {
        boost::sort(jpp_dtst.second, compare);
    }
    return {departure, arrival};
}

CachedNextStopTime::DtStFromJpp::DtStFromJpp(const vDtStByJpp& map) {
    size_t s = 0;
    for (const auto elt : map) {
        s += elt.second.size();
    }
    dtsts.reserve(s);
    until.assign(map, 0);
    for (const auto elt : map) {
        boost::push_back(dtsts, elt.second);
        until[elt.first] = dtsts.size();
    }
    dtsts.shrink_to_fit();
}

boost::iterator_range<CachedNextStopTime::vDtSt::const_iterator> CachedNextStopTime::DtStFromJpp::operator[](
    const JppIdx& jpp_idx) const {
    const auto from = jpp_idx.val == 0 ? 0 : until[JppIdx(jpp_idx.val - 1)];
    const auto begin = dtsts.begin();
    return boost::make_iterator_range(begin + from, begin + until[jpp_idx]);
}

std::pair<const type::StopTime*, DateTime> CachedNextStopTime::next_stop_time(const StopEvent stop_event,
                                                                              const JppIdx jpp_idx,
                                                                              const DateTime dt,
                                                                              const bool clockwise,
                                                                              const type::RTLevel,
                                                                              const type::VehicleProperties&,
                                                                              const bool check_freq,
                                                                              const boost::optional<DateTime>&) const {
    const auto& v = (stop_event == StopEvent::pick_up ? departure[jpp_idx] : arrival[jpp_idx]);
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

CachedNextStopTimeManager::~CachedNextStopTimeManager() {
    auto logger = log4cplus::Logger::getInstance("logger");
    LOG4CPLUS_INFO(logger, "Cache miss : " << lru.get_nb_cache_miss() << " / " << lru.get_nb_calls());
}

std::shared_ptr<const CachedNextStopTime> CachedNextStopTimeManager::load(
    const DateTime from,
    const type::RTLevel rt_level,
    const type::AccessibiliteParams& accessibilite_params) {
    CachedNextStopTimeKey key(DateTimeUtils::date(from), rt_level, accessibilite_params);
    return lru(key);
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
                            const DateTime dt,
                            const type::FrequencyVehicleJourney& freq_vj,
                            const type::StopTime& st,
                            const type::RTLevel rt_level) {
    const u_int32_t considered_time = (stop_event == StopEvent::pick_up) ? st.boarding_time : st.alighting_time;
    const u_int32_t lower_bound = DateTimeUtils::hour(freq_vj.start_time + considered_time);
    const u_int32_t upper_bound = DateTimeUtils::hour(freq_vj.end_time + considered_time);

    auto hour = DateTimeUtils::hour(dt);
    auto date = DateTimeUtils::date(dt);

    const bool normal_case = lower_bound <= upper_bound;

    // in the 'normal' case, hour should be in [lower, higher]
    // but in case of midnight overrun, hour should be in [higher, lower]
    if (normal_case) {
        if (hour <= lower_bound) {
            if (freq_vj.is_valid(date, rt_level)) {
                return DateTimeUtils::set(date, lower_bound);
            }
            return DateTimeUtils::inf;
        }
        if (hour > upper_bound) {
            // no solution on the day
            return DateTimeUtils::inf;
        }
    }

    if (normal_case ||  // in classic case, we must be in [start, end]
        (!normal_case && within(hour, {lower_bound, DateTimeUtils::SECONDS_PER_DAY}))) {
        // we need to check if the vj is valid for our day
        if (!freq_vj.is_valid(date, rt_level)) {
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
    // we need to see if the vj was valid the day before
    if (!freq_vj.is_valid(date - 1, rt_level)) {
        // the vj was not valid, next departure is lower
        return DateTimeUtils::set(date, lower_bound);
    }

    double diff = hour - lower_bound + DateTimeUtils::SECONDS_PER_DAY;
    const int32_t x = std::ceil(diff / double(freq_vj.headway_secs));

    return DateTimeUtils::set(date, DateTimeUtils::hour(lower_bound + x * freq_vj.headway_secs));
}

DateTime get_previous_stop_time(const StopEvent stop_event,
                                const DateTime dt,
                                const type::FrequencyVehicleJourney& freq_vj,
                                const type::StopTime& st,
                                const type::RTLevel rt_level) {
    const u_int32_t considered_time = (stop_event == StopEvent::pick_up) ? st.boarding_time : st.alighting_time;
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
            // no solution on the day
            return DateTimeUtils::not_valid;
        }
    }

    if (normal_case ||  // in classic case, we must be in [start, end]
        (!normal_case && within(hour, {lower_bound, DateTimeUtils::SECONDS_PER_DAY}))) {
        // we need to check if the vj is valid for our day
        if (!freq_vj.is_valid(date, rt_level)) {
            return DateTimeUtils::not_valid;
        }

        double diff = hour - lower_bound;
        const uint32_t x = std::floor(diff / double(freq_vj.headway_secs));
        return DateTimeUtils::set(date, lower_bound + x * freq_vj.headway_secs);
    }

    // overnight case and hour > midnight
    if (hour > upper_bound) {
        // case with hour in [upper_bound, lower_boud]
        if (freq_vj.is_valid(date - 1, rt_level)) {
            return DateTimeUtils::set(date, upper_bound);
        }
        return DateTimeUtils::not_valid;
    }
    // we need to see if the vj was valid the day before
    if (!freq_vj.is_valid(date - 1, rt_level)) {
        // the vj was not valid, next departure is lower
        return DateTimeUtils::not_valid;
    }

    const double diff = hour - lower_bound + DateTimeUtils::SECONDS_PER_DAY;
    const int32_t x = std::floor(diff / double(freq_vj.headway_secs));

    return DateTimeUtils::set(date - 1, lower_bound + x * freq_vj.headway_secs);
}

}  // namespace routing
}  // namespace navitia
