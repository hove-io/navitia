#include "best_trip.h"

namespace navitia { namespace routing {

std::pair<type::idx_t, uint32_t>
best_trip(const type::JourneyPattern & journey_pattern, const unsigned int order,
          const navitia::type::DateTime &dt,
          const type::Properties &required_properties,
          const bool clockwise, const type::Data &data) {
    if(clockwise)
        return earliest_trip(journey_pattern, order, dt, data, required_properties);
    else
        return tardiest_trip(journey_pattern, order, dt, data, required_properties);
}



/** Which is the first valid stop_time in this range ?
 *  Returns invalid_idx is none is
 */
type::idx_t valid_pick_up(type::idx_t idx, type::idx_t end, uint32_t date, uint32_t hour, const type::Data &data, const type::Properties &required_properties){
    for(; idx < end; ++idx) {
        bool valid_date = data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_forward[idx]].test(date);

        if (valid_date) {
            type::idx_t st_idx = data.dataRaptor.st_idx_forward[idx];
            const type::StopTime & st = data.pt_data.stop_times[st_idx];
            if( st.pick_up_allowed() && st.valid_hour(hour)
                    && data.pt_data.vehicle_journeys[st.vehicle_journey_idx].accessible(required_properties) ){
                return idx;
            }
        }
    }
    return type::invalid_idx;
}

type::idx_t valid_drop_off(type::idx_t idx, type::idx_t end, uint32_t date, uint32_t hour, const type::Data &data, const type::Properties &required_properties){
    for(; idx < end; ++idx) {
        bool valid_date = data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_backward[idx]].test(date);

        if (valid_date) {
            type::idx_t st_idx = data.dataRaptor.st_idx_backward[idx];
            const type::StopTime & st = data.pt_data.stop_times[st_idx];
            if( st.drop_off_allowed() && st.valid_hour(hour)
                    && data.pt_data.vehicle_journeys[st.vehicle_journey_idx].accessible(required_properties) ){
                return idx;
            }
        }
    }
    return type::invalid_idx;
}

std::pair<type::idx_t, uint32_t> 
    earliest_trip(const type::JourneyPattern & journey_pattern, const unsigned int order,
                  const navitia::type::DateTime &dt, const type::Data &data, 
                  const type::Properties &required_properties) {

    // If the stop_point doesn’t match the required properties, we don’t bother looking further
    if(!data.pt_data.stop_points[data.pt_data.journey_pattern_points[journey_pattern.journey_pattern_point_list[order]].stop_point_idx].accessible(required_properties))
        return std::make_pair(type::invalid_idx, 0);


    //On cherche le plus petit stop time de la journey_pattern >= dt.hour()
    std::vector<uint32_t>::const_iterator begin = data.dataRaptor.departure_times.begin() +
            data.dataRaptor.first_stop_time[journey_pattern.idx] +
            order * data.dataRaptor.nb_trips[journey_pattern.idx];
    std::vector<uint32_t>::const_iterator end = begin + data.dataRaptor.nb_trips[journey_pattern.idx];


    auto it = std::lower_bound(begin, end, dt.hour(),
                               [](uint32_t departure_time, uint32_t hour){
                               return departure_time < hour;});

    type::idx_t idx = it - data.dataRaptor.departure_times.begin();
    type::idx_t end_idx = (begin - data.dataRaptor.departure_times.begin()) +  data.dataRaptor.nb_trips[journey_pattern.idx];

    //On renvoie le premier trip valide
    type::idx_t first_st = valid_pick_up(idx, end_idx, dt.date(), dt.hour(), data, required_properties);

    // If no trip was found, we look for the next day
    if(first_st == type::invalid_idx){
        idx = begin - data.dataRaptor.departure_times.begin();
        first_st = valid_pick_up(idx, end_idx, dt.date() + 1, dt.hour(), data, required_properties);
    }

    if(first_st != type::invalid_idx){
        const type::StopTime & st = data.pt_data.stop_times[data.dataRaptor.st_idx_forward[first_st]];
        return std::make_pair(st.vehicle_journey_idx,
                              !st.is_frequency() ? 0 : compute_gap(dt.hour(), st.start_time, st.headway_secs));
    }

    //Cette journey_pattern ne comporte aucun trip compatible
    return std::make_pair(type::invalid_idx, 0);
}


std::pair<type::idx_t, uint32_t> 
tardiest_trip(const type::JourneyPattern & journey_pattern, const unsigned int order,
              const navitia::type::DateTime &dt, const type::Data &data,
              const type::Properties &required_properties) {
    if(!data.pt_data.stop_points[data.pt_data.journey_pattern_points[journey_pattern.journey_pattern_point_list[order]].stop_point_idx].accessible(required_properties))
        return std::make_pair(type::invalid_idx, 0);
    //On cherche le plus grand stop time de la journey_pattern <= dt.hour()
    const auto begin = data.dataRaptor.arrival_times.begin() +
                       data.dataRaptor.first_stop_time[journey_pattern.idx] +
                       order * data.dataRaptor.nb_trips[journey_pattern.idx];
    const auto end = begin + data.dataRaptor.nb_trips[journey_pattern.idx];

    auto it = std::lower_bound(begin, end, dt.hour(),
                               [](uint32_t arrival_time, uint32_t hour){
                                  return arrival_time > hour;}
                              );

    type::idx_t idx = it - data.dataRaptor.arrival_times.begin();
    type::idx_t end_idx = (begin - data.dataRaptor.arrival_times.begin()) +  data.dataRaptor.nb_trips[journey_pattern.idx];

    type::idx_t first_st = valid_drop_off(idx, end_idx, dt.date(), dt.hour(), data, required_properties);

    // If no trip was found, we look for the next day
    if(first_st == type::invalid_idx && dt.date() > 0){
        idx = begin - data.dataRaptor.arrival_times.begin();
        first_st = valid_drop_off(idx, end_idx, dt.date() -1, dt.hour(), data, required_properties);
    }

    if(first_st != type::invalid_idx){
        const type::StopTime & st = data.pt_data.stop_times[data.dataRaptor.st_idx_backward[first_st]];
        return std::make_pair(st.vehicle_journey_idx,
                              !st.is_frequency() ? 0 : compute_gap(dt.hour(), st.start_time, st.headway_secs));
    }

    //Cette journey_pattern ne comporte aucun trip compatible
    return std::make_pair(type::invalid_idx, 0);
}
}}

