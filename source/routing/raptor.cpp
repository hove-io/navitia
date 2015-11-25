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

#include "raptor_solution_reader.h"
#include "raptor.h"
#include "raptor_visitors.h"
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/fill.hpp>
#include <chrono>

namespace bt = boost::posix_time;

namespace navitia { namespace routing {

static DateTime limit_bound(const bool clockwise, const DateTime departure_datetime, const DateTime bound) {
    return clockwise ?
        std::min(departure_datetime + DateTimeUtils::SECONDS_PER_DAY, bound) :
        std::max(departure_datetime > DateTimeUtils::SECONDS_PER_DAY ?
                 departure_datetime - DateTimeUtils::SECONDS_PER_DAY :
                 0,
                 bound);
}

/*
 * Check if the given vj is valid for the given datetime,
 * If it is for every stoptime of the vj,
 * If the journey_pattern_point associated to it is improved by this stop time
 * we mark it.
 * If the given vj also has an extension we apply it.
 */
template<typename Visitor>
bool RAPTOR::apply_vj_extension(const Visitor& v,
                                const nt::RTLevel rt_level,
                                const RoutingState& state) {
    auto& working_labels = labels[count];
    auto workingDt = state.workingDate;
    auto vj = state.vj;
    bool result = false;
    while(vj) {
        const auto& stop_time_list = v.stop_time_list(vj);
        const auto& st_begin = stop_time_list.front();
        workingDt = st_begin.section_end(workingDt, v.clockwise());
        // If the vj is not valid for the first stop it won't be valid at all
        if (!st_begin.is_valid_day(DateTimeUtils::date(workingDt), !v.clockwise(), rt_level)) {
            return result;
        }
        for (const type::StopTime& st: stop_time_list) {
            workingDt = st.section_end(workingDt, v.clockwise());
            if (!st.valid_end(v.clockwise())) {
                continue;
            }
            if (state.l_zone != std::numeric_limits<uint16_t>::max() &&
               state.l_zone == st.local_traffic_zone) {
                continue;
            }
            auto sp = st.stop_point;
            const auto sp_idx = SpIdx(*sp);

            if (! v.comp(workingDt, best_labels_pts[sp_idx])) { continue; }

            working_labels.mut_dt_pt(sp_idx) = workingDt;
            best_labels_pts[sp_idx] = workingDt;
            result = true;
        }
        vj = v.get_extension_vj(vj);
    }
    return result;
}


template<typename Visitor>
bool RAPTOR::foot_path(const Visitor& v) {
    bool result = false;
    auto& working_labels = labels[count];
    const auto& cnx_list = v.clockwise() ?
                data.dataRaptor->connections.forward_connections :
                data.dataRaptor->connections.backward_connections;

    for (const auto sp_cnx: cnx_list) {
        // for all stop point, we check if we can improve the stop points they are in connection with
        const auto& sp_idx = sp_cnx.first;

        if (! working_labels.pt_is_initialized(sp_idx)) { continue; }

        const DateTime previous = working_labels.dt_pt(sp_idx);

        for (const auto& conn: sp_cnx.second) {
            const SpIdx destination_sp_idx = conn.sp_idx;
            const DateTime next = v.combine(previous, conn.duration);

            if (! v.comp(next, best_labels_transfers[destination_sp_idx])) { continue; }

            //if we can improve the best label, we mark it
            working_labels.mut_dt_transfer(destination_sp_idx) = next;
            best_labels_transfers[destination_sp_idx] = next;
            result = true;
        }
    }

    for (const auto sp_jpps: jpps_from_sp) {
        if (! working_labels.transfer_is_initialized(sp_jpps.first)) { continue; }

        // we mark the jpp order
        for (const auto& jpp: sp_jpps.second) {
            if (v.comp(jpp.order, Q[jpp.jp_idx])) {
                Q[jpp.jp_idx] = jpp.order;
            }
        }
    }

    return result;
}


void RAPTOR::clear(const bool clockwise, const DateTime bound) {
    const int queue_value = clockwise ?  std::numeric_limits<int>::max() : -1;
    Q.assign(data.dataRaptor->jp_container.get_jps_values(), queue_value);
    if (labels.empty()) {
        labels.resize(5);
    }
    const Labels& clean_labels =
        clockwise ? data.dataRaptor->labels_const : data.dataRaptor->labels_const_reverse;
    for(auto& lbl_list : labels) {
        lbl_list.clear(clean_labels);
    }

    boost::fill(best_labels_pts.values(), bound);
    boost::fill(best_labels_transfers.values(), bound);
}

void RAPTOR::init(const map_stop_point_duration& dep,
                  const DateTime bound,
                  const bool clockwise,
                  const type::Properties& properties) {
    for (const auto& sp_dt: dep) {
        if (! get_sp(sp_dt.first)->accessible(properties)) { continue; }

        const DateTime sn_dur = sp_dt.second.total_seconds();
        const DateTime begin_dt = bound + (clockwise ? sn_dur : -sn_dur);
        labels[0].mut_dt_transfer(sp_dt.first) = begin_dt;
        best_labels_transfers[sp_dt.first] = begin_dt;
        for (const auto jpp: jpps_from_sp[sp_dt.first]) {
            if (clockwise && Q[jpp.jp_idx] > jpp.order) {
                Q[jpp.jp_idx] = jpp.order;
            } else if (! clockwise && Q[jpp.jp_idx] < jpp.order) {
                Q[jpp.jp_idx] = jpp.order;
            }
        }
    }
}

void RAPTOR::first_raptor_loop(const map_stop_point_duration& dep,
                               const DateTime& departure_datetime,
                               const nt::RTLevel rt_level,
                               const DateTime& b,
                               const uint32_t max_transfers,
                               const type::AccessibiliteParams& accessibilite_params,
                               const std::vector<std::string>& forbidden_uri,
                               bool clockwise) {

    const DateTime bound = limit_bound(clockwise, departure_datetime, b);
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden_uri,
                         rt_level);
    next_st.load(data,
                 clockwise ? departure_datetime : bound,
                 clockwise ? bound : departure_datetime,
                 rt_level,
                 accessibilite_params);

