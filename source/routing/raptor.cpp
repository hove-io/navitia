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
#include "raptor_solution_filter.h"

namespace bt = boost::posix_time;

namespace navitia { namespace routing {

/*
 * Check if the given vj is valid for the given datetime,
 * If it is for every stoptime of the vj,
 * If the journey_pattern_point associated to it is improved by this stop time
 * we mark it.
 * If the given vj also has an extension we apply it.
 */
template<typename Visitor>
bool RAPTOR::apply_vj_extension(const Visitor& v,
                                const bool disruption_active,
                                const RoutingState& state) {
    auto& working_labels = labels[count];
    auto workingDt = state.workingDate;
    auto vj = state.vj;
    bool add_vj = false;
    bool result = false;
    while(vj) {
        const auto& stop_time_list = v.stop_time_list(vj);
        const auto& st_begin = stop_time_list.front();
        const auto current_time = st_begin.section_end_time(v.clockwise(),
                                DateTimeUtils::hour(workingDt));
        DateTimeUtils::update(workingDt, current_time, v.clockwise());
        // If the vj is not valid for the first stop it won't be valid at all
        if (!st_begin.is_valid_day(DateTimeUtils::date(workingDt), !v.clockwise(), disruption_active)) {
            return result;
        }
        for (const type::StopTime& st: stop_time_list) {
            const auto current_time = st.section_end_time(v.clockwise(),
                                    DateTimeUtils::hour(workingDt));
            DateTimeUtils::update(workingDt, current_time, v.clockwise());
            if (!st.valid_end(v.clockwise())) {
                continue;
            }
            if (state.l_zone != std::numeric_limits<uint16_t>::max() &&
               state.l_zone == st.local_traffic_zone) {
                continue;
            }
            auto sp = st.journey_pattern_point->stop_point;
            const auto sp_idx = SpIdx(*sp);

            // for performance reason, we desactivate weakly
            // dominated solutions for the moment.
            //if (v.comp(best_labels_pts[sp_idx], workingDt)) { continue; }
            //if (!v.comp(workingDt, working_labels.dt_pt(sp_idx))) { continue; }
            if (! v.comp(workingDt, best_labels_pts[sp_idx])) { continue; }

            working_labels.mut_dt_pt(sp_idx) = workingDt;
            best_labels_pts[sp_idx] = workingDt;
            add_vj = true;
            result = true;
        }
        //If we never marked a vj, we don't want to continue
        //This is usefull when there is a loop
        if(add_vj) {
            vj = v.get_extension_vj(vj);
        } else {
            vj = nullptr;
        }
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

            // for performance reason, we desactivate weakly
            // dominated solutions for the moment.
            //if (v.comp(best_labels_transfers[destination_sp_idx], next)) { continue; }
            //if (! v.comp(next, working_labels.mut_dt_transfer(destination_sp_idx))) { continue; }
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
    Q.assign(data.pt_data->journey_patterns, queue_value);
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

void RAPTOR::init(const vec_stop_point_duration& dep,
                  const DateTime bound,
                  const bool clockwise,
                  const type::Properties &properties) {
    for (const auto& sp_dt: dep) {
        if (! get_sp(sp_dt.first)->accessible(properties)) { continue; }

        const DateTime sn_dur = sp_dt.second.total_seconds();
        const DateTime begin_dt = bound + (clockwise ? sn_dur : -sn_dur);
        labels[0].mut_dt_transfer(sp_dt.first) = begin_dt;
        for (const auto jpp: jpps_from_sp[sp_dt.first]) {
            if (clockwise && Q[jpp.jp_idx] > jpp.order) {
                Q[jpp.jp_idx] = jpp.order;
            } else if (! clockwise && Q[jpp.jp_idx] < jpp.order) {
                Q[jpp.jp_idx] = jpp.order;
            }
        }
    }
}

void RAPTOR::first_raptor_loop(const vec_stop_point_duration& dep,
                               const vec_stop_point_duration& arr,
                               const DateTime& departure_datetime,
                               bool disruption_active,
                               bool allow_odt,
                               const DateTime& bound,
                               const uint32_t max_transfers,
                               const type::AccessibiliteParams& accessibilite_params,
                               const std::vector<std::string>& forbidden_uri,
                               bool clockwise) {

    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden_uri,
                         disruption_active,
                         allow_odt,
                         dep,
                         arr);

    clear(clockwise, bound);
    init(dep, departure_datetime, clockwise, accessibilite_params.properties);

    boucleRAPTOR(accessibilite_params, clockwise, disruption_active, max_transfers);
}

std::vector<Path>
RAPTOR::compute_all(const vec_stop_point_duration& departures_,
                    const vec_stop_point_duration& destinations,
                    const DateTime& departure_datetime,
                    bool disruption_active,
                    bool allow_odt,
                    const DateTime& bound,
                    const uint32_t max_transfers,
                    const type::AccessibiliteParams& accessibilite_params,
                    const std::vector<std::string>& forbidden_uri,
                    bool clockwise) {

    std::vector<Path> result;

    auto calc_dep = clockwise ? departures_ : destinations;
    auto calc_dest = clockwise ? destinations : departures_;

    first_raptor_loop(calc_dep, calc_dest, departure_datetime, disruption_active, allow_odt,
                      bound, max_transfers, accessibilite_params, forbidden_uri, clockwise);

    const auto reader_results = read_solutions(*this, clockwise, departures_, destinations, disruption_active, accessibilite_params);

    return filter_journeys(reader_results);
}

std::vector<std::pair<type::EntryPoint, std::vector<Path>>>
RAPTOR::compute_nm_all(const std::vector<std::pair<type::EntryPoint, vec_stop_point_duration>>& departures,
                       const std::vector<std::pair<type::EntryPoint, vec_stop_point_duration>>& arrivals,
                       const DateTime& departure_datetime,
                       bool disruption_active,
                       bool allow_odt,
                       const DateTime& bound,
                       const uint32_t max_transfers,
                       const type::AccessibiliteParams& accessibilite_params,
                       const std::vector<std::string>& forbidden_uri,
                       bool clockwise) {
    std::vector<std::pair<type::EntryPoint, std::vector<Path>>> result;
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden_uri,
                         disruption_active,
                         allow_odt);

    const auto& n_points = clockwise ? departures : arrivals;

    std::vector<std::pair<SpIdx, navitia::time_duration> > calc_dep;
    for(const auto& n_point : n_points)
        for (const auto& n_stop_point : n_point.second)
            calc_dep.push_back(n_stop_point);

    clear(clockwise, bound);
    init(calc_dep, departure_datetime, clockwise, accessibilite_params.properties);

    boucleRAPTOR(accessibilite_params, clockwise, disruption_active, max_transfers);

    const auto& m_points = clockwise ? arrivals : departures;

    for(const auto& m_point : m_points) {
        const type::EntryPoint& m_entry_point = m_point.first;

        std::vector<Path> paths;

        // just returns destination stop points
        for (auto& m_stop_point : m_point.second) {
            Path path;
            path.nb_changes = 0;
            PathItem path_item;
            const navitia::type::StopPoint* sp = get_sp(m_stop_point.first);
            path_item.stop_points.push_back(sp);
            path_item.type = public_transport;
            path.items.push_back(path_item);
            paths.push_back(path);
        }

        result.push_back(std::make_pair(m_entry_point, paths));
    }

    return result;
}

void
RAPTOR::isochrone(const vec_stop_point_duration& departures,
                  const DateTime& departure_datetime,
                  const DateTime& bound,
                  uint32_t max_transfers,
                  const type::AccessibiliteParams& accessibilite_params,
                  const std::vector<std::string>& forbidden,
                  bool clockwise,
                  bool disruption_active,
                  bool allow_odt) {
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden,
                         disruption_active,
                         allow_odt);
    clear(clockwise, bound);
    init(departures, departure_datetime, true, accessibilite_params.properties);

