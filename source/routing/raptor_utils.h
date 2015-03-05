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

#include "type/datetime.h"
#include "utils/idx_map.h"

namespace navitia {

namespace type {
struct JourneyPatternPoint;
struct JourneyPattern;
struct StopPoint;
struct StopTime;
}

namespace routing {

typedef Idx<type::JourneyPatternPoint> JppIdx;
typedef Idx<type::JourneyPattern> JpIdx;
typedef Idx<type::StopPoint> SpIdx;

struct Labels {
    // initialize the structure according to the number of jpp
    inline void init_inf(const std::vector<type::StopPoint*>& stops) {
        init(stops, DateTimeUtils::inf);
    }
    // initialize the structure according to the number of jpp
    inline void init_min(const std::vector<type::StopPoint*>& stops) {
        init(stops, DateTimeUtils::min);
    }
    // clear the structure according to a given structure. Same as a
    // copy without touching the boarding_jpp fields
    inline void clear(const Labels& clean) {
        dt_pts = clean.dt_pts;
        dt_transfers = clean.dt_transfers;
        boarding_jpp_pts.resize(clean.boarding_jpp_pts);
        boarding_sp_transfers.resize(clean.boarding_sp_transfers);
        used_jpps.resize(clean.used_jpps);
    }
    inline const SpIdx&
    boarding_sp_transfer(SpIdx sp_idx) const {
        return boarding_sp_transfers[sp_idx];
    }
    inline const JppIdx&
    boarding_jpp_pt(SpIdx sp_idx) const {
        return boarding_jpp_pts[sp_idx];
    }
    inline const DateTime& dt_transfer(SpIdx sp_idx) const {
        return dt_transfers[sp_idx];
    }
    inline const DateTime& dt_pt(SpIdx sp_idx) const {
        return dt_pts[sp_idx];
    }

    inline SpIdx&
    mut_boarding_sp_transfer(SpIdx sp_idx) {
        return boarding_sp_transfers[sp_idx];
    }
    inline JppIdx&
    mut_boarding_jpp_pt(SpIdx sp_idx) {
        return boarding_jpp_pts[sp_idx];
    }
    inline DateTime& mut_dt_transfer(SpIdx sp_idx) {
        return dt_transfers[sp_idx];
    }
    inline DateTime& mut_dt_pt(SpIdx sp_idx) {
        return dt_pts[sp_idx];
    }

    inline bool pt_is_initialized(SpIdx sp_idx) const {
        return dt_pt(sp_idx) != DateTimeUtils::inf && dt_pt(sp_idx) != DateTimeUtils::min;
    }
    inline bool transfer_is_initialized(SpIdx sp_idx) const {
        return dt_transfer(sp_idx) != DateTimeUtils::inf && dt_transfer(sp_idx) != DateTimeUtils::min;
    }

    inline JppIdx& mut_used_jpp(SpIdx sp_idx) {
        return used_jpps[sp_idx];
    }
    inline const JppIdx& used_jpp(SpIdx sp_idx) const {
        return used_jpps[sp_idx];
    }
private:
    inline void init(const std::vector<type::StopPoint*>& stops, DateTime val) {
        dt_pts.assign(stops, val);
        dt_transfers.assign(stops, val);
        boarding_jpp_pts.resize(stops);
        boarding_sp_transfers.resize(stops);
        used_jpps.resize(stops);
    }

    // All these vectors are indexed by sp_idx
    //
    // At what time can we reach this label with public transport
    IdxMap<type::StopPoint, DateTime> dt_pts;
    // At what time wan we reach this label with a transfer
    IdxMap<type::StopPoint, DateTime> dt_transfers;
    // jpp used to reach this label with public transport
    IdxMap<type::StopPoint, JppIdx> boarding_jpp_pts;
    // sp used to reach this label with a transfer.
    // Note: the sp is enough here (no need for jpp), it is used only for path reconstruction
    IdxMap<type::StopPoint, SpIdx> boarding_sp_transfers;

    // jpp used to mark the stop point. Used for path reconstruction, only because of the the vj extention.
    // The service extention makes it impossible to find the used jpp withe the boarding jpp
    IdxMap<type::StopPoint, JppIdx> used_jpps;
};

} // namespace routing
} // namespace navitia