    clear(clockwise, bound);
    init(dep, departure_datetime, clockwise, accessibilite_params.properties);

    boucleRAPTOR(clockwise, rt_level, max_transfers);
}

namespace {
struct Dom {
    Dom(bool c): clockwise(c) {}
    bool clockwise;
    typedef std::pair<size_t, StartingPointSndPhase> Arg;
    inline bool operator()(const Arg& lhs, const Arg& rhs) const {
        return lhs.second.count <= rhs.second.count
            && (clockwise ? lhs.second.end_dt <= rhs.second.end_dt
                          : lhs.second.end_dt >= rhs.second.end_dt)
            && lhs.second.fallback_dur <= rhs.second.fallback_dur;
    }
};
struct CompSndPhase {
    CompSndPhase(bool c): clockwise(c) {}
    bool clockwise;
    typedef StartingPointSndPhase Arg;
    inline bool operator()(const Arg& lhs, const Arg& rhs) const {
        if (lhs.has_priority != rhs.has_priority) {
            return lhs.has_priority;
        }
        if (lhs.count != rhs.count) {
            return lhs.count < rhs.count;
        }
        if (lhs.end_dt != rhs.end_dt) {
            return (clockwise ? lhs.end_dt < rhs.end_dt : lhs.end_dt > rhs.end_dt);
        }
        if (lhs.fallback_dur != rhs.fallback_dur) {
            return lhs.fallback_dur < rhs.fallback_dur;
        }
        return lhs.sp_idx < rhs.sp_idx; // just to avoid randomness
    }
};

std::vector<StartingPointSndPhase>
make_starting_points_snd_phase(const RAPTOR& raptor,
                               const routing::map_stop_point_duration& arrs,
                               const type::AccessibiliteParams& accessibilite_params,
                               const bool clockwise)
{
    std::vector<StartingPointSndPhase> res;
    auto overfilter = ParetoFront<std::pair<size_t, StartingPointSndPhase>, Dom>(Dom(clockwise));

    for (unsigned count = 1; count <= raptor.count; ++count) {
        const auto& working_labels = raptor.labels[count];
        for (const auto& a: arrs) {
            if (! working_labels.pt_is_initialized(a.first)) { continue; }
            if (! raptor.get_sp(a.first)->accessible(accessibilite_params.properties)) { continue; }

            const unsigned walking_t = a.second.total_seconds();
            StartingPointSndPhase starting_point = {
                a.first,
                count,
                (clockwise ? working_labels.dt_pt(a.first) + walking_t
                           : working_labels.dt_pt(a.first) - walking_t),
                walking_t,
                false
            };
            overfilter.add({res.size(), starting_point});
            res.push_back(std::move(starting_point));
        }
    }

    for (const auto& pair : overfilter) {
        res[pair.first].has_priority = true;
    }

    std::sort(res.begin(), res.end(), CompSndPhase(clockwise)); // most interesting solutions first

    return res;
}

// creation of a fake journey from informations known after first pass
// this journey aims at being the best we can hope from this StartingPointSndPhase
Journey convert_to_bound(const StartingPointSndPhase& sp,
                         uint32_t lower_bound_fb,
                         uint32_t lower_bound_conn,
                         const navitia::time_duration& transfer_penalty,
                         bool clockwise) {
    Journey journey;
    journey.sections.resize(sp.count); // only the number of sections is part of the dominance function
    journey.sn_dur = navitia::time_duration(0, 0, sp.fallback_dur + lower_bound_fb, 0);
    uint32_t nb_conn = (sp.count >= 1 ? sp.count - 1 : 0);
    if (clockwise) {
        journey.arrival_dt = sp.end_dt;
        journey.departure_dt = sp.end_dt - journey.sn_dur.seconds() - nb_conn * lower_bound_conn;
    } else {
        journey.arrival_dt = sp.end_dt + journey.sn_dur.seconds() + nb_conn * lower_bound_conn;
        journey.departure_dt = sp.end_dt;
    }

    journey.transfer_dur = transfer_penalty * sp.count + navitia::seconds(nb_conn * lower_bound_conn);
    // provide best values on unknown criteria
    journey.min_waiting_dur = navitia::time_duration(boost::date_time::pos_infin);
    journey.nb_vj_extentions = 0;
    return journey;
}

// To be used for debug purpose (see JourneyParetoFrontVisitor commented use in RAPTOR::compute_all)
//
//struct JourneyParetoFrontVisitor {
//    JourneyParetoFrontVisitor() = default;
//    JourneyParetoFrontVisitor(const JourneyParetoFrontVisitor&) = default;
//    JourneyParetoFrontVisitor& operator=(const JourneyParetoFrontVisitor&) = default;
//
//    void is_dominated_by(const Journey& to_insert, const Journey& /*front_cur*/) {
//        LOG4CPLUS_DEBUG(logger, "[  ]JourneyParetoFront " << to_insert);
//        ++nb_is_dominated_by;
//    }
//    void dominates(const Journey& /*to_insert*/, const Journey& front_cur) {
//        LOG4CPLUS_DEBUG(logger, "[--]JourneyParetoFront " << front_cur);
//        ++nb_dominates;
//    }
//    void inserted(const Journey& to_insert) {
//        LOG4CPLUS_DEBUG(logger, "[++]JourneyParetoFront " << to_insert);
//        ++nb_inserted;
//    }
//
//    ~JourneyParetoFrontVisitor() {
//        LOG4CPLUS_DEBUG(logger, "JourneyParetoFront summary: nb_is_dominated_by = " << nb_is_dominated_by
//                << ", nb_dominates = " << nb_dominates
//                << ", nb_inserted = " << nb_inserted);
//
//    }
//
//private:
//    size_t nb_is_dominated_by = 0;
//    size_t nb_dominates = 0;
//    size_t nb_inserted = 0;
//    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
//};
}

// copy and do the off by one for strict comparison for the second pass.
static IdxMap<type::StopPoint, DateTime>
snd_pass_best_labels(const bool clockwise, IdxMap<type::StopPoint, DateTime> best_labels) {
    for (auto& dt: best_labels.values()) {
        if (is_dt_initialized(dt)) { dt += clockwise ? -1 : 1; }
    }
    return std::move(best_labels);
}
// Set the departure bounds on best_labels_pts for the second pass.
static void init_best_pts_snd_pass(const routing::map_stop_point_duration& departures,
                                   const DateTime& departure_datetime,
                                   const bool clockwise,
                                   IdxMap<type::StopPoint, DateTime>& best_labels) {
    for (const auto& d: departures) {
        if (clockwise) {
            best_labels[d.first] = std::min(best_labels[d.first],
                                            departure_datetime + d.second.total_seconds() - 1);
        } else {
            best_labels[d.first] = std::max(best_labels[d.first],
                                            departure_datetime - d.second.total_seconds() + 1);
        }
    }
}

std::vector<Path>
RAPTOR::compute_all(const map_stop_point_duration& departures,
                    const map_stop_point_duration& destinations,
                    const DateTime& departure_datetime,
                    const nt::RTLevel rt_level,
                    const navitia::time_duration& transfer_penalty,
                    const DateTime& bound,
                    const uint32_t max_transfers,
                    const type::AccessibiliteParams& accessibilite_params,
                    const std::vector<std::string>& forbidden_uri,
                    bool clockwise,
                    const boost::optional<navitia::time_duration>& direct_path_dur,
                    const size_t max_extra_second_pass) {
    auto start_raptor = std::chrono::system_clock::now();

    auto solutions = ParetoFront<Journey, Dominates/*, JourneyParetoFrontVisitor*/>(Dominates(clockwise));

    if (direct_path_dur) {
        Journey j;
        j.sn_dur = *direct_path_dur;
        if (clockwise) {
            j.departure_dt = departure_datetime;
            j.arrival_dt = j.departure_dt + j.sn_dur;
        } else {
            j.arrival_dt = departure_datetime;
            j.departure_dt = j.arrival_dt - j.sn_dur;
        }
        solutions.add(j);
    }

    const auto& calc_dep = clockwise ? departures : destinations;
    const auto& calc_dest = clockwise ? destinations : departures;

    first_raptor_loop(calc_dep, departure_datetime, rt_level,
                      bound, max_transfers, accessibilite_params, forbidden_uri, clockwise);

    auto end_first_pass = std::chrono::system_clock::now();

    // Now, we do the second pass.  In case of clockwise (resp
    // anticlockwise) search, the goal of the second pass is to find
    // the earliest (resp. tardiest) departure (resp arrival)
    // datetime.  For each count and arrival (resp departure), we
    // launch a backward raptor.
    //
    // As we do a backward raptor, the bound computed during the first
    // pass can be used in the second pass.  The arrival at a stop
    // point (as in best_labels_transfers) is a bound to the get in
    // (as in best_labels_pt) in the second pass.  Then, we can reuse
    // these bounds, modulo an off by one because of strict comparison
    // on best_labels.
    auto starting_points =
        make_starting_points_snd_phase(*this, calc_dest, accessibilite_params, clockwise);
    swap(labels, first_pass_labels);
    auto best_labels_pts_for_snd_pass = snd_pass_best_labels(clockwise, best_labels_transfers);
    init_best_pts_snd_pass(calc_dep, departure_datetime, clockwise, best_labels_pts_for_snd_pass);
    auto best_labels_transfers_for_snd_pass = snd_pass_best_labels(clockwise, best_labels_pts);

    unsigned lower_bound_fb = std::numeric_limits<unsigned>::max();
    for (const auto& pair_sp_dt : calc_dep) {
        lower_bound_fb = std::min(lower_bound_fb, unsigned(pair_sp_dt.second.seconds()));
    }

    size_t nb_snd_pass = 0, nb_useless= 0, last_usefull_2nd_pass = 0, supplementary_2nd_pass = 0;
    for (const auto& start: starting_points) {
        Journey fake_journey = convert_to_bound(start,
                                                lower_bound_fb,
                                                data.dataRaptor->min_connection_time,
                                                transfer_penalty,
                                                clockwise);
        if (solutions.contains_better_than(fake_journey)) {
            continue;
        }

        if (!start.has_priority) {
            ++supplementary_2nd_pass;
        }
        if (supplementary_2nd_pass > max_extra_second_pass) {
            break;
        }

        const auto& working_labels = first_pass_labels[start.count];

        clear(!clockwise, departure_datetime + (clockwise ? -1 : 1));
        map_stop_point_duration init_map;
        init_map[start.sp_idx] = 0_s;
        best_labels_pts = best_labels_pts_for_snd_pass;
        best_labels_transfers = best_labels_transfers_for_snd_pass;
        init(init_map, working_labels.dt_pt(start.sp_idx),
             !clockwise, accessibilite_params.properties);
        boucleRAPTOR(!clockwise, rt_level, max_transfers);
        read_solutions(*this,
                       solutions,
                       !clockwise,
                       departure_datetime,
                       departures,
                       destinations,
                       rt_level,
                       accessibilite_params,
                       transfer_penalty,
                       start);

        ++nb_snd_pass;
    }
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "[2nd pass] lower bound fallback duration = " << lower_bound_fb
            << " s, lower bound connection duration = " << data.dataRaptor->min_connection_time << " s");
    LOG4CPLUS_DEBUG(logger, "[2nd pass] number of 2nd pass = " << nb_snd_pass << " / " << starting_points.size()
            << " (nb useless = " << nb_useless << ", last usefull try = " << last_usefull_2nd_pass << ")");
    auto end_raptor = std::chrono::system_clock::now();
    LOG4CPLUS_DEBUG(logger, "[2nd pass] Run times: 1st pass = "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_first_pass - start_raptor).count()
            << ", 2nd pass = "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_raptor - end_first_pass).count());

    std::vector<Path> result;
    for (const auto& s: solutions) {
        if (s.sections.empty()) { continue; }
        result.push_back(make_path(s, data));
    }
    return result;
}

