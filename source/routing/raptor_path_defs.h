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
    // We try to find the connection that was taken by the algorithm
    auto find_predicate = [&](type::StopPointConnection* connection)->bool {
        return departure == connection->departure && destination == connection->destination;
    };

    auto it = std::find_if(connections.begin(), connections.end(), find_predicate);
    type::StopPointConnection* stop_point_connection = nullptr;
    boost::posix_time::ptime dep_ptime, arr_ptime;
    // It might not be find, for instance if we stayed on the same stop point
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


/*
 * /!\ This function has a side effect on variable workingDate /!\
 * If clockwise, since we read the path on the reverse way we computed it,
 * we first want to update workingDate with departure_time, then with arrival_time.
 * And vice and versa with !clockwise
 * */
inline
std::pair<boost::posix_time::ptime, boost::posix_time::ptime>
handle_st(const type::StopTime* st, DateTime& workingDate, bool clockwise, const type::Data &data){
    boost::posix_time::ptime departure, arrival;
    if(clockwise) {
        if(st->is_frequency())
            DateTimeUtils::update(workingDate, st->f_departure_time(DateTimeUtils::hour(workingDate), !clockwise), !clockwise);
        else
            DateTimeUtils::update(workingDate, st->departure_time, !clockwise);
        departure = navitia::to_posix_time(workingDate, data);
        if(st->is_frequency())
            DateTimeUtils::update(workingDate, st->f_arrival_time(DateTimeUtils::hour(workingDate), !clockwise), !clockwise);
        else
            DateTimeUtils::update(workingDate, st->arrival_time, !clockwise);
        arrival = navitia::to_posix_time(workingDate, data);
    } else {
        if(st->is_frequency())
            DateTimeUtils::update(workingDate, st->f_arrival_time(DateTimeUtils::hour(workingDate), !clockwise), !clockwise);
        else
            DateTimeUtils::update(workingDate, st->arrival_time, !clockwise);
        arrival = navitia::to_posix_time(workingDate, data);
        if(st->is_frequency())
            DateTimeUtils::update(workingDate, st->f_departure_time(DateTimeUtils::hour(workingDate), !clockwise), !clockwise);
        else
            DateTimeUtils::update(workingDate, st->departure_time, !clockwise);
        departure = navitia::to_posix_time(workingDate, data);
    }
    return std::make_pair(departure, arrival);
}



template<typename Visitor>
void handle_vj(const size_t countb, navitia::type::idx_t current_jpp_idx, Visitor& v,
               bool clockwise, bool disruption_active, const type::AccessibiliteParams & accessibilite_params,
               const RAPTOR &raptor_) {
    v.init_vj();
    auto boarding_jpp_idx = raptor_.get_boarding_jpp(countb, current_jpp_idx);
    const type::StopTime* current_st;
    DateTime workingDate;
    std::tie(current_st, workingDate) = get_current_stidx_gap(countb, current_jpp_idx, raptor_.labels,
                                                              accessibilite_params, clockwise,
                                                              raptor_.data, disruption_active);
    while(boarding_jpp_idx != current_jpp_idx) {
        // There is a side effect on workingDate caused by workingDate
        auto departure_arrival = handle_st(current_st, workingDate, clockwise, raptor_.data);
        v.loop_vj(current_st, departure_arrival.first, departure_arrival.second);
        size_t order = current_st->journey_pattern_point->order;
        //If are still on the same vj
        if((clockwise && order > 0) || (!clockwise && order < (current_st->vehicle_journey->stop_time_list.size()-1))) {
            // We read labels with the reverse order than the one we used during computation
            if(clockwise){
                order--;
            }
            else{
                order++;
            }
            current_st = current_st->vehicle_journey->stop_time_list[order];
        }
        else {
            // Here we have to change of vehicle journey, this happen with connection_stay_in
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
    // There is a side effect on workingDate caused by workingDate
    auto departure_arrival = handle_st(current_st, workingDate, clockwise, raptor_.data);
    v.loop_vj(current_st, departure_arrival.first, departure_arrival.second);
    boarding_jpp_idx = navitia::type::invalid_idx ;
    v.finish_vj(clockwise);
}

/*
 * Start from a destination point until it reaches a departure label
 *
 * Call functions of the visitor along the way.
 *
 * */
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
            handle_vj(countb, current_jpp_idx, v, clockwise, disruption_active, accessibilite_params, raptor_);
            current_jpp_idx = raptor_.get_boarding_jpp(countb, current_jpp_idx);
            --countb;
        }
        label_type = raptor_.get_type(countb, current_jpp_idx);
        v.final_step(current_jpp_idx, countb, raptor_.labels);
    }
}


} }
