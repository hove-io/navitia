#include "best_trip.h"

namespace navitia { namespace routing {





int earliest_trip(const type::Route & route, const unsigned int order, const DateTime &dt, const type::Data &data) {
    //On cherche le plus petit stop time de la route >= dt.hour()
    std::vector<uint32_t>::const_iterator begin = data.dataRaptor.departure_times.begin() +
            data.dataRaptor.first_stop_time[route.idx] +
            order * data.dataRaptor.nb_trips[route.idx];
    std::vector<uint32_t>::const_iterator end = begin + data.dataRaptor.nb_trips[route.idx];


    auto it = std::lower_bound(begin, end, dt.hour(),
                               [](uint32_t departure_time, uint32_t hour){
                               return departure_time < hour;});
    int idx = it - data.dataRaptor.departure_times.begin();
    auto date = dt.date();

    //On renvoie le premier trip valide
    for(; it != end; ++it) {
        if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_forward[idx]].test(date)
                && data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].pick_up_allowed())
            return data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].vehicle_journey_idx;
        ++idx;
    }

    //Si on en a pas trouvé, on cherche le lendemain

    ++date;
    idx = begin - data.dataRaptor.departure_times.begin();
    for(it = begin; it != end; ++it) {
        if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_forward[idx]].test(date)
                && data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].pick_up_allowed())
            return data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].vehicle_journey_idx;
        ++idx;
    }

    //Cette route ne comporte aucun trip compatible
    return -1;
}










int tardiest_trip(const type::Route & route, const unsigned int order, const DateTime &dt, const type::Data &data) {
    //On cherche le plus grand stop time de la route <= dt.hour()
    const auto begin = data.dataRaptor.arrival_times.begin() +
                       data.dataRaptor.first_stop_time[route.idx] +
                       order * data.dataRaptor.nb_trips[route.idx];
    const auto end = begin + data.dataRaptor.nb_trips[route.idx];

    auto it = std::lower_bound(begin, end, dt.hour(),
                               [](uint32_t arrival_time, uint32_t hour){
                                  return arrival_time > hour;}
                              );

    int idx = it - data.dataRaptor.arrival_times.begin();
    auto date = dt.date();
    //On renvoie le premier trip valide
    for(; it != end; ++it) {
        if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_backward[idx]].test(date)
                && data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].drop_off_allowed())
            return data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].vehicle_journey_idx;
        ++idx;
    }

    //Si on en a pas trouvé, on cherche la veille
    if(date > 0) {
        --date;
        idx = begin - data.dataRaptor.arrival_times.begin();
        for(it = begin; it != end; ++it) {
            if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_backward[idx]].test(date)
                    && data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].drop_off_allowed())
                return data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].vehicle_journey_idx;
            ++idx;
        }
    }


    //Cette route ne comporte aucun trip compatible
    return -1;
}



}}

