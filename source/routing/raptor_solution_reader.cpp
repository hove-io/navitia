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

#include "raptor_solution_reader.h"

#include "journey.h"
#include "raptor.h"
#include "raptor_visitors.h"
#include "utils/logger.h"

#include <boost/container/flat_map.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <utility>

namespace navitia {
namespace routing {

namespace {

struct PathElt {
    explicit PathElt(const type::StopTime& b,
                     const DateTime b_dt,
                     const type::StopTime& e,
                     const DateTime e_dt,
                     const PathElt* p = nullptr)
        : begin_st(b), begin_dt(b_dt), end_st(e), end_dt(e_dt), prev(p) {}
    PathElt(const PathElt&) = delete;
    PathElt& operator=(const PathElt&) = delete;
    PathElt(const PathElt&&) = delete;
    PathElt& operator=(const PathElt&&) = delete;

    const type::StopTime& begin_st;
    const DateTime begin_dt;
    const type::StopTime& end_st;
    const DateTime end_dt;
    const PathElt* prev;
};

template <typename Visitor>
struct RaptorSolutionReader;

std::pair<const type::StopTime*, DateTime> get_out_st_dt(const std::pair<const type::StopTime*, DateTime>& in_st_dt,
                                                         const JppIdx& target_jpp,
                                                         const JourneyPatternContainer& jp_container) {
    DateTime cur_dt = in_st_dt.second;
    DateTime base_dt = in_st_dt.first->base_dt(cur_dt, true);
    auto st_range = raptor_visitor().st_range(*in_st_dt.first);
    for (const auto& st : st_range.advance_begin(1)) {
        cur_dt = st.section_end(base_dt, true);
        if (jp_container.get_jpp(st) == target_jpp) {
            // check if the get_out is valid?
            return {&st, cur_dt};
        }
    }
    for (const auto* stay_in_vj = in_st_dt.first->vehicle_journey->next_vj; stay_in_vj != nullptr;
         stay_in_vj = stay_in_vj->next_vj) {
        base_dt = raptor_visitor().get_base_dt_extension(base_dt, stay_in_vj);
        for (const auto& st : stay_in_vj->stop_time_list) {
            cur_dt = st.section_end(base_dt, true);
            if (jp_container.get_jpp(st) == target_jpp) {
                // check if the get_out is valid?
                return {&st, cur_dt};
            }
        }
    }
    return {nullptr, 0};
}

template <typename Visitor>
bool is_valid(const RaptorSolutionReader<Visitor>& reader, const Journey& j) {
    // We don't want journeys that begins before (resp. after) the requested datetime
    if (reader.v.clockwise()) {
        if (j.departure_dt < reader.departure_datetime) {
            return false;
        }
    } else {
        if (j.arrival_dt > reader.departure_datetime) {
            return false;
        }
    }
    // We don't want journeys with a transfert between 2 estimated stop times.
    for (auto it = j.sections.begin(), it_prev = it++; it != j.sections.end(); it_prev = it++) {
        if (it_prev->get_out_st->date_time_estimated() && it->get_in_st->date_time_estimated()) {
            return false;
        }
    }
    return true;
}

// align every section to left, ie take the first vj of a jp, as
// opposed to wait to another one.
template <typename Visitor>
void align_left(const RaptorSolutionReader<Visitor>& reader, Journey& j) {
    if (j.sections.size() < 3) {
        // as the begin and the end is optimal by construction,
        // there is nothing to do if we have less than 3 sections.
        return;
    }
    const auto& jp_container = reader.raptor.data.dataRaptor->jp_container;
    const auto* prev_s = &j.sections.at(0);
    for (auto& cur_s : boost::make_iterator_range(j.sections.begin() + 1, j.sections.end() - 1)) {
        const auto& cur_jpp_idx = jp_container.get_jpp(*cur_s.get_in_st);
        const auto* conn = reader.raptor.data.pt_data->get_stop_point_connection(*prev_s->get_out_st->stop_point,
                                                                                 *cur_s.get_in_st->stop_point);
        assert(conn != nullptr);
        // const auto new_st_dt = reader.raptor.next_st->next_stop_time(StopEvent::pick_up, cur_jpp_idx,
        //                                                             prev_s->get_out_dt + conn->duration, true);
        const auto new_st_dt = reader.raptor.next_st->next_stop_time(
            StopEvent::pick_up, cur_jpp_idx, prev_s->get_out_dt + conn->duration, true, reader.rt_level,
            reader.accessibilite_params.vehicle_properties);

        if (new_st_dt.first && new_st_dt.second < cur_s.get_in_dt) {
            const auto out_st_dt = get_out_st_dt(new_st_dt, jp_container.get_jpp(*cur_s.get_out_st), jp_container);
            if (out_st_dt.first) {
                cur_s.get_in_st = new_st_dt.first;
                cur_s.get_in_dt = new_st_dt.second;
                cur_s.get_out_st = out_st_dt.first;
                cur_s.get_out_dt = out_st_dt.second;
            } else {
                // there is a problem with the journey pattern, we
                // cannot shift the date time, we do not change
                // anything
            }
        }
        prev_s = &cur_s;
    }
}

uint8_t count_vj_extentions(const Journey& j) {
    uint8_t nb_ext = 0;
    for (const auto& s : j.sections) {
        for (auto* vj = s.get_in_st->vehicle_journey; true; vj = vj->next_vj) {
            if (vj == nullptr) { /* we should not stop here */
                assert(false);
                break;
            }
            const auto search =
                boost::find_if(vj->stop_time_list, [&](const type::StopTime& st) { return &st == s.get_out_st; });
            if (search != vj->stop_time_list.end()) {
                break;
            }
            ++nb_ext;
        }
    }
    return nb_ext;
}

std::pair<navitia::time_duration, navitia::time_duration> get_transfer_waiting(const type::PT_Data& data,
                                                                               const Journey::Section& from,
                                                                               const Journey::Section& to) {
    const auto* conn = data.get_stop_point_connection(*from.get_out_st->stop_point, *to.get_in_st->stop_point);
    assert(conn);
    if (!conn) {
        return std::make_pair(0_s, 0_s);
    }  // it should be dead code
    const auto dur_conn = conn->display_duration;
    const auto dur_transfer = to.get_in_dt - from.get_out_dt;
    return std::make_pair(navitia::seconds(dur_conn), navitia::seconds(dur_transfer - dur_conn));
}

template <typename Visitor>
const Journey& make_journey(const PathElt& path, RaptorSolutionReader<Visitor>& reader) {
    Journey& j = reader.journey_cache.get();

    // constructing sections
    for (const PathElt* elt = &path; elt != nullptr; elt = elt->prev) {
        if (reader.v.clockwise()) {
            j.sections.emplace_back(elt->begin_st, elt->begin_dt, elt->end_st, elt->end_dt);
        } else {
            j.sections.emplace_back(elt->end_st, elt->end_dt, elt->begin_st, elt->begin_dt);
        }
    }
    if (reader.v.clockwise()) {
        boost::reverse(j.sections);
    } else {
        // the request is clockwise (opposite of the reader
        // direction), thus we need to left align the sections
        // (also known as third pass)
        align_left(reader, j);
    }

    // getting departure/arrival values
    const Journey::Section& dep_section = j.sections.front();
    const Journey::Section& arr_section = j.sections.back();
    const auto dep_sp_idx = SpIdx(*dep_section.get_in_st->stop_point);
    const auto arr_sp_idx = SpIdx(*arr_section.get_out_st->stop_point);

    // is_sp_idx(sp_idx)(sp_dur) returns true if sp_idx == sp_dur.first
    const auto is_sp_idx = [](const SpIdx idx) {
        return [idx](routing::map_stop_point_duration::const_reference sp_dur) { return sp_dur.first == idx; };
    };
    const auto dep_sp_dur_it = boost::find_if(reader.sp_dur_deps, is_sp_idx(dep_sp_idx));
    assert(dep_sp_dur_it != reader.sp_dur_deps.end());
    const auto arr_sp_dur_it = boost::find_if(reader.sp_dur_arrs, is_sp_idx(arr_sp_idx));
    assert(arr_sp_dur_it != reader.sp_dur_arrs.end());
    const auto dep_sn_dur = dep_sp_dur_it->second;
    const auto arr_sn_dur = arr_sp_dur_it->second;

    // setting objectives
    j.departure_dt = dep_section.get_in_dt - dep_sn_dur.total_seconds();
    j.arrival_dt = arr_section.get_out_dt + arr_sn_dur.total_seconds();
    j.sn_dur = dep_sn_dur + arr_sn_dur;
    j.nb_vj_extentions = count_vj_extentions(j);

    // transfer objectives
    j.transfer_dur = reader.arrival_transfer_penalty * (j.nb_vj_extentions);
    j.total_waiting_dur = 0_s;
    if (j.sections.size() > 1) {
        const auto& data = *reader.raptor.data.pt_data;
        const auto first_transfer_waiting = get_transfer_waiting(data, j.sections[0], j.sections[1]);
        j.transfer_dur += first_transfer_waiting.first;
        j.min_waiting_dur = first_transfer_waiting.second;
        j.total_waiting_dur += first_transfer_waiting.second;
        const auto* prev = &j.sections[1];
        for (auto it = j.sections.begin() + 2; it != j.sections.end(); prev = &*it, ++it) {
            const auto cur_transfer_waiting = get_transfer_waiting(data, *prev, *it);
            j.transfer_dur += cur_transfer_waiting.first;
            j.min_waiting_dur = std::min(j.min_waiting_dur, cur_transfer_waiting.second);
            j.total_waiting_dur += cur_transfer_waiting.second;
        }
    }

    return j;
}

struct VehicleSection {
    VehicleSection(const Journey::Section& s, const type::VehicleJourney* v) : section(s), vj(v) {}
    const Journey::Section& section;
    const type::VehicleJourney* vj;

