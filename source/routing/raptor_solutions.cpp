#include "raptor_solutions.h"

namespace bt = boost::posix_time;
namespace navitia { namespace routing {

Solutions
getSolutions(const std::vector<std::pair<type::idx_t, bt::time_duration> > &departs,
             const std::vector<std::pair<type::idx_t, bt::time_duration> > &destinations,
             bool clockwise, const std::vector<label_vector_t> &labels,
             const type::AccessibiliteParams & accessibilite_params, const type::Data &data,
             bool disruption_active) {
      Solutions result;
      auto pareto_front = getParetoFront(clockwise, departs, destinations, labels,
              accessibilite_params, data, disruption_active);
      result.insert(pareto_front.begin(), pareto_front.end());

      if(!pareto_front.empty()) {
          auto walking_solutions = getWalkingSolutions(clockwise, departs, destinations,
                  *pareto_front.rbegin(), labels, data);
          if(!walking_solutions.empty()) {
            result.insert(walking_solutions.begin(), walking_solutions.end());
          }
      }
      return result;
}


Solutions
getSolutions(const std::vector<std::pair<type::idx_t, bt::time_duration> > &departs, const DateTime &dep, bool clockwise, const type::Data & data, bool) {
    Solutions result;
    for(auto dep_dist : departs) {
        for(auto journey_pattern : data.pt_data->stop_points[dep_dist.first]->journey_pattern_point_list) {
            Solution d;
            d.count = 0;
            d.rpidx = journey_pattern->idx;
            d.walking_time = dep_dist.second;
            if(clockwise)
                d.arrival = dep + d.walking_time.total_seconds();
            else
                d.arrival = dep - d.walking_time.total_seconds();
            result.insert(d);
        }
    }
    return result;
}

// Does the current date improves compared to best_so_far – we must not forget to take the walking duration
bool improves(const DateTime & best_so_far, bool clockwise, const DateTime & current, int walking_duration) {
    if(clockwise) {
        return (current - walking_duration) > best_so_far;
    } else {
        return (current + walking_duration) < best_so_far;
    }
}

Solutions
getParetoFront(bool clockwise, const std::vector<std::pair<type::idx_t, bt::time_duration> > &departs,
               const std::vector<std::pair<type::idx_t, bt::time_duration> > &destinations,
               const std::vector<label_vector_t> &labels,
               const type::AccessibiliteParams & accessibilite_params, const type::Data &data, bool disruption_active){
    Solutions result;

    DateTime best_dt, best_dt_jpp;
    if(clockwise) {
        best_dt = DateTimeUtils::min;
        best_dt_jpp = DateTimeUtils::min;
    } else {
        best_dt = DateTimeUtils::inf;
        best_dt_jpp = DateTimeUtils::inf;
    }
    for(unsigned int round=1; round < labels.size(); ++round) {
        // For every round with look for the best journey pattern point that belongs to one of the destination stop points
        // We must not forget to walking duration
        type::idx_t best_jpp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto journey_pattern_point : data.pt_data->stop_points[spid_dist.first]->journey_pattern_point_list) {
                type::idx_t jppidx = journey_pattern_point->idx;
                auto type = labels[round][journey_pattern_point->idx].type;
                auto& l = labels[round][jppidx];
                if((type == boarding_type::vj) &&
                   l.dt != DateTimeUtils::inf &&
                   l.dt != DateTimeUtils::min &&
                   improves(best_dt, clockwise, l.dt, spid_dist.second.total_seconds()) ) {
                    best_jpp = jppidx;
                    best_dt_jpp = l.dt;
                    // Dans le sens horaire : lors du calcul on gardé que l’heure de départ, mais on veut l’arrivée
                    // Il faut donc retrouver le stop_time qui nous intéresse avec best_stop_time
                    const type::StopTime* st;
                    DateTime dt = 0;

                    std::tie(st, dt) = best_stop_time(journey_pattern_point, l.dt, accessibilite_params.vehicle_properties, !clockwise, disruption_active, data, true);
                    BOOST_ASSERT(st);
                    if(st != nullptr) {
                        if(clockwise) {
                            auto arrival_time = !st->is_frequency() ? st->arrival_time : st->f_arrival_time(DateTimeUtils::hour(dt));
                            DateTimeUtils::update(best_dt_jpp, arrival_time, !clockwise);
                        } else {
                            auto departure_time = !st->is_frequency() ? st->departure_time : st->f_departure_time(DateTimeUtils::hour(dt));
                            DateTimeUtils::update(best_dt_jpp, departure_time, !clockwise);
                        }
                    }
                    if(clockwise)
                        best_dt = l.dt - (spid_dist.second.total_seconds());
                    else
                        best_dt = l.dt + (spid_dist.second.total_seconds());
                }
            }
        }
        if(best_jpp != type::invalid_idx) {
            Solution s;
            s.rpidx = best_jpp;
            s.count = round;
            s.walking_time = getWalkingTime(round, best_jpp, departs, destinations, clockwise, labels, data);
            s.arrival = best_dt_jpp;
            s.ratio = 0;
            type::idx_t final_rpidx;
            std::tie(final_rpidx, s.upper_bound) = getFinalRpidAndDate(round, best_jpp, clockwise, labels);
            for(auto spid_dep : departs) {
                if(data.pt_data->journey_pattern_points[final_rpidx]->stop_point->idx == spid_dep.first) {
                    if(clockwise) {
                        s.upper_bound = s.upper_bound + (spid_dep.second.total_seconds());
                    }else {
                        s.upper_bound = s.upper_bound - (spid_dep.second.total_seconds());
                    }
                }
            }
            result.insert(s);
        }
    }

