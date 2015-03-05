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
#include "type/type.h"
#include "type/data.h"
#include "routing.h"
#include "raptor_utils.h"

namespace navitia { namespace routing {

//Forward declare

struct RAPTOR;

// Does the current date improves compared to best_so_far – we must not forget to take the walking duration
static bool improves(const DateTime a,
                     const DateTime b,
                     bool clockwise) {
    return clockwise ? a > b : a < b;
}

struct Solution {
    SpIdx sp_idx;  //arrival stop point of the solution
    uint32_t count;
    DateTime arrival, upper_bound, total_arrival;
    float ratio;
    navitia::time_duration walking_time = {};
    bool clockwise = true;

    Solution():
        sp_idx(),
        count(0),
        arrival(DateTimeUtils::inf),
        upper_bound(DateTimeUtils::inf),
        total_arrival(DateTimeUtils::inf),
        ratio(std::numeric_limits<float>::min())
    {}

    bool operator<(const Solution& s) const {
        if (this->total_arrival != s.total_arrival) {
            return improves(total_arrival, s.total_arrival, clockwise);
        }
        if (this->arrival != s.arrival) {
            return improves(arrival, s.arrival, clockwise);
        }
        if(this->count != s.count) {
            return this->count < s.count;
        }
        if (this->sp_idx != s.sp_idx) {
            return this->sp_idx < s.sp_idx;
        }
        if(this->upper_bound != s.upper_bound) {
            return this->upper_bound < s.upper_bound;
        }
        if(this->walking_time != s.walking_time) {
            return this->walking_time < s.walking_time;
        }
        return this->ratio < s.ratio;
    }

};
typedef std::set<Solution> Solutions;

Solutions
get_solutions(const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
              const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations, bool clockwise,
              const type::AccessibiliteParams & accessibilite_params,
              bool disruption_active, const RAPTOR& raptor);

//This one is hacky, it's used to retrieve the departures.
Solutions
init_departures(const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
             const DateTime &dep, bool clockwise, bool disruption_active);

Solutions
get_walking_solutions(bool clockwise, const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
                      const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations, const Solution &best,
                      const bool disruption_active, const type::AccessibiliteParams &accessibilite_params,
                      const navitia::routing::RAPTOR &raptor);

Solutions
get_pareto_front(bool clockwise, const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
               const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations,
               const type::AccessibiliteParams & accessibilite_params, bool disruption_active, const RAPTOR& raptor);

std::pair<SpIdx, DateTime>
get_final_spidx_and_date(int count, SpIdx sp_idx, bool clockwise, bool disruption_active, const type::AccessibiliteParams &accessibilite_params, const navitia::routing::RAPTOR &raptor);

navitia::time_duration
getWalkingTime(int count, SpIdx sp_idx, const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
               const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations,
               bool clockwise, bool disruption_active, const type::AccessibiliteParams &accessibilite_params,
               const navitia::routing::RAPTOR &raptor);

}}
