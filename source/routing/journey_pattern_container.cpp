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

#include "journey_pattern_container.h"
#include "type/pt_data.h"

namespace navitia { namespace routing {

namespace nt = navitia::type;

template<> std::vector<const nt::DiscreteVehicleJourney*>&
JourneyPattern::get_vjs<nt::DiscreteVehicleJourney>() {
    return discrete_vjs;
}
template<> std::vector<const nt::FrequencyVehicleJourney*>&
JourneyPattern::get_vjs<nt::FrequencyVehicleJourney>() {
    return freq_vjs;
}

void JourneyPatternContainer::init(const nt::PT_Data& pt_data) {
    map.clear();
    jps.clear();
    jpps.clear();
    jps_from_route.assign(pt_data.routes);
    for (const auto* jp: pt_data.journey_patterns) {
        for (const auto& vj: jp->discrete_vehicle_journey_list) { add_vj(*vj); }
        for (const auto& vj: jp->frequency_vehicle_journey_list) { add_vj(*vj); }
    }
}

JourneyPatternContainer::JpKey JourneyPatternContainer::make_key(const nt::VehicleJourney& vj) {
    JourneyPatternContainer::JpKey key;
    key.route_idx = RouteIdx(*vj.journey_pattern->route);
    key.is_freq = dynamic_cast<const nt::FrequencyVehicleJourney*>(&vj) != nullptr;
    for (const auto& st: vj.stop_time_list) {
        key.jpp_keys.emplace_back(SpIdx(*st.journey_pattern_point->stop_point),
                                  st.local_traffic_zone,
                                  st.properties);
    }
    return key;
}

static bool st_are_equals(const nt::VehicleJourney& vj1, const nt::VehicleJourney& vj2) {
    for (auto it1 = vj1.stop_time_list.begin(), it2 = vj2.stop_time_list.begin();
         it1 !=  vj1.stop_time_list.end() && it2 !=  vj2.stop_time_list.end();
         ++it1, ++it2) {
        if (it1->arrival_time != it2->arrival_time || it1->departure_time != it2->departure_time) {
            return false;
        }
    }
    return true;
}

template<typename VJ> static bool
overtake(const VJ& vj, const std::vector<const VJ*> vjs) {
    if (vj.stop_time_list.empty()) { return false; }
    for (const VJ* cur_vj: vjs) {
        assert(vj.stop_time_list.size() == cur_vj->stop_time_list.size());

        // if the validity patterns do not overlap, it can't overtake
        if ((vj.validity_pattern->days & cur_vj->validity_pattern->days).none() &&
            (vj.adapted_validity_pattern->days & cur_vj->adapted_validity_pattern->days).none()) {
            continue;
        }

        // if the stop times are the same, they don't overtake
        if (st_are_equals(vj, *cur_vj)) { continue; }

        const bool vj_is_first =
            vj.stop_time_list.front().departure_time < cur_vj->stop_time_list.front().departure_time;
        const VJ& vj1 = vj_is_first ? vj : *cur_vj;
        const VJ& vj2 = vj_is_first ? *cur_vj : vj;
        for (auto it1 = vj1.stop_time_list.begin(), it2 = vj2.stop_time_list.begin();
             it1 !=  vj1.stop_time_list.end() && it2 !=  vj2.stop_time_list.end();
             ++it1, ++it2) {
            if (it1->arrival_time >= it2->arrival_time ||
                it1->departure_time >= it2->departure_time) {
                return true;
            }            
        }
    }
    return false;
}

template<typename VJ>
void JourneyPatternContainer::add_vj(const VJ& vj) {
    // Get the existing jps according to the key.
    const auto key = make_key(vj);
    auto& jps = map[key];

    // The vj must not overtake the other vj in its jp, thus searching
    // for one satisfying jp.
    for (auto& jp_idx: jps) {
        auto& jp = get(jp_idx);
        auto& vjs = jp.template get_vjs<VJ>();
        if (! overtake(vj, vjs)) {
            // Great, found a valid jp, inserting and stopping
            vjs.push_back(&vj);
            return;
        }
    }

    // We did not find a jp, creating a new one.
    const auto jp_idx = make_jp(key);
    jps.push_back(jp_idx);
    jps_from_route[key.route_idx].push_back(jp_idx);
    get(jp_idx).template get_vjs<VJ>().push_back(&vj);
}

JpIdxT JourneyPatternContainer::make_jp(const JpKey& key) {
    const auto idx = JpIdxT(jps.size());
    jps.emplace_back();
    auto& jp = jps.back();
    uint32_t order = 0;
    for (const auto& jpp_key: key.jpp_keys) {
        jp.jpps.push_back(make_jpp(jpp_key.sp_idx, order++));
    }
    return idx;
}

JppIdxT JourneyPatternContainer::make_jpp(const SpIdx& sp_idx, uint32_t order) {
    const auto idx = JppIdxT(jpps.size());
    jpps.push_back({sp_idx, order});
    return idx;
}

JourneyPattern& JourneyPatternContainer::get(const JpIdxT& idx) {
    assert(idx.val < jps.size());
    return jps[idx.val];
}


}}// namespace navitia::routing
