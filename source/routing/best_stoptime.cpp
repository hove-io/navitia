#include "best_stoptime.h"

namespace navitia { namespace routing {

std::pair<const type::StopTime*, uint32_t>
best_stop_time(const type::JourneyPatternPoint* jpp,
               const DateTime dt,
               const type::VehicleProperties& vehicle_properties,
               const bool clockwise, bool without_disrupt, const type::Data &data, bool reconstructing_path) {
    if(clockwise)
        return earliest_stop_time(jpp, dt, data, without_disrupt, reconstructing_path, vehicle_properties);
    else
        return tardiest_stop_time(jpp, dt, data, without_disrupt, reconstructing_path, vehicle_properties);
}



/** Which is the first valid stop_time in this range ?
 *  Returns invalid_idx is none is
 */
const type::StopTime* valid_pick_up(type::idx_t idx, const type::idx_t end, const DateTime dt,
        const type::Data &data, bool reconstructing_path,
        const type::VehicleProperties &required_vehicle_properties,
        bool without_disrupt){
    const auto date = DateTimeUtils::date(dt);
    const auto hour = DateTimeUtils::hour(dt);
    for(; idx < end; ++idx) {
        const type::StopTime* st = data.dataRaptor.st_idx_forward[idx];
        bool check_vp = (without_disrupt? st->departure_adapted_validity_pattern->check(date) : st->departure_validity_pattern->check(date));
        if (st->valid_end(reconstructing_path) && st->valid_hour(hour, true) &&
            check_vp &&
            st->vehicle_journey->accessible(required_vehicle_properties) ){
                return st;
        }
    }
    return nullptr;
}


const type::StopTime* valid_drop_off(type::idx_t idx, const type::idx_t end, const DateTime dt,
               const type::Data &data, bool reconstructing_path,
               const type::VehicleProperties &required_vehicle_properties,
               bool without_disrupt){
    const auto date = DateTimeUtils::date(dt);
    const auto hour = DateTimeUtils::hour(dt);
    for(; idx < end; ++idx) {
        const type::StopTime* st = data.dataRaptor.st_idx_backward[idx];
        bool check_vp = (without_disrupt? st->arrival_adapted_validity_pattern->check(date) : st->arrival_validity_pattern->check(date));
        if (st->valid_end(!reconstructing_path) && st->valid_hour(hour, false) &&
            check_vp &&
            st->vehicle_journey->accessible(required_vehicle_properties) ){
                return st;
        }
    }
    return nullptr;
}

std::pair<const type::StopTime*, uint32_t>
earliest_stop_time(const type::JourneyPatternPoint* jpp,
                   const DateTime dt, const type::Data &data,
                   bool without_disrupt,
                   bool reconstructing_path,
                   const type::VehicleProperties& vehicle_properties) {
    //On cherche le plus petit stop time de la journey_pattern >= dt.hour()
    auto begin = data.dataRaptor.departure_times.begin() +
            data.dataRaptor.first_stop_time[jpp->journey_pattern->idx] +
            jpp->order * data.dataRaptor.nb_trips[jpp->journey_pattern->idx];
    auto end = begin + data.dataRaptor.nb_trips[jpp->journey_pattern->idx];
    auto it = std::lower_bound(begin, end, DateTimeUtils::hour(dt),
                               bound_predicate_earliest);

    type::idx_t idx = it - data.dataRaptor.departure_times.begin();
    type::idx_t end_idx = (begin - data.dataRaptor.departure_times.begin()) +
                           data.dataRaptor.nb_trips[jpp->journey_pattern->idx];

    //On renvoie le premier trip valide
    const type::StopTime* first_st = valid_pick_up(idx, end_idx,
            dt, data, reconstructing_path, vehicle_properties,without_disrupt);
    auto working_dt = dt;
    // If no trip was found, we look for one the day after
    if(first_st == nullptr) {
        idx = begin - data.dataRaptor.departure_times.begin();
        working_dt = DateTimeUtils::set(DateTimeUtils::date(dt)+1, 0);
        first_st = valid_pick_up(idx, end_idx, working_dt,
            data, reconstructing_path, vehicle_properties,without_disrupt);
    }

    if(first_st != nullptr) {
        if(!first_st->is_frequency()) {
            DateTimeUtils::update(working_dt, first_st->departure_time);
        } else {
            working_dt = dt;
            const DateTime tmp_dt = f_departure_time(DateTimeUtils::hour(working_dt), first_st);
            DateTimeUtils::update(working_dt, DateTimeUtils::hour(tmp_dt));
        }
        return std::make_pair(first_st, working_dt);
    }

    //Cette journey_pattern ne comporte aucun trip compatible
    return std::make_pair(nullptr, 0);
}


std::pair<const type::StopTime*, uint32_t>
tardiest_stop_time(const type::JourneyPatternPoint* jpp,
                   const DateTime dt, const type::Data &data, bool without_disrupt,
                   bool reconstructing_path,
                   const type::VehicleProperties& vehicle_properties) {
    //On cherche le plus grand stop time de la journey_pattern <= dt.hour()
    const auto begin = data.dataRaptor.arrival_times.begin() +
                       data.dataRaptor.first_stop_time[jpp->journey_pattern->idx] +
                       jpp->order * data.dataRaptor.nb_trips[jpp->journey_pattern->idx];
    const auto end = begin + data.dataRaptor.nb_trips[jpp->journey_pattern->idx];
    auto it = std::lower_bound(begin, end, DateTimeUtils::hour(dt), bound_predicate_tardiest);

    type::idx_t idx = it - data.dataRaptor.arrival_times.begin();
    type::idx_t end_idx = (begin - data.dataRaptor.arrival_times.begin()) +
                           data.dataRaptor.nb_trips[jpp->journey_pattern->idx];

    const type::StopTime* first_st = valid_drop_off(idx, end_idx,
            dt, data,
            reconstructing_path, vehicle_properties, without_disrupt);

    auto working_dt = dt;
    // If no trip was found, we look for one the day before
    if(first_st == nullptr && DateTimeUtils::date(dt) > 0){
        idx = begin - data.dataRaptor.arrival_times.begin();
        working_dt = DateTimeUtils::set(DateTimeUtils::date(working_dt) - 1,
                                        DateTimeUtils::SECONDS_PER_DAY - 1);
        first_st = valid_drop_off(idx, end_idx, working_dt, data, reconstructing_path,
                vehicle_properties, without_disrupt);
    }

    if(first_st != nullptr){
        if(!first_st->is_frequency()) {
            DateTimeUtils::update(working_dt, DateTimeUtils::hour(first_st->arrival_time), false);
        } else {
            working_dt = dt;
            const DateTime tmp_dt = f_arrival_time(DateTimeUtils::hour(working_dt), first_st);
            DateTimeUtils::update(working_dt, DateTimeUtils::hour(tmp_dt), false);
        }
        return std::make_pair(first_st, working_dt);
    }

    //Cette journey_pattern ne comporte aucun trip compatible
    return std::make_pair(nullptr, 0);
}
}}