    struct StopTimeDatetime {
        StopTimeDatetime(const type::StopTime& s, const DateTime dep, const DateTime arr)
            : st(s), departure(dep), arrival(arr) {}
        const type::StopTime& st;
        DateTime departure, arrival;
    };

    // all stop times of the section + their departure and arrival time
    std::vector<StopTimeDatetime> stop_times_and_dt;
};

std::vector<VehicleSection> get_vjs(const Journey::Section& section) {
    std::vector<VehicleSection> res;
    auto current_st = section.get_in_st;
    auto end_st = section.get_out_st;
    auto base_dt = section.get_in_st->base_dt(section.get_in_dt, true);
    DateTime current_dep;
    DateTime current_arr;

    auto order = current_st->order();
    for (const auto* vj = current_st->vehicle_journey; vj; vj = vj->next_vj) {
        if (!res.empty()) {
            // only update base_dt for vj extensions
            base_dt = raptor_visitor().get_base_dt_extension(base_dt, vj);
        }
        res.emplace_back(section, current_st->vehicle_journey);

        for (const auto& st :
             boost::make_iterator_range(vj->stop_time_list.begin() + order.val, vj->stop_time_list.end())) {
            current_dep = st.departure(base_dt);
            current_arr = st.arrival(base_dt);
            res.back().stop_times_and_dt.emplace_back(st, current_dep + st.get_boarding_duration(),
                                                      current_arr - st.get_alighting_duration());

            if (&st == end_st) {
                return res;
            }
        }
        order = type::RankStopTime(0);  // for the stay in vj's, we start from the first stop time
    }
    throw navitia::recoverable_exception("impossible to rebuild path");
}

Journey make_bound_journey(DateTime beg,
                           const navitia::time_duration& beg_sn_dur,
                           DateTime end,
                           const navitia::time_duration& end_sn_dur,
                           unsigned count,
                           const navitia::time_duration& transfer_duration,
                           bool clockwise) {
    Journey journey;
    journey.sections.resize(count);  // only the number of sections is part of the dominance function
    journey.sn_dur = beg_sn_dur + end_sn_dur;
    if (clockwise) {
        journey.departure_dt = beg - beg_sn_dur.total_seconds();
        journey.arrival_dt = end + end_sn_dur.total_seconds();
    } else {
        journey.departure_dt = end + end_sn_dur.total_seconds();
        journey.arrival_dt = beg - beg_sn_dur.total_seconds();
    }

    // for the rest KPI, we don't know yet the accurate values, so we'll provide the best lb possible
    journey.transfer_dur = transfer_duration;
    journey.min_waiting_dur = navitia::time_duration(boost::date_time::pos_infin);
    journey.total_waiting_dur = navitia::seconds(0);
    journey.nb_vj_extentions = 0;
    return journey;
}

struct stop_search : public std::exception {};

template <typename Visitor>
struct RaptorSolutionReader {
    using StDt = std::pair<const type::StopTime*, DateTime>;
    struct Transfer {
        DateTime end_vj;
        unsigned nb_stay_in;
        unsigned waiting_dur;
        unsigned transfer_dur;
        StDt end_st_dt;
        StDt begin_st_dt;

