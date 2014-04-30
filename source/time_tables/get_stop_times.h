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
#include "routing/routing.h"
#include "type/data.h"


namespace navitia { namespace timetables {
typedef std::pair<DateTime, const type::StopTime*> datetime_stop_time;

/**
 * @brief get_stop_times: Return all departures leaving from the journey_pattern points
 * @param journey_pattern_points: list of the journey_pattern_point we want to start from
 * @param dt: departure date time
 * @param calendar_id: id of the calendar (optional)
 * @param max_dt: maximal Datetime
 * @param nb_departures: max number of departure
 * @param data: data container
 * @param accessibilite_params: accebility criteria to restrict the stop times
 * @return: a list of pair <datetime, departure st.idx>. The list is sorted on the datetimes.
 */
std::vector<datetime_stop_time> get_stop_times(const std::vector<type::idx_t> &journey_pattern_points, const DateTime &dt,
                                   const DateTime &max_dt, const size_t max_departures, const type::Data & data, bool disruption_active,
                                   boost::optional<const std::string> calendar_id = {},
                                   const type::AccessibiliteParams & accessibilite_params = type::AccessibiliteParams());



}}
