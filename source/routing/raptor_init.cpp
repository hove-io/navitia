#include "raptor_init.h"
namespace navitia { namespace routing {

std::vector<Departure_Type>
getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations,
              bool clockwise, const float walking_speed, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings,
              const type::Properties &required_properties, const type::Data &data) {
      std::vector<Departure_Type> result;

      auto pareto_front = getParetoFront(clockwise, departs, destinations, walking_speed, labels, boardings, required_properties, data);
      result.insert(result.end(), pareto_front.begin(), pareto_front.end());

      auto walking_solutions = getWalkingSolutions(clockwise, departs, destinations, pareto_front.back(), walking_speed, labels, boardings, data);

      for(auto s : walking_solutions) {
          bool find = false;
          for(auto s2 : result) {
              if(s.rpidx == s2.rpidx && s.count == s2.count) {
                  find = true;
                  break;
              }
          }
          if(!find) {
              result.push_back(s);
          }
      }

      return result;
}


std::vector<Departure_Type>
getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const navitia::type::DateTime &dep, bool clockwise, const float walking_speed, const type::Data & data) {
    std::vector<Departure_Type> result;
    for(auto dep_dist : departs) {
        for(auto jppidx : data.pt_data.stop_points[dep_dist.first].journey_pattern_point_list) {
            Departure_Type d;
            d.count = 0;
            d.rpidx = jppidx;
            d.walking_time = dep_dist.second/walking_speed;
            if(clockwise)
                d.arrival = dep + d.walking_time;
            else
                d.arrival = dep - d.walking_time;
            result.push_back(d);
        }
    }
    return result;
}

// Does the current date improves compared to best_so_far – we must not forget to take the walking duration
bool improves(const type::DateTime & best_so_far, bool clockwise, const type::DateTime & current, int walking_duration) {
    if(clockwise) {
        return current - walking_duration > best_so_far;
    } else {
        return current + walking_duration < best_so_far;
    }
}

std::vector<Departure_Type>
getParetoFront(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations,
               const float walking_speed, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings, const type::Properties &required_properties, const type::Data &data){
    std::vector<Departure_Type> result;

    navitia::type::DateTime best_dt, best_dt_jpp;
    if(clockwise) {
        best_dt = navitia::type::DateTime::min;
        best_dt_jpp = navitia::type::DateTime::min;
    }
    for(unsigned int round=0; round < labels.size(); ++round) {
        // For every round with look for the best journey pattern point that belongs to one of the destination stop points
        // We must not forget to walking duration
        type::idx_t best_jpp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto jppidx : data.pt_data.stop_points[spid_dist.first].journey_pattern_point_list) {
                auto type = get_type(round, jppidx, labels, boardings, data);
                if((type != boarding_type::uninitialized) && improves(best_dt, clockwise, labels[round][jppidx], spid_dist.second/walking_speed) ) {
                    best_jpp = jppidx;
                    best_dt_jpp = labels[round][jppidx];
                    if(type == boarding_type::vj) {
                        // Dans le sens horaire : lors du calcul on gardé que l’heure de départ, mais on veut l’arrivée
                        // Il faut donc retrouver le stop_time qui nous intéresse avec best_stop_time
                        type::idx_t stop_time_idx;
                        uint32_t gap;
                        const auto &jpp = data.pt_data.journey_pattern_points[jppidx];
                        std::tie(stop_time_idx, gap) = best_stop_time(jpp, labels[round][jppidx], required_properties, !clockwise, data);
                        const auto &st = data.pt_data.stop_times[stop_time_idx];
                        if(clockwise) {
                            best_dt_jpp.update(st.arrival_time + gap, !clockwise);
                        } else {
                            best_dt_jpp.update(st.departure_time + gap, !clockwise);
                        }
                    }
                    if(clockwise)
                        best_dt = best_dt_jpp - (spid_dist.second/walking_speed);
                    else
                        best_dt = best_dt_jpp + (spid_dist.second/walking_speed);
                }
            }
        }
        if(best_jpp != type::invalid_idx) {
            Departure_Type s;
            s.rpidx = best_jpp;
            s.count = round;
            s.walking_time = getWalkingTime(round, best_jpp, departs, destinations, clockwise, labels, boardings, data);
            s.arrival = best_dt_jpp;
            type::idx_t final_rpidx;
            std::tie(final_rpidx, s.upper_bound) = getFinalRpidAndDate(round, best_jpp, clockwise, labels, boardings, data);
            for(auto spid_dep : departs) {
                if(data.pt_data.journey_pattern_points[final_rpidx].stop_point_idx == spid_dep.first) {
                    if(clockwise) {
                        s.upper_bound = s.upper_bound + (spid_dep.second/walking_speed);
                    }else {
                        s.upper_bound = s.upper_bound - (spid_dep.second/walking_speed);
                    }
                }
            }
            result.push_back(s);
        }
    }

    return result;
}





