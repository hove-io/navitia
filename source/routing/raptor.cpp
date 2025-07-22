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

#include "raptor.h"

#include "raptor_visitors.h"
#include "type/meta_data.h"

#include "utils/logger.h"

#include <boost/functional/hash.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

#include <chrono>

namespace navitia {
namespace routing {

DateTime limit_bound(const bool clockwise, const DateTime departure_datetime, const DateTime bound) {
    auto depart_clockwise = departure_datetime + DateTimeUtils::SECONDS_PER_DAY;
    auto depart_anticlockwise =
        departure_datetime > DateTimeUtils::SECONDS_PER_DAY ? departure_datetime - DateTimeUtils::SECONDS_PER_DAY : 0;
    return clockwise ? std::min(depart_clockwise, bound) : std::max(depart_anticlockwise, bound);
}

/*
 * Check if the given vj is valid for the given datetime,
 * If it is for every stoptime of the vj,
 * If the journey_pattern_point associated to it is improved by this stop time
 * we mark it.
 * If the given vj also has an extension we apply it.
 */
template <typename Visitor>
bool RAPTOR::apply_vj_extension(const Visitor& v,
                                const nt::RTLevel rt_level,
                                const type::VehicleJourney* vj,
                                const uint16_t l_zone,
                                DateTime base_dt,
                                DateTime working_walking_duration,
                                SpIdx boarding_stop_point) {
    auto& working_labels = labels[count];
    bool result = false;
    while (vj) {
        base_dt = v.get_base_dt_extension(base_dt, vj);
        const auto& stop_time_list = v.stop_time_list(vj);
        const auto& st_begin = stop_time_list.front();
        const auto first_dt = st_begin.section_end(base_dt, v.clockwise());

        // If the vj is not valid for the first stop it won't be valid at all
        if (!st_begin.is_valid_day(DateTimeUtils::date(first_dt), !v.clockwise(), rt_level)) {
            return result;
        }
        for (const type::StopTime& st : stop_time_list) {
            const auto workingDt = st.section_end(base_dt, v.clockwise());
            if (!st.valid_end(v.clockwise())) {
                continue;
            }
            if (l_zone != std::numeric_limits<uint16_t>::max() && l_zone == st.local_traffic_zone) {
                continue;
            }
            auto sp = st.stop_point;
            const auto sp_idx = SpIdx(*sp);

            if (!valid_stop_points[sp_idx.val]) {
                continue;
            }

            Label& best_label = best_labels[sp_idx];
            Label& working_label = working_labels[sp_idx];

            if (v.comp(workingDt, best_label.dt_pt)
                || (workingDt == best_label.dt_pt && working_walking_duration < best_label.walking_duration_pt)) {
                LOG4CPLUS_TRACE(raptor_logger, "Updating label dt count : "
                                                   << count << " sp " << data.pt_data->stop_points[sp_idx.val]->uri
                                                   << " from " << iso_string(working_label.dt_pt, data) << " to "
                                                   << iso_string(workingDt, data) << " throught : "
                                                   << st.vehicle_journey->route->line->uri << " boarding_stop_point : "
                                                   << data.pt_data->stop_points[boarding_stop_point.val]->uri
                                                   << " fallback : " << navitia::str(working_walking_duration));
                working_label.dt_pt = workingDt;
                working_label.walking_duration_pt = working_walking_duration;
                BOOST_ASSERT(working_walking_duration != DateTimeUtils::not_valid);
                best_label.dt_pt = workingDt;
                best_label.walking_duration_pt = working_walking_duration;
                result = true;
            }
        }
        vj = v.get_extension_vj(vj);
    }
    return result;
}

template <typename Visitor>
bool RAPTOR::foot_path(const Visitor& v) {
    bool result = false;
    auto& working_labels = labels[count];
    const auto& cnx_list = v.clockwise() ? data.dataRaptor->connections.forward_connections
                                         : data.dataRaptor->connections.backward_connections;

    for (const auto sp_cnx : cnx_list) {
        // for all stop point, we check if we can improve the stop points they are in connection with
        const auto& sp_idx = sp_cnx.first;
        const Label& working_label = working_labels[sp_idx];

        if (!is_dt_initialized(working_label.dt_pt)) {
            continue;
        }

        const DateTime start_connection_date = working_label.dt_pt;

        for (const auto& conn : sp_cnx.second) {
            const SpIdx destination_sp_idx = conn.sp_idx;
            const DateTime end_connection_date = v.combine(start_connection_date, conn.duration);
            const DateTime candidate_walking_duration = working_label.walking_duration_pt + conn.walking_duration;

            Label& destination_working_label = working_labels[destination_sp_idx];
            Label& destination_best_label = best_labels[destination_sp_idx];

            if (v.comp(end_connection_date, destination_best_label.dt_transfer)
                || (end_connection_date == destination_best_label.dt_transfer
                    && candidate_walking_duration < destination_best_label.walking_duration_transfer)) {
                LOG4CPLUS_TRACE(raptor_logger,
                                "Updating label transfer count : "
                                    << count << " sp " << data.pt_data->stop_points[destination_sp_idx.val]->uri
                                    << " from " << iso_string(destination_working_label.dt_transfer, data) << " to "
                                    << iso_string(end_connection_date, data)
                                    << " throught connection : " << data.pt_data->stop_points[sp_idx.val]->uri
                                    << " walking : " << navitia::str(candidate_walking_duration)
                                    << " previous best : " << iso_string(destination_best_label.dt_transfer, data));

                // if we can improve the best label, we mark it
                destination_working_label.dt_transfer = end_connection_date;
                destination_working_label.walking_duration_transfer = candidate_walking_duration;
                destination_best_label.dt_transfer = end_connection_date;
                destination_best_label.walking_duration_transfer = candidate_walking_duration;
                result = true;
            }
        }
    }

    for (const auto sp_jpps : jpps_from_sp) {
        const auto& working_label = working_labels[sp_jpps.first];
        if (!is_dt_initialized(working_label.dt_transfer)) {
            continue;
        }

        // we mark the jpp order
        for (const auto& jpp : sp_jpps.second) {
            if (v.comp(jpp.order, Q[jpp.jp_idx])) {
                Q[jpp.jp_idx] = jpp.order;
            }
        }
    }

    return result;
}

void RAPTOR::clear(const bool clockwise, const DateTime bound) {
    const int queue_value = clockwise ? std::numeric_limits<int>::max() : -1;
    Q.assign(data.dataRaptor->jp_container.get_jps_values(), queue_value);
    if (labels.empty()) {
        labels.resize(5);
    }
    const Labels& clean_labels = clockwise ? data.dataRaptor->labels_const : data.dataRaptor->labels_const_reverse;
    for (auto& lbl_list : labels) {
        lbl_list = clean_labels;
    }

    best_labels.fill_values(bound, bound, DateTimeUtils::not_valid, DateTimeUtils::not_valid);
}

void RAPTOR::init(const map_stop_point_duration& dep,
                  const DateTime bound,
                  const bool clockwise,
                  const type::Properties& properties) {
    for (const auto& sp_dt : dep) {
        if (!get_sp(sp_dt.first)->accessible(properties)) {
            continue;
        }
        const DateTime sn_dur = sp_dt.second.total_seconds();
        const DateTime begin_dt = bound + (clockwise ? sn_dur : -static_cast<int32_t>(sn_dur));

        Label& label = labels[0][sp_dt.first];
        label.dt_transfer = begin_dt;
        label.walking_duration_transfer = sn_dur;

        Label& best_label = best_labels[sp_dt.first];
        best_label.dt_transfer = begin_dt;
        best_label.walking_duration_transfer = begin_dt;

        for (const auto& jpp : jpps_from_sp[sp_dt.first]) {
            if (clockwise && Q[jpp.jp_idx] > jpp.order) {
                Q[jpp.jp_idx] = jpp.order;
            } else if (!clockwise && Q[jpp.jp_idx] < jpp.order) {
                Q[jpp.jp_idx] = jpp.order;
            }
        }
    }
}

RAPTOR::NEXT_STOPTIME_TYPE RAPTOR::choose_next_stop_time_type(
    const DateTime& departure_datetime,
    const boost::optional<boost::posix_time::ptime>& current_datetime) const {
    boost::posix_time::ptime now =
        current_datetime ? *current_datetime : boost::posix_time::second_clock::universal_time();
    size_t now_days = static_cast<size_t>((now.date() - data.meta->production_date.begin()).days());
    auto requested_days = static_cast<size_t>(departure_datetime / navitia::DateTimeUtils::SECONDS_PER_DAY);

    size_t cache_size = data.dataRaptor->cached_next_st_manager->get_max_size();
    if (now_days <= requested_days && requested_days < (now_days + cache_size)) {
        return NEXT_STOPTIME_TYPE::CACHED;
    }
    return NEXT_STOPTIME_TYPE::UNCACHED;
}

void RAPTOR::set_next_stop_time(const DateTime& departure_datetime,
                                const nt::RTLevel rt_level,
                                const DateTime& bound,
                                const type::AccessibiliteParams& accessibilite_params,
                                const bool clockwise,
                                const NEXT_STOPTIME_TYPE next_st_type) {
    assert(data.dataRaptor->cached_next_st_manager);

    switch (next_st_type) {
        case RAPTOR::NEXT_STOPTIME_TYPE::CACHED:
            LOG4CPLUS_INFO(raptor_logger, "Raptor: Using cached next_stop_time");
            next_st = data.dataRaptor->cached_next_st_manager->load(clockwise ? departure_datetime : bound, rt_level,
                                                                    accessibilite_params);
            break;
        case RAPTOR::NEXT_STOPTIME_TYPE::UNCACHED:
            LOG4CPLUS_INFO(raptor_logger, "Raptor: Using uncached next_stop_time");
            next_st = uncached_next_st;
            break;
        default:
            throw std::runtime_error("Raptor Unknown next stop time type");
            break;
    }
}

void RAPTOR::first_raptor_loop(const map_stop_point_duration& departures,
                               const DateTime& departure_datetime,
                               const nt::RTLevel rt_level,
                               const DateTime& bound_limit,
                               const uint32_t max_transfers,
                               const type::AccessibiliteParams& accessibilite_params,
                               const bool clockwise,
                               const boost::optional<boost::posix_time::ptime>& current_datetime) {
    const DateTime bound = limit_bound(clockwise, departure_datetime, bound_limit);

    auto next_st_type = choose_next_stop_time_type(clockwise ? departure_datetime : bound, current_datetime);

    set_next_stop_time(departure_datetime, rt_level, bound, accessibilite_params, clockwise, next_st_type);

    clear(clockwise, bound);
    init(departures, departure_datetime, clockwise, accessibilite_params.properties);

    boucleRAPTOR(accessibilite_params, clockwise, rt_level, max_transfers);
}

namespace {
struct Dom {
    explicit Dom(bool c) : clockwise(c) {}
    bool clockwise;
    using Arg = std::pair<size_t, StartingPointSndPhase>;
    inline bool operator()(const Arg& lhs, const Arg& rhs) const {
        /*
         * When multiple arrival with [same walking, same time, same number of sections] are
         * possible we keep them all as they are equally interesting
         */
        if (lhs.second.count == rhs.second.count && lhs.second.end_dt == rhs.second.end_dt
            && lhs.second.walking_dur == rhs.second.walking_dur) {
            return false;
        }
        return lhs.second.count <= rhs.second.count
               && (clockwise ? lhs.second.end_dt <= rhs.second.end_dt : lhs.second.end_dt >= rhs.second.end_dt)
               && lhs.second.walking_dur <= rhs.second.walking_dur;
    }
};
struct CompSndPhase {
    explicit CompSndPhase(bool c) : clockwise(c) {}
    bool clockwise;
    using Arg = StartingPointSndPhase;
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
        if (lhs.walking_dur != rhs.walking_dur) {
            return lhs.walking_dur < rhs.walking_dur;
        }
        return lhs.sp_idx < rhs.sp_idx;  // just to avoid randomness
    }
};

std::vector<StartingPointSndPhase> make_starting_points_snd_phase(const RAPTOR& raptor,
                                                                  const routing::map_stop_point_duration& arrs,
                                                                  const type::AccessibiliteParams& accessibilite_params,
                                                                  const bool clockwise) {
    std::vector<StartingPointSndPhase> res;
    auto overfilter = ParetoFront<std::pair<size_t, StartingPointSndPhase>, Dom>(Dom(clockwise));

    for (unsigned count = 1; count <= raptor.count; ++count) {
        const auto& working_labels = raptor.labels[count];
        for (const auto& a : arrs) {
            const Label& working_label = working_labels[a.first];
            if (!is_dt_initialized(working_label.dt_pt)) {
                continue;
            }
            if (!raptor.get_sp(a.first)->accessible(accessibilite_params.properties)) {
                continue;
            }

            const unsigned fallback_duration_to_arrival_stop_point = a.second.total_seconds();
            const DateTime walking_duration_from_departure_stop_point = working_label.walking_duration_pt;
            const DateTime total_walking_duration =
                fallback_duration_to_arrival_stop_point + walking_duration_from_departure_stop_point;
            const DateTime arrival_date_time =
                working_label.dt_pt
                + (clockwise ? fallback_duration_to_arrival_stop_point
                             : -static_cast<int32_t>(fallback_duration_to_arrival_stop_point));
            StartingPointSndPhase starting_point = {a.first, count, arrival_date_time, total_walking_duration, false};

            overfilter.add({res.size(), starting_point});
            res.push_back(starting_point);
        }
    }

    for (const auto& pair : overfilter) {
        res[pair.first].has_priority = true;
    }

    std::sort(res.begin(), res.end(), CompSndPhase(clockwise));  // most interesting solutions first

    return res;
}

// creation of a fake journey from informations known after first pass
// this journey aims at being the best we can hope from this StartingPointSndPhase
Journey convert_to_bound(const StartingPointSndPhase& sp, bool clockwise) {
    Journey journey;
    journey.sections.resize(sp.count);  // only the number of sections is part of the dominance function
    journey.sn_dur = navitia::time_duration(0, 0, sp.walking_dur, 0);
    if (clockwise) {
        journey.arrival_dt = sp.end_dt;
        journey.departure_dt = sp.end_dt - journey.sn_dur.seconds();
    } else {
        journey.arrival_dt = sp.end_dt + journey.sn_dur.seconds();
        journey.departure_dt = sp.end_dt;
    }

    journey.transfer_dur = 0_s;
    journey.min_waiting_dur = navitia::time_duration(boost::date_time::pos_infin);
    journey.total_waiting_dur = 0_s;
    journey.nb_vj_extentions = 0;
    return journey;
}

}  // namespace

// copy and do the off by one for strict comparison for the second pass.
static Labels& snd_pass_best_labels(const bool clockwise, Labels& best_labels) {
    for (auto& dt : best_labels.values()) {
        if (is_dt_initialized(dt.dt_pt)) {
            dt.dt_pt += clockwise ? -1 : 1;
        }
        if (is_dt_initialized(dt.dt_transfer)) {
            dt.dt_transfer += clockwise ? -1 : 1;
        }
    }
    return best_labels;
}
// Set the departure bounds on best_labels_pts for the second pass.
static void init_best_pts_snd_pass(const routing::map_stop_point_duration& departures,
                                   const DateTime& departure_datetime,
                                   const bool clockwise,
                                   Labels& best_labels) {
    for (const auto& d : departures) {
        Label& best_label = best_labels[d.first];
        if (clockwise) {
            best_label.dt_pt = std::min(best_label.dt_pt, uint(departure_datetime + d.second.total_seconds() - 1));
        } else {
            best_label.dt_pt = std::max(best_label.dt_pt, uint(departure_datetime - d.second.total_seconds() + 1));
        }
    }
}

std::vector<Path> RAPTOR::compute_all(const map_stop_point_duration& departures,
                                      const map_stop_point_duration& destinations,
                                      const DateTime& departure_datetime,
                                      const nt::RTLevel rt_level,
                                      const navitia::time_duration& arrival_transfer_penalty,
                                      const navitia::time_duration& walking_transfer_penalty,
                                      const DateTime& bound,
                                      const uint32_t max_transfers,
                                      const type::AccessibiliteParams& accessibilite_params,
                                      const std::vector<std::string>& forbidden_uri,
                                      const std::vector<std::string>& allowed_ids,
                                      bool clockwise,
                                      const boost::optional<navitia::time_duration>& direct_path_dur,
                                      const size_t max_extra_second_pass,
                                      const boost::optional<boost::posix_time::ptime>& current_datetime) {
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime), accessibilite_params, forbidden_uri, allowed_ids,
                         rt_level);

    const auto& journeys =
        compute_all_journeys(departures, destinations, departure_datetime, rt_level, arrival_transfer_penalty,
                             walking_transfer_penalty, bound, max_transfers, accessibilite_params, clockwise,
                             direct_path_dur, max_extra_second_pass, current_datetime);
    return from_journeys_to_path(journeys);
}