        explicit Transfer(const DateTime& end_vj,
                          unsigned nb_stay_in,
                          unsigned waiting_dur,
                          unsigned transfer_dur,
                          StDt end_st_dt,
                          StDt begin_st_dt)
            : end_vj(end_vj),
              nb_stay_in(nb_stay_in),
              waiting_dur(waiting_dur),
              transfer_dur(transfer_dur),
              end_st_dt(std::move(end_st_dt)),
              begin_st_dt(std::move(begin_st_dt)) {}
    };
    struct DomTr {
        // This dominance function is used to choose the different
        // transfer to go to a given journey pattern.  First, we want
        // only the best vj.  Then, for the transfers that go to the
        // best vj, we want the different tradeoff between maximizing
        // the waiting duration and minimizing the transfer duration.
        inline bool operator()(const Transfer& lhs, const Transfer& rhs) const {
            const Visitor v{};
            if (v.comp(lhs.end_vj, rhs.end_vj)) {
                return true;
            }
            if (v.comp(rhs.end_vj, lhs.end_vj)) {
                return false;
            }
            if (lhs.nb_stay_in != rhs.nb_stay_in) {
                return lhs.nb_stay_in <= rhs.nb_stay_in;
            }
            return lhs.waiting_dur >= rhs.waiting_dur && lhs.transfer_dur <= rhs.transfer_dur;
        }
    };
    // A small structure to reuse the memory. A c.get(id) give a
    // default initialized structure reusing memory from previous
    // calls. Such a call should not be done when the structure given
    // by the previous call is still in use (else, bad things
    // happen...). id must be small, as this is the index of a deque.
    template <typename T>
    struct Cache {
        T& get(const size_t level = 0) {
            static const T empty = T();
            if (level >= v.size()) {
                v.resize(level + 1);
            }
            v[level] = empty;
            return v[level];
        }

