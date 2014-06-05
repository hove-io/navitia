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

namespace bt = boost::posix_time;

namespace navitia { namespace routing {

void RAPTOR::make_queue() {
    marked_sp.reset();
}

/*
 * Check if the given vj is valid for the given datetime,
 * If it is for every stoptime of the vj,
 * If the journey_pattern_point associated to it is improved by this stop time
 * we mark it.
 * If the given vj also has an extension we apply it.
 */
template<typename Visitor>
void RAPTOR::apply_vj_extension(const Visitor& v, const bool global_pruning,
                                const type::VehicleJourney* prev_vj, type::idx_t boarding_jpp_idx,
                                DateTime workingDt, const uint16_t l_zone,
                                const bool disruption_active) {
    auto& working_labels = labels[count];
    const type::VehicleJourney* vj = v.get_extension_vj(prev_vj);
    bool add_vj = false;
    while(vj) {
        BOOST_FOREACH(type::StopTime* st, v.stop_time_list(vj)) {
            if(!st->valid_end(v.clockwise()) ||
                    !st->is_valid_day(DateTimeUtils::date(workingDt), !v.clockwise(), disruption_active)) {
                continue;
            }
            if(l_zone != std::numeric_limits<uint16_t>::max() &&
               l_zone == st->local_traffic_zone) {
                continue;
            }
            auto jpp = st->journey_pattern_point;
            const auto current_time = st->section_end_time(v.clockwise(),
                                    DateTimeUtils::hour(workingDt));
            DateTimeUtils::update(workingDt, current_time, v.clockwise());
            const DateTime bound = (v.comp(best_labels[jpp->idx], b_dest.best_now) || !global_pruning) ?
                                        best_labels[jpp->idx] : b_dest.best_now;
            if(!v.comp(workingDt, bound)) {
                continue;
            }
            working_labels[jpp->idx].dt = workingDt;
            working_labels[jpp->idx].boarding_jpp = boarding_jpp_idx;
            working_labels[jpp->idx].type = boarding_type::connection_stay_in;
            best_labels[jpp->idx] = working_labels[jpp->idx].dt;
            add_vj = true;
            if(!this->b_dest.add_best(v, jpp->idx, working_labels[jpp->idx].dt, this->count)) {
                this->marked_sp.set(jpp->stop_point->idx);
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
}


template<typename Visitor>
void RAPTOR::foot_path(const Visitor & v, const type::Properties &required_properties) {

    int last = 0;
    const auto foot_path_list = v.clockwise() ? data.dataRaptor->foot_path_forward :
                                                data.dataRaptor->foot_path_backward;
    auto it = foot_path_list.begin();
    auto &current_labels = labels[count];
    for(auto stop_point_idx = marked_sp.find_first(); stop_point_idx != marked_sp.npos;
        stop_point_idx = marked_sp.find_next(stop_point_idx)) {
        //On cherche le meilleur jpp du stop point
        const type::StopPoint* stop_point = data.pt_data->stop_points[stop_point_idx];
        if(stop_point->accessible(required_properties)) {
            DateTime best_arrival = v.worst_datetime();
            type::idx_t best_jpp = type::invalid_idx;

            for(auto journey_pattern_point : stop_point->journey_pattern_point_list) {
                type::idx_t jppidx = journey_pattern_point->idx;
                boarding_type b_type = get_type(count, jppidx);
                //On regarde si on est arrivé avec un vj ou un departure,
                //Puis on compare avec la meilleure arrivée trouvée pour ce stoppoint
                if((b_type == boarding_type::vj || b_type == boarding_type::departure ||
                    b_type == boarding_type::connection_stay_in) &&
                    v.comp(current_labels[jppidx].dt, best_arrival)) {
                    best_arrival = current_labels[jppidx].dt;
                    best_jpp = jppidx;
                }
            }
            // Si on a trouvé un journey pattern pour ce stop point
            // NB : l'inverse arrive lorsqu'on a déjà marqué le stop point avec une autre correspondance
            if(best_jpp != type::invalid_idx) {
                const DateTime best_departure = v.combine(best_arrival, 120);
                //On marque tous les journey_pattern points du stop point
                for(auto jpp : stop_point->journey_pattern_point_list) {
                    type::idx_t jpp_idx = jpp->idx;
                    if(jpp_idx != best_jpp && v.comp(best_departure, best_labels[jpp_idx])) {
                       current_labels[jpp_idx].dt = best_departure;
                       current_labels[jpp_idx].boarding_jpp = best_jpp;
                       current_labels[jpp_idx].type = boarding_type::connection;
                       best_labels[jpp_idx] = best_departure;

                       if(v.comp(jpp->order, Q[jpp->journey_pattern->idx])) {
                           Q[jpp->journey_pattern->idx] = jpp->order;
                       }
                    }
                }
                //On va maintenant chercher toutes les connexions et on marque tous les journey_pattern_points concernés
                //On récupère l'index dans les footpath
                const pair_int & index = (v.clockwise()) ? data.dataRaptor->footpath_index_forward[stop_point_idx] :
                                                         data.dataRaptor->footpath_index_backward[stop_point_idx];
                //int prec_duration = -1;
                DateTime next = v.worst_datetime(),
                         previous = current_labels[best_jpp].dt;
                it += index.first - last;
                const auto end = it + index.second;

                for(; it != end; ++it) {
                    const type::StopPointConnection* spc = *it;
                    const auto destination = v.clockwise() ? spc->destination : spc->departure;
                    next = v.combine(previous, spc->duration); // ludo
                    if(destination->accessible(required_properties)) {
                        for(auto destination_jpp : destination->journey_pattern_point_list) {
                            type::idx_t destination_jpp_idx = destination_jpp->idx;
                            if(best_jpp != destination_jpp_idx) {
                                if(v.comp(next, best_labels[destination_jpp_idx]) || next == best_labels[destination_jpp_idx]) {
                                    current_labels[destination_jpp_idx].dt = next;
                                    current_labels[destination_jpp_idx].boarding_jpp = best_jpp;
                                    current_labels[destination_jpp_idx].type = boarding_type::connection;
                                    best_labels[destination_jpp_idx] = next;

                                    if(v.comp(destination_jpp->order, Q[destination_jpp->journey_pattern->idx])) {
                                        Q[destination_jpp->journey_pattern->idx] = destination_jpp->order;
                                   }
                                }
                            }
                        }
                    }
                }
                last = index.first + index.second;
            }
        }
    }
}


void RAPTOR::clear(const type::Data & data, bool clockwise, DateTime borne) {
    if(clockwise) {
        //Q.assign(data.pt_data->journey_patterns.size(), std::numeric_limits<int>::max());
        memset32<int>(&Q[0], data.pt_data->journey_patterns.size(), std::numeric_limits<int>::max());
        labels.resize(10);
        labels[0] = data.dataRaptor->labels_const;
    } else {
        //Q.assign(data.pt_data->journey_patterns.size(), -1);
        memset32<int>(&Q[0], data.pt_data->journey_patterns.size(), -1);
        labels.resize(10);
        labels[0] = data.dataRaptor->labels_const_reverse;
    }
    for(auto& lbl_list : labels) {
        for(Label& l : lbl_list) {
            l.type = boarding_type::uninitialized;
            l.dt = clockwise ? DateTimeUtils::inf : DateTimeUtils::min;
            l.boarding_jpp = type::invalid_idx;
        }
    }

    b_dest.reinit(data.pt_data->journey_pattern_points.size(), borne);
    this->make_queue();
    if(clockwise)
        best_labels.assign(data.pt_data->journey_pattern_points.size(), DateTimeUtils::inf);
    else
        best_labels.assign(data.pt_data->journey_pattern_points.size(), DateTimeUtils::min);
}

void RAPTOR::clear_and_init(Solutions departs,
                  std::vector<std::pair<type::idx_t, navitia::time_duration> > destinations,
                  DateTime bound,  const bool clockwise,
                  const type::Properties &required_properties) {

    this->clear(data, clockwise, bound);

    for(Solution item : departs) {
        const type::JourneyPatternPoint* journey_pattern_point = data.pt_data->journey_pattern_points[item.rpidx];
        const type::StopPoint* stop_point = journey_pattern_point->stop_point;
        if(stop_point->accessible(required_properties) &&
                ((clockwise && item.arrival <= bound) || (!clockwise && item.arrival >= bound))) {
            labels[0][item.rpidx].dt = item.arrival;
            labels[0][item.rpidx].type = boarding_type::departure;
            best_labels[item.rpidx] = item.arrival;

            if(clockwise && Q[journey_pattern_point->journey_pattern->idx] > journey_pattern_point->order)
                Q[journey_pattern_point->journey_pattern->idx] = journey_pattern_point->order;
            else if(!clockwise &&  Q[journey_pattern_point->journey_pattern->idx] < journey_pattern_point->order)
                Q[journey_pattern_point->journey_pattern->idx] = journey_pattern_point->order;
            if(item.arrival != DateTimeUtils::min && item.arrival != DateTimeUtils::inf) {
                marked_sp.set(stop_point->idx);
            }
        }
    }

    for(auto item : destinations) {
        const type::StopPoint* sp = data.pt_data->stop_points[item.first];
        if(sp->accessible(required_properties)) {
            for(auto journey_pattern_point : sp->journey_pattern_point_list) {
                type::idx_t jppidx = journey_pattern_point->idx;
                if(journey_patterns_valides.test(journey_pattern_point->journey_pattern->idx)) {
                        b_dest.add_destination(jppidx, item.second, clockwise);
                        best_labels[jppidx] = clockwise ?
                                    std::min(bound, labels[0][jppidx].dt) :
                                    std::max(bound, labels[0][jppidx].dt);
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
                    const std::vector<std::string> & forbidden,
                    bool clockwise) {
    std::vector<Path> result;
    set_journey_patterns_valides(DateTimeUtils::date(departure_datetime), forbidden, disruption_active, allow_odt);

    auto calc_dep = clockwise ? departures_ : destinations;
    auto calc_dest = clockwise ? destinations : departures_;

    auto departures = get_solutions(calc_dep, departure_datetime, clockwise, data, disruption_active);
    clear_and_init(departures, calc_dest, bound, clockwise);

    boucleRAPTOR(accessibilite_params, clockwise, disruption_active, false, max_transfers);
    //auto tmp = makePathes(calc_dep, calc_dest, accessibilite_params, *this, clockwise, disruption_active);
    //result.insert(result.end(), tmp.begin(), tmp.end());
    // Aucune solution n’a été trouvée :'(
    if(b_dest.best_now_jpp_idx == type::invalid_idx) {
        return result;
    }

    // Second phase
    // If we asked for a earliest arrival time, we now try to find the tardiest departure time
    // and vice and versa
    departures = get_solutions(calc_dep, calc_dest, !clockwise,
                               accessibilite_params, disruption_active, *this);
    for(auto departure : departures) {
        clear_and_init({departure}, calc_dep, departure_datetime, !clockwise);

        boucleRAPTOR(accessibilite_params, !clockwise, disruption_active, true, max_transfers);

        if(b_dest.best_now_jpp_idx == type::invalid_idx) {
            continue;
        }
        std::vector<Path> temp = makePathes(calc_dest, calc_dep, accessibilite_params, *this, !clockwise, disruption_active);
        result.insert(result.end(), temp.begin(), temp.end());
    }
    return result;
}


void
RAPTOR::isochrone(const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departures_,
          const DateTime &departure_datetime, const DateTime &bound, uint32_t max_transfers,
          const type::AccessibiliteParams & accessibilite_params,
          const std::vector<std::string> & forbidden,
          bool clockwise, bool disruption_active, bool allow_odt) {
    set_journey_patterns_valides(DateTimeUtils::date(departure_datetime), forbidden, disruption_active, allow_odt);
    auto departures = get_solutions(departures_, departure_datetime, true, data, disruption_active);
    clear_and_init(departures, {}, bound, true);

    boucleRAPTOR(accessibilite_params, clockwise, true, max_transfers);
}


void RAPTOR::set_journey_patterns_valides(uint32_t date, const std::vector<std::string> & forbidden,
                                          bool disruption_active,
                                          bool allow_odt) {

    if(disruption_active){
        journey_patterns_valides = data.dataRaptor->jp_adapted_validity_pattern[date];
    }else{
        journey_patterns_valides = data.dataRaptor->jp_validity_patterns[date];
    }
    boost::dynamic_bitset<> forbidden_journey_patterns(data.pt_data->journey_patterns.size());
    for(const type::JourneyPattern* journey_pattern : data.pt_data->journey_patterns) {
        if ((!allow_odt) && (journey_pattern->is_odt)){
            forbidden_journey_patterns.set(journey_pattern->idx);
            continue;
        }
        const type::Line* line = journey_pattern->route->line;
        // On gère la liste des interdits
        for(auto forbid_uri : forbidden){
            if       ( (line && forbid_uri == line->uri)
                    || (journey_pattern->route && forbid_uri == journey_pattern->route->uri)
                    || (forbid_uri == journey_pattern->uri)
                    || (journey_pattern->commercial_mode && forbid_uri == journey_pattern->commercial_mode->uri)
                    || (journey_pattern->physical_mode && forbid_uri == journey_pattern->physical_mode->uri)
                    || (line->network && forbid_uri == line->network->uri) )
            {
                forbidden_journey_patterns.set(journey_pattern->idx);
                break;
            }
        }
    }
    journey_patterns_valides &= ~forbidden_journey_patterns;
}

template<typename Visitor>
void RAPTOR::raptor_loop(Visitor visitor, const type::AccessibiliteParams & accessibilite_params, bool disruption_active,
        bool global_pruning, uint32_t max_transfers) {
    bool end = false;
    count = 0; //< Count iteration of raptor algorithm

    while(!end && count <= max_transfers) {
        ++count;
        end = true;
        if(count == labels.size()) {
            if(visitor.clockwise()) {
                this->labels.push_back(this->data.dataRaptor->labels_const);
            } else {
                this->labels.push_back(this->data.dataRaptor->labels_const_reverse);
            }
        }
        const auto & prec_labels=labels[count -1];
        auto & working_labels = labels[this->count];
        this->make_queue();

        for(const auto & journey_pattern : data.pt_data->journey_patterns) {
            if(Q[journey_pattern->idx] != std::numeric_limits<int>::max()
                    && Q[journey_pattern->idx] != -1
                    && journey_patterns_valides.test(journey_pattern->idx)) {
                const type::JourneyPatternPoint* boarding = nullptr; //< The boarding journey pattern point
                DateTime workingDt = visitor.worst_datetime();
                typename Visitor::stop_time_iterator it_st;
                uint16_t l_zone = std::numeric_limits<uint16_t>::max();
                const auto & jpp_to_explore = visitor.journey_pattern_points(
                                                this->data.pt_data->journey_pattern_points,
                                                journey_pattern,Q[journey_pattern->idx]);

                BOOST_FOREACH(const type::JourneyPatternPoint* jpp, jpp_to_explore) {
                    if(!jpp->stop_point->accessible(accessibilite_params.properties)) {
                        continue;
                    }
                    type::idx_t jpp_idx = jpp->idx;
                    if(boarding != nullptr) {
                        ++it_st;
                        // We update workingDt with the new arrival time
                        // We need at each journey pattern point when we have a st
                        // If we don't it might cause problem with overmidnight vj
                        const type::StopTime* st = *it_st;
                        const auto current_time = st->section_end_time(visitor.clockwise(),
                                                DateTimeUtils::hour(workingDt));
                        DateTimeUtils::update(workingDt, current_time, visitor.clockwise());
                        // We check if there are no drop_off_only and if the local_zone is okay
                        if(st->valid_end(visitor.clockwise())&&
                                (l_zone == std::numeric_limits<uint16_t>::max() ||
                                 l_zone != st->local_traffic_zone)) {
                            const DateTime bound = (visitor.comp(best_labels[jpp_idx], b_dest.best_now) || !global_pruning) ?
                                                        best_labels[jpp_idx] : b_dest.best_now;
                            // We want to update the labels, if it's better than the one computed before
                            // Or if it's an destination point if it's equal and not unitialized before
                            const bool best_add_result = this->b_dest.add_best(visitor, jpp->idx, workingDt, this->count);
                            if(visitor.comp(workingDt, bound) ||
                                    (best_add_result && get_type(this->count-1, jpp_idx) == boarding_type::uninitialized)) {
                                working_labels[jpp_idx].dt = workingDt;
                                working_labels[jpp_idx].boarding_jpp = boarding->idx;
                                working_labels[jpp_idx].type = boarding_type::vj;
                                best_labels[jpp_idx] = working_labels[jpp_idx].dt;
                                // We want to apply connection only if it's not a destination point
                                if(!best_add_result) {
                                    this->marked_sp.set(jpp->stop_point->idx);
                                    end = false;
                                }
                            }
                        }
                    }

                    // We try to get on a vehicle, if we were already on a vehicle, but we arrived
                    // before on the previous via a connection, we try to catch a vehicle leaving this
                    // journey pattern point before
                    const DateTime previous_dt = prec_labels[jpp_idx].dt;
                    const boarding_type b_type = get_type(this->count-1, jpp_idx);
                    if((b_type == boarding_type::connection || b_type == boarding_type::departure) &&
                       (boarding == nullptr || visitor.better_or_equal(previous_dt, workingDt, *it_st))) {
                        const auto tmp_st_dt = best_stop_time(jpp, previous_dt,
                                               accessibilite_params.vehicle_properties,
                                               visitor.clockwise(), disruption_active, data);

                        if(tmp_st_dt.first != nullptr && (boarding == nullptr || tmp_st_dt.first != *it_st)) {
                            boarding = jpp;
                            it_st = visitor.first_stoptime(tmp_st_dt.first);
                            workingDt = tmp_st_dt.second;
                            BOOST_ASSERT(visitor.comp(previous_dt, workingDt) || previous_dt == workingDt);
                            l_zone = (*it_st)->local_traffic_zone;
                        }
                    }
                }
                if(boarding) {
                    this->apply_vj_extension(visitor, global_pruning, (*it_st)->vehicle_journey, boarding->idx,
                                             workingDt, l_zone, disruption_active);
                }
            }
            Q[journey_pattern->idx] = visitor.init_queue_item();
        }
        this->foot_path(visitor, accessibilite_params.properties);
    }
}


void RAPTOR::boucleRAPTOR(const type::AccessibiliteParams & accessibilite_params, bool clockwise, bool disruption_active, bool global_pruning, uint32_t max_transfers){
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
        uint32_t max_transfers) {
    std::vector<std::pair<type::idx_t, navitia::time_duration> > departures, destinations;

    for(const type::StopPoint* sp : departure->stop_point_list) {
        departures.push_back({sp->idx, {}});
    }

    for(const type::StopPoint* sp : destination->stop_point_list) {
        destinations.push_back({sp->idx, {}});
    }

    return compute_all(departures, destinations, DateTimeUtils::set(departure_day, departure_hour), disruption_active, allow_odt, borne, max_transfers, accessibilite_params, {}, clockwise);
}


int RAPTOR::best_round(type::idx_t journey_pattern_point_idx){
    for(size_t i = 0; i < labels.size(); ++i){
        if(labels[i][journey_pattern_point_idx].dt == best_labels[journey_pattern_point_idx]){
            return i;
        }
    }
    return -1;
}

}}
