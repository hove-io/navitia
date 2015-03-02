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
#include "utils/multi_obj_pool.h"

#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/algorithm/find_if.hpp>

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

template<typename Visitor> struct RaptorSolutionReader;

struct Journey {
    struct Section;

    template<typename Visitor>
    Journey(const PathElt& path, const RaptorSolutionReader<Visitor>& reader) {
        // constructing sections
        for (const PathElt* elt = &path; elt != nullptr; elt = elt->prev) {
            if (reader.v.clockwise()) {
                sections.emplace_back(elt->begin_st, elt->begin_dt, elt->end_st, elt->end_dt);
            } else {
                sections.emplace_back(elt->end_st, elt->end_dt, elt->begin_st, elt->begin_dt);
            }
        }
        if (reader.v.clockwise()) {
            boost::reverse(sections);
        } else {
            // the request is clockwise, thus we need to left align
            // the sections (also known as third pass)
            align_left(reader);
        }

        // getting departure/arrival values
        const Section& dep_section = sections.front();
        const Section& arr_section = sections.back();
        const auto dep_sp_idx = SpIdx(*dep_section.get_in_st->journey_pattern_point->stop_point);
        const auto arr_sp_idx = SpIdx(*arr_section.get_out_st->journey_pattern_point->stop_point);

        // is_sp_idx(sp_idx)(sp_dur) returns true if sp_idx == sp_dur.first
        const auto is_sp_idx = [](const SpIdx idx) {
            return [idx](RAPTOR::vec_stop_point_duration::const_reference sp_dur) {
                return sp_dur.first == idx;
            };
        };
        const auto dep_sp_dur_it = boost::find_if(reader.sp_dur_deps, is_sp_idx(dep_sp_idx));
        assert(dep_sp_dur_it != reader.sp_dur_deps.end());
        const auto arr_sp_dur_it = boost::find_if(reader.sp_dur_arrs, is_sp_idx(arr_sp_idx));
        assert(arr_sp_dur_it != reader.sp_dur_arrs.end());
        const auto dep_sn_dur = dep_sp_dur_it->second;
        const auto arr_sn_dur = arr_sp_dur_it->second;

        // setting objectives
        departure_dt = dep_section.get_in_dt - dep_sn_dur.total_seconds();
        arrival_dt = arr_section.get_out_dt + arr_sn_dur.total_seconds();
        sn_dur = dep_sn_dur + arr_sn_dur;
        nb_vj_extentions = count_vj_extentions();

        // transfer objectives
        if (sections.size() > 1) {
            const auto& data = *reader.raptor.data.pt_data;
            const auto first_transfer_waiting = get_transfer_waiting(data, sections[0], sections[1]);
            transfer_dur = first_transfer_waiting.first;
            min_waiting_dur = first_transfer_waiting.second;
            const auto* prev = &sections[1];
            for (auto it = sections.begin() + 2; it != sections.end(); prev = &*it, ++it) {
                const auto cur_transfer_waiting = get_transfer_waiting(data, *prev, *it);
                transfer_dur += cur_transfer_waiting.first;
                min_waiting_dur = std::min(min_waiting_dur, cur_transfer_waiting.second);
            }
        }
    }
    std::pair<const type::StopTime*, DateTime>
    get_out_st_dt(const std::pair<const type::StopTime*, DateTime>& in_st_dt,
                  const type::JourneyPatternPoint*const target_jpp) const
    {
        DateTime cur_dt = in_st_dt.second;
        for (const auto& st: raptor_visitor().st_range(*in_st_dt.first).advance_begin(1)) {
            const auto current_time = st.section_end_time(true, DateTimeUtils::hour(cur_dt));
            DateTimeUtils::update(cur_dt, current_time, true);
            if (st.journey_pattern_point == target_jpp) {
                // check if the get_out is valid?
                return {&st, cur_dt};
            }
        }
        for (const auto* stay_in_vj = in_st_dt.first->vehicle_journey->next_vj;
             stay_in_vj != nullptr;
             stay_in_vj = stay_in_vj->next_vj) {
            for (const auto& st: stay_in_vj->stop_time_list) {
                const auto current_time = st.section_end_time(true, DateTimeUtils::hour(cur_dt));
                DateTimeUtils::update(cur_dt, current_time, true);
                if (st.journey_pattern_point == target_jpp) {
                    // check if the get_out is valid?
                    return {&st, cur_dt};
                }
            }
        }
        assert(false);
        return {nullptr, 0};
    }
    template<typename Visitor>
    void align_left(const RaptorSolutionReader<Visitor>& reader) {
        if (sections.size() < 3) { return; }
        const auto* prev_s = &sections.at(0);
        for (auto& cur_s: boost::make_iterator_range(sections.begin() + 1, sections.end() - 1)) {
            const auto* cur_jpp = cur_s.get_in_st->journey_pattern_point;
            const auto* conn = reader.raptor.data.pt_data->get_stop_point_connection(
                *prev_s->get_out_st->journey_pattern_point->stop_point,
                *cur_jpp->stop_point);
            assert(conn != nullptr);
            const auto new_st_dt = reader.raptor.next_st.earliest_stop_time(
                JppIdx(*cur_jpp),
                prev_s->get_out_dt + conn->duration,
                reader.disruption_active,
                reader.accessibilite_params.vehicle_properties);
            if (new_st_dt.second < cur_s.get_in_dt) {
                std::cout << "align left: " << new_st_dt.second << " < " << cur_s.get_in_dt << std::endl;
                cur_s.get_in_st = new_st_dt.first;
                cur_s.get_in_dt = new_st_dt.second;
                const auto out_st_dt = get_out_st_dt(new_st_dt, cur_s.get_out_st->journey_pattern_point);
                cur_s.get_out_st = out_st_dt.first;
                cur_s.get_out_dt = out_st_dt.second;
            }
            prev_s = &cur_s;
        }
    }
    uint8_t count_vj_extentions() const {
        uint8_t nb_ext = 0;
        for (const auto& s: sections) {
            for (auto* vj = s.get_in_st->vehicle_journey; true; vj = vj->next_vj) {
                if (vj == nullptr) { /* we should not stop here */ assert(false); break; }
                const auto search = boost::find_if(vj->stop_time_list, [&](const type::StopTime& st) {
                        return &st == s.get_out_st;
                    });
                if (search != vj->stop_time_list.end()) { break; }
                ++nb_ext;
            }
        }
        return nb_ext;
    }
    std::pair<navitia::time_duration, navitia::time_duration>
    get_transfer_waiting(const type::PT_Data& data, const Section& from, const Section& to) const {
        const auto* conn = data.get_stop_point_connection(
            *from.get_out_st->journey_pattern_point->stop_point,
            *to.get_in_st->journey_pattern_point->stop_point);
        assert(conn);
        if (! conn) { return std::make_pair(0_s, 0_s); }// it should be dead code
        const auto dur_conn = conn->display_duration;
        const auto dur_transfer = to.get_in_dt - from.get_out_dt;
        return std::make_pair(navitia::seconds(dur_conn), navitia::seconds(dur_transfer - dur_conn));
    }
    bool better_on_dt(const Journey& that, bool request_clockwise) const {
        if (request_clockwise) {
            if (arrival_dt != that.arrival_dt) { return arrival_dt <= that.arrival_dt; }
            if (departure_dt != that.departure_dt) { return departure_dt >= that.departure_dt; }
        } else {
            if (departure_dt != that.departure_dt) { return departure_dt >= that.departure_dt; }
            if (arrival_dt != that.arrival_dt) { return arrival_dt <= that.arrival_dt; }
        }
        // FIXME: I don't like this objective, for me, this is a
        // transfer objective, but then you can return some solutions
        // that we didn't return before.
        if (sections.size() != that.sections.size()) { return true; }
        if (min_waiting_dur != that.min_waiting_dur) {
            return min_waiting_dur >= that.min_waiting_dur;
        }
        return transfer_dur <= that.transfer_dur;
    }
    bool better_on_transfer(const Journey& that, bool) const {
        if (sections.size() != that.sections.size()) {
            return sections.size() <= that.sections.size();
        }
        return nb_vj_extentions <= that.nb_vj_extentions;
    }
    bool better_on_sn(const Journey& that, bool) const {
        return sn_dur <= that.sn_dur;
    }
    friend std::ostream& operator<<(std::ostream& os, const Journey& j) {
        os << "([" << j.departure_dt << ", " << j.arrival_dt << ", "
           << j.min_waiting_dur << ", " << j.transfer_dur << "], ["
           << j.sections.size() << ", " << unsigned(j.nb_vj_extentions) << "], "
           << j.sn_dur << ") ";
        for (const auto& s: j.sections) {
            os << "("
               << s.get_in_st->journey_pattern_point->journey_pattern->route->line->uri << ": "
               << s.get_in_st->journey_pattern_point->stop_point->uri << "@"
               << s.get_in_dt << ", "
               << s.get_out_st->journey_pattern_point->stop_point->uri << "@"
               << s.get_out_dt << ")";
        }
        return os;
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
    navitia::time_duration sn_dur = 0_s;// street network duration
    navitia::time_duration transfer_dur = 0_s;// walking duration during transfer
    navitia::time_duration min_waiting_dur = 0_s;// minimal waiting duration on every transfers
    DateTime departure_dt = 0;// the departure dt of the journey, including sn
    DateTime arrival_dt = 0;// the arrival dt of the journey, including sn
    uint8_t nb_vj_extentions = 0;
};

// this structure compare 2 solutions.  It chooses which solutions
// will be kept at the end (only non dominated solutions will be kept
// by the pool).
struct Dominates {
    bool request_clockwise;
    Dominates(bool rc): request_clockwise(rc) {}
    bool operator()(const Journey& lhs, const Journey& rhs) const {
        return lhs.better_on_dt(rhs, request_clockwise)
            && lhs.better_on_transfer(rhs, request_clockwise)
            && lhs.better_on_sn(rhs, request_clockwise);
    }
};

template<typename Visitor>
struct RaptorSolutionReader {
    RaptorSolutionReader(const RAPTOR& r,
                         const Visitor& vis,// 2nd pass visitor
                         const RAPTOR::vec_stop_point_duration& deps,
                         const RAPTOR::vec_stop_point_duration& arrs,
                         const bool disruption,
                         const type::AccessibiliteParams& access):
        raptor(r),
        v(vis),
        sp_dur_deps(deps),
        sp_dur_arrs(arrs),
        disruption_active(disruption),
        accessibilite_params(access),
        // Dominates need request_clockwise (the negation of 2nd pass clockwise)
        solutions(Dominates(! v.clockwise()))
    {}
    const RAPTOR& raptor;
    const Visitor& v;
    const RAPTOR::vec_stop_point_duration& sp_dur_deps;// departures (not clockwise dependent)
    const RAPTOR::vec_stop_point_duration& sp_dur_arrs;// arrivals (not clockwise dependent)
    const bool disruption_active;
    const type::AccessibiliteParams& accessibilite_params;
    ParetoFront<Journey, Dominates> solutions;

