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

#pragma once

#include "type/data.h"
#include "routing/dataraptor.h"


namespace navitia { namespace routing {
/// Earliest stop time for a given datetime (dt)
/// Look for the first stop_time leaving after dt on the journey_pattern for the journey_pattern point order
/// Return the stop time and the next departure datetime
std::pair<const type::StopTime*, uint32_t>
earliest_stop_time(const type::JourneyPatternPoint* jpp,
              const DateTime dt,
              const type::Data &data, bool disruption_active, bool reconstructing_path,
              const type::VehicleProperties & vehicle_properties = type::VehicleProperties());

/// Return a list of all {time in the day, stoptime} leaving the jpp for the given calendar
std::vector<std::pair<uint32_t, const type::StopTime*>>
get_all_stop_times(const type::JourneyPatternPoint* jpp,
                   const std::string calendar_id,
                   const type::VehicleProperties& vehicle_properties = type::VehicleProperties());

/// Look for the last stop_time leaving after dt on the journey_pattern for the journey_pattern point order
/// Return the stop time and the first arrival datetime
std::pair<const type::StopTime*, uint32_t>
tardiest_stop_time(const type::JourneyPatternPoint* jpp,
              const DateTime dt, const type::Data &data, bool disruption_active, bool reconstructing_path,
              const type::VehicleProperties & vehicle_properties = type::VehicleProperties());

/// Returns the next stop time at given journey pattern point
/// either a vehicle that leaves or that arrives depending on clockwise
std::pair<const type::StopTime*, uint32_t>
best_stop_time(const type::JourneyPatternPoint* jpp,
               const DateTime dt,
               const type::VehicleProperties & vehicle_properties,
               const bool clockwise,
               bool disruption_active,
               const type::Data &data,
               bool reconstructing_path = false);

std::pair<const type::StopTime*, uint32_t>
best_stop_time(const JpIdx jp_idx,
               const uint16_t jpp_order,
               const DateTime dt,
               const type::VehicleProperties& vehicle_properties,
               const bool clockwise,
               bool disruption_active,
               const type::Data &data,
               bool reconstructing_path = false,
               bool check_freq = true);


/// For timetables in frequency-model
inline bool within(u_int32_t val, std::pair<u_int32_t, u_int32_t> bound) {
    return val >= bound.first && val <= bound.second;
}

inline bool is_valid(const nt::StopTime* st, uint32_t date, bool is_arrival, bool disruption_active, bool reconstructing_path, const type::VehicleProperties & vehicle_properties) {
    return st->valid_end(reconstructing_path) &&
        st->is_valid_day(date, is_arrival, disruption_active)
        && st->vehicle_journey->accessible(vehicle_properties);
}

DateTime get_next_departure(DateTime dt, const type::FrequencyVehicleJourney& freq_vj, const type::StopTime& st, const bool adapted = false);
DateTime get_previous_arrival(DateTime dt, const type::FrequencyVehicleJourney& freq_vj, const type::StopTime& st, const bool adapted = false);

inline bool bound_predicate_earliest(const uint32_t departure_time, const uint32_t hour) {
    return departure_time < hour;
}
inline bool bound_predicate_tardiest(const uint32_t arrival_time, const uint32_t hour) {
    return arrival_time > hour;
}
}}
