/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "raptor_solutions.h"
#include "raptor_path.h"
#include "raptor.h"
#include "raptor_path_defs.h"


namespace bt = boost::posix_time;
namespace navitia { namespace routing {

Solutions
get_solutions(const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
             const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations,
             bool clockwise, const type::AccessibiliteParams & accessibilite_params,
             bool disruption_active, const RAPTOR& raptor) {
      Solutions result;
      auto pareto_front = get_pareto_front(clockwise, departs, destinations,
              accessibilite_params, disruption_active, raptor);
      result.insert(pareto_front.begin(), pareto_front.end());

      if(!pareto_front.empty()) {
          auto walking_solutions = get_walking_solutions(clockwise, departs, destinations,
                  *pareto_front.rbegin(), disruption_active, accessibilite_params, raptor);
          if(!walking_solutions.empty()) {
            result.insert(walking_solutions.begin(), walking_solutions.end());
          }
      }
      return result;
}


Solutions
get_solutions(const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
              const DateTime &dep, bool clockwise, const type::Data & data, bool) {
    Solutions result;
    for(auto dep_dist : departs) {
        for(auto journey_pattern : data.pt_data->stop_points[dep_dist.first]->journey_pattern_point_list) {
            Solution d;
            d.count = 0;
            d.jpp_idx = journey_pattern->idx;
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
get_pareto_front(bool clockwise, const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
               const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations,
               const type::AccessibiliteParams & accessibilite_params, bool disruption_active, const RAPTOR& raptor) {
    Solutions result;

    DateTime best_dt, best_dt_jpp;
    if(clockwise) {
        best_dt = DateTimeUtils::min;
        best_dt_jpp = DateTimeUtils::min;
    } else {
        best_dt = DateTimeUtils::inf;
        best_dt_jpp = DateTimeUtils::inf;
    }
    for(unsigned int round=1; round <= raptor.count; ++round) {
        // For every round with look for the best journey pattern point that belongs to one of the destination stop points
        // We must not forget to walking duration
        type::idx_t best_jpp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto journey_pattern_point : raptor.data.pt_data->stop_points[spid_dist.first]->journey_pattern_point_list) {
                type::idx_t jppidx = journey_pattern_point->idx;
                auto& l = raptor.labels[round][jppidx];
                if(l.pt_is_initialized() &&
                   improves(best_dt, clockwise, l.dt_pt, spid_dist.second.total_seconds()) ) {
                    best_jpp = jppidx;
                    best_dt_jpp = l.dt_pt;
                    // Dans le sens horaire : lors du calcul on gardé que l’heure de départ, mais on veut l’arrivée
                    // Il faut donc retrouver le stop_time qui nous intéresse avec best_stop_time
                    const type::StopTime* st;
                    DateTime dt = 0;

                    std::tie(st, dt) = best_stop_time(journey_pattern_point, l.dt_pt, accessibilite_params.vehicle_properties,
                                                      !clockwise, disruption_active, raptor.data, true);
                    BOOST_ASSERT(st);
                    if(st != nullptr) {
                        if(clockwise) {
                            auto arrival_time = !st->is_frequency() ? st->arrival_time : st->f_arrival_time(DateTimeUtils::hour(dt));
                            DateTimeUtils::update(best_dt_jpp, arrival_time, false);
                        } else {
                            auto departure_time = !st->is_frequency() ? st->departure_time : st->f_departure_time(DateTimeUtils::hour(dt));
                            DateTimeUtils::update(best_dt_jpp, departure_time, true);
                        }
                    }
                    if(clockwise)
                        best_dt = l.dt_pt - (spid_dist.second.total_seconds());
                    else
                        best_dt = l.dt_pt + (spid_dist.second.total_seconds());
                }
            }
        }
        if(best_jpp != type::invalid_idx) {
            Solution s;
            s.jpp_idx = best_jpp;
            s.count = round;
            s.walking_time = getWalkingTime(round, best_jpp, departs, destinations, clockwise, disruption_active,
                                            accessibilite_params, raptor);
            s.arrival = best_dt_jpp;
            s.ratio = 0;
            type::idx_t final_jpp_idx;
            std::tie(final_jpp_idx, s.upper_bound) = get_final_jppidx_and_date(round, best_jpp, clockwise,
                                                                             disruption_active, accessibilite_params, raptor);
            for(auto spid_dep : departs) {
                if(raptor.data.pt_data->journey_pattern_points[final_jpp_idx]->stop_point->idx == spid_dep.first) {
                    if(clockwise) {
                        s.upper_bound = s.upper_bound + spid_dep.second.total_seconds();
                        s.total_arrival = raptor.labels[round][best_jpp].dt_pt - spid_dep.second.total_seconds();
                    }else {
                        s.upper_bound = s.upper_bound - (spid_dep.second.total_seconds());
                        s.total_arrival = raptor.labels[round][best_jpp].dt_pt + spid_dep.second.total_seconds();
                    }
                }
            }
            result.insert(s);
        }
    }

    return result;
}





Solutions
get_walking_solutions(bool clockwise, const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
                      const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations, const Solution& best,
                      const bool disruption_active, const type::AccessibiliteParams &accessibilite_params,
                      const RAPTOR& raptor) {
    Solutions result;

    std::/*unordered_*/map<type::idx_t, Solution> tmp;
    // We start at 1 because we don't want results of the first round
    for(uint32_t i=1; i <= raptor.count; ++i) {
        for(auto spid_dist : destinations) {
            Solution best_departure;
            best_departure.ratio = 2;
            best_departure.jpp_idx = type::invalid_idx;
            for(auto journey_pattern_point : raptor.data.pt_data->stop_points[spid_dist.first]->journey_pattern_point_list) {
                type::idx_t jppidx = journey_pattern_point->idx;
                // We only want solution ending by a vehicle journey or a stay_in
                if(raptor.labels[i][journey_pattern_point->idx].pt_is_initialized()) {
                    navitia::time_duration walking_time = getWalkingTime(i, jppidx, departs, destinations, clockwise,
                                                                         disruption_active, accessibilite_params, raptor);
                    if(best.walking_time < walking_time) {
                        continue;
                    }
                    float lost_time;
                    if(clockwise)
                        lost_time = best.total_arrival -
                                    (raptor.labels[i][jppidx].dt_pt - best.walking_time.total_seconds());
                    else
                        lost_time = (raptor.labels[i][jppidx].dt_pt + spid_dist.second.total_seconds()) -
                                best.total_arrival;


                    //Si je gagne 5 minutes de marche a pied, je suis pret à perdre jusqu'à 10 minutes.
                    int walking_time_diff_in_s = (best.walking_time - walking_time).total_seconds();
                    if (walking_time_diff_in_s > 0) {
                        float ratio = lost_time / walking_time_diff_in_s;
                        if( ratio >= best_departure.ratio) {
                            continue;
                        }
                        Solution s;
                            s.jpp_idx = jppidx;
                        s.count = i;
                        s.ratio = ratio;
                        s.walking_time = walking_time;
                        s.arrival = raptor.labels[i][jppidx].dt_pt;
                        type::idx_t final_jpp_idx;
                        DateTime last_time;
                        std::tie(final_jpp_idx, last_time) = get_final_jppidx_and_date(i, jppidx, clockwise,
                                            disruption_active, accessibilite_params, raptor);

                        if(clockwise) {
                            s.upper_bound = last_time;
                            for(auto spid_dep : departs) {
                                if(raptor.data.pt_data->journey_pattern_points[final_jpp_idx]->stop_point->idx == spid_dep.first) {
                                    s.upper_bound = s.upper_bound + (spid_dep.second.total_seconds());
                                }
                            }
                        } else {
                            s.upper_bound = last_time;
                            for(auto spid_dep : departs) {
                                if(raptor.data.pt_data->journey_pattern_points[final_jpp_idx]->stop_point->idx == spid_dep.first) {
                                    s.upper_bound = s.upper_bound - (spid_dep.second.total_seconds());
                                }
                            }
                        }
                        best_departure = s;
                    }
                }
            }
            if(best_departure.jpp_idx != type::invalid_idx) {
                if(tmp.find(best_departure.jpp_idx) == tmp.end()) {
                    tmp.insert(std::make_pair(best_departure.jpp_idx, best_departure));
                } else if(tmp[best_departure.jpp_idx].ratio > best_departure.ratio) {
                    tmp[best_departure.jpp_idx] = best_departure;
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

struct VisitorFinalJppAndDate : public BasePathVisitor {
    type::idx_t current_jpp = type::invalid_idx;
    DateTime last_time = DateTimeUtils::inf;
    void final_step(const type::idx_t current_jpp, size_t count, const std::vector<label_vector_t> &labels) {
        this->current_jpp = current_jpp;
        last_time = labels[count][current_jpp].dt_pt;
    }
};

// Reparcours l’itinéraire rapidement pour avoir le JPP et la date de départ (si on cherchait l’arrivée au plus tôt)
std::pair<type::idx_t, DateTime>
get_final_jppidx_and_date(int count, type::idx_t jpp_idx, bool clockwise, bool disruption_active,
                          const type::AccessibiliteParams & accessibilite_params, const RAPTOR& raptor) {
    VisitorFinalJppAndDate v;
    read_path(v, jpp_idx, count, !clockwise, disruption_active, accessibilite_params, raptor);
    return std::make_pair(v.current_jpp, v.last_time);
}


struct VisitorWalkingTime : public BasePathVisitor {
    navitia::time_duration walking_time = {};
    void connection(type::StopPoint* , type::StopPoint* ,
                boost::posix_time::ptime dep_time, boost::posix_time::ptime arr_time,
                type::StopPointConnection*) {

        walking_time += navitia::seconds((arr_time - dep_time).total_seconds());
    }
};


navitia::time_duration getWalkingTime(int count, type::idx_t jpp_idx, const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departs,
                     const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations,
                     bool clockwise, bool disruption_active, const type::AccessibiliteParams & accessibilite_params,
                     const RAPTOR& raptor) {

    const type::JourneyPatternPoint* current_jpp = raptor.data.pt_data->journey_pattern_points[jpp_idx];
    navitia::time_duration walking_time = {};

    //Marche à la fin
    for(auto dest_dist : destinations) {
        if(dest_dist.first == current_jpp->stop_point->idx) {
            walking_time = dest_dist.second;
            break;
        }
    }

    VisitorWalkingTime v;
    read_path(v, current_jpp->idx, count, !clockwise, disruption_active, accessibilite_params, raptor);
    walking_time += v.walking_time;
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

