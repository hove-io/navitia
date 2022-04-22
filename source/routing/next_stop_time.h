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
#include "routing/raptor_utils.h"
#include "utils/idx_map.h"
#include "utils/lru.h"
#include "type/rt_level.h"
#include "type/type_interfaces.h"
#include "type/connection.h"
#include "type/stop_point.h"
#include "type/accessibility_params.h"

#include <boost/range/algorithm/lower_bound.hpp>
#include <boost/range/algorithm/upper_bound.hpp>
#include <boost/optional.hpp>
#include <boost/dynamic_bitset.hpp>

namespace navitia {

namespace type {
class PT_Data;
class Data;
struct FrequencyVehicleJourney;
typedef std::bitset<8> VehicleProperties;
struct AccessibiliteParams;
}  // namespace type

namespace routing {

struct JourneyPatternContainer;
struct dataRAPTOR;

struct NextStopTimeInterface {
    virtual std::pair<const type::StopTime*, DateTime> next_stop_time(
        const StopEvent stop_event,
        const JppIdx jpp_idx,
        const DateTime dt,
        const bool clockwise,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props,
        const bool check_freq = true,
        const boost::optional<DateTime>& bound = boost::none) const = 0;
    virtual ~NextStopTimeInterface() = default;
};

struct NextStopTimeData {
    using StopTimeIter = boost::iterator_range<std::vector<const type::StopTime*>::const_iterator>;
    using StopTimeReverseIter = boost::iterator_range<std::vector<const type::StopTime*>::const_reverse_iterator>;

    void load(const JourneyPatternContainer&);

    // Returns the range of the stop times in increasing time order
    inline StopTimeIter stop_time_range_forward(const JppIdx jpp_idx, const StopEvent stop_event) const {
        if (stop_event == StopEvent::pick_up) {
            return departure[jpp_idx].stop_time_range();
        } else {
            return arrival[jpp_idx].stop_time_range();
        }
    }
    // Returns the range of the stop times in decreasing time order
    inline StopTimeReverseIter stop_time_range_backward(const JppIdx jpp_idx, const StopEvent stop_event) const {
        if (stop_event == StopEvent::pick_up) {
            return departure[jpp_idx].reverse_stop_time_range();
        } else {
            return arrival[jpp_idx].reverse_stop_time_range();
        }
    }
    // Returns the range of the stop times in increasing time order beginning after hour(dt)
    inline StopTimeIter stop_time_range_after(const JppIdx jpp_idx,
                                              const DateTime dt,
                                              const StopEvent stop_event) const {
        if (stop_event == StopEvent::pick_up) {
            return departure[jpp_idx].next_stop_time_range(dt);
        } else {
            return arrival[jpp_idx].next_stop_time_range(dt);
        }
    }
    // Returns the range of the stop times in decreasing time order ending before hour(dt)
    inline StopTimeReverseIter stop_time_range_before(const JppIdx jpp_idx,
                                                      const DateTime dt,
                                                      const StopEvent stop_event) const {
        if (stop_event == StopEvent::pick_up) {
            return departure[jpp_idx].prev_stop_time_range(dt);
        } else {
            return arrival[jpp_idx].prev_stop_time_range(dt);
        }
    }

private:
    struct Departure {
        DateTime get_time(const type::StopTime& st) const;
        bool is_valid(const type::StopTime& st) const;
    };
    struct Arrival {
        DateTime get_time(const type::StopTime& st) const;
        bool is_valid(const type::StopTime& st) const;
    };
    // This structure allow to iterate on stop times in the interesting order
    template <typename Getter>
    struct TimesStopTimes {
        // times is sorted according to cmp
        // for all i, cmp.get_time(stop_times[i]) == times[i]
        std::vector<DateTime> times;
        std::vector<const type::StopTime*> stop_times;
        Getter getter;

