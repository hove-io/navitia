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

#include "dataraptor.h"

#include "routing.h"
#include "routing/raptor_utils.h"

#include <boost/range/algorithm_ext.hpp>

namespace navitia {
namespace routing {

void dataRAPTOR::Connections::load(const type::PT_Data& data) {
    forward_connections.assign(data.stop_points);
    backward_connections.assign(data.stop_points);
    for (const auto* conn : data.stop_point_connections) {
        const SpIdx departure_stop_point = SpIdx(*conn->departure);
        const SpIdx arrival_stop_point = SpIdx(*conn->destination);
        const auto walking_duration = DateTime(conn->display_duration);
        const auto total_duration = DateTime(conn->duration);
        forward_connections[departure_stop_point].push_back({total_duration, walking_duration, arrival_stop_point});
        backward_connections[arrival_stop_point].push_back({total_duration, walking_duration, departure_stop_point});
    }
    for (auto& conns : forward_connections.values()) {
        conns.shrink_to_fit();
    }
    for (auto& conns : backward_connections.values()) {
        conns.shrink_to_fit();
    }
}

void dataRAPTOR::JppsFromSp::load(const type::PT_Data& data, const JourneyPatternContainer& jp_container) {
    jpps_from_sp.assign(data.stop_points);
    for (const auto jp : jp_container.get_jps()) {
        for (const auto& jpp_idx : jp.second.jpps) {
            const auto& jpp = jp_container.get(jpp_idx);
            jpps_from_sp[jpp.sp_idx].push_back({jpp_idx, jp.first, jpp.order.val});
        }
    }
    for (auto& jpps : jpps_from_sp.values()) {
        jpps.shrink_to_fit();
    }
}

void dataRAPTOR::JppsFromSp::filter_jpps(const boost::dynamic_bitset<>& valid_jpps) {
    for (auto& jpps : jpps_from_sp.values()) {
        boost::remove_erase_if(jpps, [&](const Jpp& jpp) { return !valid_jpps[jpp.idx.val]; });
    }
}

void dataRAPTOR::JppsFromJp::load(const JourneyPatternContainer& jp_container) {
    jpps_from_jp.assign(jp_container.get_jps_values());
    for (const auto jp : jp_container.get_jps()) {
        const bool has_freq = !jp.second.freq_vjs.empty();
        for (const auto& jpp_idx : jp.second.jpps) {
            const auto& jpp = jp_container.get(jpp_idx);
            jpps_from_jp[jp.first].push_back({jpp_idx, jpp.sp_idx, has_freq});
        }
    }
    for (auto& jpps : jpps_from_jp.values()) {
        jpps.shrink_to_fit();
    }
}

void dataRAPTOR::load(const type::PT_Data& pt_data, size_t cache_size) {
    jp_container.load(pt_data);
    labels_const.init_inf(pt_data.stop_points);
    labels_const_reverse.init_min(pt_data.stop_points);

    connections.load(pt_data);
    jpps_from_sp.load(pt_data, jp_container);
    jpps_from_jp.load(jp_container);
    next_stop_time_data.load(jp_container);

    for (auto level_cont : jp_validity_patterns) {
        const auto rt_level = level_cont.first;
        auto& jp_vp = level_cont.second;
        jp_vp.assign(366, boost::dynamic_bitset<>(jp_container.nb_jps()));
        for (const auto jp : jp_container.get_jps()) {
            for (int i = 0; i <= 365; ++i) {
                jp.second.for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                    if (vj.validity_patterns[rt_level]->check2(i)) {
                        jp_vp[i].set(jp.first.val);
                        return false;
                    }
                    return true;
                });
            }
        }
    }

    min_connection_time = std::numeric_limits<uint32_t>::max();
    for (const auto conns : connections.forward_connections) {
        for (const auto& conn : conns.second) {
            min_connection_time = std::min(min_connection_time, conn.duration);
        }
    }

    cached_next_st_manager = std::make_unique<CachedNextStopTimeManager>(*this, cache_size);
}

void dataRAPTOR::warmup(const dataRAPTOR& other) {
    this->cached_next_st_manager->warmup(*other.cached_next_st_manager);
}

}  // namespace routing
}  // namespace navitia