RAPTOR::Journeys RAPTOR::compute_all_journeys(const map_stop_point_duration& departures,
                                              const map_stop_point_duration& destinations,
                                              const DateTime& departure_datetime,
                                              const nt::RTLevel rt_level,
                                              const navitia::time_duration& arrival_transfer_penalty,
                                              const navitia::time_duration& walking_transfer_penalty,
                                              const DateTime& bound,
                                              const uint32_t max_transfers,
                                              const type::AccessibiliteParams& accessibilite_params,
                                              bool clockwise,
                                              const boost::optional<navitia::time_duration>& direct_path_dur,
                                              const size_t max_extra_second_pass,
                                              const boost::optional<boost::posix_time::ptime>& current_datetime) {
    auto start_raptor = std::chrono::system_clock::now();

    // auto solutions = ParetoFront<Journey, Dominates /*, JourneyParetoFrontVisitor*/>(Dominates(clockwise));
    auto dominator = Dominates(clockwise, arrival_transfer_penalty, walking_transfer_penalty);
    auto solutions = Solutions(dominator);

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

    first_raptor_loop(calc_dep, departure_datetime, rt_level, bound, max_transfers, accessibilite_params, clockwise,
                      current_datetime);

    LOG4CPLUS_TRACE(raptor_logger, "labels after first pass : " << std::endl << print_all_labels());

    auto end_first_pass = std::chrono::system_clock::now();

    LOG4CPLUS_DEBUG(raptor_logger, "end first pass with count : " << count);

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
    auto starting_points = make_starting_points_snd_phase(*this, calc_dest, accessibilite_params, clockwise);
    swap(labels, first_pass_labels);

    const auto inrows_labels = best_labels.inrow_labels();

    auto best_labels_for_snd_pass = Labels(
        // durations and transfers are swapped for the 2nd pass as we are going backward in time
        inrows_labels[1],   // dt transfers as pts
        inrows_labels[0],   // dt pts as transfers
        inrows_labels[3],   // walking duration transfers as walking duration
        inrows_labels[2]);  // walking duration pts as walking duration transfers

    snd_pass_best_labels(clockwise, best_labels_for_snd_pass);
    init_best_pts_snd_pass(calc_dep, departure_datetime, clockwise, best_labels_for_snd_pass);

    LOG4CPLUS_TRACE(raptor_logger, "starting points 2nd phase " << std::endl
                                                                << print_starting_points_snd_phase(starting_points));

    size_t nb_snd_pass = 0, nb_useless = 0, last_usefull_2nd_pass = 0, supplementary_2nd_pass = 0;
    for (const auto& start : starting_points) {
        navitia::type::StopPoint* start_stop_point = data.pt_data->stop_points[start.sp_idx.val];

        LOG4CPLUS_TRACE(raptor_logger, std::endl
                                           << "Second pass from " << start_stop_point->uri
                                           << "   count : " << start.count);

        if (!start.has_priority) {
            Journey fake_journey = convert_to_bound(start, clockwise);

            if (solutions.contains_better_than(fake_journey)) {
                LOG4CPLUS_TRACE(raptor_logger,
                                "already found a better solution than the fake journey from " << start_stop_point->uri);
                continue;
            }

            ++supplementary_2nd_pass;

            if (supplementary_2nd_pass > max_extra_second_pass) {
                LOG4CPLUS_DEBUG(raptor_logger, "max second pass reached");
                break;
            }
        }

        const auto& working_labels = first_pass_labels[start.count];

        clear(!clockwise, departure_datetime + (clockwise ? -1 : 1));
        map_stop_point_duration init_map;
        init_map[start.sp_idx] = 0_s;
        best_labels = best_labels_for_snd_pass;
        const Label& working_label = working_labels[start.sp_idx];
        init(init_map, working_label.dt_pt, !clockwise, accessibilite_params.properties);
        boucleRAPTOR(accessibilite_params, !clockwise, rt_level, max_transfers);

        read_solutions(*this, solutions, !clockwise, departure_datetime, departures, destinations, rt_level,
                       accessibilite_params, arrival_transfer_penalty, start);

        LOG4CPLUS_DEBUG(raptor_logger, "end of raptor loop body, nb of solutions : " << solutions.size());

        ++nb_snd_pass;
    }

    LOG4CPLUS_DEBUG(raptor_logger, "[2nd pass] number of 2nd pass = "
                                       << nb_snd_pass << " / " << starting_points.size() << " (nb useless = "
                                       << nb_useless << ", last usefull try = " << last_usefull_2nd_pass << ")");
    auto end_raptor = std::chrono::system_clock::now();
    LOG4CPLUS_DEBUG(raptor_logger,
                    "[2nd pass] Run times: 1st pass = "
                        << std::chrono::duration_cast<std::chrono::milliseconds>(end_first_pass - start_raptor).count()
                        << ", 2nd pass = "
                        << std::chrono::duration_cast<std::chrono::milliseconds>(end_raptor - end_first_pass).count());

    return solutions.get_pool();
}