    private:
        std::deque<T> v;
    };
    using Transfers = boost::container::flat_map<JpIdx, ParetoFront<Transfer, DomTr>>;
    Cache<Transfers> transfers_cache;
    Cache<Journey> journey_cache;

    RaptorSolutionReader(const RAPTOR& r,
                         Solutions& solutions,
                         const Visitor& vis,  // 3rd pass visitor
                         const DateTime& departure_dt,
                         const routing::map_stop_point_duration& deps,
                         const routing::map_stop_point_duration& arrs,
                         const type::RTLevel rt_level,
                         const type::AccessibiliteParams& access,
                         navitia::time_duration arrival_transfer_penalty,
                         const StartingPointSndPhase& end_point)
        : raptor(r),
          v(vis),
          departure_datetime(departure_dt),
          sp_dur_deps(deps),
          sp_dur_arrs(arrs),
          rt_level(rt_level),
          accessibilite_params(access),
          arrival_transfer_penalty(std::move(arrival_transfer_penalty)),
          end_point(end_point),
          solutions(solutions) {}
    const RAPTOR& raptor;
    const Visitor& v;
    const DateTime departure_datetime;
    const routing::map_stop_point_duration& sp_dur_deps;  // departures (not clockwise dependent)
    const routing::map_stop_point_duration& sp_dur_arrs;  // arrivals (not clockwise dependent)
    const type::RTLevel rt_level;
    const type::AccessibiliteParams& accessibilite_params;
    const navitia::time_duration arrival_transfer_penalty;
    const StartingPointSndPhase& end_point;
    Solutions& solutions;  // raptor's solutions pool

    size_t nb_sol_added = 0;
    void handle_solution(const PathElt& path) {
        const Journey& j = make_journey(path, *this);
        if (!is_valid(*this, j)) {
            return;
        }
        ++nb_sol_added;
        solutions.add(j);
        if (nb_sol_added > 1000) {
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("raptor"));
            LOG4CPLUS_WARN(logger, "raptor_solution_reader: too much solutions, stopping...");
            throw stop_search();
        }
    }

