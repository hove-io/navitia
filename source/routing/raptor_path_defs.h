#pragma once
#include "raptor_path.h"
#include "type/type.h"
#include "raptor.h"


namespace navitia { namespace routing {

template<typename Visitor>
void handle_connection(const size_t countb, const navitia::type::idx_t current_jpp_idx, Visitor& v,
                       bool clockwise, const RAPTOR &raptor_) {
    auto departure = raptor_.data.pt_data->journey_pattern_points[current_jpp_idx]->stop_point;
    const auto boarding_jpp_idx = raptor_.get_boarding_jpp(countb, current_jpp_idx);
    auto destination_jpp = raptor_.data.pt_data->journey_pattern_points[boarding_jpp_idx];
    auto destination = destination_jpp->stop_point;
    auto connections = departure->stop_point_connection_list;
    auto l = raptor_.labels[countb][current_jpp_idx].dt;
    auto find_predicate = [&](type::StopPointConnection* connection)->bool {
        return departure == connection->departure && destination == connection->destination;
    };

    auto it = std::find_if(connections.begin(), connections.end(), find_predicate);
    type::StopPointConnection* stop_point_connection = nullptr;
    boost::posix_time::ptime dep_ptime, arr_ptime;
    if(it == connections.end()) {
        auto r2 = raptor_.labels[countb][raptor_.get_boarding_jpp(countb, current_jpp_idx)];
        if(clockwise) {
            dep_ptime = to_posix_time(r2.dt, raptor_.data);
            arr_ptime = to_posix_time(l, raptor_.data);
        } else {
            dep_ptime = to_posix_time(l, raptor_.data);
            arr_ptime = to_posix_time(r2.dt, raptor_.data);
        }
    } else {
        stop_point_connection = *it;
        const auto display_duration = stop_point_connection->display_duration;
        if(clockwise) {
            dep_ptime = to_posix_time(l - display_duration, raptor_.data);
            arr_ptime = to_posix_time(l, raptor_.data);
        } else {
            dep_ptime = to_posix_time(l, raptor_.data);
            arr_ptime = to_posix_time(l + display_duration, raptor_.data);
        }
    }

    v.connection(departure, destination, dep_ptime, arr_ptime, stop_point_connection);
}


template<typename Visitor>
void handle_vj(const size_t countb, const navitia::type::idx_t current_jpp_idx, Visitor& v,
               bool disruption_active, const type::AccessibiliteParams & accessibilite_params,
               const RAPTOR &raptor_) {
    v.init_vj();
    auto boarding_jpp = raptor_.get_boarding_jpp(countb, current_jpp_idx);
    const type::StopTime* current_st;
    DateTime workingDate;
    std::tie(current_st, workingDate) = get_current_stidx_gap(countb, current_jpp_idx, raptor_.labels,
                                                              accessibilite_params, clockwise,
                                                              raptor_.data, disruption_active);
    while(boarding_jpp != current_jpp_idx) {
        auto departure_arrival = handle_st(current_st, workingDate, clockwise, raptor_.data);
        v.loop_vj(current_st, departure_arrival.first, departure_arrival.second);
        size_t order = current_st->journey_pattern_point->order;
        if((clockwise && order > 0) || (!clockwise && order < (current_st->vehicle_journey->stop_time_list.size()-1))) {
            // On parcourt les données dans le sens contraire du calcul
            if(clockwise){
                order--;
            }
            else{
                order++;
            }
            current_st = current_st->vehicle_journey->stop_time_list[order];
        }
        else {
            auto prev_st = current_st;
            // If it was a connection_stay_in we want to take the the previous vj
            if(clockwise) {
                current_st = current_st->vehicle_journey->prev_vj->stop_time_list.back();
            } else {
                current_st = current_st->vehicle_journey->next_vj->stop_time_list.front();
            }
            auto prev_time = raptor_.labels[countb][prev_st->journey_pattern_point->idx].dt,
                 current_time = raptor_.labels[countb][current_st->journey_pattern_point->idx].dt;
            v.change_vj(prev_st, current_st, to_posix_time(prev_time, raptor_.data),
                        to_posix_time(current_time, raptor_.data), clockwise);
        }
        current_jpp_idx = current_st->journey_pattern_point->idx;
    }
    auto departure_arrival = handle_st(current_st, workingDate, clockwise, raptor_.data);
    v.loop_vj(current_st, departure_arrival.first, departure_arrival.second);
    --countb;
    boarding_jpp = navitia::type::invalid_idx ;
    v.finish_vj(clockwise);
}
}


template<typename Visitor>
void read_path(Visitor& v, type::idx_t destination_idx, size_t countb, bool clockwise, bool disruption_active,
          const type::AccessibiliteParams & accessibilite_params, const RAPTOR &raptor_) {
    type::idx_t current_jpp_idx = destination_idx;
    auto label_type = raptor_.get_type(countb, current_jpp_idx);
    while(label_type != boarding_type::departure) {
        if(label_type == boarding_type::connection) {
            handle_connection(countb, current_jpp_idx, v, clockwise, raptor_);
            current_jpp_idx = raptor_.get_boarding_jpp(countb, current_jpp_idx);
        } else if(label_type == boarding_type::vj ||
                  label_type == boarding_type::connection_stay_in) {

        label_type = raptor_.get_type(countb, current_jpp_idx);
    }
    v.final_step(current_jpp_idx, countb, raptor_.labels);
}



} }
