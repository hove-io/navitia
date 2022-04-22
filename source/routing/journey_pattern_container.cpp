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

#include "journey_pattern_container.h"

#include "tests/utils_test.h"
#include "type/pt_data.h"

#include <type_traits>

namespace navitia {
namespace routing {

namespace nt = navitia::type;

const type::StopTime& get_corresponding_stop_time(const type::VehicleJourney& vj,
                                                  const RankJourneyPatternPoint& order) {
    return vj.stop_time_list.at(order.val);
}

template <>
std::vector<const nt::DiscreteVehicleJourney*>& JourneyPattern::get_vjs<nt::DiscreteVehicleJourney>() {
    return discrete_vjs;
}
template <>
std::vector<const nt::FrequencyVehicleJourney*>& JourneyPattern::get_vjs<nt::FrequencyVehicleJourney>() {
    return freq_vjs;
}
std::ostream& operator<<(std::ostream& os, const JourneyPatternPoint& jpp) {
    return os << "Jpp(" << jpp.sp_idx << ", " << jpp.order << ")";
}
std::ostream& operator<<(std::ostream& os, const JourneyPattern& jp) {
    return os << "Jp(" << jp.jpps << ", " << jp.discrete_vjs << ", " << jp.freq_vjs << ")";
}

const JppIdx& JourneyPattern::get_jpp_idx(const RankJourneyPatternPoint& order) const {
    return jpps.at(order.val);
}

void JourneyPatternContainer::load(const nt::PT_Data& pt_data) {
    map.clear();
    jps.clear();
    jpps.clear();
    jps_from_route.assign(pt_data.routes);
    jp_from_vj.assign(pt_data.vehicle_journeys);
    jps_from_phy_mode.assign(pt_data.physical_modes);
    jpps_from_phy_mode.assign(pt_data.physical_modes);
    for (const auto* route : pt_data.routes) {
        for (const auto& vj : route->discrete_vehicle_journey_list) {
            add_vj(*vj);
        }
        for (const auto& vj : route->frequency_vehicle_journey_list) {
            add_vj(*vj);
        }
    }
}

const JppIdx& JourneyPatternContainer::get_jpp(const type::StopTime& st) const {
    const auto& jp = get(jp_from_vj[VjIdx(*st.vehicle_journey)]);
    return jp.jpps.at(st.order().val);
}

std::string JourneyPatternContainer::get_id(const JpIdx& jp_idx) const {
    std::stringstream ss;
    ss << "journey_pattern:" << jp_idx.val;
    return ss.str();
}
std::string JourneyPatternContainer::get_id(const JppIdx& jpp_idx) const {
    std::stringstream ss;
    ss << "journey_pattern_point:" << jpp_idx.val;
    return ss.str();
}
boost::optional<JpIdx> JourneyPatternContainer::get_jp_from_id(const std::string& id) const {
    static const std::string prefix = "journey_pattern:";
    if (id.size() > prefix.size() && id.substr(0, prefix.size()) == prefix) {
        try {
            const auto idx = boost::lexical_cast<idx_t>(id.substr(prefix.size()));
            if (idx < nb_jps()) {
                return JpIdx(idx);
            }
        } catch (boost::bad_lexical_cast&) {
        }
    }
    return boost::none;
}
boost::optional<JppIdx> JourneyPatternContainer::get_jpp_from_id(const std::string& id) const {
    static const std::string prefix = "journey_pattern_point:";
    if (id.size() > prefix.size() && id.substr(0, prefix.size()) == prefix) {
        try {
            const auto idx = boost::lexical_cast<idx_t>(id.substr(prefix.size()));
            if (idx < nb_jpps()) {
                return JppIdx(idx);
            }
        } catch (boost::bad_lexical_cast&) {
        }
    }
    return boost::none;
}

template <typename VJ>
JourneyPatternContainer::JpKey JourneyPatternContainer::make_key(const VJ& vj) {
    JourneyPatternContainer::JpKey key;
    key.route_idx = RouteIdx(*vj.route);
    key.phy_mode_idx = PhyModeIdx(*vj.physical_mode);
    key.is_freq = std::is_same<VJ, nt::FrequencyVehicleJourney>::value;
    for (const auto& st : vj.stop_time_list) {
        key.jpp_keys.emplace_back(SpIdx(*st.stop_point), st.local_traffic_zone, st.properties);
    }
    return key;
}

static bool st_are_equals(const nt::VehicleJourney& vj1, const nt::VehicleJourney& vj2) {
    if (vj1.stop_time_list.size() != vj2.stop_time_list.size()) {
        return false;
    }
    for (auto it1 = vj1.stop_time_list.begin(), it2 = vj2.stop_time_list.begin();
         it1 != vj1.stop_time_list.end() && it2 != vj2.stop_time_list.end(); ++it1, ++it2) {
        if (it1->arrival_time != it2->arrival_time || it1->departure_time != it2->departure_time
            || it1->alighting_time != it2->alighting_time || it1->boarding_time != it2->boarding_time) {
            return false;
        }
    }
    return true;
}

template <typename VJ>
static bool overtake(const VJ& vj, const std::vector<const VJ*>& vjs) {
    if (vj.stop_time_list.empty()) {
        return false;
    }
    for (const VJ* cur_vj : vjs) {
        assert(vj.stop_time_list.size() == cur_vj->stop_time_list.size());

        // if the validity patterns do not overlap, it can't overtake
        static const auto levels = {nt::RTLevel::Base, nt::RTLevel::Adapted, nt::RTLevel::RealTime};
        const bool dont_overlap = std::all_of(std::begin(levels), std::end(levels), [&](nt::RTLevel lvl) {
            if (vj.validity_patterns[lvl] == nullptr) {
                return true;
            }
            if (cur_vj->validity_patterns[lvl] == nullptr) {
                return true;
            }
            return (vj.validity_patterns[lvl]->days & cur_vj->validity_patterns[lvl]->days).none();
        });
        if (dont_overlap) {
            continue;
        }

        // if the stop times are the same, they don't overtake
        if (st_are_equals(vj, *cur_vj)) {
            continue;
        }

        const bool vj_is_first = vj.stop_time_list.front().boarding_time < cur_vj->stop_time_list.front().boarding_time;
        const VJ& vj1 = vj_is_first ? vj : *cur_vj;
        const VJ& vj2 = vj_is_first ? *cur_vj : vj;
        for (auto it1 = vj1.stop_time_list.begin(), it2 = vj2.stop_time_list.begin();
             it1 != vj1.stop_time_list.end() && it2 != vj2.stop_time_list.end(); ++it1, ++it2) {
            if (it1->alighting_time >= it2->alighting_time || it1->boarding_time >= it2->boarding_time) {
                return true;
            }
        }
    }
    return false;
}

template <typename VJ>
void JourneyPatternContainer::add_vj(const VJ& vj) {
    // Get the existing jps according to the key.
    const auto key = make_key(vj);
    auto& jps = map[key];

    // The vj must not overtake the other vj in its jp, thus searching
    // for one satisfying jp.
    for (auto& jp_idx : jps) {
        auto& jp = get_mut(jp_idx);
        auto& vjs = jp.template get_vjs<VJ>();
        if (!overtake(vj, vjs)) {
            // Great, found a valid jp, inserting and stopping
            vjs.push_back(&vj);
            jp_from_vj[VjIdx(vj)] = jp_idx;
            return;
        }
    }

    // We did not find a jp, creating a new one.
    const auto jp_idx = make_jp(key);
    jps.push_back(jp_idx);
    jps_from_route[key.route_idx].push_back(jp_idx);
    jps_from_phy_mode[key.phy_mode_idx].push_back(jp_idx);
    get_mut(jp_idx).template get_vjs<VJ>().push_back(&vj);
    jp_from_vj[VjIdx(vj)] = jp_idx;
}

JpIdx JourneyPatternContainer::make_jp(const JpKey& key) {
    const auto jp_idx = JpIdx(jps.size());
    JourneyPattern jp;
    jp.route_idx = key.route_idx;
    jp.phy_mode_idx = key.phy_mode_idx;
    RankJourneyPatternPoint order(0);
    for (const auto& jpp_key : key.jpp_keys) {
        jp.jpps.push_back(make_jpp(jp_idx, jpp_key.sp_idx, order++));
    }
    jpps_from_phy_mode[jp.phy_mode_idx].insert(jpps_from_phy_mode[jp.phy_mode_idx].end(), jp.jpps.begin(),
                                               jp.jpps.end());
    jps.push_back(std::move(jp));
    return jp_idx;
}

JppIdx JourneyPatternContainer::make_jpp(const JpIdx& jp_idx,
                                         const SpIdx& sp_idx,
                                         const RankJourneyPatternPoint& order) {
    const auto idx = JppIdx(jpps.size());
    jpps.push_back({jp_idx, sp_idx, order});
    return idx;
}

JourneyPattern& JourneyPatternContainer::get_mut(const JpIdx& idx) {
    assert(idx.val < jps.size());
    return jps[idx.val];
}

}  // namespace routing
}  // namespace navitia
