#pragma once
#include "raptor_path.h"
#include "type/type.h"
#include "raptor.h"


namespace navitia { namespace routing {

/**
 * Returns the jpp used to board on the jp for a given stop point and label
 */
inline const JppIdx get_boarding_jpp(const size_t countb, const SpIdx current_sp_idx, const RAPTOR &raptor_) {
    const auto boarding_sp_idx = raptor_.labels[countb].boarding_sp_transfer(current_sp_idx);

    if (! boarding_sp_idx.is_valid()) {
        return JppIdx();
    }
    //TODO com'!
    return raptor_.labels[countb].used_jpp(boarding_sp_idx);
}
/**
 * Returns the jpp used to mark the stop point in the label
 *
 * Note: this should not be necessary, since we should be able to recompute this used jpp
 * (by iterating on the jpp of the label.boarding_jpp's journey pattern), but because of service extension we cannot do that
 */
inline const JppIdx get_used_jpp(const size_t countb, const SpIdx current_sp_idx, const RAPTOR &raptor_) {
    return raptor_.labels[countb].used_jpp(current_sp_idx);
}

template<typename Visitor>
void handle_connection(const size_t countb, const SpIdx current_sp_idx, Visitor& v,
                       bool clockwise, const RAPTOR &raptor_) {
    const auto& departure = raptor_.get_sp(current_sp_idx);

    const auto& destination_sp_idx = raptor_.labels[countb].boarding_sp_transfer(current_sp_idx);

    const auto& destination_sp = raptor_.get_sp(destination_sp_idx);
    const auto& connections = departure->stop_point_connection_list;
    const auto& l = raptor_.labels[countb].dt_transfer(current_sp_idx);
    // We try to find the connection that was taken by the algorithm
    auto find_predicate = [&](type::StopPointConnection* connection)->bool {
        return departure == connection->departure && destination_sp == connection->destination;
    };

    auto it = std::find_if(connections.begin(), connections.end(), find_predicate);
    type::StopPointConnection* stop_point_connection = nullptr;
    boost::posix_time::ptime dep_ptime, arr_ptime;
    // It might not be found, for instance if we stayed on the same stop point
    if(it == connections.end()) {
        const auto& dt_pt = raptor_.labels[countb].dt_pt(SpIdx(*destination_sp));
        if(clockwise) {
            dep_ptime = to_posix_time(dt_pt, raptor_.data);
            arr_ptime = to_posix_time(l, raptor_.data);
        } else {
            dep_ptime = to_posix_time(l, raptor_.data);
            arr_ptime = to_posix_time(dt_pt, raptor_.data);
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

    v.connection(departure, destination_sp, dep_ptime, arr_ptime, stop_point_connection);
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
    if (!st) {
        LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"), "Unable to find stop_time");
        throw navitia::recoverable_exception("impossible to rebuild the public transport path");
    }
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

/**
 *TODO remonte le vj + extension
 */
template<typename Visitor>
void handle_vj(const size_t countb, SpIdx current_sp_idx, Visitor& v,
               bool clockwise, bool disruption_active, const type::AccessibiliteParams& accessibilite_params,
               const RAPTOR &raptor_) {
    v.init_vj();
    const auto& boarding_jpp_idx = raptor_.labels[countb].boarding_jpp_pt(current_sp_idx);
    const type::StopTime* current_st;
    DateTime workingDate;

    auto current_jpp_idx = get_used_jpp(countb, current_sp_idx, raptor_);
    std::tie(current_st, workingDate) = get_current_stidx_gap(countb, current_sp_idx, raptor_.labels,
                                                              accessibilite_params, clockwise,
                                                              raptor_, disruption_active);
    while(boarding_jpp_idx != current_jpp_idx) {
        // There is a side effect on workingDate caused by workingDate
        //TODO remove the side effect
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
            current_st = &current_st->vehicle_journey->stop_time_list[order];
        }
        else {
            // Here we have to change of vehicle journey, this happen with connection_stay_in
            auto prev_st = current_st;
            // If it was a connection_stay_in we want to take the the previous vj

            if ((clockwise && ! current_st->vehicle_journey->prev_vj)
                    || (!clockwise && ! current_st->vehicle_journey->next_vj)) {
                LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"), "no service extension for vj "
                                << current_st->vehicle_journey->uri << " impossible to find good boarding jpp (idx="
                                << boarding_jpp_idx << ")");
                throw navitia::recoverable_exception("impossible to rebuild the public transport path");
            }
            if(clockwise) {
                current_st = &current_st->vehicle_journey->prev_vj->stop_time_list.back();
            } else {
                current_st = &current_st->vehicle_journey->next_vj->stop_time_list.front();
            }
            auto prev_time = prev_st->section_end_time(clockwise, workingDate),
                 current_time = current_st->section_end_time(!clockwise, workingDate);
            v.change_vj(prev_st, current_st, to_posix_time(prev_time, raptor_.data),
                        to_posix_time(current_time, raptor_.data), clockwise);
        }
        current_jpp_idx = JppIdx(*current_st->journey_pattern_point);
    }
    // There is a side effect on workingDate caused by workingDate
    auto departure_arrival = handle_st(current_st, workingDate, clockwise, raptor_.data);
    v.loop_vj(current_st, departure_arrival.first, departure_arrival.second);
    v.finish_vj(clockwise);
}

/*
 * Start from a destination point until it reaches a departure label
 *
 * Call functions of the visitor along the way.
 *
 * */
template<typename Visitor>
void read_path(Visitor& v, SpIdx destination_idx, size_t countb, bool clockwise, bool disruption_active,
          const type::AccessibiliteParams & accessibilite_params, const RAPTOR &raptor_) {
    SpIdx current_sp_idx = destination_idx;
    while (countb>0) {
        handle_vj(countb, current_sp_idx, v, clockwise, disruption_active, accessibilite_params, raptor_);
        auto current_jpp_idx = raptor_.labels[countb].boarding_jpp_pt(current_sp_idx);
        current_sp_idx = SpIdx(*raptor_.get_jpp(current_jpp_idx)->stop_point);

        --countb;
        v.final_step(current_sp_idx, countb, raptor_.labels);
        if (countb == 0) {
            break;
        }
        handle_connection(countb, current_sp_idx, v, clockwise, raptor_);
        current_jpp_idx = get_boarding_jpp(countb, current_sp_idx, raptor_);
        current_sp_idx = SpIdx(*raptor_.get_jpp(current_jpp_idx)->stop_point);
        v.final_step(current_sp_idx, countb, raptor_.labels);
    }
}


} }