    void handle_solution(const PathElt& path) {
        Journey j(path, *this);
        std::cout << "  " << j << std::endl;
        solutions.add(std::move(j));
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
                            const RAPTOR::vec_stop_point_duration& deps,
                            const RAPTOR::vec_stop_point_duration& arrs,
                            const bool disruption_active,
                            const type::AccessibiliteParams& accessibilite_params) {
    return RaptorSolutionReader<Visitor>(r, v, deps, arrs, disruption_active, accessibilite_params);
}

template<typename Visitor>
size_t read_solutions(const RAPTOR& raptor,
                    const Visitor& v,
                    const RAPTOR::vec_stop_point_duration& deps,
                    const RAPTOR::vec_stop_point_duration& arrs,
                    const bool disruption_active,
                    const type::AccessibiliteParams& accessibilite_params)
{
    auto reader = make_raptor_solution_reader(
        raptor, v, deps, arrs, disruption_active, accessibilite_params);
    std::cout << "begin reader" << std::endl;
    for (unsigned count = 1; count <= raptor.count; ++count) {
        auto& working_labels = raptor.labels[count];
        for (const auto& a: v.clockwise() ? deps : arrs) {
            if (working_labels.pt_is_initialized(a.first)) {
                reader.begin_pt(count, a.first, working_labels.dt_pt(a.first));
            }
        }
    }
    std::cout << "solutions:" << std::endl;
    for (const auto& s: reader.solutions) {
        std::cout << "  " << s << std::endl;
    }
    std::cout << "end reader" << std::endl;
    return reader.solutions.size();
}

} // anonymous namespace

size_t read_solutions(const RAPTOR& raptor,
                      const bool clockwise,
                      const RAPTOR::vec_stop_point_duration& deps,
                      const RAPTOR::vec_stop_point_duration& arrs,
                      const bool disruption_active,
                      const type::AccessibiliteParams& accessibilite_params)
{
    if (clockwise) {
        return read_solutions(raptor, raptor_reverse_visitor(), deps, arrs,
                              disruption_active, accessibilite_params);
    } else {
        return read_solutions(raptor, raptor_visitor(), deps, arrs,
                              disruption_active, accessibilite_params);
    }
}

}} // namespace navitia::routing
