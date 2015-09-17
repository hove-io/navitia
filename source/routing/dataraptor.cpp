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
    forward_connections.assign(data.stop_points);
    backward_connections.assign(data.stop_points);
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
    jpps_from_sp.assign(data.stop_points);
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

void dataRAPTOR::JppsFromJp::load(const type::PT_Data &data) {
    jpps_from_jp.assign(data.journey_patterns);
    for (const auto* jp: data.journey_patterns) {
        const bool has_freq = !jp->frequency_vehicle_journey_list.empty();
        for (const auto* jpp: jp->journey_pattern_point_list) {
            jpps_from_jp[JpIdx(*jp)].push_back(
                {JppIdx(*jpp), SpIdx(*jpp->stop_point), has_freq});
        }
    }
    for (auto& jpps: jpps_from_jp.values()) { jpps.shrink_to_fit(); }
}


void dataRAPTOR::load(const type::PT_Data &data)
{
    labels_const.init_inf(data.stop_points);
    labels_const_reverse.init_min(data.stop_points);

    connections.load(data);
    jpps_from_sp.load(data);
    jpps_from_jp.load(data);
    next_stop_time_data.load(data);

    jp_validity_patterns.assign(366, boost::dynamic_bitset<>(data.journey_patterns.size()));
    jp_adapted_validity_pattern.assign(366, boost::dynamic_bitset<>(data.journey_patterns.size()));

    for(const type::JourneyPattern* journey_pattern : data.journey_patterns) {
        for(int i=0; i<=365; ++i) {
            journey_pattern->for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                if(vj.theoric_validity_pattern()->check2(i)) {
                    jp_validity_patterns[i].set(journey_pattern->idx);
                    return false;
                }
                return true;
            });
        }

        //A journey pattern is valid is at least one validity pattern of its vj is valid on [day-1;day+1]
        for(int i=0; i<=365; ++i) {
            journey_pattern->for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                if(vj.adapted_validity_pattern()->check2(i)) {
                    jp_adapted_validity_pattern[i].set(journey_pattern->idx);
                    return false;
                }
                return true;
            });
        }
    }
}

}}
