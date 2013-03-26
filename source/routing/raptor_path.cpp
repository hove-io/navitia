#include "raptor_path.h"
#include "raptor.h"
namespace navitia { namespace routing { namespace raptor {

    
std::vector<Path> 
makePathes(std::vector<std::pair<type::idx_t, double> > destinations,
           navitia::type::DateTime dt, const float walking_speed, const RAPTOR &raptor_ ) {
    std::vector<Path> result;
    navitia::type::DateTime best_dt = dt;
    for(unsigned int i=1;i<=raptor_.count;++i) {
        type::idx_t best_rp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto dest : raptor_.data.pt_data.stop_points[spid_dist.first].journey_pattern_point_list) {
                if(raptor_.labels[i][dest].type != uninitialized) {
                    navitia::type::DateTime current_dt = raptor_.labels[i][dest].departure + (spid_dist.second/walking_speed);
                    if(current_dt <= best_dt) {
                        best_dt = current_dt;
                        best_rp = dest;
                    }
                }
            }
        }
        if(best_rp != type::invalid_idx)
            result.push_back(makePath(best_rp, i, true, raptor_));
    }

    return result;
}





std::vector<Path> 
makePathesreverse(std::vector<std::pair<type::idx_t, double> > destinations,
                          navitia::type::DateTime dt, const float walking_speed,
                          const RAPTOR &raptor_) {
    std::vector<Path> result;

    navitia::type::DateTime best_dt = dt;

    for(unsigned int i=1;i<=raptor_.count;++i) {
        type::idx_t best_rp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto dest : raptor_.data.pt_data.stop_points[spid_dist.first].journey_pattern_point_list) {
                if(raptor_.labels[i][dest].type != uninitialized) {
                    navitia::type::DateTime current_dt = raptor_.labels[i][dest].departure - (spid_dist.second/walking_speed);
                    if(current_dt >= best_dt) {
                        best_dt = current_dt;
                        best_rp = dest;
                    }
                }
            }
        }

        if(best_rp != type::invalid_idx)
            result.push_back(makePath(best_rp, i, false, raptor_));
    }
    return result;
}







Path 
makePath(type::idx_t destination_idx, unsigned int countb, bool clockwise,
         const RAPTOR &raptor_) {
    Path result;
    unsigned int current_rpid = destination_idx;
    label l = raptor_.labels[countb][current_rpid];
    navitia::type::DateTime workingDate;
    if(clockwise)
        workingDate = l.arrival;
    else
        workingDate = l.departure;

    navitia::type::StopTime current_st;
    navitia::type::idx_t rpid_embarquement = navitia::type::invalid_idx;

    bool stop = false;

    PathItem item;
    // On boucle tant
    while(!stop) {

        // Est-ce qu'on a une section marche à pied ?
        if(raptor_.labels[countb][current_rpid].type == connection ||
           raptor_.labels[countb][current_rpid].type == connection_extension ||
           raptor_.labels[countb][current_rpid].type == connection_guarantee) {
            l = raptor_.labels[countb][current_rpid];
            auto r2 = raptor_.labels[countb][l.rpid_embarquement];
            if(clockwise) {
                item = PathItem(r2.arrival, l.departure);
            } else {
                item = PathItem(l.departure, r2.arrival);
            }
            item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[current_rpid].stop_point_idx);
            item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[l.rpid_embarquement].stop_point_idx);
            if(l.type == connection)
                item.type = walking;
            else if(l.type == connection_extension)
                item.type = extension;
            else if(l.type == connection_guarantee)
                item.type = guarantee;
            result.items.push_back(item);
            rpid_embarquement = navitia::type::invalid_idx;
            current_rpid = l.rpid_embarquement;
       
        } else { // Sinon c'est un trajet TC
            // Est-ce que qu'on a à faire à un nouveau trajet ?
            if(rpid_embarquement == navitia::type::invalid_idx) {
                l = raptor_.labels[countb][current_rpid];
                rpid_embarquement = l.rpid_embarquement;
                current_st = raptor_.data.pt_data.stop_times.at(l.stop_time_idx);
                //Sert pour les horaires en  fréquences
                uint32_t gap = l.arrival.hour() - current_st.arrival_time%raptor_.data.dataRaptor.SECONDS_PER_DAY;

                item = PathItem();
                item.type = public_transport;

                if(clockwise) {
                    workingDate = l.departure;
                }
                else {
                    workingDate = l.arrival;
                }
                item.vj_idx = current_st.vehicle_journey_idx;
                while(rpid_embarquement != current_rpid) {

                    //On stocke le sp, et les temps
                    item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[current_rpid].stop_point_idx);
                    if(clockwise) {
                        workingDate.update(current_st.departure_time+gap, false);
                        item.departures.push_back(workingDate);
                        workingDate.update(current_st.arrival_time+gap, false);
                        item.arrivals.push_back(workingDate);
                    } else {
                        workingDate.update(current_st.arrival_time+gap);
                        item.arrivals.push_back(workingDate);
                        workingDate.update(current_st.departure_time+gap);
                        item.departures.push_back(workingDate);
                    }

                    //On va chercher le prochain stop time
                    if(clockwise)
                        current_st = raptor_.data.pt_data.stop_times.at(current_st.idx - 1);
                    else
                        current_st = raptor_.data.pt_data.stop_times.at(current_st.idx + 1);

                    //Est-ce que je suis sur un journey_pattern point de fin 
                    current_rpid = current_st.journey_pattern_point_idx;
                }
                // Je stocke le dernier stop point, et ses temps d'arrivée et de départ
                item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[current_rpid].stop_point_idx);
                if(clockwise) {
                    workingDate.update(current_st.departure_time+gap, false);
                    item.departures.push_back(workingDate);
                    workingDate.update(current_st.arrival_time+gap, false);
                    item.arrivals.push_back(workingDate);
                    item.arrival = item.arrivals.front();
                    item.departure = item.departures.back();
                } else {
                    workingDate.update(current_st.arrival_time+gap);
                    item.arrivals.push_back(workingDate);
                    workingDate.update(current_st.departure_time+gap);
                    item.departures.push_back(workingDate);
                    item.arrival = item.arrivals.back();
                    item.departure = item.departures.front();
                }

                //On stocke l'item créé
                result.items.push_back(item);

                --countb;
                rpid_embarquement = navitia::type::invalid_idx ;

            }
        }
        if(raptor_.labels[countb][current_rpid].type == depart)
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
    for(auto t: raptor_.best_labels) {
        if(t.type != uninitialized) {
            ++count_visites;
        }
    }
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
makePathreverse(unsigned int destination_idx, unsigned int countb,
                const RAPTOR &raptor_) {
    return makePath(destination_idx, countb, false, raptor_);
}



} } }
