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

#include "dataraptor.h"
#include "routing.h"
#include "routing/raptor_utils.h"

#include <boost/range/algorithm_ext.hpp>

namespace navitia { namespace routing {

void dataRAPTOR::Connections::load(const type::PT_Data &data) {
    forward_connections.assign(data.stop_points.size());
    backward_connections.assign(data.stop_points.size());
    for (const auto* conn: data.stop_point_connections) {
        forward_connections[SpIdx(*conn->departure)].push_back(
            {DateTime(conn->duration), SpIdx(*conn->destination)});
        backward_connections[SpIdx(*conn->destination)].push_back(
            {DateTime(conn->duration), SpIdx(*conn->departure)});
    }
    for (auto& conns: forward_connections.values()) { conns.shrink_to_fit(); }
    for (auto& conns: backward_connections.values()) { conns.shrink_to_fit(); }
}

void dataRAPTOR::JppsFromSp::load(const type::PT_Data &data) {
    jpps_from_sp.assign(data.stop_points.size());
    for (const auto* sp: data.stop_points) {
        for (const auto* jpp: sp->journey_pattern_point_list) {
            jpps_from_sp[SpIdx(*sp)].push_back({JppIdx(*jpp), JpIdx(*jpp->journey_pattern), jpp->order});
        }
    }
    for (auto& jpps: jpps_from_sp.values()) { jpps.shrink_to_fit(); }
}

void dataRAPTOR::JppsFromSp::filter_jpps(const boost::dynamic_bitset<>& valid_jpps) {
    for (auto& jpps: jpps_from_sp.values()) {
        boost::remove_erase_if(jpps, [&](const Jpp& jpp) {
            return !valid_jpps[jpp.idx.val];
        });
    }
}

void dataRAPTOR::load(const type::PT_Data &data)
{
    labels_const.init_inf(data.journey_pattern_points.size());
    labels_const_reverse.init_min(data.journey_pattern_points.size());

    connections.load(data);
    jpps_from_sp.load(data);

    arrival_times.clear();
    departure_times.clear();
    st_forward.clear();
    st_backward.clear();
    first_stop_time.assign(data.journey_patterns.size());
    nb_trips.assign(data.journey_patterns.size());

    for(int i=0; i<=365; ++i) {
        jp_validity_patterns.push_back(boost::dynamic_bitset<>(data.journey_patterns.size()));
    }

    for(int i=0; i<=365; ++i) {
        jp_adapted_validity_pattern.push_back(boost::dynamic_bitset<>(data.journey_patterns.size()));
    }

    for(const type::JourneyPattern* journey_pattern : data.journey_patterns) {
        const JpIdx jp_idx = JpIdx(*journey_pattern);
        first_stop_time[jp_idx] = arrival_times.size();
        nb_trips[jp_idx] = journey_pattern->discrete_vehicle_journey_list.size();

        //we group all descrete stop times from all journey_pattern_point
        //the frequency stop times are not considered here, they are search for a different way in best_stop_time
        for(unsigned int i=0; i < journey_pattern->journey_pattern_point_list.size(); ++i) {
            std::vector<const type::StopTime*> vec_st;
            for(const auto& vj : journey_pattern->discrete_vehicle_journey_list) {
                assert(vj->stop_time_list[i].journey_pattern_point == journey_pattern->journey_pattern_point_list[i]);
                vec_st.push_back(&vj->stop_time_list[i]);
            }
            std::sort(vec_st.begin(), vec_st.end(),
                      [&](const type::StopTime* st1, const type::StopTime* st2)->bool{
                        uint32_t time1 = DateTimeUtils::hour(st1->departure_time);
                        uint32_t time2 = DateTimeUtils::hour(st2->departure_time);
                        if(time1 == time2) {
                            const auto& st1_first = st1->vehicle_journey->stop_time_list.front();
                            const auto& st2_first = st2->vehicle_journey->stop_time_list.front();
                            if(st1_first.departure_time == st2_first.departure_time) {
                                return st1_first.vehicle_journey->idx < st2_first.vehicle_journey->idx;
                            }
                            return st1_first.departure_time < st2_first.departure_time;
                        }
                        return time1 < time2;});

            st_forward.insert(st_forward.end(), vec_st.begin(), vec_st.end());

            for(auto st : vec_st) {
                departure_times.push_back(DateTimeUtils::hour(st->departure_time));
            }

            std::sort(vec_st.begin(), vec_st.end(),
                  [&](const type::StopTime* st1, const type::StopTime* st2)->bool{
                      uint32_t time1 = DateTimeUtils::hour(st1->arrival_time);
                      uint32_t time2 = DateTimeUtils::hour(st2->arrival_time);
                      if(time1 == time2) {
                          const auto& st1_first = st1->vehicle_journey->stop_time_list.front();
                          const auto& st2_first = st2->vehicle_journey->stop_time_list.front();
                          if(st1_first.arrival_time == st2_first.arrival_time) {
                              return st1_first.vehicle_journey->idx > st2_first.vehicle_journey->idx;
                          }
                          return st1_first.arrival_time > st2_first.arrival_time;
                      }
                      return time1 > time2;});

            st_backward.insert(st_backward.end(), vec_st.begin(), vec_st.end());
            for(auto st : vec_st) {
                arrival_times.push_back(DateTimeUtils::hour(st->arrival_time));
            }
        }

        // On dit que le journey pattern est valide en date j s'il y a au moins une circulation à j-1/j+1
        for(int i=0; i<=365; ++i) {

            journey_pattern->for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                if(vj.validity_pattern->check2(i)) {
                    jp_validity_patterns[i].set(journey_pattern->idx);
                    return false;
                }
                return true;
            });
        }

        // On dit que le journey pattern est valide en date j s'il y a au moins une circulation à j-1/j+1
        for(int i=0; i<=365; ++i) {
            journey_pattern->for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                if(vj.adapted_validity_pattern->check2(i)) {
                    jp_adapted_validity_pattern[i].set(journey_pattern->idx);
                    return false;
                }
                return true;
            });
        }
    }


}

}}