    return result;
}





Solutions
getWalkingSolutions(bool clockwise, const std::vector<std::pair<type::idx_t, bt::time_duration> > &departs, const std::vector<std::pair<type::idx_t, bt::time_duration> > &destinations, Solution best,
                    const std::vector<label_vector_t> &labels, const type::Data &data){
    Solutions result;

    std::/*unordered_*/map<type::idx_t, Solution> tmp;

    for(uint32_t i=0; i<labels.size(); ++i) {
        for(auto spid_dist : destinations) {
            Solution best_departure;
            best_departure.ratio = 2;
            best_departure.rpidx = type::invalid_idx;
            for(auto journey_pattern_point : data.pt_data->stop_points[spid_dist.first]->journey_pattern_point_list) {
                type::idx_t jppidx = journey_pattern_point->idx;
                if(labels[i][journey_pattern_point->idx].type != boarding_type::uninitialized) {
                    bt::time_duration walking_time = getWalkingTime(i, jppidx, departs, destinations, clockwise, labels, data);
                    if(best.walking_time <= walking_time) {
                        continue;
                    }
                    float lost_time;
                    if(clockwise)
                        lost_time = labels[i][jppidx].dt - (spid_dist.second.total_seconds()) - best.arrival;
                    else
                        lost_time = labels[i][jppidx].dt + (spid_dist.second.total_seconds()) - best.arrival;


                    //Si je gagne 5 minutes de marche a pied, je suis pret à perdre jusqu'à 10 minutes.
                    int walking_time_diff_in_s = (best.walking_time - walking_time).total_seconds();
                    if (walking_time_diff_in_s > 0) {
                        float ratio = lost_time / walking_time_diff_in_s;
                        if( ratio < best_departure.ratio) {
                            Solution s;
                            s.rpidx = jppidx;
                            s.count = i;
                            s.ratio = ratio;
                            s.walking_time = walking_time;
                            s.arrival = labels[i][jppidx].dt;
                            type::idx_t final_rpidx;
                            DateTime last_time;
                            std::tie(final_rpidx, last_time) = getFinalRpidAndDate(i, jppidx, clockwise, labels);
                            if(clockwise) {
                                s.upper_bound = last_time;
                                for(auto spid_dep : departs) {
                                    if(data.pt_data->journey_pattern_points[final_rpidx]->stop_point->idx == spid_dep.first) {
                                        s.upper_bound = s.upper_bound + (spid_dep.second.total_seconds());
                                    }
                                }
                            } else {
                                s.upper_bound = last_time;
                                for(auto spid_dep : departs) {
                                    if(data.pt_data->journey_pattern_points[final_rpidx]->stop_point->idx == spid_dep.first) {
                                        s.upper_bound = s.upper_bound - (spid_dep.second.total_seconds());
                                    }
                                }
                            }

                            best_departure = s;
                        }
                    }
                }
            }
            if(best_departure.rpidx != type::invalid_idx) {
                type::idx_t journey_pattern = data.pt_data->journey_pattern_points[best_departure.rpidx]->journey_pattern->idx;
                if(tmp.find(journey_pattern) == tmp.end()) {
                    tmp.insert(std::make_pair(journey_pattern, best_departure));
                } else if(tmp[journey_pattern].ratio > best_departure.ratio) {
                    tmp[journey_pattern] = best_departure;
                }
            }

        }
    }


    std::vector<Solution> to_sort;
    for(auto p : tmp) {
        to_sort.push_back(p.second);
    }
    std::sort(to_sort.begin(), to_sort.end(),
            [](const Solution s1, const Solution s2) { return s1.ratio < s2.ratio;});

    if(to_sort.size() > 2)
        to_sort.resize(2);
    result.insert(to_sort.begin(), to_sort.end());

    return result;
}