void RAPTOR::isochrone(const map_stop_point_duration& departures,
                       const DateTime& departure_datetime,
                       const DateTime& b,
                       const uint32_t max_transfers,
                       const type::AccessibiliteParams& accessibilite_params,
                       const std::vector<std::string>& forbidden,
                       const std::vector<std::string>& allowed,
                       const bool clockwise,
                       const nt::RTLevel rt_level) {
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime), accessibilite_params, forbidden, allowed, rt_level);

    first_raptor_loop(departures, departure_datetime, rt_level, b, max_transfers, accessibilite_params, clockwise);
}

namespace {
struct ObjsFromIds {
    boost::dynamic_bitset<> jps;
    boost::dynamic_bitset<> jpps;
    boost::dynamic_bitset<> sps;
    ObjsFromIds(const std::vector<std::string>& ids, const JourneyPatternContainer& jp_container, const nt::Data& data)
        : jps(jp_container.nb_jps()), jpps(jp_container.nb_jpps()), sps(data.pt_data->stop_points.size()) {
        for (const auto& id : ids) {
            const auto it_line = data.pt_data->lines_map.find(id);
            if (it_line != data.pt_data->lines_map.end()) {
                for (const auto route : it_line->second->route_list) {
                    for (const auto& jp_idx : jp_container.get_jps_from_route()[RouteIdx(*route)]) {
                        jps.set(jp_idx.val, true);
                    }
                }
                continue;
            }
            const auto it_route = data.pt_data->routes_map.find(id);
            if (it_route != data.pt_data->routes_map.end()) {
                for (const auto& jp_idx : jp_container.get_jps_from_route()[RouteIdx(*it_route->second)]) {
                    jps.set(jp_idx.val, true);
                }
                continue;
            }
            const auto it_commercial_mode = data.pt_data->commercial_modes_map.find(id);
            if (it_commercial_mode != data.pt_data->commercial_modes_map.end()) {
                for (const auto line : it_commercial_mode->second->line_list) {
                    for (auto route : line->route_list) {
                        for (const auto& jp_idx : jp_container.get_jps_from_route()[RouteIdx(*route)]) {
                            jps.set(jp_idx.val, true);
                        }
                    }
                }
                continue;
            }
            const auto it_physical_mode = data.pt_data->physical_modes_map.find(id);
            if (it_physical_mode != data.pt_data->physical_modes_map.end()) {
                const auto phy_mode_idx = PhyModeIdx(*it_physical_mode->second);
                for (const auto& jp_idx : jp_container.get_jps_from_phy_mode()[phy_mode_idx]) {
                    jps.set(jp_idx.val, true);
                }
                continue;
            }
            const auto it_network = data.pt_data->networks_map.find(id);
            if (it_network != data.pt_data->networks_map.end()) {
                for (const auto line : it_network->second->line_list) {
                    for (const auto route : line->route_list) {
                        for (const auto& jp_idx : jp_container.get_jps_from_route()[RouteIdx(*route)]) {
                            jps.set(jp_idx.val, true);
                        }
                    }
                }
                continue;
            }
            const auto it_sp = data.pt_data->stop_points_map.find(id);
            if (it_sp != data.pt_data->stop_points_map.end()) {
                sps.set(it_sp->second->idx, true);
                for (const auto& jpp : data.dataRaptor->jpps_from_sp[SpIdx(*it_sp->second)]) {
                    jpps.set(jpp.idx.val, true);
                }
                continue;
            }
            const auto it_sa = data.pt_data->stop_areas_map.find(id);
            if (it_sa != data.pt_data->stop_areas_map.end()) {
                for (const auto sp : it_sa->second->stop_point_list) {
                    sps.set(sp->idx, true);
                    for (const auto& jpp : data.dataRaptor->jpps_from_sp[SpIdx(*sp)]) {
                        jpps.set(jpp.idx.val, true);
                    }
                }
                continue;
            }
        }
    }
};
}  // namespace

