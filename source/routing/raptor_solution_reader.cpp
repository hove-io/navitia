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

#include "raptor.h"
#include "raptor_visitors.h"

#include <boost/range/algorithm/reverse.hpp>

namespace navitia { namespace routing {

namespace {

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

struct Journey {
    template<typename Visitor>
    Journey(const PathElt& path, const RaptorSolutionReader<Visitor>& reader):
        clockwise(reader.v.clockwise()) {
        for (const PathElt* elt = &path; elt != nullptr; elt = elt->prev) {
            if (clockwise) {
                sections.emplace_back(elt->begin_st, elt->begin_dt, elt->end_st, elt->end_dt);
            } else {
                sections.emplace_back(elt->end_st, elt->end_dt, elt->begin_st, elt->begin_dt);
            }
        }
        if (clockwise) { boost::reverse(sections); }
    }

    struct Section {
        Section(const type::StopTime& in,
                const DateTime in_dt,
                const type::StopTime& out,
                const DateTime out_dt):
            get_in_st(&in), get_in_dt(in_dt), get_out_st(&out), get_out_dt(out_dt)
        {}
        const type::StopTime* get_in_st;
        DateTime get_in_dt;
        const type::StopTime* get_out_st;
        DateTime get_out_dt;
    };
    std::vector<Section> sections;
    navitia::time_duration sn_dur;// street network duration
    navitia::time_duration transfer_dur;// walking duration during transfer
    navitia::time_duration min_waiting_dur;// minimal waiting duration on every transfers
    DateTime end_dt;// the end of the journey, the main objective
    DateTime begin_dt;// the begin of the journey,
    uint8_t nb_vj_extentions;
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
        Journey journey(path, v.clockwise());

        for (const auto& s: journey.sections) {
            std::cout << "("
                      << s.get_in_st->journey_pattern_point->journey_pattern->route->line->uri << ": "
                      << s.get_in_st->journey_pattern_point->stop_point->uri << "@"
                      << s.get_in_dt << ", "
                      << s.get_out_st->journey_pattern_point->stop_point->uri << "@"
                      << s.get_out_dt << ")";
        }
        std::cout << std::endl;
    }

    void transfer(const unsigned count,
                  const SpIdx sp_idx,
                  const DateTime begin_dt,
                  const PathElt* path) {
        const auto& cnx_list = v.clockwise() ?
            raptor.data.dataRaptor->connections.forward_connections :
            raptor.data.dataRaptor->connections.backward_connections;
        for (const auto& conn: cnx_list[sp_idx]) {
            const DateTime transfer_limit = raptor.labels[count].dt_pt(conn.sp_idx);
            const DateTime transfer_end = v.combine(begin_dt, conn.duration);
            if (v.comp(transfer_limit, transfer_end)) { continue; }

            // transfer is OK
            begin_pt(count, conn.sp_idx, transfer_end, path);
        }
    }

    template<typename Range>
    void end_pt(const unsigned count,
                const PathElt* path,
                const std::pair<const type::StopTime*, DateTime>& begin_st_dt,
                const uint16_t begin_zone,
                DateTime& cur_dt,
                const Range& st_range) {
        static const auto no_zone = std::numeric_limits<uint16_t>::max();

        for (const auto& end_st: st_range) {
            const auto current_time = end_st.section_end_time(
                v.clockwise(), DateTimeUtils::hour(cur_dt));
            DateTimeUtils::update(cur_dt, current_time, v.clockwise());

            // trying to end
            if (! end_st.valid_end(v.clockwise())) { continue; }
            if (begin_zone != no_zone && begin_zone == end_st.local_traffic_zone) { continue; }
            const SpIdx end_sp_idx = SpIdx(*end_st.journey_pattern_point->stop_point);
            if (! raptor.labels[count - 1].transfer_is_initialized(end_sp_idx)) { continue; }
            const DateTime end_limit = raptor.labels[count - 1].dt_transfer(end_sp_idx);
            if (v.comp(end_limit, cur_dt)) { continue; }

            // great, we can end
            const PathElt new_path(*begin_st_dt.first, begin_st_dt.second, end_st, cur_dt, path);
            if (count == 1) {
                // we've finished, and it's a valid end as it is initialized
                handle_solution(new_path);
            } else {
                transfer(count - 1, end_sp_idx, cur_dt, &new_path);
            }
        }
    }

    void begin_pt(const unsigned count,
                  const SpIdx begin_sp_idx,
                  const DateTime begin_dt,
                  const PathElt* path = nullptr) {
        const DateTime begin_limit = raptor.labels[count].dt_pt(begin_sp_idx);
        for (const auto jpp: raptor.data.dataRaptor->jpps_from_sp[begin_sp_idx]) {
            // trying to begin
            const auto begin_st_dt = raptor.next_st.next_stop_time(
                jpp.idx, begin_dt, v.clockwise(), disruption_active,
                accessibilite_params.vehicle_properties/*, jpp.has_freq*/);
            if (begin_st_dt.first == nullptr) { continue; }
            if (v.comp(begin_limit, begin_st_dt.second)) { continue; }

            // great, we can begin
            auto cur_dt = begin_st_dt.second;
            const auto begin_zone = begin_st_dt.first->local_traffic_zone;
            end_pt(count, path, begin_st_dt, begin_zone, cur_dt,
                   v.st_range(*begin_st_dt.first).advance_begin(1));

            // continuing in the stay in
            for (const auto* stay_in_vj = v.get_extension_vj(begin_st_dt.first->vehicle_journey);
                 stay_in_vj != nullptr;
                 stay_in_vj = v.get_extension_vj(stay_in_vj)) {
                end_pt(count, path, begin_st_dt, begin_zone, cur_dt,
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
                reader.begin_pt(count, a.first, working_labels.dt_pt(a.first));
            }
        }
    }
    std::cout << "end reader" << std::endl;
    return reader.nb_solutions;
}

} // anonymous namespace

size_t read_solutions(const RAPTOR& raptor,
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