std::vector<std::pair<type::EntryPoint, std::vector<Path>>>
RAPTOR::compute_nm_all(const std::vector<std::pair<type::EntryPoint, map_stop_point_duration>>& departures,
                       const std::vector<std::pair<type::EntryPoint, map_stop_point_duration>>& arrivals,
                       const DateTime& departure_datetime,
                       const nt::RTLevel rt_level,
                       const DateTime& b,
                       const uint32_t max_transfers,
                       const type::AccessibiliteParams& accessibilite_params,
                       const std::vector<std::string>& forbidden_uri,
                       bool clockwise) {
    std::vector<std::pair<type::EntryPoint, std::vector<Path>>> result;
    const DateTime bound = limit_bound(clockwise, departure_datetime, b);
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden_uri,
                         rt_level);
    next_st.load(data,
                 clockwise ? departure_datetime : bound,
                 clockwise ? bound : departure_datetime,
                 rt_level,
                 accessibilite_params);

    map_stop_point_duration calc_dep_map;
    for (const auto& n_point : (clockwise ? departures : arrivals)) {
        calc_dep_map.insert(n_point.second.begin(), n_point.second.end());
    }

    clear(clockwise, bound);
    init(calc_dep_map, departure_datetime, clockwise, accessibilite_params.properties);

    boucleRAPTOR(clockwise, rt_level, max_transfers);

    for(const auto& m_point : (clockwise ? arrivals : departures)) {
        const type::EntryPoint& m_entry_point = m_point.first;

        std::vector<Path> paths;

        // just returns destination stop points
        for (auto& m_stop_point : m_point.second) {
            Path path;
            path.nb_changes = 0;
            PathItem path_item;
            const navitia::type::StopPoint* sp = get_sp(m_stop_point.first);
            path_item.stop_points.push_back(sp);
            path_item.type = ItemType::public_transport;
            path.items.push_back(path_item);
            paths.push_back(path);
        }

        result.push_back(std::make_pair(m_entry_point, paths));
    }

    return result;
}