    boucleRAPTOR(accessibilite_params, clockwise, max_transfers);
}


void RAPTOR::set_valid_jp_and_jpp(
    uint32_t date,
    const type::AccessibiliteParams & accessibilite_params,
    const std::vector<std::string> & forbidden,
    bool disruption_active,
    bool allow_odt,
    const vec_stop_point_duration &departures_,
    const vec_stop_point_duration &destinations)
{

    if(disruption_active){
        valid_journey_patterns = data.dataRaptor->jp_adapted_validity_pattern[date];
    }else{
        valid_journey_patterns = data.dataRaptor->jp_validity_patterns[date];
    }
    boost::dynamic_bitset<> valid_journey_pattern_points(data.pt_data->journey_pattern_points.size());
    valid_journey_pattern_points.set();

    // We will forbiden every object designated in forbidden
    for (const auto& uri : forbidden) {
        const auto it_line = data.pt_data->lines_map.find(uri);
        if (it_line != data.pt_data->lines_map.end()) {
            for (const auto route : it_line->second->route_list) {
                for (const auto jp  : route->journey_pattern_list) {
                    valid_journey_patterns.set(jp->idx, false);
                }
            }
            continue;
        }
        const auto it_route = data.pt_data->routes_map.find(uri);
        if (it_route != data.pt_data->routes_map.end()) {
            for (const auto jp  : it_route->second->journey_pattern_list) {
                valid_journey_patterns.set(jp->idx, false);
            }
            continue;
        }
        const auto it_commercial_mode = data.pt_data->commercial_modes_map.find(uri);
        if (it_commercial_mode != data.pt_data->commercial_modes_map.end()) {
            for (const auto line : it_commercial_mode->second->line_list) {
                for (auto route : line->route_list) {
                    for (const auto jp  : route->journey_pattern_list) {
                        valid_journey_patterns.set(jp->idx, false);
                    }
                }
            }
            continue;
        }
        const auto it_physical_mode = data.pt_data->physical_modes_map.find(uri);
        if (it_physical_mode != data.pt_data->physical_modes_map.end()) {
            for (const auto jp  : it_physical_mode->second->journey_pattern_list) {
                valid_journey_patterns.set(jp->idx, false);
            }
            continue;
        }
        const auto it_network = data.pt_data->networks_map.find(uri);
        if (it_network != data.pt_data->networks_map.end()) {
            for (const auto line : it_network->second->line_list) {
                for (const auto route : line->route_list) {
                    for (const auto jp  : route->journey_pattern_list) {
                        valid_journey_patterns.set(jp->idx, false);
                    }
                }
            }
            continue;
        }
        const auto it_jp = data.pt_data->journey_patterns_map.find(uri);
        if (it_jp != data.pt_data->journey_patterns_map.end()) {
            valid_journey_patterns.set(it_jp->second->idx, false);
        }
        const auto it_jpp = data.pt_data->journey_pattern_points_map.find(uri);
        if (it_jpp !=  data.pt_data->journey_pattern_points_map.end()) {
            valid_journey_pattern_points.set(it_jpp->second->idx, false);
            continue;
        }
        const auto it_sp = data.pt_data->stop_points_map.find(uri);
        if (it_sp !=  data.pt_data->stop_points_map.end()) {
            for (const auto jpp : it_sp->second->journey_pattern_point_list) {
                valid_journey_pattern_points.set(jpp->idx, false);
            }
            continue;
        }
        const auto it_sa = data.pt_data->stop_areas_map.find(uri);
        if (it_sa !=  data.pt_data->stop_areas_map.end()) {
            for (const auto sp : it_sa->second->stop_point_list) {
                for (const auto jpp : sp->journey_pattern_point_list) {
                    valid_journey_pattern_points.set(jpp->idx, false);
                }
            }
            continue;
        }
    }
    if (!allow_odt) {
        for(const type::JourneyPattern* journey_pattern : data.pt_data->journey_patterns) {
            if(journey_pattern->odt_properties.is_zonal_odt()){
            //We want to display to the user he has to call when it's virtual
                    valid_journey_patterns.set(journey_pattern->idx, false);
                    continue;
            }
        }
    } else {
        // We want to be able to take zonal odt only to leave (or arrive) the 
        // the departure (to the destination).
        // Thus, we can forbid all odt journey patterns without any stop point
        // in the set of departures or destinations.

        //Let's build the set of allowed journey patterns
        std::set<const type::JourneyPattern*> allowed_jp;
        for (const vec_stop_point_duration& vec_sp_duration : {departures_, destinations}) {
            for (const stop_point_duration & sp_duration : vec_sp_duration) {
                const auto sp = get_sp(sp_duration.first);
                for (const auto jpp : sp->journey_pattern_point_list) {
                    if (!jpp->journey_pattern->odt_properties.is_zonal_odt()) {
                        continue;
                    }
                    allowed_jp.insert(jpp->journey_pattern);
                }
            }
        }

        for (const auto journey_pattern : data.pt_data->journey_patterns) {
            if(!journey_pattern->odt_properties.is_zonal_odt()){
                continue;
            }
            if (allowed_jp.count(journey_pattern) == 0) {
                valid_journey_patterns.set(journey_pattern->idx, false);
            }
        }
    }

    // filter accessibility
    if (accessibilite_params.properties.any()) {
        for (const auto* sp: data.pt_data->stop_points) {
            if (sp->accessible(accessibilite_params.properties)) { continue; }
            for (const auto* jpp: sp->journey_pattern_point_list) {
                valid_journey_pattern_points.set(jpp->idx, false);
            }
        }
    }

    // propagate the invalid jp in their jpp
    for (JpIdx jp_idx = JpIdx(0); jp_idx.val < valid_journey_patterns.size(); ++jp_idx.val) {
        if (valid_journey_patterns[jp_idx.val]) { continue; }
        const auto* jp = get_jp(jp_idx);
        for (const auto* jpp: jp->journey_pattern_point_list) {
            valid_journey_pattern_points.set(jpp->idx, false);
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
                         const type::AccessibiliteParams& accessibilite_params,
                         bool disruption_active,
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
                JppIdx boarding_idx; //< The boarding journey pattern point
                DateTime workingDt = visitor.worst_datetime();
                typename Visitor::stop_time_iterator it_st;
                uint16_t l_zone = std::numeric_limits<uint16_t>::max();
                const auto& jpps_to_explore = visitor.jpps_from_order(data.dataRaptor->jpps_from_jp,
                                                                      jp_idx,
                                                                      q_elt.second);

                for (const auto& jpp: jpps_to_explore) {
                    if(boarding_idx.is_valid()) {
                        ++it_st;
                        // We update workingDt with the new arrival time
                        // We need at each journey pattern point when we have a st
                        // If we don't it might cause problem with overmidnight vj
                        const type::StopTime& st = *it_st;
                        const auto current_time = st.section_end_time(visitor.clockwise(),
                                                DateTimeUtils::hour(workingDt));
                        DateTimeUtils::update(workingDt, current_time, visitor.clockwise());
                        // We check if there are no drop_off_only and if the local_zone is okay
                        if(st.valid_end(visitor.clockwise())&&
                                (l_zone == std::numeric_limits<uint16_t>::max() ||
                                 l_zone != st.local_traffic_zone)) {

                            // for performance reason, we desactivate weakly
                            // dominated solutions for the moment.
                            //if (! visitor.comp(best_labels_pts[jpp.sp_idx], workingDt)
                            //    && visitor.comp(workingDt, working_labels.mut_dt_pt(jpp.sp_idx)))
                            if (visitor.comp(workingDt, best_labels_pts[jpp.sp_idx]))
                            {
                                working_labels.mut_dt_pt(jpp.sp_idx) = workingDt;
                                best_labels_pts[jpp.sp_idx] = working_labels.dt_pt(jpp.sp_idx);
                                continue_algorithm = true;
                            }
                        }
                    }

                    // We try to get on a vehicle, if we were already on a vehicle, but we arrived
                    // before on the previous via a connection, we try to catch a vehicle leaving this
                    // journey pattern point before
                    const DateTime previous_dt = prec_labels.dt_transfer(jpp.sp_idx);
                    if(prec_labels.transfer_is_initialized(jpp.sp_idx) &&
                       (!boarding_idx.is_valid() || visitor.better_or_equal(previous_dt, workingDt, *it_st))) {
                        const auto tmp_st_dt = next_st.next_stop_time(
                            jpp.idx, previous_dt, visitor.clockwise(), disruption_active,
                            accessibilite_params.vehicle_properties, jpp.has_freq);

                        if (tmp_st_dt.first != nullptr) {
                            if (! boarding_idx.is_valid() || &*it_st != tmp_st_dt.first) {
                                // st_range is quite cache
                                // unfriendly, so avoid using it if
                                // not really needed.
                                it_st = visitor.st_range(*tmp_st_dt.first).begin();
                            }
                            boarding_idx = jpp.idx;
                            workingDt = tmp_st_dt.second;
                            l_zone = it_st->local_traffic_zone;
                            continue_algorithm = true;
                            BOOST_ASSERT(! visitor.comp(workingDt, previous_dt));
                        }
                    }
                }
                if(boarding_idx.is_valid()) {
                    const type::VehicleJourney* vj_stay_in = visitor.get_extension_vj(it_st->vehicle_journey);
                    if (vj_stay_in) {
                        states_stay_in.emplace_back(vj_stay_in, boarding_idx, l_zone, workingDt);
                    }
                }
            }
            q_elt.second = visitor.init_queue_item();
        }
        for (auto state : states_stay_in) {
            bool applied = apply_vj_extension(visitor, disruption_active, state);
            continue_algorithm = continue_algorithm || applied;
        }
        continue_algorithm = continue_algorithm && this->foot_path(visitor);
    }
}


