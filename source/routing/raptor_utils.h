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
#include <unordered_map>
#include "type/type.h"
#include "type/datetime.h"

namespace navitia { namespace routing {

struct Labels {
    inline void assign_inf(size_t nb) {
        init(nb, DateTimeUtils::inf);
    }
    inline void assign_min(size_t nb) {
        init(nb, DateTimeUtils::min);
    }
    inline void clear(const Labels& clean) {
        dt_pts = clean.dt_pts;
        dt_transfers = clean.dt_transfers;
        boarding_jpp_pts.resize(clean.boarding_jpp_pts.size());
        boarding_jpp_transfers.resize(clean.boarding_jpp_transfers.size());
    }
    inline const navitia::type::idx_t&
    boarding_jpp_transfer(navitia::type::idx_t idx) const {
        return boarding_jpp_transfers[idx];
    }
    inline const navitia::type::idx_t&
    boarding_jpp_pt(navitia::type::idx_t idx) const {
        return boarding_jpp_pts[idx];
    }
    inline const DateTime& dt_transfer(navitia::type::idx_t idx) const {
        return dt_transfers[idx];
    }
    inline const DateTime& dt_pt(navitia::type::idx_t idx) const {
        return dt_pts[idx];
    }

    inline navitia::type::idx_t&
    mut_boarding_jpp_transfer(navitia::type::idx_t idx) {
        return boarding_jpp_transfers[idx];
    }
    inline navitia::type::idx_t&
    mut_boarding_jpp_pt(navitia::type::idx_t idx) {
        return boarding_jpp_pts[idx];
    }
    inline DateTime& mut_dt_transfer(navitia::type::idx_t idx) {
        return dt_transfers[idx];
    }
    inline DateTime& mut_dt_pt(navitia::type::idx_t idx) {
        return dt_pts[idx];
    }

    inline bool pt_is_initialized(navitia::type::idx_t idx) const {
        return dt_pt(idx) != DateTimeUtils::inf && dt_pt(idx) != DateTimeUtils::min;
    }
    inline bool transfer_is_initialized(navitia::type::idx_t idx) const {
        return dt_transfer(idx) != DateTimeUtils::inf && dt_transfer(idx) != DateTimeUtils::min;
    }

private:
    inline void init(size_t nb, DateTime val) {
        dt_pts.assign(nb, val);
        dt_transfers.assign(nb, val);
        boarding_jpp_pts.resize(nb);
        boarding_jpp_transfers.resize(nb);
    }

    std::vector<DateTime> dt_pts;
    std::vector<DateTime> dt_transfers;
    std::vector<navitia::type::idx_t> boarding_jpp_pts;
    std::vector<navitia::type::idx_t> boarding_jpp_transfers;
};

typedef std::pair<int, int> pair_int;
typedef std::vector<navitia::type::idx_t> vector_idx;
typedef std::pair<navitia::type::idx_t, int> pair_idx_int;
typedef std::vector<int> queue_t;
typedef std::vector<pair_int> vector_pairint;
typedef std::pair<navitia::type::idx_t, DateTime> idx_label;
typedef std::vector<idx_label> vector_idxlabel;
typedef std::pair<navitia::type::idx_t, double> idx_distance;
typedef std::vector<idx_distance> vec_idx_distance;

template<typename T>
inline void memset32(T*buf, uint n, T c)
{
    for(uint i = 0; i < n; i++) *buf++ = c;
}



struct best_dest {
    std::vector<navitia::time_duration> jpp_idx_duration;
    DateTime best_now;
    type::idx_t best_now_jpp_idx;
    size_t count;

    void add_destination(const type::JourneyPatternPoint* jpp, const time_duration duration_to_dest) {
        jpp_idx_duration[jpp->idx] = duration_to_dest; //AD, check if there are some rounding problems
    }


    inline bool is_eligible_solution(const type::idx_t jpp_idx) const {
        return jpp_idx_duration[jpp_idx] != boost::posix_time::pos_infin;
    }


    template<typename Visitor>
    inline bool add_best(const Visitor & v, type::idx_t jpp_idx, const DateTime &t, size_t cnt) {
        if(is_eligible_solution(jpp_idx)) {
            if(v.clockwise())
                return add_best_clockwise(jpp_idx, t, cnt);
            else
                return add_best_unclockwise(jpp_idx, t, cnt);
        }
        return false;
    }


    inline bool add_best_clockwise(type::idx_t jpp_idx, const DateTime &t, size_t cnt) {
        if(t != DateTimeUtils ::inf) {
            const auto tmp_dt = t + jpp_idx_duration[jpp_idx];
            if((tmp_dt < best_now) || ((tmp_dt == best_now) && (count > cnt))) {
                best_now = tmp_dt;
                best_now_jpp_idx = jpp_idx;
                count = cnt;
                return true;
            }
         }

        return false;
    }

    inline bool add_best_unclockwise(type::idx_t jpp_idx, const DateTime &t, size_t cnt) {
        if(t != DateTimeUtils::min) {
            const auto tmp_dt = t - jpp_idx_duration[jpp_idx];
            if((tmp_dt > best_now) || ((tmp_dt == best_now) && (count > cnt))) {
                best_now = tmp_dt;
                best_now_jpp_idx = jpp_idx;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    void reinit(const size_t nb_jpp_idx) {
        jpp_idx_duration.resize(nb_jpp_idx);
        memset32<navitia::time_duration>(&jpp_idx_duration[0], nb_jpp_idx, boost::posix_time::pos_infin);
        best_now = DateTimeUtils::inf;
        best_now_jpp_idx = type::invalid_idx;
        count = std::numeric_limits<size_t>::max();
    }

    void reinit(size_t nb_jpp_idx, const DateTime &borne) {
        reinit(nb_jpp_idx);
        best_now = borne;
    }

    void reverse() {
        best_now = DateTimeUtils::min;
    }
};
}}
