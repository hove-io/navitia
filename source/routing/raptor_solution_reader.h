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

#pragma once

#include "raptor.h"
#include "raptor_visitors.h"

namespace navitia { namespace routing {

struct PathElt {
    explicit PathElt(const type::StopTime& b,
                     const DateTime b_dt,
                     const type::StopTime& e,
                     const DateTime e_dt,
                     const PathElt* p = nullptr):
        begin_st(b),
        begin_dt(b_dt),
        end_st(e),
        end_dt(e_dt),
        prev(p) {}
    PathElt(const PathElt&) = delete;
    PathElt& operator=(const PathElt&) = delete;
    const type::StopTime& begin_st;
    const DateTime begin_dt;
    const type::StopTime& end_st;
    const DateTime end_dt;
    const PathElt* prev;
};

template<typename Visitor>
struct RaptorSolutionReader {
    RaptorSolutionReader(const RAPTOR& r,
                         const Visitor& vis,
                         const RAPTOR::vec_stop_point_duration& d,
                         const bool disruption,
                         const type::AccessibiliteParams& access):
        raptor(r), v(vis), dep(d), disruption_active(disruption), accessibilite_params(access)
    {}
    const RAPTOR& raptor;
    const Visitor& v;
    const RAPTOR::vec_stop_point_duration& dep;
    const bool disruption_active;
    const type::AccessibiliteParams& accessibilite_params;

    void found_solution(const PathElt& path) const {
        for (const PathElt* elt = &path; elt != nullptr; elt = elt->prev) {
            std::cout << "(" << elt->begin_st.journey_pattern_point->stop_point->uri << ": "
                      << elt->begin_dt << ", "
                      << elt->end_st.journey_pattern_point->stop_point->uri << ": "
                      << elt->end_dt << ")";
        }
        std::cout << std::endl;
    }

    void transfer(const unsigned count,
                   const SpIdx sp_idx,
                   const DateTime begin,
                   const PathElt* prev) const {
        const auto& cnx_list = v.clockwise() ?
            raptor.data.dataRaptor->connections.forward_connections :
            raptor.data.dataRaptor->connections.backward_connections;
        const DateTime transfer_limit = raptor.labels[count].dt_pt(sp_idx);
        for (const auto& conn: cnx_list[sp_idx]) {
            const SpIdx destination_sp_idx = conn.sp_idx;
            const DateTime next = v.combine(begin, conn.duration);
            if (v.comp(transfer_limit, next)) { continue; }
            pt(count, destination_sp_idx, next, prev);
        }
    }

    void pt(const unsigned count,
            const SpIdx sp_idx,
            const DateTime begin,
            const PathElt* prev = nullptr) const {
        const DateTime get_in_limit = raptor.labels[count].dt_pt(sp_idx);
        for (const auto jpp: raptor.data.dataRaptor->jpps_from_sp[sp_idx]) {
            // trying to get in
            const auto tmp_st_dt = raptor.next_st.next_stop_time(
                jpp.idx, begin, v.clockwise(), disruption_active,
                accessibilite_params.vehicle_properties/*, jpp.has_freq*/);
            if (tmp_st_dt.first == nullptr) { continue; }
            const auto get_in_st = *tmp_st_dt.first;
            if (v.comp(get_in_limit, tmp_st_dt.second)) { continue; }

            // great, we are in
            auto workingDt = tmp_st_dt.second;
            const auto l_zone = get_in_st.local_traffic_zone;
            static const auto no_zone = std::numeric_limits<uint16_t>::max();
            for (const auto& st: v.first_stoptime(get_in_st)) {
                const auto current_time = st.section_end_time(
                    v.clockwise(), DateTimeUtils::hour(workingDt));
                DateTimeUtils::update(workingDt, current_time, v.clockwise());

                // trying to get out
                if (! st.valid_end(v.clockwise())) { continue; }
                if (l_zone != no_zone && l_zone == st.local_traffic_zone) { continue; }
                const SpIdx sp_idx = SpIdx(*st.journey_pattern_point->stop_point);
                if (! raptor.labels[count - 1].transfer_is_initialized(sp_idx)) { continue; }
                const DateTime get_out_limit = raptor.labels[count - 1].dt_transfer(sp_idx);
                if (v.comp(get_out_limit, workingDt)) { continue; }

                // great, we are out
                const PathElt path(get_in_st, tmp_st_dt.second, st, workingDt, prev);
                if (count == 1) {
                    // we've finished, and it's a destination point as it is initialized
                    found_solution(path);
                } else {
                    transfer(count - 1, sp_idx, workingDt, &path);
                }
            }
        }
    }
};
template<typename Visitor> RaptorSolutionReader<Visitor>
make_raptor_solution_reader(const RAPTOR& r,
                            const Visitor& v,
                            const RAPTOR::vec_stop_point_duration& d,
                            const bool disruption_active,
                            const type::AccessibiliteParams& accessibilite_params) {
    return RaptorSolutionReader<Visitor>(r, v, d, disruption_active, accessibilite_params);
}

template<typename Visitor>
void read_solutions(const RAPTOR& raptor,
                    const Visitor& v,
                    const RAPTOR::vec_stop_point_duration& dep,
                    const RAPTOR::vec_stop_point_duration& arr,
                    const bool disruption_active,
                    const type::AccessibiliteParams& accessibilite_params)
{
    const auto reader = make_raptor_solution_reader(raptor, v, dep, disruption_active, accessibilite_params);
    std::cout << "begin reader" << std::endl;
    for (unsigned count = 1; count <= raptor.count; ++count) {
        auto& working_labels = raptor.labels[count];
        for (const auto& a: arr) {
            if (working_labels.pt_is_initialized(a.first)) {
                reader.pt(count, a.first, working_labels.dt_pt(a.first));
            }
        }
    }
    std::cout << "end reader" << std::endl;
}

inline void read_solutions(const RAPTOR& raptor,
                           const bool clockwise,
                           const RAPTOR::vec_stop_point_duration& dep,
                           const RAPTOR::vec_stop_point_duration& arr,
                           const bool disruption_active,
                           const type::AccessibiliteParams& accessibilite_params)
{
    if (clockwise) {
        read_solutions(raptor, raptor_reverse_visitor(), dep, arr,
                       disruption_active, accessibilite_params);
    } else {
        read_solutions(raptor, raptor_visitor(), dep, arr,
                       disruption_active, accessibilite_params);
    }
}

}} // namespace navitia::routing
