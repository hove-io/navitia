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

#pragma once

#include "raptor_utils.h"
#include "utils/rank.h"

#include <boost/optional.hpp>

namespace navitia {
namespace type {

class PT_Data;
struct DiscreteVehicleJourney;
struct FrequencyVehicleJourney;
struct StopTime;

}  // namespace type
}  // namespace navitia

namespace navitia {
namespace routing {

using RankJourneyPatternPoint = Rank<JourneyPatternPoint>;

// Accessor method to retrieve the StopTime of a VJ corresponding to a given JPP rank
const type::StopTime& get_corresponding_stop_time(const type::VehicleJourney& vj, const RankJourneyPatternPoint& order);

// TODO: constructor private with JourneyPatternContainer friend?
struct JourneyPatternPoint {
    JpIdx jp_idx;
    SpIdx sp_idx;
    RankJourneyPatternPoint order;
    bool operator==(const JourneyPatternPoint& other) const { return sp_idx == other.sp_idx && order == other.order; }
};
std::ostream& operator<<(std::ostream&, const JourneyPatternPoint&);

// TODO: constructor private with JourneyPatternContainer friend?
struct JourneyPattern {
    std::vector<JppIdx> jpps;
    std::vector<const type::DiscreteVehicleJourney*> discrete_vjs;
    std::vector<const type::FrequencyVehicleJourney*> freq_vjs;
    RouteIdx route_idx;
    PhyModeIdx phy_mode_idx;

    std::vector<JppIdx>::const_iterator jpps_begin() const { return jpps.begin(); }
    std::vector<JppIdx>::const_iterator jpps_end() const { return jpps.end(); }
    const JppIdx& get_jpp_idx(const RankJourneyPatternPoint& order) const;
    bool operator==(const JourneyPattern& other) const {
        return jpps == other.jpps && discrete_vjs == other.discrete_vjs && freq_vjs == other.freq_vjs
               && route_idx == other.route_idx;
    }
    template <typename VJ>
    std::vector<const VJ*>& get_vjs();
    template <typename F>
    void for_each_vehicle_journey(const F f) const {
        for (const auto& vj : discrete_vjs) {
            if (!f(*vj)) {
                return;
            }
        }
        for (const auto& vj : freq_vjs) {
            if (!f(*vj)) {
                return;
            }
        }
    }
};
std::ostream& operator<<(std::ostream&, const JourneyPattern&);

struct JourneyPatternContainer {
    using Jps = std::vector<JourneyPattern>;
    using Jpps = std::vector<JourneyPatternPoint>;
    using JpIterator = IdxMapIterator<JpIdx, Jps::const_iterator>;
    using JppIterator = IdxMapIterator<JppIdx, Jpps::const_iterator>;
    using JpRange = boost::iterator_range<JpIterator>;
    using JppRange = boost::iterator_range<JppIterator>;

    void load(const navitia::type::PT_Data&);
    size_t nb_jps() const { return jps.size(); }
    size_t nb_jpps() const { return jpps.size(); }
    const JourneyPattern& get(const JpIdx& idx) const {
        assert(idx.val < jps.size());
        return jps[idx.val];
    }
    const JourneyPatternPoint& get(const JppIdx& idx) const {
        assert(idx.val < jpps.size());
        return jpps[idx.val];
    }
    JpRange get_jps() const {
        return boost::make_iterator_range(JpIterator(0, jps.begin()), JpIterator(jps.size(), jps.end()));
    }
    const Jps& get_jps_values() const { return jps; }
    JppRange get_jpps() const {
        return boost::make_iterator_range(JppIterator(0, jpps.begin()), JppIterator(jpps.size(), jpps.end()));
    }
    const Jpps& get_jpps_values() const { return jpps; }
    const IdxMap<type::Route, std::vector<JpIdx>>& get_jps_from_route() const { return jps_from_route; }
    const IdxMap<type::PhysicalMode, std::vector<JpIdx>>& get_jps_from_phy_mode() const { return jps_from_phy_mode; }
    const IdxMap<type::PhysicalMode, std::vector<JppIdx>>& get_jpps_from_phy_mode() const { return jpps_from_phy_mode; }
    const IdxMap<type::VehicleJourney, JpIdx>& get_jp_from_vj() const { return jp_from_vj; }
    const JppIdx& get_jpp(const type::StopTime&) const;
    std::string get_id(const JpIdx&) const;
    std::string get_id(const JppIdx&) const;
    boost::optional<JpIdx> get_jp_from_id(const std::string&) const;
    boost::optional<JppIdx> get_jpp_from_id(const std::string&) const;

private:
    struct JppKey {
        JppKey(const SpIdx& sp, const uint16_t& ltz, const std::bitset<8> p)
            : sp_idx(sp), local_traffic_zone(ltz), properties(p) {}
        bool operator<(const JppKey& other) const {
            if (sp_idx != other.sp_idx) {
                return sp_idx < other.sp_idx;
            }
            if (local_traffic_zone != other.local_traffic_zone) {
                return local_traffic_zone < other.local_traffic_zone;
            }
            return properties.to_ulong() < other.properties.to_ulong();
        }

        // As stop time, but without departure and arrival time.
        SpIdx sp_idx;
        uint16_t local_traffic_zone;
        std::bitset<8> properties;
    };
    struct JpKey {
        // To be in the same jp, the vjs must have the same stop times
        // (without the departure and arrival times)
        std::vector<JppKey> jpp_keys;

        // The route is not needed by raptor, but needed by the other
        // users of get_stop_times, thus we keep it for the moment
        // (and in practice, it should change nothing).
        RouteIdx route_idx;

        // The physical mode is not needed by raptor, but used by
        // forbidden uris.  To remove it, we may precompute a big
        // matrix for next_stop_time.  Also, there is ideas of a meta
        // journey pattern that will not contain the high level
        // information as the route and the physical mode.
        PhyModeIdx phy_mode_idx;

        // We do not want a jp with frequency and discrete vj as it is
        // difficult to check if they overtake.
        bool is_freq = false;

        bool operator<(const JpKey& other) const {
            if (route_idx != other.route_idx) {
                return route_idx < other.route_idx;
            }
            if (phy_mode_idx != other.phy_mode_idx) {
                return phy_mode_idx < other.phy_mode_idx;
            }
            if (is_freq != other.is_freq) {
                return is_freq < other.is_freq;
            }
            return jpp_keys < other.jpp_keys;
        }
    };

    // We have a vector to manage overtaking vjs
    using Map = std::map<JpKey, std::vector<JpIdx>>;

    Map map;
    std::vector<JourneyPattern> jps;
    std::vector<JourneyPatternPoint> jpps;
    IdxMap<type::Route, std::vector<JpIdx>> jps_from_route;
    IdxMap<type::VehicleJourney, JpIdx> jp_from_vj;
    IdxMap<type::PhysicalMode, std::vector<JpIdx>> jps_from_phy_mode;
    IdxMap<type::PhysicalMode, std::vector<JppIdx>> jpps_from_phy_mode;

    template <typename VJ>
    void add_vj(const VJ&);
    template <typename VJ>
    static JpKey make_key(const VJ&);
    JpIdx make_jp(const JpKey&);
    JppIdx make_jpp(const JpIdx&, const SpIdx&, const RankJourneyPatternPoint& order);
    JourneyPattern& get_mut(const JpIdx&);
};

}  // namespace routing
}  // namespace navitia
