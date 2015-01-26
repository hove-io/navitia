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

#include "routing/raptor_utils.h"
#include "idx_map.h"

#include <boost/range/algorithm/lower_bound.hpp>

namespace navitia {

namespace type {
struct PT_Data;
class Data;
struct FrequencyVehicleJourney;
typedef std::bitset<8> VehicleProperties;
}

namespace routing {

struct NextStopTimeData {
    void load(const navitia::type::PT_Data &data);

    // Returns the range of the stop times in increasing departure
    // time order
    boost::iterator_range<std::vector<const type::StopTime*>::const_iterator>
    stop_time_range_forward(const JppIdx jpp_idx) const {
        const auto& sts = forward[jpp_idx].stop_times;
        return boost::make_iterator_range(sts.begin(), sts.end());
    }
    // Returns the range of the stop times in decreasing arrival
    // time order
    boost::iterator_range<std::vector<const type::StopTime*>::const_iterator>
    stop_time_range_backward(const JppIdx jpp_idx) const {
        const auto& sts = backward[jpp_idx].stop_times;
        return boost::make_iterator_range(sts.begin(), sts.end());
    }
    // Returns the range of the stop times in increasing departure
    // time order begining after hour(dt)
    boost::iterator_range<std::vector<const type::StopTime*>::const_iterator>
    stop_time_range_after(const JppIdx jpp_idx, const DateTime dt) const {
        const auto& tst = forward[jpp_idx];
        const auto it = boost::lower_bound(tst.times, DateTimeUtils::hour(dt),
                                           std::less<DateTime>());
        const auto idx = it - tst.times.begin();
        return boost::make_iterator_range(tst.stop_times.begin() + idx, tst.stop_times.end());
    }
    // Returns the range of the stop times in decreasing arrival
    // time order ending before hour(dt)
    boost::iterator_range<std::vector<const type::StopTime*>::const_iterator>
    stop_time_range_before(const JppIdx jpp_idx, const DateTime dt) const {
        const auto& tst = backward[jpp_idx];
        const auto it = boost::lower_bound(tst.times, DateTimeUtils::hour(dt),
                                           std::greater<DateTime>());
        const auto idx = it - tst.times.begin();
        return boost::make_iterator_range(tst.stop_times.begin() + idx, tst.stop_times.end());
    }

private:
    struct TimesStopTimes {
        std::vector<DateTime> times;
        std::vector<const type::StopTime*> stop_times;
    };
    IdxMap<type::JourneyPatternPoint, TimesStopTimes> forward;
    IdxMap<type::JourneyPatternPoint, TimesStopTimes> backward;

    template<typename Cmp>
    static void init(const Cmp& cmp,
                     const type::JourneyPattern* jp,
                     const type::JourneyPatternPoint* jpp,
                     TimesStopTimes& tst);
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
    next_stop_time(const bool clockwise,
                   const JppIdx jpp_idx,
                   const DateTime dt,
                   const type::VehicleProperties& vehicle_properties,
                   const bool disruption_active,
                   const bool reconstructing_path = false,
                   const bool check_freq = true) {
        if (clockwise) {
            return next_forward_stop_time(jpp_idx, dt, vehicle_properties, disruption_active,
                                          reconstructing_path, check_freq);
        } else {
            return next_backward_stop_time(jpp_idx, dt, vehicle_properties, disruption_active,
                                           reconstructing_path, check_freq);
        }
    }

    /// Next forward stop time for a given datetime (dt).  Look for
    /// the first stop_time leaving after dt on the journey pattern
    /// point.  Return the stop time and the next departure
    /// hour(datetime).
    std::pair<const type::StopTime*, DateTime>
    next_forward_stop_time(const JppIdx jpp_idx,
                           const DateTime dt,
                           const type::VehicleProperties& vehicle_properties,
                           const bool disruption_active,
                           const bool reconstructing_path = false,
                           const bool check_freq = true);

    /// Next backward stop time for a given datetime (dt).  Look for
    /// the first stop_time arriving before dt on the journey pattern
    /// point.  Return the stop time and the next arrival
    /// hour(datetime).
    std::pair<const type::StopTime*, DateTime>
    next_backward_stop_time(const JppIdx jpp_idx,
                            const DateTime dt,
                            const type::VehicleProperties& vehicle_properties,
                            const bool disruption_active,
                            const bool reconstructing_path = false,
                            const bool check_freq = true);

private:
    const type::Data& data;
};

DateTime get_next_departure(DateTime dt,
                            const type::FrequencyVehicleJourney& freq_vj,
                            const type::StopTime& st,
                            const bool adapted = false);
DateTime get_previous_arrival(DateTime dt,
                              const type::FrequencyVehicleJourney& freq_vj,
                              const type::StopTime& st,
                              const bool adapted = false);


} // namespace routing
} // namespace navitia
