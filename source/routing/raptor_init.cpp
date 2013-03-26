#include "raptor_init.h"
namespace navitia { namespace routing { namespace raptor { namespace init {

std::vector<Departure_Type> getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations,
                                          bool clockwise, const map_labels_t &labels, const type::Data &data, const float walking_speed) {
      std::vector<Departure_Type> result;

      auto pareto_front = getParetoFront(clockwise, departs, destinations, labels, data, walking_speed);
      result.insert(result.end(), pareto_front.begin(), pareto_front.end());

      auto walking_solutions = getWalkingSolutions(clockwise, departs, destinations, pareto_front.back(), labels, data, walking_speed);

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


std::vector<Departure_Type> getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const navitia::type::DateTime &dep, bool clockwise, const type::Data &data, const float walking_speed) {
    std::vector<Departure_Type> result;

    for(auto dep_dist : departs) {
        for(auto rpidx : data.pt_data.stop_points[dep_dist.first].journey_pattern_point_list) {
            Departure_Type d;
            d.count = 0;
            d.rpidx = rpidx;
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


std::vector<Departure_Type> getParetoFront(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, const map_labels_t &labels, const type::Data &data, const float walking_speed) {
    std::vector<Departure_Type> result;

    navitia::type::DateTime best_dt, best_dt_rp;
    if(clockwise) {
        best_dt = navitia::type::DateTime::min;
        best_dt_rp = navitia::type::DateTime::min;
    }
    for(unsigned int i=0; i < labels.size(); ++i) {
        type::idx_t best_rp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto rpidx : data.pt_data.stop_points[spid_dist.first].journey_pattern_point_list) {
                if((labels[i][rpidx].type != uninitialized) &&
                        ((((clockwise && ((labels[i][rpidx].departure-(spid_dist.second/walking_speed)) > best_dt)))) || (!clockwise && ((labels[i][rpidx].arrival+(spid_dist.second/1.38)) < best_dt)))) {
                    best_rp = rpidx;
                    if(clockwise) {
                        best_dt_rp = labels[i][rpidx].arrival;
                        best_dt = labels[i][rpidx].arrival-(spid_dist.second/walking_speed);
                    } else {
                        best_dt_rp = labels[i][rpidx].departure;
                        best_dt = labels[i][rpidx].departure+(spid_dist.second/walking_speed);
                    }
                }
            }
        }
        if(best_rp != type::invalid_idx) {
            Departure_Type s;
            s.rpidx = best_rp;
            s.count = i;
            s.walking_time = getWalkingTime(i, best_rp, departs, destinations, clockwise, labels, data);
            s.arrival = best_dt_rp;
            type::idx_t final_rpidx;
            navitia::type::DateTime last_time;
            std::tie(final_rpidx, last_time) = getFinalRpidAndDate(i, best_rp, labels, clockwise, data);
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

            result.push_back(s);
        }
    }

    return result;
}





std::vector<Departure_Type> getWalkingSolutions(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations,
                                                Departure_Type best, const map_labels_t &labels, const type::Data &data, const float walking_speed) {
    std::vector<Departure_Type> result;

    std::/*unordered_*/map<type::idx_t, Departure_Type> tmp;

    for(uint32_t i=0; i<labels.size(); ++i) {
        for(auto spid_dist : destinations) {
            Departure_Type best_departure;
            best_departure.ratio = 2;
            best_departure.rpidx = type::invalid_idx;
            for(auto rpidx : data.pt_data.stop_points[spid_dist.first].journey_pattern_point_list) {
                if(labels[i][rpidx].type != uninitialized) {
                    float lost_time;
                    if(clockwise)
                        lost_time = labels[i][rpidx].departure-(spid_dist.second/walking_speed) - best.arrival;
                    else
                        lost_time = labels[i][rpidx].arrival+(spid_dist.second/walking_speed) - best.arrival;

                    float walking_time = getWalkingTime(i, rpidx, departs, destinations, clockwise, labels, data);

                    //Si je perds 10 minutes, je suis pret à marcher jusqu'à 5 minutes de plus
                    if(walking_time < best.walking_time && (lost_time/(best.walking_time-walking_time)) < best_departure.ratio) {
                        Departure_Type s;
                        s.rpidx = rpidx;
                        s.count = i;
                        s.ratio = lost_time/(best.walking_time-walking_time);
                        s.walking_time = walking_time;
                        if(clockwise)
                            s.arrival = labels[i][rpidx].departure;
                        else
                            s.arrival = labels[i][rpidx].arrival;
                        type::idx_t final_rpidx;
                        navitia::type::DateTime last_time;
                        std::tie(final_rpidx, last_time) = getFinalRpidAndDate(i, rpidx, labels, clockwise, data);
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

std::pair<type::idx_t, navitia::type::DateTime> getFinalRpidAndDate(int count, type::idx_t rpid, const map_labels_t &labels, bool clockwise, const type::Data &data) {
    type::idx_t current_rpid = rpid;
    int cnt = count;

    navitia::type::DateTime last_time = !clockwise ? labels[cnt][current_rpid].arrival : labels[cnt][current_rpid].departure;
    while(labels[cnt][current_rpid].type != depart) {
        if(labels[cnt][current_rpid].type == vj) {
            const type::StopTime &st1 = data.pt_data.stop_times[labels[cnt][current_rpid].stop_time_idx];
            const type::StopTime &st2 = data.pt_data.stop_times[data.pt_data.vehicle_journeys[st1.vehicle_journey_idx].stop_time_list[data.pt_data.journey_pattern_points[labels[cnt][current_rpid].rpid_embarquement].order]];
            if(!clockwise)
                last_time.update(st2.arrival_time, false);
            else
                last_time.update(st2.departure_time);
            current_rpid = labels[cnt][current_rpid].rpid_embarquement;
            --cnt;
        } else {
            current_rpid = labels[cnt][current_rpid].rpid_embarquement;
            last_time = !clockwise ? labels[cnt][current_rpid].arrival : labels[cnt][current_rpid].departure;
        }
    }
    return std::make_pair(current_rpid, last_time);
}


float getWalkingTime(int count, type::idx_t rpid, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations,
                     bool clockwise, const map_labels_t &labels, const type::Data &data) {
    type::idx_t current_rpid = rpid;
    int cnt = count;
    float walking_time = 0;
    //Marche à la fin
    for(auto dest_dist : destinations) {
        if(dest_dist.first == data.pt_data.journey_pattern_points[current_rpid].stop_point_idx) {
            walking_time = dest_dist.second;
        }
    }
    //Marche pendant les correspondances
    while(labels[cnt][current_rpid].type != depart) {
        if(labels[cnt][current_rpid].type == vj) {
            current_rpid = labels[cnt][current_rpid].rpid_embarquement;
            --cnt;
        } else {
            if(clockwise)
                walking_time += labels[cnt][labels[cnt][current_rpid].rpid_embarquement].arrival - labels[cnt][current_rpid].departure;
            else
                walking_time += labels[cnt][current_rpid].departure - labels[cnt][labels[cnt][current_rpid].rpid_embarquement].arrival;
            current_rpid = labels[cnt][current_rpid].rpid_embarquement;

        }

    }
    //Marche au départ
    for(auto dep_dist : departs) {
        if(dep_dist.first == data.pt_data.journey_pattern_points[current_rpid].stop_point_idx)
            walking_time += dep_dist.second;
    }

    return walking_time;
}

}}}}