// Reparcours l’itinéraire rapidement pour avoir le JPP et la date de départ (si on cherchait l’arrivée au plus tôt)
std::pair<type::idx_t, DateTime>
getFinalRpidAndDate(int count, type::idx_t jpp_idx, bool clockwise, const std::vector<label_vector_t> &labels) {
    type::idx_t current_jpp = jpp_idx;
    int cnt = count;

    DateTime last_time = labels[cnt][current_jpp].dt;
    while(labels[cnt][current_jpp].type != boarding_type::departure) {
        if(labels[cnt][current_jpp].type == boarding_type::vj) {
            DateTimeUtils::update(last_time, DateTimeUtils::hour(labels[cnt][current_jpp].dt), clockwise);
            current_jpp = labels[cnt][current_jpp].boarding->idx;
            --cnt;
        } else {
            current_jpp = labels[cnt][current_jpp].boarding->idx;
            last_time = labels[cnt][current_jpp].dt;
        }
    }
    return std::make_pair(current_jpp, last_time);
}


boost::posix_time::time_duration getWalkingTime(int count, type::idx_t jpp_idx, const std::vector<std::pair<type::idx_t, bt::time_duration> > &departs,
                     const std::vector<std::pair<type::idx_t, bt::time_duration> > &destinations,
                     bool clockwise, const std::vector<label_vector_t> &labels, const type::Data &data) {

    const type::JourneyPatternPoint* current_jpp = data.pt_data->journey_pattern_points[jpp_idx];
    int cnt = count;
    bt::time_duration walking_time = {};

    //Marche à la fin
    for(auto dest_dist : destinations) {
        if(dest_dist.first == current_jpp->stop_point->idx) {
            walking_time = dest_dist.second;
            break;
        }
    }
    //Marche pendant les correspondances
    auto boarding_type_value = labels[count][jpp_idx].type;
    while(boarding_type_value != boarding_type::departure /*&& boarding_type_value != boarding_type::uninitialized*/) {
        if(boarding_type_value == boarding_type::vj) {
            current_jpp = labels[cnt][current_jpp->idx].boarding;
            --cnt;
            boarding_type_value = labels[cnt][current_jpp->idx].type;
        } else {
            const type::JourneyPatternPoint* boarding = labels[cnt][current_jpp->idx].boarding;
            if(boarding_type_value == boarding_type::connection) {
                type::idx_t connection_idx = data.dataRaptor->get_stop_point_connection_idx(boarding->stop_point->idx,
                                                                                           current_jpp->stop_point->idx,
                                                                                           clockwise, *data.pt_data);
                if(connection_idx != type::invalid_idx)
                    walking_time += bt::seconds(data.pt_data->stop_point_connections[connection_idx]->duration);
            }
            current_jpp = boarding;
            boarding_type_value = labels[cnt][current_jpp->idx].type;

        }
    }
    //Marche au départ
    for(auto dep_dist : departs) {
        if(dep_dist.first == current_jpp->stop_point->idx) {
            walking_time += dep_dist.second;
            break;
        }
    }

    return walking_time;
}

}}