void
RAPTOR::isochrone(const map_stop_point_duration& departures,
                  const DateTime& departure_datetime,
                  const DateTime& b,
                  uint32_t max_transfers,
                  const type::AccessibiliteParams& accessibilite_params,
                  const std::vector<std::string>& forbidden,
                  bool clockwise,
                  const nt::RTLevel rt_level) {
    const DateTime bound = limit_bound(clockwise, departure_datetime, b);
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden,
                         rt_level);
    next_st.load(data,
                 clockwise ? departure_datetime : bound,
                 clockwise ? bound : departure_datetime,
                 rt_level,
                 accessibilite_params);

    clear(clockwise, bound);
    init(departures, departure_datetime, clockwise, accessibilite_params.properties);

    boucleRAPTOR(clockwise, rt_level, max_transfers);
}


void RAPTOR::set_valid_jp_and_jpp(
    uint32_t date,
    const type::AccessibiliteParams& accessibilite_params,
    const std::vector<std::string>& forbidden,
    const nt::RTLevel rt_level)
{
    const auto& jp_container = data.dataRaptor->jp_container;
    valid_journey_patterns = data.dataRaptor->jp_validity_patterns[rt_level][date];
    boost::dynamic_bitset<> valid_journey_pattern_points(jp_container.nb_jpps());
    valid_journey_pattern_points.set();


    valid_stop_points.set();

    // We will forbiden every object designated in forbidden
    for (const auto& uri : forbidden) {
        const auto it_line = data.pt_data->lines_map.find(uri);
        if (it_line != data.pt_data->lines_map.end()) {
            for (const auto route : it_line->second->route_list) {
                for (const auto& jp_idx: jp_container.get_jps_from_route()[RouteIdx(*route)]) {
                    valid_journey_patterns.set(jp_idx.val, false);
                }
            }
            continue;
        }
        const auto it_route = data.pt_data->routes_map.find(uri);
        if (it_route != data.pt_data->routes_map.end()) {
            for (const auto& jp_idx: jp_container.get_jps_from_route()[RouteIdx(*it_route->second)]) {
                valid_journey_patterns.set(jp_idx.val, false);
            }
            continue;
        }
        const auto it_commercial_mode = data.pt_data->commercial_modes_map.find(uri);
        if (it_commercial_mode != data.pt_data->commercial_modes_map.end()) {
            for (const auto line : it_commercial_mode->second->line_list) {
                for (auto route : line->route_list) {
                    for (const auto& jp_idx: jp_container.get_jps_from_route()[RouteIdx(*route)]) {
                        valid_journey_patterns.set(jp_idx.val, false);
                    }
                }
            }
            continue;
        }
        const auto it_physical_mode = data.pt_data->physical_modes_map.find(uri);
        if (it_physical_mode != data.pt_data->physical_modes_map.end()) {
            const auto phy_mode_idx = PhyModeIdx(*it_physical_mode->second);
            for (const auto& jp_idx: jp_container.get_jps_from_phy_mode()[phy_mode_idx]) {
                valid_journey_patterns.set(jp_idx.val, false);
            }
            continue;
        }
        const auto it_network = data.pt_data->networks_map.find(uri);
        if (it_network != data.pt_data->networks_map.end()) {
            for (const auto line : it_network->second->line_list) {
                for (const auto route : line->route_list) {
                    for (const auto& jp_idx: jp_container.get_jps_from_route()[RouteIdx(*route)]) {
                        valid_journey_patterns.set(jp_idx.val, false);
                    }
                }
            }
            continue;
        }
        const auto it_sp = data.pt_data->stop_points_map.find(uri);
        if (it_sp !=  data.pt_data->stop_points_map.end()) {
            valid_stop_points.set(it_sp->second->idx, false);
            for (const auto& jpp: data.dataRaptor->jpps_from_sp[SpIdx(*it_sp->second)]) {
                valid_journey_pattern_points.set(jpp.idx.val, false);
            }
            continue;
        }
        const auto it_sa = data.pt_data->stop_areas_map.find(uri);
        if (it_sa !=  data.pt_data->stop_areas_map.end()) {
            for (const auto sp : it_sa->second->stop_point_list) {
                valid_stop_points.set(sp->idx, false);
                for (const auto& jpp: data.dataRaptor->jpps_from_sp[SpIdx(*sp)]) {
                    valid_journey_pattern_points.set(jpp.idx.val, false);
                }
            }
            continue;
        }
    }

    // filter accessibility
    if (accessibilite_params.properties.any()) {
        for (const auto* sp: data.pt_data->stop_points) {
            if (sp->accessible(accessibilite_params.properties)) { continue; }
            valid_stop_points.set(sp->idx, false);
            for (const auto& jpp: data.dataRaptor->jpps_from_sp[SpIdx(*sp)]) {
                valid_journey_pattern_points.set(jpp.idx.val, false);
            }
        }
    }

    // propagate the invalid jp in their jpp
    for (JpIdx jp_idx = JpIdx(0); jp_idx.val < valid_journey_patterns.size(); ++jp_idx.val) {
        if (valid_journey_patterns[jp_idx.val]) { continue; }
        const auto& jp = jp_container.get(jp_idx);
        for (const auto& jpp_idx: jp.jpps) {
            valid_journey_pattern_points.set(jpp_idx.val, false);
        }
    }

    // We get our own copy of jpps_from_sp to filter every invalid
    // jpps.  Thanks to that, we don't need to check
    // valid_journey_pattern[_point]s as we iterate only on the
    // feasible ones.
    jpps_from_sp = data.dataRaptor->jpps_from_sp;
    jpps_from_sp.filter_jpps(valid_journey_pattern_points);
}