std::vector<Departure_Type>
getWalkingSolutions(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, Departure_Type best,  const float walking_speed, const std::vector<label_vector_t> &labels,
                                                const std::vector<vector_idx> &boardings, const type::Data &data){
    std::vector<Departure_Type> result;

    std::/*unordered_*/map<type::idx_t, Departure_Type> tmp;

    for(uint32_t i=0; i<labels.size(); ++i) {
        for(auto spid_dist : destinations) {
            Departure_Type best_departure;
            best_departure.ratio = 2;
            best_departure.rpidx = type::invalid_idx;
            for(auto rpidx : data.pt_data.stop_points[spid_dist.first].journey_pattern_point_list) {
                if(get_type(i, rpidx, labels, boardings, data) != boarding_type::uninitialized) {
                    float lost_time;
                    if(clockwise)
                        lost_time = labels[i][rpidx]-(spid_dist.second/walking_speed) - best.arrival;
                    else
                        lost_time = labels[i][rpidx]+(spid_dist.second/walking_speed) - best.arrival;

                    float walking_time = getWalkingTime(i, rpidx, departs, destinations, clockwise, labels, boardings, data);

                    //Si je perds 10 minutes, je suis pret à marcher jusqu'à 5 minutes de plus
                    if(walking_time < best.walking_time && (lost_time/(best.walking_time-walking_time)) < best_departure.ratio) {
                        Departure_Type s;
                        s.rpidx = rpidx;
                        s.count = i;
                        s.ratio = lost_time/(best.walking_time-walking_time);
                        s.walking_time = walking_time;
//                        if(clockwise)
                            s.arrival = labels[i][rpidx];
//                        else
//                            s.arrival = labels[i][rpidx];
                        type::idx_t final_rpidx;
                        navitia::type::DateTime last_time;
                        std::tie(final_rpidx, last_time) = getFinalRpidAndDate(i, rpidx, clockwise, labels, boardings, data);
                        if(clockwise) {
                            s.upper_bound = last_time;
                            for(auto spid_dep : departs) {
                                if(data.pt_data.journey_pattern_points[final_rpidx].stop_point_idx == spid_dep.first) {
                                    s.upper_bound = s.upper_bound + (spid_dep.second/walking_speed);
                                }
                            }
                        } else {
                            s.upper_bound = last_time;
                            for(auto spid_dep : departs) {
                                if(data.pt_data.journey_pattern_points[final_rpidx].stop_point_idx == spid_dep.first) {
                                    s.upper_bound = s.upper_bound - (spid_dep.second/walking_speed);
                                }
                            }
                        }

                        best_departure = s;
                    }
                }
            }
            if(best_departure.rpidx != type::invalid_idx) {
                type::idx_t journey_pattern = data.pt_data.journey_pattern_points[best_departure.rpidx].journey_pattern_idx;
                if(tmp.find(journey_pattern) == tmp.end()) {
                    tmp.insert(std::make_pair(journey_pattern, best_departure));
                } else if(tmp[journey_pattern].ratio > best_departure.ratio) {
                    tmp[journey_pattern] = best_departure;
                }
            }

        }
    }


    for(auto p : tmp) {
        result.push_back(p.second);
    }
    std::sort(result.begin(), result.end(), [](Departure_Type s1, Departure_Type s2) {return s1.ratio > s2.ratio;});

    if(result.size() > 2)
        result.resize(2);

    return result;
}

std::pair<type::idx_t, navitia::type::DateTime>
getFinalRpidAndDate(int count, type::idx_t rpid, bool clockwise, const std::vector<label_vector_t> &labels,
                    const std::vector<vector_idx> &boardings, const type::Data &data) {
    type::idx_t current_jpp = rpid;
    int cnt = count;

    navitia::type::DateTime last_time = labels[cnt][current_jpp];
    while(get_type(cnt, current_jpp, labels, boardings, data)!= boarding_type::departure) {
        if(get_type(cnt, current_jpp, labels, boardings, data) == boarding_type::vj) {
            const type::StopTime &st = data.pt_data.stop_times[boardings[cnt][current_jpp]];
            last_time.update(st.arrival_time, clockwise);
            current_jpp = get_boarding_jpp(cnt, current_jpp, labels, boardings, data);
            --cnt;
        } else {
            current_jpp = get_boarding_jpp(cnt, current_jpp, labels, boardings, data);
            last_time = labels[cnt][current_jpp];
        }
    }
    return std::make_pair(current_jpp, last_time);
}


float getWalkingTime(int count, type::idx_t rpid, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, bool clockwise, const std::vector<label_vector_t> &labels,
                     const std::vector<vector_idx> &boardings, const type::Data &data) {
    type::idx_t current_jpp = rpid;
    int cnt = count;
    float walking_time = 0;

    //Marche à la fin
    for(auto dest_dist : destinations) {
        if(dest_dist.first == data.pt_data.journey_pattern_points[current_jpp].stop_point_idx) {
            walking_time = dest_dist.second;
        }
    }
    //Marche pendant les correspondances
    while(get_type(cnt, current_jpp, labels, boardings, data) != boarding_type::departure) {
        if(get_type(cnt, current_jpp, labels, boardings, data) == boarding_type::vj) {
            current_jpp = get_boarding_jpp(cnt, current_jpp, labels, boardings, data);
            --cnt;
        } else {
            type::idx_t boarding = get_boarding_jpp(cnt, current_jpp, labels, boardings, data);
            type::idx_t connection_idx = data.dataRaptor.get_connection_idx(boarding, current_jpp, clockwise, data.pt_data);
            if(connection_idx != type::invalid_idx)
                walking_time += data.pt_data.connections[connection_idx].duration;
            current_jpp = boarding;
        }
    }
    //Marche au départ
    for(auto dep_dist : departs) {
        if(dep_dist.first == data.pt_data.journey_pattern_points[current_jpp].stop_point_idx)
            walking_time += dep_dist.second;
    }

    return walking_time;
}

}}

