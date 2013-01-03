#include "raptor_init.h"
namespace navitia { namespace routing { namespace raptor { namespace init {

std::vector<Departure_Type> getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations,
                                          bool clockwise, const map_retour_t &retour, const type::Data &data, const float walking_speed) {
      std::vector<Departure_Type> result;

      auto pareto_front = getParetoFront(clockwise, departs, destinations, retour, data, walking_speed);
      result.insert(result.end(), pareto_front.begin(), pareto_front.end());

      auto walking_solutions = getWalkingSolutions(clockwise, departs, destinations, pareto_front.back(), retour, data, walking_speed);

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


std::vector<Departure_Type> getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const DateTime &dep, bool clockwise, const type::Data &data, const float walking_speed) {
    std::vector<Departure_Type> result;

    for(auto dep_dist : departs) {
        for(auto rpidx : data.pt_data.stop_points[dep_dist.first].route_point_list) {
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


std::vector<Departure_Type> getParetoFront(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, const map_retour_t &retour, const type::Data &data, const float walking_speed) {
    std::vector<Departure_Type> result;

    DateTime best_dt, best_dt_rp;
    if(clockwise) {
        best_dt = DateTime::min;
        best_dt_rp = DateTime::min;
    }
    for(unsigned int i=0; i < retour.size(); ++i) {
        type::idx_t best_rp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto rpidx : data.pt_data.stop_points[spid_dist.first].route_point_list) {
                if((retour[i][rpidx].type != uninitialized) &&
                        ((((clockwise && ((retour[i][rpidx].departure-(spid_dist.second/walking_speed)) > best_dt)))) || (!clockwise && ((retour[i][rpidx].arrival+(spid_dist.second/1.38)) < best_dt)))) {
                    best_rp = rpidx;
                    if(clockwise) {
                        best_dt_rp = retour[i][rpidx].arrival;
                        best_dt = retour[i][rpidx].arrival-(spid_dist.second/walking_speed);
                    } else {
                        best_dt_rp = retour[i][rpidx].departure;
                        best_dt = retour[i][rpidx].departure+(spid_dist.second/walking_speed);
                    }
                }
            }
        }
        if(best_rp != type::invalid_idx) {
            Departure_Type s;
            s.rpidx = best_rp;
            s.count = i;
            s.walking_time = getWalkingTime(i, best_rp, departs, destinations, clockwise, retour, data);
            s.arrival = best_dt_rp;
            type::idx_t final_rpidx;
            DateTime last_time;
            std::tie(final_rpidx, last_time) = getFinalRpidAndDate(i, best_rp, retour, clockwise, data);
            if(clockwise) {
                s.upper_bound = last_time;
                for(auto spid_dep : departs) {
                    if(data.pt_data.route_points[final_rpidx].stop_point_idx == spid_dep.first) {
                        s.upper_bound = s.upper_bound + (spid_dep.second/walking_speed);
                    }
                }
            } else {
                s.upper_bound = last_time;
                for(auto spid_dep : departs) {
                    if(data.pt_data.route_points[final_rpidx].stop_point_idx == spid_dep.first) {
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
                                                Departure_Type best, const map_retour_t &retour, const type::Data &data, const float walking_speed) {
    std::vector<Departure_Type> result;

    std::/*unordered_*/map<type::idx_t, Departure_Type> tmp;

    for(uint32_t i=0; i<retour.size(); ++i) {
        for(auto spid_dist : destinations) {
            Departure_Type best_departure;
            best_departure.ratio = 2;
            best_departure.rpidx = type::invalid_idx;
            for(auto rpidx : data.pt_data.stop_points[spid_dist.first].route_point_list) {
                if(retour[i][rpidx].type != uninitialized) {
                    float lost_time;
                    if(clockwise)
                        lost_time = retour[i][rpidx].departure-(spid_dist.second/walking_speed) - best.arrival;
                    else
                        lost_time = retour[i][rpidx].arrival+(spid_dist.second/walking_speed) - best.arrival;

                    float walking_time = getWalkingTime(i, rpidx, departs, destinations, clockwise, retour, data);

                    //Si je perds 10 minutes, je suis pret à marcher jusqu'à 5 minutes de plus
                    if(walking_time < best.walking_time && (lost_time/(best.walking_time-walking_time)) < best_departure.ratio) {
                        Departure_Type s;
                        s.rpidx = rpidx;
                        s.count = i;
                        s.ratio = lost_time/(best.walking_time-walking_time);
                        s.walking_time = walking_time;
                        if(clockwise)
                            s.arrival = retour[i][rpidx].departure;
                        else
                            s.arrival = retour[i][rpidx].arrival;
                        type::idx_t final_rpidx;
                        DateTime last_time;
                        std::tie(final_rpidx, last_time) = getFinalRpidAndDate(i, rpidx, retour, clockwise, data);
                        if(clockwise) {
                            s.upper_bound = last_time;
                            for(auto spid_dep : departs) {
                                if(data.pt_data.route_points[final_rpidx].stop_point_idx == spid_dep.first) {
                                    s.upper_bound = s.upper_bound + (spid_dep.second/walking_speed);
                                }
                            }
                        } else {
                            s.upper_bound = last_time;
                            for(auto spid_dep : departs) {
                                if(data.pt_data.route_points[final_rpidx].stop_point_idx == spid_dep.first) {
                                    s.upper_bound = s.upper_bound - (spid_dep.second/walking_speed);
                                }
                            }
                        }

                        best_departure = s;
                    }
                }
            }
            if(best_departure.rpidx != type::invalid_idx) {
                type::idx_t route = data.pt_data.route_points[best_departure.rpidx].route_idx;
                if(tmp.find(route) == tmp.end()) {
                    tmp.insert(std::make_pair(route, best_departure));
                } else if(tmp[route].ratio > best_departure.ratio) {
                    tmp[route] = best_departure;
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

std::pair<type::idx_t, DateTime> getFinalRpidAndDate(int count, type::idx_t rpid, const map_retour_t &retour, bool clockwise, const type::Data &data) {
    type::idx_t current_rpid = rpid;
    int cnt = count;

    DateTime last_time = !clockwise ? retour[cnt][current_rpid].arrival : retour[cnt][current_rpid].departure;
    while(retour[cnt][current_rpid].type != depart) {
        if(retour[cnt][current_rpid].type == vj) {
            const type::StopTime &st1 = data.pt_data.stop_times[retour[cnt][current_rpid].stop_time_idx];
            const type::StopTime &st2 = data.pt_data.stop_times[data.pt_data.vehicle_journeys[st1.vehicle_journey_idx].stop_time_list[data.pt_data.route_points[retour[cnt][current_rpid].rpid_embarquement].order]];
            if(!clockwise)
                last_time.updatereverse(st2.arrival_time);
            else
                last_time.update(st2.departure_time);
            current_rpid = retour[cnt][current_rpid].rpid_embarquement;
            --cnt;
        } else {
            current_rpid = retour[cnt][current_rpid].rpid_embarquement;
            last_time = !clockwise ? retour[cnt][current_rpid].arrival : retour[cnt][current_rpid].departure;
        }
    }
    return std::make_pair(current_rpid, last_time);
}


float getWalkingTime(int count, type::idx_t rpid, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations,
                     bool clockwise, const map_retour_t &retour, const type::Data &data) {
    type::idx_t current_rpid = rpid;
    int cnt = count;
    float walking_time = 0;
    //Marche à la fin
    for(auto dest_dist : destinations) {
        if(dest_dist.first == data.pt_data.route_points[current_rpid].stop_point_idx) {
            walking_time = dest_dist.second;
        }
    }
    //Marche pendant les correspondances
    while(retour[cnt][current_rpid].type != depart) {
        if(retour[cnt][current_rpid].type == vj) {
            current_rpid = retour[cnt][current_rpid].rpid_embarquement;
            --cnt;
        } else {
            if(clockwise)
                walking_time += retour[cnt][retour[cnt][current_rpid].rpid_embarquement].arrival - retour[cnt][current_rpid].departure;
            else
                walking_time += retour[cnt][current_rpid].departure - retour[cnt][retour[cnt][current_rpid].rpid_embarquement].arrival;
            current_rpid = retour[cnt][current_rpid].rpid_embarquement;

        }

    }
    //Marche au départ
    for(auto dep_dist : departs) {
        if(dep_dist.first == data.pt_data.route_points[current_rpid].stop_point_idx)
            walking_time += dep_dist.second;
    }

    return walking_time;
}

}}}}

