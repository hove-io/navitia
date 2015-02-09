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
    inline void init_inf(size_t nb_sp) {
        init(nb_sp, DateTimeUtils::inf);
    }
    // initialize the structure according to the number of jpp
    inline void init_min(size_t nb_sp) {
        init(nb_sp, DateTimeUtils::min);
    }
    // clear the structure according to a given structure. Same as a
    // copy without touching the boarding_jpp fields
    inline void clear(const Labels& clean) {
        dt_pts = clean.dt_pts;
        dt_transfers = clean.dt_transfers;
        boarding_jpp_pts.resize(clean.boarding_jpp_pts.size());
        boarding_sp_transfers.resize(clean.boarding_sp_transfers.size());
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

private:
    inline void init(size_t nb_sp, DateTime val) {
        dt_pts.assign(nb_sp, val);
        dt_transfers.assign(nb_sp, val);
        boarding_jpp_pts.resize(nb_sp);
        boarding_sp_transfers.resize(nb_sp);
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
};


struct best_dest {
    // fallback time at the end of the journey. Only the stoppoint that can be reached by the fallback are marked
    IdxMap<type::StopPoint, navitia::time_duration> sp_idx_duration;
    DateTime best_now;
    SpIdx best_now_sp_idx;
    size_t count;

    void add_destination(const SpIdx sp, const time_duration duration_to_dest) {
        sp_idx_duration[sp] = duration_to_dest;
    }

    inline bool is_eligible_solution(const SpIdx sp_idx) const {
        return sp_idx_duration[sp_idx] != boost::posix_time::pos_infin;
    }

    template<typename Visitor>
    inline bool add_best(const Visitor & v, SpIdx sp_idx, const DateTime &t, size_t cnt) {
        if (is_eligible_solution(sp_idx)) {
            if(v.clockwise())
                return add_best_clockwise(sp_idx, t, cnt);
            else
                return add_best_unclockwise(sp_idx, t, cnt);
        }
        return false;
    }


    inline bool add_best_clockwise(SpIdx sp_idx, const DateTime& t, size_t cnt) {
        if (t != DateTimeUtils ::inf) {
            const auto tmp_dt = t + sp_idx_duration[sp_idx];
            if((tmp_dt < best_now) || ((tmp_dt == best_now) && (count > cnt))) {
                best_now = tmp_dt;
                best_now_sp_idx = sp_idx;
                count = cnt;
                return true;
            }
         }

        return false;
    }

    inline bool add_best_unclockwise(SpIdx sp_idx, const DateTime& t, size_t cnt) {
        if (t != DateTimeUtils::min) {
            const auto tmp_dt = t - sp_idx_duration[sp_idx];
            if((tmp_dt > best_now) || ((tmp_dt == best_now) && (count > cnt))) {
                best_now = tmp_dt;
                best_now_sp_idx = sp_idx;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    void reinit(const size_t nb_sp_idx) {
        sp_idx_duration.assign(nb_sp_idx, boost::posix_time::pos_infin);
        best_now = DateTimeUtils::inf;
        best_now_sp_idx = SpIdx();
        count = std::numeric_limits<size_t>::max();
    }

    void reinit(size_t nb_sp_idx, const DateTime& bound) {
        reinit(nb_sp_idx);
        best_now = bound;
    }

//    void reverse() {
//        best_now = DateTimeUtils::min;
//    }
};

} // namespace routing
} // namespace navitia