// Returns valid_jpps
void RAPTOR::set_valid_jp_and_jpp(uint32_t date,
                                  const type::AccessibiliteParams& accessibilite_params,
                                  const std::vector<std::string>& forbidden,
                                  const std::vector<std::string>& allowed,
                                  const nt::RTLevel rt_level) {
    const auto& jp_container = data.dataRaptor->jp_container;
    valid_journey_patterns = data.dataRaptor->jp_validity_patterns[rt_level][date];
    boost::dynamic_bitset<> valid_journey_pattern_points(jp_container.nb_jpps());
    valid_journey_pattern_points.set();
    valid_stop_points.set();

    auto forbidden_objs = ObjsFromIds(forbidden, jp_container, data);
    valid_journey_patterns &= forbidden_objs.jps.flip();
    valid_journey_pattern_points &= forbidden_objs.jpps.flip();
    valid_stop_points &= forbidden_objs.sps.flip();

    const auto allowed_objs = ObjsFromIds(allowed, jp_container, data);
    if (allowed_objs.jps.any()) {
        // If a journey pattern is present in allowed_obj, the
        // constraint is setted. Else, there is no constraint at the
        // journey pattern level.
        valid_journey_patterns &= allowed_objs.jps;
    }
    if (allowed_objs.jpps.any()) {
        // If a journey point pattern is present in allowed_obj, the
        // constraint is setted. Else, there is no constraint at the
        // journey pattern point level.
        valid_journey_pattern_points &= allowed_objs.jpps;
        valid_stop_points &= allowed_objs.sps;
    }

    // filter accessibility
    if (accessibilite_params.properties.any()) {
        for (const auto* sp : data.pt_data->stop_points) {
            if (sp->accessible(accessibilite_params.properties)) {
                continue;
            }
            valid_stop_points.set(sp->idx, false);
            for (const auto& jpp : data.dataRaptor->jpps_from_sp[SpIdx(*sp)]) {
                valid_journey_pattern_points.set(jpp.idx.val, false);
            }
        }
    }

    // propagate the invalid jp in their jpp
    for (JpIdx jp_idx = JpIdx(0); jp_idx.val < valid_journey_patterns.size(); ++jp_idx.val) {
        if (valid_journey_patterns[jp_idx.val]) {
            continue;
        }
        const auto& jp = jp_container.get(jp_idx);
        for (const auto& jpp_idx : jp.jpps) {
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

template <typename Visitor>
void RAPTOR::raptor_loop(Visitor visitor,
                         const type::AccessibiliteParams& accessibilite_params,
                         const nt::RTLevel rt_level,
                         uint32_t max_transfers) {
    bool continue_algorithm = true;
    count = 0;  //< Count iteration of raptor algorithm

    while (continue_algorithm && count <= max_transfers) {
        ++count;
        continue_algorithm = false;
        if (count == labels.size()) {
            if (visitor.clockwise()) {
                this->labels.push_back(this->data.dataRaptor->labels_const);
            } else {
                this->labels.push_back(this->data.dataRaptor->labels_const_reverse);
            }
        }
        const auto& prec_labels = labels[count - 1];
        auto& working_labels = labels[this->count];
        /*
         * We need to store it so we can apply stay_in after applying normal vjs
         * We want to do it, to favoritize normal vj against stay_in vjs
         */
        for (auto q_elt : Q) {
            /// We will scan the journey_pattern q_elt.first, starting from its stop numbered q_elt.second

            const JpIdx jp_idx = q_elt.first;

            const RouteIdx route_idx = data.dataRaptor->jp_container.get(jp_idx).route_idx;

            /// q_elt.second == visitor.init_queue_item() means that
            /// this journey_pattern is marked "not to be scanned"
            if (q_elt.second != visitor.init_queue_item()) {
                /// we begin scanning the journey_pattern as if we were not yet aboard a vehicle
                bool is_onboard = false;
                DateTime workingDt = visitor.worst_datetime();
                DateTime base_dt = workingDt;
                DateTime working_walking_duration = DateTimeUtils::not_valid;
                SpIdx boarding_stop_point = SpIdx();

                /// will be used to iterate through the StopTimeS of
                /// the vehicle journey of the current journey_pattern (jp_idx)
                ///  with the relevant departure date
                typename Visitor::stop_time_iterator it_st;  /// item = type::StopTime
                uint16_t l_zone = std::numeric_limits<uint16_t>::max();

                LOG4CPLUS_TRACE(raptor_logger, " Scanning line  " << data.pt_data->routes[route_idx.val]->line->uri);

                const auto& jpps_to_explore =
                    visitor.jpps_from_order(data.dataRaptor->jpps_from_jp, jp_idx, q_elt.second);
                for (const dataRAPTOR::JppsFromJp::Jpp& jpp : jpps_to_explore) {
                    if (is_onboard) {
                        ++it_st;
                        // We update workingDt with the new arrival time
                        // We need at each journey pattern point when we have a st
                        // If we don't it might cause problem with overmidnight vj
                        const type::StopTime& st = *it_st;
                        workingDt = st.section_end(base_dt, visitor.clockwise());
                        // We check if there are no drop_off_only and if the local_zone is okay

                        Label& best_label = best_labels[jpp.sp_idx];
                        Label& working_label = working_labels[jpp.sp_idx];

                        const bool has_better_label = visitor.comp(workingDt, best_label.dt_pt)
                                                      || (workingDt == best_label.dt_pt
                                                          && working_walking_duration < best_label.walking_duration_pt);

                        if (st.valid_end(visitor.clockwise())
                            && (l_zone == std::numeric_limits<uint16_t>::max() || l_zone != st.local_traffic_zone)
                            && has_better_label
                            && valid_stop_points[jpp.sp_idx.val])  // we need to check the accessibility
                        {
                            LOG4CPLUS_TRACE(raptor_logger,
                                            "Updating label dt "
                                                << "count : " << count << " sp "
                                                << data.pt_data->stop_points[jpp.sp_idx.val]->uri << " from "
                                                << iso_string(working_label.dt_pt, data) << " to "
                                                << iso_string(workingDt, data) << " throught : "
                                                << st.vehicle_journey->route->line->uri << " boarding_stop_point : "
                                                << data.pt_data->stop_points[boarding_stop_point.val]->uri
                                                << " walking : " << navitia::str(working_walking_duration)
                                                << " old best : " << iso_string(best_label.dt_pt, data));

                            working_label.dt_pt = workingDt;
                            working_label.walking_duration_pt = working_walking_duration;
                            BOOST_ASSERT(working_walking_duration != DateTimeUtils::not_valid);
                            best_label.dt_pt = workingDt;
                            best_label.walking_duration_pt = working_walking_duration;
                            continue_algorithm = true;
                        }
                    }

                    // We try to get on a vehicle, if we were already on a vehicle, but we arrived
                    // before on the previous via a connection, we try to catch a vehicle leaving this
                    // journey pattern point before
                    const Label& prec_label = prec_labels[jpp.sp_idx];

                    // if we cannot board at this stop point, nothing to do
                    if (!is_dt_initialized(prec_label.dt_transfer) || !valid_stop_points[jpp.sp_idx.val]) {
                        continue;
                    }

                    /// the time at which we arrive at stop point jpp.sp_idx (using at most count-1 transfers)
                    //  hence we can board any vehicle arriving after previous_dt
                    const DateTime previous_dt = prec_label.dt_transfer;
                    const DateTime previous_walking_duration = prec_label.walking_duration_transfer;

                    /// we are at stop point jpp.idx at time previous_dt
                    /// waiting for the next vehicle journey of the journey_pattern jpp.jp_idx to embark on
                    /// the next vehicle journey will arrive at
                    ///   tmp_st_dt.second
                    /// the corresponding StopTime is
                    ///    tmp_st_dt.first

                    const auto tmp_st_dt =
                        next_st->next_stop_time(visitor.stop_event(), jpp.idx, previous_dt, visitor.clockwise(),
                                                rt_level, accessibilite_params.vehicle_properties, jpp.has_freq);
                    // const auto tmp_st_dt =
                    //    next_st->next_stop_time(visitor.stop_event(), jpp.idx, previous_dt, visitor.clockwise());

                    /// if there is no vehicle arriving after previous_dt, nothing to do
                    if (tmp_st_dt.first == nullptr) {
                        continue;
                    }

                    const auto candidate_board_time = tmp_st_dt.second;
                    const auto candidate_base_dt = tmp_st_dt.first->base_dt(candidate_board_time, visitor.clockwise());
                    const auto candidate_debark_time = visitor.clockwise()
                                                           ? tmp_st_dt.first->arrival(candidate_base_dt)
                                                           : tmp_st_dt.first->departure(candidate_base_dt);

                    bool update_boarding_stop_point = !is_onboard || visitor.comp(candidate_debark_time, workingDt)
                                                      || (candidate_debark_time == workingDt
                                                          && previous_walking_duration <= working_walking_duration);

                    // LOG4CPLUS_TRACE(raptor_logger, "Try boarding stop point  "
                    //                                    << data.pt_data->stop_points[jpp.sp_idx.val]->uri
                    //                                    << " debark : " << iso_string(candidate_debark_time, data)
                    //                                    << " onboard  : " << iso_string(tmp_st_dt.second, data)
                    //                                    << " waiting  : " << iso_string(previous_dt, data)
                    //                                    << " walking : " << navitia::str(previous_walking_duration)
                    //                                    << "\n vs : "
                    //                                    << " working dt : " << iso_string(workingDt, data)
                    //                                    << " walking : " << navitia::str(working_walking_duration));

                    if (update_boarding_stop_point) {
                        /// we are at stop point jpp.idx at time previous_dt
                        /// waiting for the next vehicle journey of the journey_pattern jpp.jp_idx to embark on
                        /// the next vehicle journey will arrive at
                        ///   tmp_st_dt.second
                        /// the corresponding StopTime is
                        ///    tmp_st_dt.first
                        // const auto tmp_st_dt =
                        //     next_st->next_stop_time(visitor.stop_event(), jpp.idx, previous_dt, visitor.clockwise());
                        if (tmp_st_dt.first != nullptr) {
                            if (!is_onboard || &*it_st != tmp_st_dt.first) {
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

                            // if (boarding_stop_point == SpIdx()) {
                            //     LOG4CPLUS_TRACE(raptor_logger,
                            //                     "Setting boarding stop point  "
                            //                         << " to " << data.pt_data->stop_points[jpp.sp_idx.val]->uri
                            //                         << " working dt : " << iso_string(tmp_st_dt.second, data)
                            //                         << " walking : " << navitia::str(previous_walking_duration));
                            // } else {
                            //     LOG4CPLUS_TRACE(raptor_logger,
                            //                     "Switching boarding stop point : "
                            //                         << " from "
                            //                         << data.pt_data->stop_points[boarding_stop_point.val]->uri << "
                            //                         to "
                            //                         << data.pt_data->stop_points[jpp.sp_idx.val]->uri
                            //                         << " working dt before : " << iso_string(workingDt, data)
                            //                         << " working dt after : " << iso_string(tmp_st_dt.second, data)
                            //                         << " walking before : " << navitia::str(working_walking_duration)
                            //                         << " walking after : " <<
                            //                         navitia::str(previous_walking_duration));
                            // }
                            BOOST_ASSERT(visitor.be(candidate_debark_time, workingDt));
                            workingDt = candidate_debark_time;
                            working_walking_duration = previous_walking_duration;
                            boarding_stop_point = jpp.sp_idx;

                            base_dt = candidate_base_dt;
                        }
                    }
                }
                if (is_onboard) {
                    const type::VehicleJourney* vj_stay_in = visitor.get_extension_vj(it_st->vehicle_journey);
                    if (vj_stay_in) {
                        bool applied = apply_vj_extension(visitor, rt_level, vj_stay_in, l_zone, base_dt,
                                                          working_walking_duration, boarding_stop_point);
                        continue_algorithm = continue_algorithm || applied;
                    }
                }
            }
            /// mark the journey_pattern as visited, no need to explore it in the next round
            q_elt.second = visitor.init_queue_item();
        }
        if (continue_algorithm) {
            continue_algorithm = this->foot_path(visitor);
        }
    }
}

void RAPTOR::boucleRAPTOR(const type::AccessibiliteParams& accessibilite_params,
                          const bool clockwise,
                          const nt::RTLevel rt_level,
                          uint32_t max_transfers) {
    if (clockwise) {
        raptor_loop(raptor_visitor(), accessibilite_params, rt_level, max_transfers);
    } else {
        raptor_loop(raptor_reverse_visitor(), accessibilite_params, rt_level, max_transfers);
    }
}

std::vector<Path> RAPTOR::compute(const type::StopArea* departure,
                                  const type::StopArea* destination,
                                  int departure_hour,
                                  int departure_day,
                                  DateTime borne,
                                  const nt::RTLevel rt_level,
                                  const navitia::time_duration& arrival_transfer_penalty,
                                  const navitia::time_duration& walking_transfer_penalty,
                                  bool clockwise,
                                  const type::AccessibiliteParams& accessibilite_params,
                                  uint32_t max_transfers,
                                  const std::vector<std::string>& forbidden_uri,
                                  const boost::optional<navitia::time_duration>& direct_path_dur,
                                  const boost::optional<boost::posix_time::ptime>& current_datetime) {
    map_stop_point_duration departures, destinations;

    for (const type::StopPoint* sp : departure->stop_point_list) {
        departures[SpIdx(*sp)] = {};
    }

    for (const type::StopPoint* sp : destination->stop_point_list) {
        destinations[SpIdx(*sp)] = {};
    }

    return compute_all(departures, destinations, DateTimeUtils::set(departure_day, departure_hour), rt_level,
                       arrival_transfer_penalty, walking_transfer_penalty, borne, max_transfers, accessibilite_params,
                       forbidden_uri, {}, clockwise, direct_path_dur, 0, current_datetime);
}

int RAPTOR::best_round(SpIdx sp_idx) {
    for (size_t i = 0; i <= this->count; ++i) {
        const Label& label = labels[i][sp_idx];
        const Label& best_label = best_labels[sp_idx];
        if (label.dt_pt == best_label.dt_pt) {
            return i;
        }
    }
    return -1;
}

std::string RAPTOR::print_all_labels() {
    std::ostringstream output;
    for (uint32_t stop_point_id = 0; stop_point_id < data.pt_data->stop_points.size(); ++stop_point_id) {
        SpIdx sp_idx = SpIdx(stop_point_id);
        navitia::type::StopPoint* stop_point = data.pt_data->stop_points[sp_idx.val];

        bool print_stop_point = false;
        for (auto& current_labels : labels) {
            const Label& current_label = current_labels[sp_idx];
            if (is_dt_initialized(current_label.dt_pt) || is_dt_initialized(current_label.dt_transfer)) {
                print_stop_point = true;
                break;
            }
        }

        if (print_stop_point) {
            output << "" << stop_point->uri << " : " << std::endl;

            for (unsigned int count_id = 0; count_id < labels.size(); ++count_id) {
                Labels& current_labels = labels[count_id];
                const Label& label = current_labels[sp_idx];
                if (!is_dt_initialized(label.dt_pt) && !is_dt_initialized(label.dt_transfer)) {
                    continue;
                }
                output << "   count : " << count_id;
                output << " dt_pt : ";

                if (is_dt_initialized(label.dt_pt)) {
                    output << iso_string(label.dt_pt, data);
                    output << ", " << navitia::str(label.walking_duration_pt);
                } else {
                    output << "not init";
                }

                output << " transfer_dt : ";

                if (is_dt_initialized(label.dt_transfer)) {
                    output << iso_string(label.dt_transfer, data);
                    output << ", " << navitia::str(label.walking_duration_transfer);
                } else {
                    output << "not init";
                }

                output << std::endl;
            }
        }
    }
    return output.str();
}

std::string RAPTOR::print_starting_points_snd_phase(std::vector<StartingPointSndPhase>& starting_points) {
    std::ostringstream output;
    for (auto start_point : starting_points) {
        navitia::type::StopPoint* stop_point = data.pt_data->stop_points[start_point.sp_idx.val];
        output << "" << stop_point->uri << " count : " << start_point.count
               << " end_dt : " << iso_string(start_point.end_dt, data)
               << " fallback_dur : " << navitia::str(start_point.walking_dur)
               << " has_priority : " << start_point.has_priority << std::endl;
    }
    return output.str();
}

}  // namespace routing
}  // namespace navitia
