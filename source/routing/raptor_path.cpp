#include "raptor_path.h"
#include "raptor.h"
namespace navitia { namespace routing {

    
std::vector<Path> 
makePathes(std::vector<std::pair<type::idx_t, double> > destinations,
           type::DateTime dt, const float walking_speed, const type::Properties &required_properties, const RAPTOR &raptor_, bool clockwise) {
    std::vector<Path> result;
    navitia::type::DateTime best_dt = clockwise ? type::DateTime::inf : type::DateTime::min;
    for(unsigned int i=1;i<=raptor_.count;++i) {
        type::idx_t best_jpp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto dest : raptor_.data.pt_data.stop_points[spid_dist.first]->journey_pattern_point_list) {
                type::idx_t dest_idx = dest->idx;
                if(raptor_.get_type(i, dest_idx) != boarding_type::uninitialized) {
                    navitia::type::DateTime current_dt = raptor_.labels[i][dest_idx];
                    if(clockwise)
                        current_dt = current_dt + spid_dist.second/walking_speed;
                    else
                        current_dt = current_dt - spid_dist.second/walking_speed;
                    if(        (clockwise && ((best_dt == type::DateTime::inf && current_dt <= dt) || (best_dt != type::DateTime::inf && current_dt < best_dt)))
                            ||(!clockwise && ((best_dt == type::DateTime::min && current_dt >= dt) || (best_dt != type::DateTime::min && current_dt > best_dt))) ){
                        best_dt = current_dt ;
                        best_jpp = dest_idx;
                    }
                }
            }
        }
        if(best_jpp != type::invalid_idx)
            result.push_back(makePath(best_jpp, i, clockwise, required_properties, raptor_));
    }

    return result;
}


Path 
makePath(type::idx_t destination_idx, unsigned int countb, bool clockwise,  const type::Properties &required_properties,
         const RAPTOR &raptor_) {
    Path result;
    unsigned int current_jpp_idx = destination_idx;
    type::DateTime l = raptor_.labels[countb][current_jpp_idx],
                   workingDate = l;

    const type::StopTime* current_st;
    type::idx_t boarding_jpp = navitia::type::invalid_idx;

    bool stop = false;

    PathItem item;
    // On boucle tant
    while(!stop) {
        // Est-ce qu'on a une section marche à pied ?
        if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection ||
           raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection_extension ||
           raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection_guarantee) {
            l = raptor_.labels[countb][current_jpp_idx];
            auto r2 = raptor_.labels[countb][raptor_.get_boarding_jpp(countb, current_jpp_idx)->idx];
            if(clockwise) {
                item = PathItem(r2, l);
            } else {
                item = PathItem(l, r2);
            }
            item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[current_jpp_idx]->stop_point->idx);
            item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[raptor_.get_boarding_jpp(countb, current_jpp_idx)->idx]->stop_point->idx);
            if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection)
                item.type = walking;
            else if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection_extension)
                item.type = extension;
            else if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection_guarantee)
                item.type = guarantee;
            result.items.push_back(item);
            boarding_jpp = type::invalid_idx;
            current_jpp_idx = raptor_.get_boarding_jpp(countb, current_jpp_idx)->idx;
       
        } else { // Sinon c'est un trajet TC
            // Est-ce que qu'on a à faire à un nouveau trajet ?
            if(boarding_jpp == type::invalid_idx) {
                l = raptor_.labels[countb][current_jpp_idx];
                boarding_jpp = raptor_.get_boarding_jpp(countb, current_jpp_idx)->idx;
                uint32_t gap_frep;
                std::tie(current_st, gap_frep) = raptor_.get_current_stidx_gap(countb, current_jpp_idx, required_properties, clockwise);
                //Sert pour les horaires en  fréquences

                item = PathItem();
                item.type = public_transport;

                workingDate = l;
                workingDate.update((clockwise?current_st->departure_time:current_st->arrival_time), clockwise);
                item.vj_idx = current_st->vehicle_journey->idx;
                while(boarding_jpp != current_jpp_idx) {

                    //On stocke le sp, et les temps
                    item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[current_jpp_idx]->stop_point->idx);
                    if(clockwise) {
                        workingDate.update(current_st->departure_time+gap_frep, !clockwise);
                        item.departures.push_back(workingDate);
                        workingDate.update(current_st->arrival_time+gap_frep, !clockwise);
                        item.arrivals.push_back(workingDate);
                    } else {
                        workingDate.update(current_st->arrival_time+gap_frep, !clockwise);
                        item.arrivals.push_back(workingDate);
                        workingDate.update(current_st->departure_time+gap_frep, !clockwise);
                        item.departures.push_back(workingDate);
                    }

                    int order = current_st->journey_pattern_point->order;
                    // On parcourt les données dans le sens contraire du calcul
                    if(clockwise){
                        BOOST_ASSERT(order>0);
                        order--;
                    }
                    else{
                        order++;
                        BOOST_ASSERT(order < current_st->vehicle_journey->stop_time_list.size());
                    }

                    current_st = current_st->vehicle_journey->stop_time_list[order];
                    current_jpp_idx = current_st->journey_pattern_point->idx;
                }
                // Je stocke le dernier stop point, et ses temps d'arrivée et de départ
                item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[current_jpp_idx]->stop_point->idx);
                if(clockwise) {
                    workingDate.update(current_st->departure_time+gap_frep, !clockwise);
                    item.departures.push_back(workingDate);
                    workingDate.update(current_st->arrival_time+gap_frep,  !clockwise);
                    item.arrivals.push_back(workingDate);
                    item.arrival = item.arrivals.front();
                    item.departure = item.departures.back();
                } else {
                    workingDate.update(current_st->arrival_time+gap_frep, !clockwise);
                    item.arrivals.push_back(workingDate);
                    workingDate.update(current_st->departure_time+gap_frep, !clockwise);
                    item.departures.push_back(workingDate);
                    item.arrival = item.arrivals.back();
                    item.departure = item.departures.front();
                }

                //On stocke l'item créé
                result.items.push_back(item);

                --countb;
                boarding_jpp = navitia::type::invalid_idx ;

            }
        }
        if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::departure)
            stop = true;

    }

    if(clockwise){
        std::reverse(result.items.begin(), result.items.end());
        for(auto & item : result.items) {
            std::reverse(item.stop_points.begin(), item.stop_points.end());
            std::reverse(item.arrivals.begin(), item.arrivals.end());
            std::reverse(item.departures.begin(), item.departures.end());
        }
    }

    if(result.items.size() > 0)
        result.duration = result.items.back().arrival - result.items.front().departure;
    else
        result.duration = 0;
    int count_visites = 0;