void RAPTOR::boucleRAPTOR(const type::AccessibiliteParams& accessibilite_params,
                          bool clockwise,
                          bool disruption_active,
                          uint32_t max_transfers){
    if(clockwise) {
        raptor_loop(raptor_visitor(), accessibilite_params, disruption_active, max_transfers);
    } else {
        raptor_loop(raptor_reverse_visitor(), accessibilite_params, disruption_active, max_transfers);
    }
}


std::vector<Path> RAPTOR::compute(const type::StopArea* departure,
        const type::StopArea* destination, int departure_hour,
        int departure_day, DateTime borne, bool disruption_active, bool allow_odt, bool clockwise,
        const type::AccessibiliteParams & accessibilite_params,
        uint32_t max_transfers, const std::vector<std::string>& forbidden_uri) {
    vec_stop_point_duration departures, destinations;

    for(const type::StopPoint* sp : departure->stop_point_list) {
        departures.push_back({SpIdx(*sp), {}});
    }

    for(const type::StopPoint* sp : destination->stop_point_list) {
        destinations.push_back({SpIdx(*sp), {}});
    }

    return compute_all(departures, destinations, DateTimeUtils::set(departure_day, departure_hour),
                       disruption_active, allow_odt, borne, max_transfers, accessibilite_params, forbidden_uri, clockwise);
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
