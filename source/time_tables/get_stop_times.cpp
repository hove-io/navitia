#include "get_stop_times.h"
#include "routing/best_stoptime.h"
#include "type/pb_converter.h"
namespace navitia { namespace timetables {

std::vector<datetime_stop_time> get_stop_times(const std::vector<type::idx_t> &journey_pattern_points, const type::DateTime &dt, const type::DateTime &max_dt, const size_t max_departures, const type::Data & data,
                                               const type::AccessibiliteParams & accessibilite_params/*const bool wheelchair*/) {
    std::vector<datetime_stop_time> result;
    auto test_add = true;

    // Prochain horaire où l’on demandera le prochain départ : on s’en sert pour ne pas obtenir plusieurs fois le même stop_time
    // Initialement, c’est l’heure demandée
    std::map<type::idx_t, type::DateTime> next_requested_datetime;
    for(auto jpp_idx : journey_pattern_points){
        next_requested_datetime[jpp_idx] = dt;
    }

    while(test_add && result.size() < max_departures) {
        test_add = false;
        for(auto jpp_idx : journey_pattern_points) {
            const type::JourneyPatternPoint* jpp = data.pt_data.journey_pattern_points[jpp_idx];            
            auto st = routing::earliest_stop_time(jpp, next_requested_datetime[jpp_idx], data, false, accessibilite_params/*wheelchair*/).first;
            if(st != nullptr) {
                type::DateTime dt_temp = dt;
                dt_temp.update(st->departure_time);
                if(dt_temp <= max_dt && result.size() < max_departures) {
                    result.push_back(std::make_pair(dt_temp, st));
                    test_add = true;
                    // Le prochain horaire observé doit être au minimum une seconde après
                    next_requested_datetime[jpp_idx] = dt_temp + 1;
                }
            }
        }
     }

    return result;
}

}} // namespace navitia::timetables