        // Returns the range of stop times
        inline StopTimeIter stop_time_range() const {
            return boost::make_iterator_range(stop_times.begin(), stop_times.end());
        }
        // Returns the reverse range of stop times
        inline StopTimeReverseIter reverse_stop_time_range() const {
            return boost::make_iterator_range(stop_times.rbegin(), stop_times.rend());
        }
        // Returns the range of stop times next to hour(dt)
        inline StopTimeIter next_stop_time_range(const DateTime dt) const {
            const auto it = boost::lower_bound(times, DateTimeUtils::hour(dt));
            const auto idx = it - times.begin();
            return boost::make_iterator_range(stop_times.begin() + idx, stop_times.end());
        }
        // Returns the range of stop times previous to hour(dt)
        inline StopTimeReverseIter prev_stop_time_range(const DateTime dt) const {
            const auto it = boost::upper_bound(times, DateTimeUtils::hour(dt));
            const auto idx = it - times.begin();
            return boost::make_iterator_range(stop_times.rend() - idx, stop_times.rend());
        }
        void init(const JourneyPattern& jp, const JourneyPatternPoint& jpp);
    };
    IdxMap<JourneyPatternPoint, TimesStopTimes<Departure>> departure;
    IdxMap<JourneyPatternPoint, TimesStopTimes<Arrival>> arrival;
};

struct NextStopTime : public NextStopTimeInterface {
    explicit NextStopTime(const type::Data& d) : data(d) {}
    ~NextStopTime() override = default;
    // Returns the next stop time at given journey pattern point
    // either a vehicle that leaves or that arrives depending on
    // clockwise The last parametter, check_freq, allow to forget to
    // test frequency vehicle journey, a big improvement in speed if
    // you know your journey pattern don't have frequency vehicle
    // journeys.
    std::pair<const type::StopTime*, DateTime> next_stop_time(
        const StopEvent stop_event,
        const JppIdx jpp_idx,
        const DateTime dt,
        const bool clockwise,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props,
        const bool check_freq = true,
        const boost::optional<DateTime>& bound = boost::none) const override {
        if (clockwise) {
            return earliest_stop_time(stop_event, jpp_idx, dt, rt_level, vehicle_props, check_freq, bound);
        } else {
            return tardiest_stop_time(stop_event, jpp_idx, dt, rt_level, vehicle_props, check_freq, bound);
        }
    }

    /// Next forward stop time for a given datetime (dt).  Look for
    /// the first stop_time leaving after dt on the journey pattern
    /// point.  Return the stop time and the next departure
    /// hour(datetime).
    std::pair<const type::StopTime*, DateTime> earliest_stop_time(
        const StopEvent stop_event,
        const JppIdx jpp_idx,
        const DateTime dt,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props,
        const bool check_freq = true,
        const boost::optional<DateTime>& bound = boost::none) const;

    /// Next backward stop time for a given datetime (dt).  Look for
    /// the first stop_time arriving before dt on the journey pattern
    /// point.  Return the stop time and the next arrival
    /// hour(datetime).
    std::pair<const type::StopTime*, DateTime> tardiest_stop_time(
        const StopEvent stop_event,
        const JppIdx jpp_idx,
        const DateTime dt,
        const type::RTLevel rt_level,
        const type::VehicleProperties& vehicle_props,
        const bool check_freq = true,
        const boost::optional<DateTime>& bound = boost::none) const;

private:
    const type::Data& data;
};

struct CachedNextStopTimeKey {
    using Day = size_t;

    Day from;                                        // first day concerned by the cache
    type::RTLevel rt_level;                          // RT-level of the cache
    type::AccessibiliteParams accessibilite_params;  // accessibility of the cache
    CachedNextStopTimeKey(Day from, type::RTLevel rt_level, const type::AccessibiliteParams& accessibilite_params)
        : from(from), rt_level(rt_level), accessibilite_params(accessibilite_params) {}

    bool operator<(const CachedNextStopTimeKey& other) const;
};

struct CachedNextStopTime : public NextStopTimeInterface {
    using DtSt = std::pair<DateTime, const type::StopTime*>;
    using vDtSt = std::vector<DtSt>;
    using vDtStByJpp = IdxMap<JourneyPatternPoint, vDtSt>;

