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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "type/datetime.h"
#include "utils/idx_map.h"

#include <boost/container/flat_map.hpp>

namespace navitia {

namespace type {
struct StopPoint;
struct StopTime;
struct Route;
struct Line;
struct VehicleJourney;
struct MetaVehicleJourney;
struct PhysicalMode;
}  // namespace type

namespace routing {

struct JourneyPattern;
struct JourneyPatternPoint;
using JppIdx = Idx<JourneyPatternPoint>;
using JpIdx = Idx<JourneyPattern>;
using SpIdx = Idx<type::StopPoint>;
using RouteIdx = Idx<type::Route>;
using LineIdx = Idx<type::Line>;
using VjIdx = Idx<type::VehicleJourney>;
using MvjIdx = Idx<type::MetaVehicleJourney>;
using PhyModeIdx = Idx<type::PhysicalMode>;

using map_stop_point_duration = boost::container::flat_map<SpIdx, navitia::time_duration>;

inline bool is_dt_initialized(const DateTime dt) {
    return dt != DateTimeUtils::inf && dt != DateTimeUtils::min;
}

struct Labels {
    inline friend void swap(Labels& lhs, Labels& rhs) {
        swap(lhs.dt_pts, rhs.dt_pts);
        swap(lhs.dt_transfers, rhs.dt_transfers);
    }
    // initialize the structure according to the number of jpp
    inline void init_inf(const std::vector<type::StopPoint*>& stops) { init(stops, DateTimeUtils::inf); }
    // initialize the structure according to the number of jpp
    inline void init_min(const std::vector<type::StopPoint*>& stops) { init(stops, DateTimeUtils::min); }
    // clear the structure according to a given structure. Same as a
    // copy without touching the boarding_jpp fields
    inline void clear(const Labels& clean) {
        dt_pts = clean.dt_pts;
        dt_transfers = clean.dt_transfers;

        fallback_duration_pts = clean.fallback_duration_pts;
        fallback_duration_transfers = clean.fallback_duration_transfers;
    }
    inline const DateTime& dt_transfer(SpIdx sp_idx) const { return dt_transfers[sp_idx]; }
    inline const DateTime& dt_pt(SpIdx sp_idx) const { return dt_pts[sp_idx]; }
    inline DateTime& mut_dt_transfer(SpIdx sp_idx) { return dt_transfers[sp_idx]; }
    inline DateTime& mut_dt_pt(SpIdx sp_idx) { return dt_pts[sp_idx]; }

    inline bool pt_is_initialized(SpIdx sp_idx) const { return is_dt_initialized(dt_pt(sp_idx)); }
    inline bool transfer_is_initialized(SpIdx sp_idx) const { return is_dt_initialized(dt_transfer(sp_idx)); }

    inline const DateTime& fallback_duration_pt(SpIdx sp_idx) const { return fallback_duration_pts[sp_idx]; }
    inline const DateTime& fallback_duration_transfer(SpIdx sp_idx) const {
        return fallback_duration_transfers[sp_idx];
    }

    inline DateTime& mut_fallback_duration_pt(SpIdx sp_idx) { return fallback_duration_pts[sp_idx]; }
    inline DateTime& mut_fallback_duration_transfer(SpIdx sp_idx) { return fallback_duration_transfers[sp_idx]; }

    const IdxMap<type::StopPoint, DateTime>& get_dt_pts() { return dt_pts; }

private:
    inline void init(const std::vector<type::StopPoint*>& stops, DateTime val) {
        dt_pts.assign(stops, val);
        dt_transfers.assign(stops, val);

        fallback_duration_pts.assign(stops, DateTimeUtils::not_valid);
        fallback_duration_transfers.assign(stops, DateTimeUtils::not_valid);
    }

    // All these vectors are indexed by sp_idx
    //
    // At what time can we reach this label with public transport
    IdxMap<type::StopPoint, DateTime> dt_pts;
    // At what time wan we reach this label with a transfer
    IdxMap<type::StopPoint, DateTime> dt_transfers;

    // fallback_duration_pts[stop_point] stores the fallback duration at departure of the
    // journey from the starting point which reaches stop point at DateTime dt_pts[stop_point] with public transport
    IdxMap<type::StopPoint, DateTime> fallback_duration_pts;
    // fallback_duration_pts[stop_point] stores the fallback duration at departure of the
    // journey from the starting point which reaches stop point at DateTime transfers_pts[stop_point] with transfers
    IdxMap<type::StopPoint, DateTime> fallback_duration_transfers;
};

}  // namespace routing
}  // namespace navitia