    const Transfers& create_transfers(const unsigned count, const PathElt* path, const StDt& begin_st_dt) {
        Transfers& transfers = transfers_cache.get(count);
        auto cur_dt = begin_st_dt.second;
        auto base_dt = begin_st_dt.first->base_dt(cur_dt, v.clockwise());
        unsigned nb_stay_in = 0;
        const auto begin_zone = begin_st_dt.first->local_traffic_zone;
        cur_dt = try_end_pt(count, path, begin_st_dt, begin_zone, base_dt,
                            v.st_range(*begin_st_dt.first).advance_begin(1), nb_stay_in, transfers);

        // continuing in the stay in
        for (const auto* stay_in_vj = v.get_extension_vj(begin_st_dt.first->vehicle_journey); stay_in_vj != nullptr;
             stay_in_vj = v.get_extension_vj(stay_in_vj)) {
            base_dt = v.get_base_dt_extension(base_dt, stay_in_vj);
            cur_dt = try_end_pt(count, path, begin_st_dt, begin_zone, base_dt, v.stop_time_list(stay_in_vj),
                                ++nb_stay_in, transfers);
        }
        return transfers;
    }

    DateTime try_end_pt(const unsigned count,
                        const PathElt* path,
                        const StDt& begin_st_dt,
                        const uint16_t begin_zone,
                        DateTime cur_dt,
                        const typename Visitor::stop_time_range& st_range,
                        const unsigned nb_stay_in,
                        Transfers& transfers) {
        static const auto no_zone = std::numeric_limits<uint16_t>::max();
        auto base_dt = cur_dt;
        for (const auto& end_st : st_range) {
            cur_dt = end_st.section_end(base_dt, v.clockwise());

            // trying to end
            if (!end_st.valid_end(v.clockwise())) {
                continue;
            }
            if (begin_zone != no_zone && begin_zone == end_st.local_traffic_zone) {
                continue;
            }
            const SpIdx end_sp_idx = SpIdx(*end_st.stop_point);
            const auto& prev_label = raptor.labels[count - 1][end_sp_idx];
            const DateTime end_limit = prev_label.dt_transfer;
            if (v.comp(end_limit, cur_dt)) {
                continue;
            }
            if (!raptor.valid_stop_points[end_sp_idx.val]) {
                continue;
            }

            // great, we can end
            if (count == 1) {
                // we've finished, and it's a valid end as it is initialized
                const PathElt new_path(*begin_st_dt.first, begin_st_dt.second, end_st, cur_dt, path);
                handle_solution(new_path);
            } else {
                try_transfer(count - 1, end_sp_idx, StDt(&end_st, cur_dt), nb_stay_in, transfers);
            }
        }
        return cur_dt;
    }

    void try_transfer(const unsigned count,
                      const SpIdx sp_idx,
                      const StDt& end_st_dt,
                      const unsigned nb_stay_in,
                      Transfers& transfers) {
        const auto& cnx_list = v.clockwise() ? raptor.data.dataRaptor->connections.forward_connections
                                             : raptor.data.dataRaptor->connections.backward_connections;

        for (const auto& conn : cnx_list[sp_idx]) {
            const DateTime transfer_limit = raptor.labels[count][conn.sp_idx].dt_pt;
            const DateTime transfer_end = v.combine(end_st_dt.second, conn.duration);
            if (v.comp(transfer_limit, transfer_end)) {
                continue;
            }

            // transfer is OK
            try_begin_pt(count, conn.sp_idx, transfer_end, end_st_dt, nb_stay_in, transfers);
        }
    }

    DateTime get_vj_end(const StDt& begin_st_dt) const {
        auto cur_dt = begin_st_dt.second;
        auto base_dt = begin_st_dt.first->base_dt(cur_dt, v.clockwise());
        // Note: We keep a ref on the range because the advance_begin return a ref on the object
        // and some gcc version optimize out the inner range
        auto r = v.st_range(*begin_st_dt.first);
        for (const auto& end_st : r.advance_begin(1)) {
            cur_dt = end_st.section_end(base_dt, v.clockwise());
        }

        return cur_dt;
    }