    CachedNextStopTime(const vDtStByJpp& d, const vDtStByJpp& a) : departure(d), arrival(a) {}
    // Returns the next stop time at given journey pattern point
    // either a vehicle that leaves or that arrives depending on
    // clockwise.
    std::pair<const type::StopTime*, DateTime> next_stop_time(const StopEvent stop_event,
                                                              const JppIdx jpp_idx,
                                                              const DateTime dt,
                                                              const bool clockwise,
                                                              const type::RTLevel,
                                                              const type::VehicleProperties&,
                                                              const bool check_freq,
                                                              const boost::optional<DateTime>&) const override;

private:
    // This structure provide the same interface as a vDtStByJpp, but
    // in a condensed and read only view.
    struct DtStFromJpp {
        DtStFromJpp(const vDtStByJpp& map);

        // Returns the range corresponding to map[jpp_idx], i.e. from
        // dtsts[until[prev(jpp_idx)]] to dtsts[until[jpp_idx]]
        // (excluded).
        boost::iterator_range<vDtSt::const_iterator> operator[](const JppIdx& jpp_idx) const;

    private:
        // let map[JppIdx(40)] == []
        //     map[JppIdx(41)] == [a, l]
        //     map[JppIdx(42)] == [x, y, z]
        //
        // until: [..., JppIdx(39), JppIdx(40), JppIdx(41), JppIdx(42), ...]
        //                  |           |            |        |
        //                  ------------+------,     |        |
        //                                     V     V        V
        // dtsts: [...................... , o, a, l, x, y, z, p, q, ...]
        //                                           ^^^^^^^
        //                                      range of values
        //                                      corresponding to
        //                                      map[JppIdx(42)]
        //
        // Every vectors of map concatenated in order
        // (flatten(map.values())).
        vDtSt dtsts;

        // dtsts[until[jpp_idx]] correspond to the end of
        // map[jpp_idx], and to the begin of map[next(jpp_idx)]
        IdxMap<JourneyPatternPoint, uint32_t> until;
    };
    DtStFromJpp departure;
    DtStFromJpp arrival;
};

struct CachedNextStopTimeManager {
    explicit CachedNextStopTimeManager(const dataRAPTOR& dataRaptor, size_t max_cache) : lru({dataRaptor}, max_cache) {}
    CachedNextStopTimeManager& operator=(CachedNextStopTimeManager&&) = default;
    ~CachedNextStopTimeManager();

    std::shared_ptr<const CachedNextStopTime> load(const DateTime from,
                                                   const type::RTLevel rt_level,
                                                   const type::AccessibiliteParams& accessibilite_params);

    size_t get_nb_cache_miss() const { return lru.get_nb_cache_miss(); }
    void warmup(const CachedNextStopTimeManager& other) { this->lru.warmup(other.lru); }
    size_t get_max_size() const { return lru.get_max_size(); }

private:
    struct CacheCreator {
        using argument_type = const CachedNextStopTimeKey&;
        using result_type = CachedNextStopTime;
        const dataRAPTOR& dataRaptor;
        CacheCreator(const dataRAPTOR& d) : dataRaptor(d) {}
        CachedNextStopTime operator()(const CachedNextStopTimeKey& key) const;
    };

    ConcurrentLru<CacheCreator> lru;
};

DateTime get_next_stop_time(const StopEvent stop_event,
                            const DateTime dt,
                            const type::FrequencyVehicleJourney& freq_vj,
                            const type::StopTime& st,
                            const type::RTLevel rt_level = type::RTLevel::Base);
DateTime get_previous_stop_time(const StopEvent stop_event,
                                const DateTime dt,
                                const type::FrequencyVehicleJourney& freq_vj,
                                const type::StopTime& st,
                                const type::RTLevel rt_level = type::RTLevel::Base);

}  // namespace routing
}  // namespace navitia
