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
#include "best_stoptime.h"

namespace navitia { namespace routing {

//Forward declare

struct RAPTOR;

struct Solution {
    type::idx_t jpp_idx;
    uint32_t count;
    DateTime arrival, upper_bound, total_arrival;
    float ratio;
    navitia::time_duration walking_time = {};

    Solution() : jpp_idx(type::invalid_idx), count(0),
                       arrival(DateTimeUtils::inf), upper_bound(DateTimeUtils::inf),
                       ratio(std::numeric_limits<float>::min()) {}

    bool operator<(const Solution& s) const {
        if (this->arrival != s.arrival) {
            return this->arrival < s.arrival;
        }
        if(this->count != s.count) {
            return this->count < s.count;
        }
        if (this->jpp_idx != s.jpp_idx) {
            return this->jpp_idx < s.jpp_idx;
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
get_solutions(const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
              const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations, bool clockwise,
              const type::AccessibiliteParams & accessibilite_params,
              bool disruption_active, const RAPTOR& raptor);

//This one is hacky, it's used to retrieve the departures.
Solutions
get_solutions(const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
             const DateTime &dep, bool clockwise, const type::Data & data, bool disruption_active);

Solutions
get_walking_solutions(bool clockwise, const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
                      const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations, const Solution &best,
                      const bool disruption_active, const type::AccessibiliteParams &accessibilite_params,
                      const navitia::routing::RAPTOR &raptor);

Solutions
get_pareto_front(bool clockwise, const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
               const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations,
               const type::AccessibiliteParams & accessibilite_params, bool disruption_active, const RAPTOR& raptor);

std::pair<type::idx_t, DateTime>
get_final_jppidx_and_date(int count, type::idx_t jpp_idx, bool clockwise, bool disruption_active, const type::AccessibiliteParams &accessibilite_params, const navitia::routing::RAPTOR &raptor);

navitia::time_duration
getWalkingTime(int count, type::idx_t jpp_idx, const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
               const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations,
               bool clockwise, bool disruption_active, const type::AccessibiliteParams &accessibilite_params,
               const navitia::routing::RAPTOR &raptor);

}}
