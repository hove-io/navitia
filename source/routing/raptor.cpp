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

#include "raptor.h"
#include "raptor_visitors.h"
#include <boost/foreach.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace bt = boost::posix_time;

namespace navitia { namespace routing {

void RAPTOR::make_queue() {
    for(auto& jpp : best_jpp_by_sp) {
        jpp = type::invalid_idx;
    }
}

/*
 * Check if the given vj is valid for the given datetime,
 * If it is for every stoptime of the vj,
 * If the journey_pattern_point associated to it is improved by this stop time
 * we mark it.
 * If the given vj also has an extension we apply it.
 */
template<typename Visitor>
bool RAPTOR::apply_vj_extension(const Visitor& v, const bool global_pruning,
                                const bool disruption_active, const RoutingState& state) {
    auto& working_labels = labels[count];
    auto workingDt = state.workingDate;
    auto vj = state.vj;
    bool add_vj = false;
    bool result = false;
    while(vj) {
        const auto& stop_time_list = v.stop_time_list(vj);
        const auto& st_begin = *stop_time_list.first;
        const auto current_time = st_begin.section_end_time(v.clockwise(),
                                DateTimeUtils::hour(workingDt));
        DateTimeUtils::update(workingDt, current_time, v.clockwise());
        // If the vj is not valid for the first stop it won't be valid at all
        if (!st_begin.is_valid_day(DateTimeUtils::date(workingDt), !v.clockwise(), disruption_active)) {
            return result;
        }
        BOOST_FOREACH(const type::StopTime& st, stop_time_list) {
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
            auto jpp = st.journey_pattern_point;
            const DateTime bound = (v.comp(best_labels[jpp->idx], b_dest.best_now) || !global_pruning) ?
                                        best_labels[jpp->idx] : b_dest.best_now;
            if(!v.comp(workingDt, bound)) {
                continue;
            }
            working_labels.mut_dt_pt(jpp->idx) = workingDt;
            working_labels.mut_boarding_jpp_pt(jpp->idx) = state.boarding_jpp_idx;
            best_labels[jpp->idx] = working_labels.dt_pt(jpp->idx);
            add_vj = true;
            this->b_dest.add_best(v, jpp->idx, working_labels.dt_pt(jpp->idx), this->count);
            auto& best_jpp = best_jpp_by_sp[jpp->stop_point->idx];
            if(best_jpp == type::invalid_idx || v.comp(workingDt, working_labels.dt_pt(best_jpp))) {
                best_jpp = jpp->idx;
                result = true;
            }
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
bool RAPTOR::foot_path(const Visitor & v) {
    bool result = false;
    auto &working_labels = labels[count];
    // Since we don't stop on a journey_pattern_point we don't really care about
    // accessibility here, it'll be check in the public transport part
    type::idx_t sp_idx = 0;
    const auto best_jpp_end = best_jpp_by_sp.end();
    for (auto best_jpp_it = best_jpp_by_sp.begin(); best_jpp_it != best_jpp_end; ++best_jpp_it, ++sp_idx) {
        const auto best_jpp_idx = *best_jpp_it;
        if(best_jpp_idx == type::invalid_idx) {
            continue;
        }

        // Now we apply all the connections
        const DateTime previous = working_labels.dt_pt(best_jpp_idx);
        const auto best_is_odt = data.pt_data->journey_pattern_points[best_jpp_idx]->journey_pattern->odt_properties.is_zonal_odt();

        const auto& conns = v.clockwise() ?
            data.dataRaptor->connections.get_forward(sp_idx) :
            data.dataRaptor->connections.get_backward(sp_idx);
        for (const auto& conn: conns) {
            const DateTime next = v.combine(previous, conn.duration);
            for(const auto& jpp: jpps_from_sp[conn.sp_idx]) {
                if (jpp.idx == best_jpp_idx) { continue; }
                if (! v.comp(next, best_labels[jpp.idx])) { continue; }
                if (best_is_odt) {
                    const auto* jpp_ptr = data.pt_data->journey_pattern_points[jpp.idx];
                    if (jpp_ptr->journey_pattern->odt_properties.is_zonal_odt()) { continue; }
                }

                working_labels.mut_dt_transfer(jpp.idx) = next;
                working_labels.mut_boarding_jpp_transfer(jpp.idx) = best_jpp_idx;
                best_labels[jpp.idx] = next;
                if(v.comp(jpp.order, Q[jpp.jp_idx])) {
                    Q[jpp.jp_idx] = jpp.order;
                    result = true;
                }
            }
        }
    }
    return result;
}


void RAPTOR::clear(const bool clockwise, const DateTime bound) {
    const int queue_value = clockwise ?  std::numeric_limits<int>::max() : -1;
    memset32<int>(&Q[0],  data.pt_data->journey_patterns.size(), queue_value);
    if (labels.empty()) {
        labels.resize(5);
    }
    const Labels& clean_labels =
        clockwise ? data.dataRaptor->labels_const : data.dataRaptor->labels_const_reverse;
    for(auto& lbl_list : labels) {
        lbl_list.clear(clean_labels);
    }

    const size_t journey_pattern_points_size = data.pt_data->journey_pattern_points.size();
    b_dest.reinit(journey_pattern_points_size, bound);
    this->make_queue();
    if(clockwise)
        std::fill(best_labels.begin(), best_labels.end(), DateTimeUtils::inf);
    else
        std::fill(best_labels.begin(), best_labels.end(), DateTimeUtils::min);
}


void RAPTOR::init(Solutions departs,
                  const std::vector<std::pair<type::idx_t, navitia::time_duration> >& destinations,
                  DateTime bound,  const bool clockwise,
                  const type::Properties &required_properties) {
    for(Solution item : departs) {
        if (! valid_journey_pattern_points[item.jpp_idx]) { continue; }
        const type::JourneyPatternPoint* journey_pattern_point = data.pt_data->journey_pattern_points[item.jpp_idx];
        const type::StopPoint* stop_point = journey_pattern_point->stop_point;
        if(stop_point->accessible(required_properties) &&
                ((clockwise && item.arrival <= bound) || (!clockwise && item.arrival >= bound))) {
            labels[0].mut_dt_transfer(item.jpp_idx) = item.arrival;

            if(clockwise && Q[journey_pattern_point->journey_pattern->idx] > journey_pattern_point->order)
                Q[journey_pattern_point->journey_pattern->idx] = journey_pattern_point->order;
            else if(!clockwise &&  Q[journey_pattern_point->journey_pattern->idx] < journey_pattern_point->order)
                Q[journey_pattern_point->journey_pattern->idx] = journey_pattern_point->order;
        }
    }

    for(auto item : destinations) {
        const type::StopPoint* sp = data.pt_data->stop_points[item.first];
        if(sp->accessible(required_properties)) {
            for(auto journey_pattern_point : sp->journey_pattern_point_list) {
                if(valid_journey_pattern_points.test(journey_pattern_point->idx)) {
                    b_dest.add_destination(journey_pattern_point, item.second);
                }
            }
        }
    }
}

std::vector<Path>
RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departures_,
                    const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations,
                    const DateTime &departure_datetime,
                    bool disruption_active, bool allow_odt,
                    const DateTime &bound,
                    const uint32_t max_transfers,
                    const type::AccessibiliteParams & accessibilite_params,
                    const std::vector<std::string> & forbidden_uri,
                    bool clockwise) {

    std::vector<Path> result;
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden_uri,
                         disruption_active,
                         allow_odt,
                         departures_,
                         destinations);

    auto calc_dep = clockwise ? departures_ : destinations;
    auto calc_dest = clockwise ? destinations : departures_;

    auto departures = get_solutions(calc_dep, departure_datetime, clockwise, data, disruption_active);
    clear(clockwise, bound);
    init(departures, calc_dest, bound, clockwise);

    boucleRAPTOR(accessibilite_params, clockwise, disruption_active, false, max_transfers);
    /// @todo put that commented lines in a ifdef only compiled when we want
    //auto tmp = makePathes(calc_dep, calc_dest, accessibilite_params, *this, clockwise, disruption_active);
    //result.insert(result.end(), tmp.begin(), tmp.end());
    // No solution found, or the solution has initialize with init
    if(b_dest.best_now_jpp_idx == type::invalid_idx || b_dest.count == 0) {
        return result;
    }

    // Second phase
    // If we asked for a earliest arrival time, we now try to find the tardiest departure time
    // and vice and versa
    departures = get_solutions(calc_dep, calc_dest, !clockwise,
                               accessibilite_params, disruption_active, *this);
    for(auto departure : departures) {
        clear(!clockwise, departure_datetime);
        init({departure}, calc_dep, departure_datetime, !clockwise);

        boucleRAPTOR(accessibilite_params, !clockwise, disruption_active, true, max_transfers);

        if(b_dest.best_now_jpp_idx == type::invalid_idx) {
            continue;
        }
        std::vector<Path> temp = makePathes(calc_dest, calc_dep, accessibilite_params, *this, !clockwise, disruption_active);

        using boost::adaptors::filtered;
        boost::push_back(result, temp | filtered([&](const Path& p) {
            // We filter invalid solutions (that will begin before the departure date)
            const auto& end_item = clockwise ? p.items.front() : p.items.back();
            const auto* stop_time = clockwise ? end_item.stop_times.front() : end_item.stop_times.back();
            type::idx_t end_idx = stop_time->journey_pattern_point->stop_point->idx;
            typedef std::pair<type::idx_t, navitia::time_duration> idx_dur;
            const auto walking_time_search = boost::find_if(calc_dep, [&](const idx_dur& elt) {
                return elt.first == end_idx;
            });
            BOOST_ASSERT(walking_time_search != calc_dep.end());
            const auto& cur_end = clockwise ? end_item.departure : end_item.arrival;
            return clockwise
                ? to_posix_time(departure_datetime + walking_time_search->second.total_seconds(), data)
                  <= cur_end
                : to_posix_time(departure_datetime - walking_time_search->second.total_seconds(), data)
                  >= cur_end;
        }));
    }
    BOOST_ASSERT( departures.size() > 0 );    //Assert that reversal search was symetric
    return result;
}

std::vector<std::pair<type::EntryPoint, std::vector<Path>>>
RAPTOR::compute_nm_all(const std::vector<std::pair<type::EntryPoint, std::vector<std::pair<type::idx_t, navitia::time_duration> > > > &departures,
                       const std::vector<std::pair<type::EntryPoint, std::vector<std::pair<type::idx_t, navitia::time_duration> > > > &arrivals,
                       const DateTime &departure_datetime,
                       bool disruption_active, bool allow_odt,
                       const DateTime &bound,
                       const uint32_t max_transfers,
                       const type::AccessibiliteParams & accessibilite_params,
                       const std::vector<std::string> & forbidden_uri,
                       bool clockwise, bool details) {
    std::vector<std::pair<type::EntryPoint, std::vector<Path>>> result;
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden_uri,
                         disruption_active,
                         allow_odt);

    const auto& n_points = clockwise ? departures : arrivals;

    std::vector<std::pair<type::idx_t, navitia::time_duration> > calc_dep;
    for(const auto& n_point : n_points)
        for (const auto& n_stop_point : n_point.second)
            calc_dep.push_back(n_stop_point);

    auto calc_dep_solutions = get_solutions(calc_dep, departure_datetime, clockwise, data, disruption_active);
    clear(clockwise, bound);
    init(calc_dep_solutions, {}, bound, clockwise); // no exit condition (should be improved)

    boucleRAPTOR(accessibilite_params, clockwise, disruption_active, false, max_transfers);

    const auto& m_points = clockwise ? arrivals : departures;

    for(const auto& m_point : m_points) {
        const type::EntryPoint& m_entry_point = m_point.first;

        std::vector<Path> paths;

        if (!details) {
          // just returns destination stop points
          for (auto& m_stop_point : m_point.second) {
              Path path;
              path.nb_changes = 0;
              PathItem path_item;
              navitia::type::StopPoint* sp = data.pt_data->stop_points[m_stop_point.first];
              path_item.stop_points.push_back(sp);
              path_item.type = public_transport;
              path.items.push_back(path_item);
              paths.push_back(path);
          }
        }
        else {
            const auto& calc_arr = m_point.second;
            paths = makePathes(calc_dep, calc_arr, accessibilite_params, *this, clockwise, disruption_active);

            for(Path& path : paths){
                path.origin.type = nt::Type_e::Unknown;

                if (path.items.empty())
                    continue;

                const PathItem& path_item = clockwise ? path.items.front() : path.items.back();
                if (path_item.stop_points.empty())
                    continue;

                // must find which item of calc_dep has been computed
                const nt::StopPoint* stop_point = clockwise ? path_item.stop_points.front() : path_item.stop_points.back();
                for(const auto& n_point : n_points) {
                    for(const auto& n_stop_point : n_point.second)
                        if (stop_point->idx == n_stop_point.first) {
                            path.origin = n_point.first;
                            break;
                        }
                    if (path.origin.type != nt::Type_e::Unknown)
                        break;
                }
            }
        }
        result.push_back(std::make_pair(m_entry_point, paths));
    }

    return result;
}

void
RAPTOR::isochrone(const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departures_,
          const DateTime &departure_datetime, const DateTime &bound, uint32_t max_transfers,
          const type::AccessibiliteParams & accessibilite_params,
          const std::vector<std::string> & forbidden,
          bool clockwise, bool disruption_active, bool allow_odt) {
    set_valid_jp_and_jpp(DateTimeUtils::date(departure_datetime),
                         accessibilite_params,
                         forbidden,
                         disruption_active,
                         allow_odt);
    auto departures = get_solutions(departures_, departure_datetime, true, data, disruption_active);
    clear(clockwise, bound);
    init(departures, {}, bound, true);

    boucleRAPTOR(accessibilite_params, clockwise, true, max_transfers);
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
                const auto sp = data.pt_data->stop_points[sp_duration.first];
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
    for (const auto* sp: data.pt_data->stop_points) {
        if (sp->accessible(accessibilite_params.properties)) { continue; }
        for (const auto* jpp: sp->journey_pattern_point_list) {
            valid_journey_pattern_points.set(jpp->idx, false);
        }
    }

    // propagate the invalid jp in their jpp
    for (type::idx_t jp_idx = 0; jp_idx < valid_journey_patterns.size(); ++jp_idx) {
        if (valid_journey_patterns[jp_idx]) { continue; }
        const auto* jp = data.pt_data->journey_patterns[jp_idx];
        for (const auto* jpp: jp->journey_pattern_point_list) {
            valid_journey_pattern_points.set(jpp->idx, false);
        }
    }

    jpps_from_sp = data.dataRaptor->jpps_from_sp;
    jpps_from_sp.filter_jpps(valid_journey_pattern_points);
}

template<typename Visitor>
void RAPTOR::raptor_loop(Visitor visitor, const type::AccessibiliteParams & accessibilite_params, bool disruption_active,
        bool global_pruning, uint32_t max_transfers) {
    bool end_algorithm = false;
    count = 0; //< Count iteration of raptor algorithm

    while(!end_algorithm && count <= max_transfers) {
        ++count;
        end_algorithm = true;
        if(count == labels.size()) {
            if(visitor.clockwise()) {
                this->labels.push_back(this->data.dataRaptor->labels_const);
            } else {
                this->labels.push_back(this->data.dataRaptor->labels_const_reverse);
            }
        }
        const auto & prec_labels=labels[count -1];
        auto& working_labels = labels[this->count];
        this->make_queue();
        /*
         * We need to store it so we can apply stay_in after applying normal vjs
         * We want to do it, to favoritize normal vj against stay_in vjs
         */
        std::vector<RoutingState> states_stay_in;
        const type::idx_t nb_jp = data.pt_data->journey_patterns.size();
        for (type::idx_t jp_idx = 0; jp_idx < nb_jp; ++jp_idx) {
            if(Q[jp_idx] != visitor.init_queue_item()) {
                type::idx_t boarding_idx = type::invalid_idx; //< The boarding journey pattern point
                DateTime workingDt = visitor.worst_datetime();
                typename Visitor::stop_time_iterator it_st;
                uint16_t l_zone = std::numeric_limits<uint16_t>::max();
                const auto* journey_pattern = data.pt_data->journey_patterns[jp_idx];
                const auto & jpp_to_explore = visitor.journey_pattern_points(
                                                this->data.pt_data->journey_pattern_points,
                                                journey_pattern,Q[jp_idx]);

                BOOST_FOREACH(const type::JourneyPatternPoint* jpp, jpp_to_explore) {
                    type::idx_t jpp_idx = jpp->idx;
                    if(boarding_idx != type::invalid_idx) {
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
                            const DateTime bound = (visitor.comp(best_labels[jpp_idx], b_dest.best_now) || !global_pruning) ?
                                                        best_labels[jpp_idx] : b_dest.best_now;
                            // We want to update the labels, if it's better than the one computed before
                            // Or if it's an destination point if it's equal and not unitialized before
                            const bool best_add_result = this->b_dest.add_best(visitor, jpp_idx, workingDt, this->count);
                            if(visitor.comp(workingDt, bound) || best_add_result ) {
                                working_labels.mut_dt_pt(jpp_idx) = workingDt;
                                working_labels.mut_boarding_jpp_pt(jpp_idx) = boarding_idx;
                                best_labels[jpp_idx] = working_labels.dt_pt(jpp_idx);
                                auto& best_jpp = best_jpp_by_sp[jpp->stop_point->idx];
                                if(best_jpp == type::invalid_idx || visitor.comp(workingDt, working_labels.dt_pt(best_jpp))) {
                                    best_jpp = jpp_idx;
                                    end_algorithm = false;
                                }
                            }
                        }
                    }

                    // We try to get on a vehicle, if we were already on a vehicle, but we arrived
                    // before on the previous via a connection, we try to catch a vehicle leaving this
                    // journey pattern point before
                    const DateTime previous_dt = prec_labels.dt_transfer(jpp_idx);
                    if(prec_labels.transfer_is_initialized(jpp_idx) &&
                       (boarding_idx == type::invalid_idx || visitor.better_or_equal(previous_dt, workingDt, *it_st))) {
                        const auto tmp_st_dt = best_stop_time(jpp, previous_dt,
                                               accessibilite_params.vehicle_properties,
                                               visitor.clockwise(), disruption_active, data);

                        if(tmp_st_dt.first != nullptr && (boarding_idx == type::invalid_idx || tmp_st_dt.first != &*it_st || tmp_st_dt.second != workingDt)) {
                            boarding_idx = jpp_idx;
                            it_st = visitor.first_stoptime(*tmp_st_dt.first);
                            workingDt = tmp_st_dt.second;
                            BOOST_ASSERT(visitor.comp(previous_dt, workingDt) || previous_dt == workingDt);
                            l_zone = it_st->local_traffic_zone;
                        }
                    }
                }
                if(boarding_idx != type::invalid_idx) {
                    const type::VehicleJourney* vj_stay_in = visitor.get_extension_vj(it_st->vehicle_journey);
                    if (vj_stay_in) {
                        states_stay_in.emplace_back(vj_stay_in, boarding_idx, l_zone, workingDt);
                    }
                }
            }
            Q[jp_idx] = visitor.init_queue_item();
        }
        for (auto state : states_stay_in) {
            end_algorithm &= !apply_vj_extension(visitor, global_pruning, disruption_active, state);
        }
        end_algorithm &= !this->foot_path(visitor);
    }
}


void RAPTOR::boucleRAPTOR(const type::AccessibiliteParams & accessibilite_params, bool clockwise, bool disruption_active,
                          bool global_pruning, uint32_t max_transfers){
    if(clockwise) {
        raptor_loop(raptor_visitor(), accessibilite_params, disruption_active, global_pruning, max_transfers);
    } else {
        raptor_loop(raptor_reverse_visitor(), accessibilite_params, disruption_active, global_pruning, max_transfers);
    }
}


std::vector<Path> RAPTOR::compute(const type::StopArea* departure,
        const type::StopArea* destination, int departure_hour,
        int departure_day, DateTime borne, bool disruption_active, bool allow_odt, bool clockwise,
        const type::AccessibiliteParams & accessibilite_params,
        uint32_t max_transfers, const std::vector<std::string>& forbidden_uri) {
    std::vector<std::pair<type::idx_t, navitia::time_duration> > departures, destinations;

    for(const type::StopPoint* sp : departure->stop_point_list) {
        departures.push_back({sp->idx, {}});
    }

    for(const type::StopPoint* sp : destination->stop_point_list) {
        destinations.push_back({sp->idx, {}});
    }

    return compute_all(departures, destinations, DateTimeUtils::set(departure_day, departure_hour),
                       disruption_active, allow_odt, borne, max_transfers, accessibilite_params, forbidden_uri, clockwise);
}


int RAPTOR::best_round(type::idx_t journey_pattern_point_idx){
    for(size_t i = 0; i <= this->count; ++i){
        if(labels[i].mut_dt_pt(journey_pattern_point_idx) == best_labels[journey_pattern_point_idx]){
            return i;
        }
    }
    return -1;
}

}}