    void try_begin_pt(const unsigned count,
                      const SpIdx begin_sp_idx,
                      const DateTime begin_dt,
                      const StDt& end_st_dt,
                      const unsigned nb_stay_in,
                      Transfers& transfers) {
        const unsigned transfer_t = v.clockwise() ? begin_dt - end_st_dt.second : end_st_dt.second - begin_dt;
        const DateTime begin_limit = raptor.labels[count][begin_sp_idx].dt_pt;
        for (const auto& jpp : raptor.jpps_from_sp[begin_sp_idx]) {
            // trying to begin
            // const auto begin_st_dt = raptor.next_st->next_stop_time(v.stop_event(), jpp.idx, begin_dt,
            // v.clockwise());
            const auto begin_st_dt =
                raptor.next_st->next_stop_time(v.stop_event(), jpp.idx, begin_dt, v.clockwise(), rt_level,
                                               accessibilite_params.vehicle_properties, true, begin_limit);
            if (begin_st_dt.first == nullptr) {
                continue;
            }
            if (v.comp(begin_limit, begin_st_dt.second)) {
                continue;
            }
            if (!raptor.valid_stop_points[begin_sp_idx.val]) {
                continue;
            }

            // great, we can begin
            const Transfer tr(get_vj_end(begin_st_dt), nb_stay_in,
                              v.clockwise() ? begin_st_dt.second - begin_dt : begin_dt - begin_st_dt.second, transfer_t,
                              end_st_dt, begin_st_dt);
            transfers[jpp.jp_idx].add(tr);
        }
    }

    void step(const unsigned count, const PathElt* path, const StDt& begin_st_dt) {
        const auto& transfers = create_transfers(count, path, begin_st_dt);
        for (const auto& pareto : transfers) {
            for (const auto& tr : pareto.second) {
                const PathElt new_path(*begin_st_dt.first, begin_st_dt.second, *tr.end_st_dt.first, tr.end_st_dt.second,
                                       path);
                step(count - 1, &new_path, tr.begin_st_dt);
            }
        }
    }

