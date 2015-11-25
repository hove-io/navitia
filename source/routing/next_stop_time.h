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

#pragma once

#include "routing/stop_event.h"
#include "routing/raptor_utils.h"
#include "utils/idx_map.h"
#include "type/rt_level.h"

#include <boost/range/algorithm/lower_bound.hpp>
#include <boost/range/algorithm/upper_bound.hpp>
#include <boost/optional.hpp>

namespace navitia {

namespace type {
struct PT_Data;
class Data;
struct FrequencyVehicleJourney;
typedef std::bitset<8> VehicleProperties;
struct AccessibiliteParams;
}

namespace routing {

struct JourneyPatternContainer;

struct NextStopTimeData {
    typedef boost::iterator_range<std::vector<const type::StopTime*>::const_iterator> StopTimeIter;
    typedef boost::iterator_range<std::vector<const type::StopTime*>::const_reverse_iterator> StopTimeReverseIter;

    void load(const JourneyPatternContainer&);

    // Returns the range of the stop times in increasing time order
    inline StopTimeIter stop_time_range_forward(const JppIdx jpp_idx,
                                                const StopEvent stop_event) const {
        if (stop_event == StopEvent::pick_up) {
            return departure[jpp_idx].stop_time_range();
        } else {
            return arrival[jpp_idx].stop_time_range();
        }
    }
    // Returns the range of the stop times in decreasing time order
    inline StopTimeReverseIter stop_time_range_backward(const JppIdx jpp_idx,
                                                        const StopEvent stop_event) const {
        if (stop_event == StopEvent::pick_up) {
            return departure[jpp_idx].reverse_stop_time_range();
        } else {
            return arrival[jpp_idx].reverse_stop_time_range();
        }
    }
    // Returns the range of the stop times in increasing time order beginning after hour(dt)
    inline StopTimeIter stop_time_range_after(const JppIdx jpp_idx, const DateTime dt,
                                              const StopEvent stop_event) const {
        if (stop_event == StopEvent::pick_up) {
            return departure[jpp_idx].next_stop_time_range(dt);
        } else {
            return arrival[jpp_idx].next_stop_time_range(dt);
        }
    }
    // Returns the range of the stop times in decreasing time order ending before hour(dt)
    inline StopTimeReverseIter stop_time_range_before(const JppIdx jpp_idx, const DateTime dt,
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
    template<typename Getter> struct TimesStopTimes {
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

struct NextStopTime {
    explicit NextStopTime(const type::Data& d): data(d) {}

    // Returns the next stop time at given journey pattern point
    // either a vehicle that leaves or that arrives depending on
    // clockwise The last parametter, check_freq, allow to forget to
    // test frequency vehicle journey, a big improvement in speed if
    // you know your journey pattern don't have frequency vehicle
    // journeys.
    inline std::pair<const type::StopTime*, DateTime>
    next_stop_time(const StopEvent stop_event,
                   const JppIdx jpp_idx,
                   const DateTime dt,
                   const bool clockwise,
                   const type::RTLevel rt_level,
                   const type::VehicleProperties& vehicle_props,
                   const bool check_freq = true,
                   const boost::optional<DateTime>& bound = boost::none) const {
        if (clockwise) {
            return earliest_stop_time(stop_event, jpp_idx, dt, rt_level, vehicle_props,
                                      check_freq, bound ? *bound : DateTimeUtils::inf);
        } else {
            return tardiest_stop_time(stop_event, jpp_idx, dt, rt_level, vehicle_props,
                                      check_freq, bound ? *bound : DateTimeUtils::min);
        }
    }

    /// Next forward stop time for a given datetime (dt).  Look for
    /// the first stop_time leaving after dt on the journey pattern
    /// point.  Return the stop time and the next departure
    /// hour(datetime).
    std::pair<const type::StopTime*, DateTime>
    earliest_stop_time(const StopEvent stop_event,
                       const JppIdx jpp_idx,
                       const DateTime dt,
                       const type::RTLevel rt_level,
                       const type::VehicleProperties& vehicle_props,
                       const bool check_freq = true,
                       const DateTime bound = DateTimeUtils::inf) const;

    /// Next backward stop time for a given datetime (dt).  Look for
    /// the first stop_time arriving before dt on the journey pattern
    /// point.  Return the stop time and the next arrival
    /// hour(datetime).
    std::pair<const type::StopTime*, DateTime>
    tardiest_stop_time(const StopEvent stop_event,
                       const JppIdx jpp_idx,
                       const DateTime dt,
                       const type::RTLevel rt_level,
                       const type::VehicleProperties& vehicle_props,
                       const bool check_freq = true,
                       const DateTime bound = DateTimeUtils::min) const;

private:
    const type::Data& data;
};

struct CachedNextStopTime {
    explicit CachedNextStopTime() {}

    void load(const type::Data& d,
              const DateTime from,
              const DateTime to,
              const type::RTLevel rt_level,
              const type::AccessibiliteParams& accessibilite_params);

    // Returns the next stop time at given journey pattern point
    // either a vehicle that leaves or that arrives depending on
    // clockwise.
    std::pair<const type::StopTime*, DateTime>
    next_stop_time(const StopEvent stop_event,
                   const JppIdx jpp_idx,
                   const DateTime dt,
                   const bool clockwise) const;

private:
    using DtSt = std::pair<DateTime, const type::StopTime*>;
    IdxMap<JourneyPatternPoint, std::vector<DtSt>> departure;
    IdxMap<JourneyPatternPoint, std::vector<DtSt>> arrival;
};

DateTime get_next_stop_time(const StopEvent stop_event,
                            DateTime dt,
                            const type::FrequencyVehicleJourney& freq_vj,
                            const type::StopTime& st,
                            const type::RTLevel rt_level = type::RTLevel::Base);
DateTime get_previous_stop_time(const StopEvent stop_event,
                                DateTime dt,
                                const type::FrequencyVehicleJourney& freq_vj,
                                const type::StopTime& st,
                                const type::RTLevel rt_level = type::RTLevel::Base);


}} // namespace navitia::routing