//    for(auto t: raptor_.best_labels) {
//        if(t.type != uninitialized) {
//            ++count_visites;
//        }
//    }
    result.percent_visited = 100*count_visites / raptor_.data.pt_data.stop_points.size();

    result.nb_changes = 0;
    if(result.items.size() > 2) {
        for(unsigned int i = 1; i <= (result.items.size()-2); ++i) {
            if(result.items[i].type == walking)
                ++ result.nb_changes;
        }
    }
    patch_datetimes(result);

    return result;
}

 void patch_datetimes(Path &path){
    PathItem previous_item;
    std::vector<std::pair<int, PathItem>> to_insert;
    for(auto item = path.items.begin(); item!= path.items.end(); ++item) {
        if(previous_item.departure != type::DateTime::inf) {
            if(item->type == walking || item->type == extension || item->type == guarantee) {
                auto duration = item->arrival - item->departure;
                item->departure = previous_item.arrival;
                item->arrival = item->departure + duration;
            } else {
                PathItem waitingItem=PathItem();
                waitingItem.departure = previous_item.arrival;
                waitingItem.arrival = item->departure;
                waitingItem.type = waiting;
                waitingItem.stop_points.push_back(previous_item.stop_points.front());
                to_insert.push_back(std::make_pair(item-path.items.begin(), waitingItem));
            }
            previous_item = *item;
        } else if(item->type == public_transport) {
            previous_item = *item;
        }
    }

    std::reverse(to_insert.begin(), to_insert.end());
    for(auto pos_value : to_insert)
        path.items.insert(path.items.begin()+pos_value.first, pos_value.second);
}



Path 
makePathreverse(unsigned int destination_idx, unsigned int countb,  const type::Properties &required_properties,
                const RAPTOR &raptor_) {
    return makePath(destination_idx, countb, false, required_properties, raptor_);
}


}}
