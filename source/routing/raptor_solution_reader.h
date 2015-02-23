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
    size_t nb_solutions = 0;

    void handle_solution(const PathElt& path) {
        ++nb_solutions;
        for (const PathElt* elt = &path; elt != nullptr; elt = elt->prev) {
            std::cout << "(" << elt->begin_st.journey_pattern_point->journey_pattern->route->line->uri << ": ";
            if (v.clockwise()) {
                std::cout << elt->begin_st.journey_pattern_point->stop_point->uri << "@"
                          << elt->begin_dt << ", "
                          << elt->end_st.journey_pattern_point->stop_point->uri << "@"
                          << elt->end_dt << ")";
            } else {
                std::cout << elt->end_st.journey_pattern_point->stop_point->uri << "@"
                          << elt->end_dt << ", "
                          << elt->begin_st.journey_pattern_point->stop_point->uri << "@"
                          << elt->begin_dt << ")";
            }
        }
        std::cout << std::endl;
    }

    void transfer(const unsigned count,
                   const SpIdx sp_idx,
                   const DateTime lb_dt,
                   const PathElt* prev) {
        const auto& cnx_list = v.clockwise() ?
            raptor.data.dataRaptor->connections.forward_connections :
            raptor.data.dataRaptor->connections.backward_connections;
        for (const auto& conn: cnx_list[sp_idx]) {
            const DateTime transfer_limit = raptor.labels[count].dt_pt(conn.sp_idx);
            const DateTime transfer_arrival = v.combine(lb_dt, conn.duration);
            if (v.comp(transfer_limit, transfer_arrival)) { continue; }

            // transfer is OK
            pt_in(count, conn.sp_idx, transfer_arrival, prev);
        }
    }

    template<typename Range>
    void pt_out(const unsigned count,
                const PathElt* prev,
                const std::pair<const type::StopTime*, DateTime>& get_in_st_dt,
                const uint16_t l_zone,
                DateTime& cur_dt,
                const Range& st_range) {
        static const auto no_zone = std::numeric_limits<uint16_t>::max();

        for (const auto& get_out_st: st_range) {
            const auto current_time = get_out_st.section_end_time(
                v.clockwise(), DateTimeUtils::hour(cur_dt));
            DateTimeUtils::update(cur_dt, current_time, v.clockwise());

            // trying to get out
            if (! get_out_st.valid_end(v.clockwise())) { continue; }
            if (l_zone != no_zone && l_zone == get_out_st.local_traffic_zone) { continue; }
            const SpIdx get_out_sp_idx = SpIdx(*get_out_st.journey_pattern_point->stop_point);
            if (! raptor.labels[count - 1].transfer_is_initialized(get_out_sp_idx)) { continue; }
            const DateTime get_out_limit = raptor.labels[count - 1].dt_transfer(get_out_sp_idx);
            if (v.comp(get_out_limit, cur_dt)) { continue; }

            // great, we are out
            const PathElt path(*get_in_st_dt.first, get_in_st_dt.second, get_out_st, cur_dt, prev);
            if (count == 1) {
                // we've finished, and it's a destination point as it is initialized
                handle_solution(path);
            } else {
                transfer(count - 1, get_out_sp_idx, cur_dt, &path);
            }
        }
    }

    void pt_in(const unsigned count,
               const SpIdx get_in_sp_idx,
               const DateTime lb_dt,
               const PathElt* prev = nullptr) {
        const DateTime get_in_limit = raptor.labels[count].dt_pt(get_in_sp_idx);
        for (const auto jpp: raptor.data.dataRaptor->jpps_from_sp[get_in_sp_idx]) {
            // trying to get in
            const auto get_in_st_dt = raptor.next_st.next_stop_time(
                jpp.idx, lb_dt, v.clockwise(), disruption_active,
                accessibilite_params.vehicle_properties/*, jpp.has_freq*/);
            if (get_in_st_dt.first == nullptr) { continue; }
            if (v.comp(get_in_limit, get_in_st_dt.second)) { continue; }

            // great, we are in
            auto cur_dt = get_in_st_dt.second;
            const auto l_zone = get_in_st_dt.first->local_traffic_zone;
            pt_out(count, prev, get_in_st_dt, l_zone, cur_dt,
                   v.st_range(*get_in_st_dt.first).advance_begin(1));

            // continuing in the stay in
            for (const auto* stay_in_vj = v.get_extension_vj(get_in_st_dt.first->vehicle_journey);
                 stay_in_vj != nullptr;
                 stay_in_vj = v.get_extension_vj(stay_in_vj)) {
                pt_out(count, prev, get_in_st_dt, l_zone, cur_dt,
                       v.stop_time_list(stay_in_vj));
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
size_t read_solutions(const RAPTOR& raptor,
                    const Visitor& v,
                    const RAPTOR::vec_stop_point_duration& dep,
                    const RAPTOR::vec_stop_point_duration& arr,
                    const bool disruption_active,
                    const type::AccessibiliteParams& accessibilite_params)
{
    auto reader = make_raptor_solution_reader(raptor, v, dep, disruption_active, accessibilite_params);
    std::cout << "begin reader" << std::endl;
    for (unsigned count = 1; count <= raptor.count; ++count) {
        auto& working_labels = raptor.labels[count];
        for (const auto& a: arr) {
            if (working_labels.pt_is_initialized(a.first)) {
                std::cout << "from " << raptor.get_sp(a.first)->uri << std::endl;
                reader.pt_in(count, a.first, working_labels.dt_pt(a.first));
            }
        }
    }
    std::cout << "end reader" << std::endl;
    return reader.nb_solutions;
}

inline size_t read_solutions(const RAPTOR& raptor,
                             const bool clockwise,
                             const RAPTOR::vec_stop_point_duration& dep,
                             const RAPTOR::vec_stop_point_duration& arr,
                             const bool disruption_active,
                             const type::AccessibiliteParams& accessibilite_params)
{
    if (clockwise) {
        return read_solutions(raptor, raptor_reverse_visitor(), dep, arr,
                              disruption_active, accessibilite_params);
    } else {
        return read_solutions(raptor, raptor_visitor(), dep, arr,
                              disruption_active, accessibilite_params);
    }
}

}} // namespace navitia::routing
