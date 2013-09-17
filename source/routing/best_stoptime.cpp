#include "best_stoptime.h"

namespace navitia { namespace routing {

std::pair<const type::StopTime*, uint32_t>
best_stop_time(const type::JourneyPatternPoint* jpp,
               const navitia::type::DateTime &dt,
               /*const type::Properties &required_properties*/
               const type::AccessibiliteParams & accessibilite_params,
               const bool clockwise, const type::Data &data, bool reconstructing_path) {
    if(clockwise)
        return earliest_stop_time(jpp, dt, data, reconstructing_path, accessibilite_params/*required_properties*/);
    else
        return tardiest_stop_time(jpp, dt, data, reconstructing_path, accessibilite_params/*required_properties*/);
}



/** Which is the first valid stop_time in this range ?
 *  Returns invalid_idx is none is
 */
const type::StopTime*
valid_pick_up(type::idx_t idx, type::idx_t end, uint32_t date,
              uint32_t hour, const type::Data &data, bool reconstructing_path,
              const type::VehicleProperties &required_vehicle_properties){
    for(; idx < end; ++idx) {
        const type::StopTime* st = data.dataRaptor.st_idx_forward[idx];
        if (st->departure_validity_pattern->check(date)) {
            if( st->valid_end(reconstructing_path) && st->valid_hour(hour, true)
                    && st->vehicle_journey->accessible(required_vehicle_properties) ){
                return st;
            }
        }
    }
    return nullptr;
}

const type::StopTime*
valid_drop_off(type::idx_t idx, type::idx_t end, uint32_t date,
               uint32_t hour, const type::Data &data,
               bool reconstructing_path, const type::VehicleProperties &required_vehicle_properties){
    for(; idx < end; ++idx) {
        const type::StopTime* st = data.dataRaptor.st_idx_backward[idx];
        if (st->arrival_validity_pattern->check(date)) {
            if( st->valid_end(!reconstructing_path) && st->valid_hour(hour, false)
                    && st->vehicle_journey->accessible(required_vehicle_properties) ){
                return st;
            }
        }
    }
    return nullptr;
}

std::pair<const type::StopTime*, uint32_t>
earliest_stop_time(const type::JourneyPatternPoint* jpp,
                   const navitia::type::DateTime &dt, const type::Data &data,
                   bool reconstructing_path,
                   const type::AccessibiliteParams & accessibilite_params
                   /*const type::Properties &required_properties*/) {

    // If the stop_point doesn’t match the required properties, we don’t bother looking further
    if(!jpp->stop_point->accessible(accessibilite_params.properties/*required_properties*/))
        return std::make_pair(nullptr, 0);


    //On cherche le plus petit stop time de la journey_pattern >= dt.hour()
    std::vector<uint32_t>::const_iterator begin = data.dataRaptor.departure_times.begin() +
            data.dataRaptor.first_stop_time[jpp->journey_pattern->idx] +
            jpp->order * data.dataRaptor.nb_trips[jpp->journey_pattern->idx];
    std::vector<uint32_t>::const_iterator end = begin + data.dataRaptor.nb_trips[jpp->journey_pattern->idx];

    auto it = std::lower_bound(begin, end, dt.hour(),
                               [](uint32_t departure_time, uint32_t hour){
                               return departure_time < hour;});

    type::idx_t idx = it - data.dataRaptor.departure_times.begin();
    type::idx_t end_idx = (begin - data.dataRaptor.departure_times.begin()) +  data.dataRaptor.nb_trips[jpp->journey_pattern->idx];

    //On renvoie le premier trip valide
    const type::StopTime* first_st = valid_pick_up(idx, end_idx, dt.date(), dt.hour(), data, reconstructing_path, accessibilite_params.vehicle_properties/*required_properties*/);

    // If no trip was found, we look for the next day
    if(first_st == nullptr){
        idx = begin - data.dataRaptor.departure_times.begin();
        first_st = valid_pick_up(idx, end_idx, dt.date() + 1, 0, data, reconstructing_path, accessibilite_params.vehicle_properties/*required_properties*/);
    }

    if(first_st != nullptr){
        return std::make_pair(first_st, !first_st->is_frequency() ? 0 : compute_gap(dt.hour(), first_st->start_time, first_st->headway_secs, true));
    }

    //Cette journey_pattern ne comporte aucun trip compatible
    return std::make_pair(nullptr, 0);
}


std::pair<const type::StopTime*, uint32_t>
tardiest_stop_time(const type::JourneyPatternPoint* jpp,
                   const navitia::type::DateTime &dt, const type::Data &data,
                   bool reconstructing_path,
                   const type::AccessibiliteParams & accessibilite_params
                   /*const type::Properties &required_properties*/) {
    if(!jpp->stop_point->accessible(accessibilite_params.properties/*required_properties*/))
        return std::make_pair(nullptr, 0);
    //On cherche le plus grand stop time de la journey_pattern <= dt.hour()
    const auto begin = data.dataRaptor.arrival_times.begin() +
                       data.dataRaptor.first_stop_time[jpp->journey_pattern->idx] +
                       jpp->order * data.dataRaptor.nb_trips[jpp->journey_pattern->idx];
    const auto end = begin + data.dataRaptor.nb_trips[jpp->journey_pattern->idx];
    auto tmp_vec = std::vector<uint32_t>(begin, end);
    auto it = std::lower_bound(begin, end, dt.hour(),
                               [](uint32_t arrival_time, uint32_t hour){
                                  return arrival_time > hour;}
                              );

    type::idx_t idx = it - data.dataRaptor.arrival_times.begin();
    type::idx_t end_idx = (begin - data.dataRaptor.arrival_times.begin()) +  data.dataRaptor.nb_trips[jpp->journey_pattern->idx];

    const type::StopTime* first_st = valid_drop_off(idx, end_idx, dt.date(), dt.hour(), data, reconstructing_path, accessibilite_params.vehicle_properties/*required_properties*/);

    // If no trip was found, we look for the next day
    if(first_st == nullptr && dt.date() > 0){
        idx = begin - data.dataRaptor.arrival_times.begin();
        first_st = valid_drop_off(idx, end_idx, dt.date() -1, type::DateTime::SECONDS_PER_DAY, data, reconstructing_path, accessibilite_params.vehicle_properties/*required_properties*/);
    }

    if(first_st != nullptr){
        return std::make_pair(first_st, !first_st->is_frequency() ? 0 : compute_gap(dt.hour(), first_st->start_time, first_st->headway_secs, false));
    }

    //Cette journey_pattern ne comporte aucun trip compatible
    return std::make_pair(nullptr, 0);
}
}}