template<typename Visitor>
void RAPTOR::raptor_loop(Visitor visitor,
                         const nt::RTLevel rt_level,
                         uint32_t max_transfers) {
    bool continue_algorithm = true;
    count = 0; //< Count iteration of raptor algorithm

    while(continue_algorithm && count <= max_transfers) {
        ++count;
        continue_algorithm = false;
        if(count == labels.size()) {
            if(visitor.clockwise()) {
                this->labels.push_back(this->data.dataRaptor->labels_const);
            } else {
                this->labels.push_back(this->data.dataRaptor->labels_const_reverse);
            }
        }
        const auto& prec_labels = labels[count -1];
        auto& working_labels = labels[this->count];
        /*
         * We need to store it so we can apply stay_in after applying normal vjs
         * We want to do it, to favoritize normal vj against stay_in vjs
         */
        std::vector<RoutingState> states_stay_in;
        for (auto q_elt: Q) {
            const JpIdx jp_idx = q_elt.first;
            if(q_elt.second != visitor.init_queue_item()) {
                bool is_onboard = false;
                DateTime workingDt = visitor.worst_datetime();
                typename Visitor::stop_time_iterator it_st;
                uint16_t l_zone = std::numeric_limits<uint16_t>::max();
                const auto& jpps_to_explore = visitor.jpps_from_order(data.dataRaptor->jpps_from_jp,
                                                                      jp_idx,
                                                                      q_elt.second);

                for (const auto& jpp: jpps_to_explore) {
                    if (is_onboard) {
                        ++it_st;
                        // We update workingDt with the new arrival time
                        // We need at each journey pattern point when we have a st
                        // If we don't it might cause problem with overmidnight vj
                        const type::StopTime& st = *it_st;
                        workingDt = st.section_end(workingDt, visitor.clockwise());

                        // We check if there are no drop_off_only and if the local_zone is okay
                        if (st.valid_end(visitor.clockwise())
                            && (l_zone == std::numeric_limits<uint16_t>::max() ||
                                l_zone != st.local_traffic_zone)
                            && visitor.comp(workingDt, best_labels_pts[jpp.sp_idx])
                            && valid_stop_points[jpp.sp_idx.val]) // we need to check the accessibility
                        {
                            working_labels.mut_dt_pt(jpp.sp_idx) = workingDt;
                            best_labels_pts[jpp.sp_idx] = working_labels.dt_pt(jpp.sp_idx);
                            continue_algorithm = true;
                        }
                    }

                    // We try to get on a vehicle, if we were already on a vehicle, but we arrived
                    // before on the previous via a connection, we try to catch a vehicle leaving this
                    // journey pattern point before
                    const DateTime previous_dt = prec_labels.dt_transfer(jpp.sp_idx);
                    if (prec_labels.transfer_is_initialized(jpp.sp_idx) &&
                        (!is_onboard || visitor.better_or_equal(previous_dt, workingDt, *it_st))) {
                        const auto tmp_st_dt = next_st.next_stop_time(
                            visitor.stop_event(), jpp.idx, previous_dt, visitor.clockwise());

                        if (tmp_st_dt.first != nullptr) {
                            if (! is_onboard || &*it_st != tmp_st_dt.first) {
                                // st_range is quite cache
                                // unfriendly, so avoid using it if
                                // not really needed.
                                it_st = visitor.st_range(*tmp_st_dt.first).begin();
                                is_onboard = true;
                                l_zone = it_st->local_traffic_zone;
                                // note that if we have found a better
                                // pickup, and that this pickup does
                                // not have the same local traffic
                                // zone, we may miss some interesting
                                // solutions.
                            } else if (l_zone != it_st->local_traffic_zone) {
                                // if we can pick up in this vj with 2
                                // different zones, we can drop off
                                // anywhere (we'll chose later at
                                // which stop we pickup)
                                l_zone = std::numeric_limits<uint16_t>::max();
                            }
                            workingDt = tmp_st_dt.second;
                            BOOST_ASSERT(! visitor.comp(workingDt, previous_dt));

                            if (tmp_st_dt.first->is_frequency()) {
                                // we need to update again the working dt for it to always
                                // be the arrival (resp departure) in the stoptimes
                                workingDt = tmp_st_dt.first->begin_from_end(workingDt, visitor.clockwise());
                            }
                        }
                    }
                }
                if (is_onboard) {
                    const type::VehicleJourney* vj_stay_in = visitor.get_extension_vj(it_st->vehicle_journey);
                    if (vj_stay_in) {
                        states_stay_in.emplace_back(vj_stay_in, l_zone, workingDt);
                    }
                }
            }
            q_elt.second = visitor.init_queue_item();
        }
        for (auto state : states_stay_in) {
            bool applied = apply_vj_extension(visitor, rt_level, state);
            continue_algorithm = continue_algorithm || applied;
        }
        continue_algorithm = continue_algorithm && this->foot_path(visitor);
    }
}