    void begin_pt(const unsigned count, const SpIdx begin_sp_idx, const DateTime begin_dt) {
        const DateTime begin_limit = raptor.labels[count][begin_sp_idx].dt_pt;
        for (const auto& jpp : raptor.jpps_from_sp[begin_sp_idx]) {
            // trying to begin
            // const auto begin_st_dt = raptor.next_st->next_stop_time(v.stop_event(), jpp.idx, begin_dt,
            // v.clockwise());
            const auto begin_st_dt = raptor.next_st->next_stop_time(v.stop_event(), jpp.idx, begin_dt, v.clockwise(),
                                                                    rt_level, accessibilite_params.vehicle_properties);
            if (begin_st_dt.first == nullptr) {
                continue;
            }
            if (v.comp(begin_limit, begin_st_dt.second)) {
                continue;
            }

            // great, we can begin
            step(count, nullptr, begin_st_dt);
        }
    }
};

template <typename Visitor>
void read_solutions(const RAPTOR& raptor,
                    Solutions& solutions,
                    const Visitor& v,
                    const DateTime& departure_datetime,
                    const routing::map_stop_point_duration& deps,
                    const routing::map_stop_point_duration& arrs,
                    const type::RTLevel rt_level,
                    const type::AccessibiliteParams& accessibilite_params,
                    const navitia::time_duration& arrival_transfer_penalty,
                    const StartingPointSndPhase& end_point) {
    auto reader = RaptorSolutionReader<Visitor>(raptor, solutions, v, departure_datetime, deps, arrs, rt_level,
                                                accessibilite_params, arrival_transfer_penalty, end_point);
    const auto end_point_street_network_duration = (v.clockwise() ? arrs : deps).at(end_point.sp_idx);
    for (unsigned count = 1; count <= raptor.count; ++count) {
        const auto& working_labels = raptor.labels[count];
        const auto& first_endpoint_label = raptor.labels[0][end_point.sp_idx];

        for (const auto& a : v.clockwise() ? deps : arrs) {
            SpIdx sp_idx = a.first;
            const auto& working_label = working_labels[sp_idx];
            navitia::type::StopPoint* stop_point = raptor.data.pt_data->stop_points[sp_idx.val];

            if (!is_dt_initialized(working_label.dt_pt)) {
                continue;
            }
            if (!raptor.get_sp(a.first)->accessible(accessibilite_params.properties)) {
                continue;
            }
            reader.nb_sol_added = 0;
            // we check that it's worth to explore this possible journey
            auto transfer_duration = working_label.walking_duration_pt - end_point_street_network_duration;
            auto j = make_bound_journey(working_label.dt_pt, a.second, first_endpoint_label.dt_transfer,
                                        end_point_street_network_duration, count, navitia::seconds(transfer_duration),
                                        v.clockwise());
            LOG4CPLUS_DEBUG(raptor.raptor_logger, "Journey from " << stop_point->uri << " count : " << count
                                                                  << std::endl
                                                                  << j);

            if (reader.solutions.contains_better_than(j)) {
                LOG4CPLUS_DEBUG(raptor.raptor_logger, "Journey discarded");

                continue;
            }
            try {
                LOG4CPLUS_DEBUG(raptor.raptor_logger, "try to build journey ");
                reader.begin_pt(count, a.first, working_label.dt_pt);
            } catch (stop_search&) {
            }
        }
    }
}

}  // anonymous namespace

std::ostream& operator<<(std::ostream& os, const Journey& j) {
    os << "(["
       << "departure_dt : " << navitia::str(j.departure_dt) << ", "
       << "arrival_dt : " << navitia::str(j.arrival_dt) << ", "
       << "min_waiting_dur : " << j.min_waiting_dur << ", "
       << "total_waiting_dur : " << j.total_waiting_dur << ", "
       << "transfer_dur : " << j.transfer_dur << "],\n ["
       << "nb sections : " << j.sections.size() << ", "
       << "nb extensions : " << unsigned(j.nb_vj_extentions) << "], "
       << "fallback duration : " << j.sn_dur << ")" << std::endl;
    for (const auto& s : j.sections) {
        if (s.get_in_st) {
            os << "(" << s.get_in_st->vehicle_journey->route->line->uri << ": " << s.get_in_st->stop_point->uri;
        } else {
            os << "(NULL";
        }
        os << "@" << navitia::str(s.get_in_dt) << ", ";
        if (s.get_out_st) {
            os << s.get_out_st->stop_point->uri;
        } else {
            os << "NULL";
        }
        os << "@" << navitia::str(s.get_out_dt) << ")\n";
    }
    return os;
}

void read_solutions(const RAPTOR& raptor,
                    Solutions& solutions,
                    const bool clockwise,
                    const DateTime& departure_datetime,
                    const routing::map_stop_point_duration& deps,
                    const routing::map_stop_point_duration& arrs,
                    const type::RTLevel rt_level,
                    const type::AccessibiliteParams& accessibilite_params,
                    const navitia::time_duration& arrival_transfer_penalty,
                    const StartingPointSndPhase& end_point) {
    if (clockwise) {
        return read_solutions(raptor, solutions, raptor_reverse_visitor(), departure_datetime, deps, arrs, rt_level,
                              accessibilite_params, arrival_transfer_penalty, end_point);
    }
    return read_solutions(raptor, solutions, raptor_visitor(), departure_datetime, deps, arrs, rt_level,
                          accessibilite_params, arrival_transfer_penalty, end_point);
}

Path make_path(const Journey& journey, const type::Data& data) {
    Path path;
    const auto posix = [&data](DateTime dt) { return to_posix_time(dt, data); };

    path.nb_changes = journey.sections.size() - 1;

    const Journey::Section* last_section = nullptr;
    for (const auto& section : journey.sections) {
        const auto dep_stop_point = section.get_in_st->stop_point;
        if (!path.items.empty() && last_section != nullptr) {
            // we add a connexion
            auto waiting_section_start = posix(last_section->get_out_dt);
            const auto previous_stop = last_section->get_out_st->stop_point;

            const auto* conn = data.pt_data->get_stop_point_connection(*previous_stop, *dep_stop_point);
            assert(conn);

            const auto end_of_transfer = posix(last_section->get_out_dt + conn->display_duration);

            // we add a transfer section only if we transfer on different stop point, else there is only a waiting
            // section
            const bool walking_transfer = dep_stop_point != previous_stop;
            if (walking_transfer) {
                path.items.emplace_back(ItemType::walking, posix(last_section->get_out_dt), end_of_transfer);
                auto& s = path.items.back();
                s.stop_points.push_back(previous_stop);
                s.stop_points.push_back(dep_stop_point);

                s.connection = conn;
                waiting_section_start = end_of_transfer;  // we update the start of the waiting section
            }

            const auto transfer_time = section.get_in_dt - last_section->get_out_dt;
            // if the transfer is bigger than the actual connection, we add a waiting section
            if (conn->display_duration < int(transfer_time) || !walking_transfer) {
                path.items.emplace_back(ItemType::waiting, waiting_section_start, posix(section.get_in_dt));
                path.items.back().stop_points.push_back(dep_stop_point);
            }
        }

        // we then need to create one section by service extension
        const VehicleSection* last_vj_section = nullptr;
        const auto& sections = get_vjs(section);

        // Add boarding section if we have boarding_time != departure_time on the first stop_time of the first vj
        const auto& boarding_st = sections.front().stop_times_and_dt.front().st;
        if (boarding_st.boarding_time != boarding_st.departure_time) {
            path.items.emplace_back(ItemType::boarding, posix(section.get_in_dt),
                                    posix(section.get_in_dt + boarding_st.get_boarding_duration()));
            auto& boarding_section = path.items.back();
            // The departure and arrival stop_point of the boarding are the first stop_point of the first section
            boarding_section.stop_points.push_back(boarding_st.stop_point);
            boarding_section.stop_points.push_back(boarding_st.stop_point);
            boarding_section.arrivals.push_back(boarding_section.arrival);
            boarding_section.departures.push_back(boarding_section.departure);
        }

        for (const auto& vj_section : sections) {
            if (last_vj_section) {
                // add a stay in
                path.items.emplace_back(ItemType::stay_in, posix(section.get_in_dt), posix(section.get_out_dt));
                auto& stay_in_section = path.items.back();
                const auto& last_st = last_vj_section->stop_times_and_dt.back();
                stay_in_section.stop_points.push_back(last_st.st.stop_point);
                const auto* first_stop_point = vj_section.stop_times_and_dt.front().st.stop_point;
                stay_in_section.stop_points.push_back(first_stop_point);
                stay_in_section.departure = posix(last_st.departure);
                stay_in_section.arrival = posix(vj_section.stop_times_and_dt.front().departure);
            }

            // add the pt section
            path.items.emplace_back(ItemType::public_transport);
            auto& item = path.items.back();
            for (const auto& st_dt : vj_section.stop_times_and_dt) {
                // We don't want to show estimated intermediate stops
                // since they are not relevant
                if (st_dt.st.date_time_estimated() && &st_dt.st != &vj_section.stop_times_and_dt.front().st
                    && &st_dt.st != &vj_section.stop_times_and_dt.back().st) {
                    continue;
                }

                item.stop_times.push_back(&st_dt.st);
                item.stop_points.push_back(st_dt.st.stop_point);
                item.arrivals.push_back(posix(st_dt.arrival));
                item.departures.push_back(posix(st_dt.departure));
            }
            item.departure = posix(vj_section.stop_times_and_dt.front().departure);
            item.arrival = posix(vj_section.stop_times_and_dt.back().arrival);
            last_vj_section = &vj_section;
        }

        // Add alighting section if we have alighting_time != arrival_time on the last stop_time of the last vj
        const auto& alighting_st = sections.back().stop_times_and_dt.back().st;
        if (alighting_st.alighting_time != alighting_st.arrival_time) {
            path.items.emplace_back(ItemType::alighting,
                                    posix(section.get_out_dt - alighting_st.get_alighting_duration()),
                                    posix(section.get_out_dt));
            auto& alighting_section = path.items.back();
            // The departure and arrival stop_point of the alighting are the last stop_point of the last section
            alighting_section.stop_points.push_back(alighting_st.stop_point);
            alighting_section.stop_points.push_back(alighting_st.stop_point);
            alighting_section.arrivals.push_back(alighting_section.arrival);
            alighting_section.departures.push_back(alighting_section.departure);
        }

        last_section = &section;
    }

    path.duration =
        navitia::time_duration::from_boost_duration(path.items.back().arrival - path.items.front().departure);

    path.sn_dur = journey.sn_dur;
    path.transfer_dur = journey.transfer_dur;
    path.min_waiting_dur = journey.min_waiting_dur;
    path.nb_vj_extentions = journey.nb_vj_extentions;
    path.nb_sections = journey.sections.size();

    return path;
}

}  // namespace routing
}  // namespace navitia
