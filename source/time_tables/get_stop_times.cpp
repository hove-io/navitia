#include "get_stop_times.h"
#include "routing/best_trip.h"
#include "type/pb_converter.h"
namespace navitia { namespace timetables {




std::vector<dt_st> get_stop_times(const std::vector<type::idx_t> &journey_pattern_points, const type::DateTime &dt, const type::DateTime &max_dt, const int nb_departures, const type::Data & data, const bool wheelchair) {
    std::vector<dt_st> result;
    std::multiset<dt_st, comp_st> result_temp;
    auto test_add = true;
    auto last_departure = dt;
    std::vector<type::idx_t> rps = journey_pattern_points; //On veut connaitre poru tous les journey_pattern points leur premier départ

    while(test_add && last_departure < max_dt &&
          (distance(result_temp.begin(), result_temp.upper_bound(std::make_pair(last_departure, type::invalid_idx))) < nb_departures)) {

        test_add = false;
        //On va chercher le prochain départ >= last_departure + 1 pour tous les journey_pattern points de la liste
        for(auto rp_idx : rps) {
            const type::JourneyPatternPoint & rp = data.pt_data.journey_pattern_points[rp_idx];
            auto etemp = routing::earliest_trip(data.pt_data.journey_patterns[rp.journey_pattern_idx], rp.order, last_departure + 1, data, wheelchair);
            if(etemp.first != type::invalid_idx) {
                type::idx_t stop_time_idx = data.pt_data.vehicle_journeys[etemp.first].stop_time_list[rp.order];
                auto st = data.pt_data.stop_times[stop_time_idx];
                type::DateTime dt_temp;
                dt_temp = last_departure;
                dt_temp.update(st.departure_time);
                result_temp.insert(std::make_pair(dt_temp, stop_time_idx));
                test_add = true;
            }
        }

        //Le prochain départ sera le premier dt >= last_departure
        last_departure = result_temp.upper_bound(std::make_pair(last_departure, type::invalid_idx))->first;
        //On met à jour la liste des journey_patternpoints à updater (ceux qui sont >= last_departure
        rps.clear();
        for(auto it_st = result_temp.lower_bound(std::make_pair(last_departure, type::invalid_idx));
            it_st!= result_temp.upper_bound(std::make_pair(last_departure, type::invalid_idx)); ++it_st){
            rps.push_back(data.pt_data.stop_times[it_st->second].journey_pattern_point_idx);
        }
     }

    int i = 0;
    for(auto it = result_temp.begin(); it != result_temp.end() && i < nb_departures; ++it) {
        if(it->first <= max_dt) {
            result.push_back(*it);
        } else {
            break;
        }
        ++i;
    }

    return result;
}


}

}