void RAPTOR::boucleRAPTOR(bool clockwise,
                          const nt::RTLevel rt_level,
                          uint32_t max_transfers) {
    if(clockwise) {
        raptor_loop(raptor_visitor(), rt_level, max_transfers);
    } else {
        raptor_loop(raptor_reverse_visitor(), rt_level, max_transfers);
    }
}


std::vector<Path> RAPTOR::compute(const type::StopArea* departure,
                                  const type::StopArea* destination,
                                  int departure_hour,
                                  int departure_day,
                                  DateTime borne,
                                  const nt::RTLevel rt_level,
                                  const navitia::time_duration& transfer_penalty,
                                  bool clockwise,
                                  const type::AccessibiliteParams& accessibilite_params,
                                  uint32_t max_transfers,
                                  const std::vector<std::string>& forbidden_uri,
                                  const boost::optional<navitia::time_duration>& direct_path_dur) {
    map_stop_point_duration departures, destinations;

    for(const type::StopPoint* sp : departure->stop_point_list) {
        departures[SpIdx(*sp)] = {};
    }

    for(const type::StopPoint* sp : destination->stop_point_list) {
        destinations[SpIdx(*sp)] = {};
    }

    return compute_all(departures,
                       destinations,
                       DateTimeUtils::set(departure_day, departure_hour),
                       rt_level,
                       transfer_penalty,
                       borne,
                       max_transfers,
                       accessibilite_params,
                       forbidden_uri,
                       clockwise,
                       direct_path_dur);
}


int RAPTOR::best_round(SpIdx sp_idx){
    for (size_t i = 0; i <= this->count; ++i) {
        if (labels[i].dt_pt(sp_idx) == best_labels_pts[sp_idx]) {
            return i;
        }
    }
    return -1;
}

}}
