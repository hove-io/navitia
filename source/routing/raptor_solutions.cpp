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
get_solutions(const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
             const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations,
             bool clockwise, const type::AccessibiliteParams & accessibilite_params,
             bool disruption_active, const RAPTOR& raptor) {
      Solutions result;
      auto pareto_front = get_pareto_front(clockwise, departs, destinations,
              accessibilite_params, disruption_active, raptor);
      result.insert(pareto_front.begin(), pareto_front.end());

      if(!pareto_front.empty()) {
          auto walking_solutions = get_walking_solutions(clockwise, departs, destinations,
                  *pareto_front.begin(), disruption_active, accessibilite_params, raptor);
          if(!walking_solutions.empty()) {
            result.insert(walking_solutions.begin(), walking_solutions.end());
          }
      }
      return result;
}


Solutions
init_departures(const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
              const DateTime &dep, bool clockwise, bool) {
    Solutions result;
    for(auto dep_dist : departs) {
        Solution d;
        d.count = 0;
        d.sp_idx = dep_dist.first;
        d.walking_time = dep_dist.second;
        if(clockwise)
            d.arrival = dep + d.walking_time.total_seconds();
        else
            d.arrival = dep - d.walking_time.total_seconds();
        result.insert(d);
    }
    return result;
}


static DateTime
combine_dt_walking_time(bool clockwise, const DateTime& current, int walking_duration) {
    return clockwise ? current - walking_duration : current + walking_duration;
}


static size_t nb_jpp_of_path(int count,
                             SpIdx sp_idx,
                             bool clockwise,
                             bool disruption_active,
                             const type::AccessibiliteParams& accessibilite_params,
                             const RAPTOR& raptor) {
    struct VisitorNbJPP : public BasePathVisitor {
        size_t nb_jpp = 0;
        void loop_vj(const type::StopTime*, boost::posix_time::ptime, boost::posix_time::ptime) {
            ++nb_jpp;
        }

        void change_vj(const type::StopTime*, const type::StopTime*,
                       boost::posix_time::ptime ,boost::posix_time::ptime,
                       bool) {
            ++nb_jpp;
        }
    };
    VisitorNbJPP v;
    read_path(v, sp_idx, count, !clockwise, disruption_active, accessibilite_params, raptor);
    return v.nb_jpp;
}

Solutions
get_pareto_front(bool clockwise, const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
               const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations,
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
        // For every round we look for the best destination stop points
        // We must not forget to take into account the walking duration
        SpIdx best_sp = SpIdx();
        size_t best_nb_jpp_of_path = std::numeric_limits<size_t>::max();
        for(auto spid_dist : destinations) {

            auto sp_idx = spid_dist.first;
            const auto& ls = raptor.labels[round];
            if (! ls.pt_is_initialized(sp_idx)) {
                continue;
            }
            size_t nb_jpp = nb_jpp_of_path(round, sp_idx, clockwise, disruption_active,
                                           accessibilite_params, raptor);
            const auto& dt_pt = ls.dt_pt(sp_idx);
            const auto total_arrival = combine_dt_walking_time(clockwise, dt_pt,
                                                               spid_dist.second.total_seconds());
            if (!improves(total_arrival, best_dt, clockwise)) {
                if (best_dt != total_arrival ||
                        best_nb_jpp_of_path <= nb_jpp) {
                    continue;
                }
            }
            best_sp = sp_idx;
            best_dt_jpp = dt_pt;
            best_nb_jpp_of_path = nb_jpp;
            // When computing with clockwise, in the second pass we store deparutre time
            // in labels, but we want arrival time, so we need to retrive the good stop_time
            // with best_stop_time
            const type::StopTime* st = nullptr;
            DateTime dt = 0;

            std::tie(st, dt) = get_current_stidx_gap(round,
                                                     sp_idx,
                                                     raptor.labels,
                                                     accessibilite_params,
                                                     !clockwise,
                                                     raptor,
                                                     disruption_active);
            if(st != nullptr) {
                if(clockwise) {
                    auto arrival_time = !st->is_frequency() ?
                                st->arrival_time :
                                st->f_arrival_time(DateTimeUtils::hour(dt));
                    DateTimeUtils::update(best_dt_jpp, arrival_time, false);
                } else {
                    auto departure_time = !st->is_frequency() ?
                                st->departure_time :
                                st->f_departure_time(DateTimeUtils::hour(dt));
                    DateTimeUtils::update(best_dt_jpp, departure_time, true);
                }
                best_dt = total_arrival;
            }
        }
        if (best_sp.is_valid()) {
            Solution s;
            s.sp_idx = best_sp;
            s.count = round;
            s.walking_time = getWalkingTime(round, best_sp, departs, destinations, clockwise, disruption_active,
                                            accessibilite_params, raptor);
            s.arrival = best_dt_jpp;
            s.ratio = 0;
            s.total_arrival = best_dt;
            s.clockwise = clockwise;
            SpIdx final_sp_idx;
            std::tie(final_sp_idx, s.upper_bound) = get_final_spidx_and_date(round, best_sp, clockwise,
                                                                             disruption_active, accessibilite_params, raptor);
            //TODO l'heure de depart (upper bound) n'est pas bon pour le moment, voir is il ya moyen de mieux le calculer
            for(auto spid_dep : departs) {
                if (final_sp_idx == spid_dep.first) {
                    s.upper_bound = combine_dt_walking_time(!clockwise, s.upper_bound, spid_dep.second.total_seconds());
                }
            }
            result.insert(s);
        }
    }

    return result;
}





Solutions
get_walking_solutions(bool clockwise, const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
                      const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations, const Solution& best,
                      const bool disruption_active, const type::AccessibiliteParams &accessibilite_params,
                      const RAPTOR& raptor) {
    Solutions result;

    std::map<SpIdx, Solution> tmp;
    // We start at 1 because we don't want results of the first round
    for(uint32_t i=1; i <= raptor.count; ++i) {
        for(auto spid_dist : destinations) {
            Solution best_departure;
            best_departure.clockwise = clockwise;
            best_departure.ratio = 2;
            best_departure.sp_idx = SpIdx();

            SpIdx sp_idx = spid_dist.first;

            if(! raptor.labels[i].pt_is_initialized(sp_idx)) {
                continue;
            }

            navitia::time_duration walking_time = getWalkingTime(i, sp_idx, departs, destinations, clockwise,
                                                                 disruption_active, accessibilite_params, raptor);
            if(best.walking_time <= walking_time) {
                continue;
            }
            // We accept to have a 10mn worst solution if we can reduce the walking time by 5mn
            int walking_time_diff_in_s = (best.walking_time - walking_time).total_seconds();
            float lost_time;
            const auto total_arrival = combine_dt_walking_time(clockwise, raptor.labels[i].dt_pt(sp_idx),
                                                               spid_dist.second.total_seconds());
            if(clockwise) {
                assert (best.total_arrival >= total_arrival);
                lost_time = best.total_arrival - total_arrival;
            }
            else {
                assert (total_arrival >= best.total_arrival);
                lost_time = total_arrival - best.total_arrival;
            }


            float ratio = lost_time / walking_time_diff_in_s;
            if( ratio >= best_departure.ratio) {
                continue;
            }
            Solution s;
            s.clockwise = clockwise;
            s.sp_idx = sp_idx;
            s.count = i;
            s.ratio = ratio;
            s.walking_time = walking_time;
            s.arrival = raptor.labels[i].dt_pt(sp_idx);
            SpIdx final_sp_idx;
            DateTime last_time;
            std::tie(final_sp_idx, last_time) = get_final_spidx_and_date(i, sp_idx, clockwise,
                                disruption_active, accessibilite_params, raptor);

            s.upper_bound = last_time;
            for(auto spid_dep : departs) {
                if(final_sp_idx == spid_dep.first) {
                    s.upper_bound = combine_dt_walking_time(!clockwise, s.upper_bound, spid_dep.second.total_seconds());
                }
            }
            best_departure = s;

            if (best_departure.sp_idx.is_valid()) {
                if (tmp.find(best_departure.sp_idx) == tmp.end()) {
                    tmp.insert(std::make_pair(best_departure.sp_idx, best_departure));
                } else if (tmp[best_departure.sp_idx].ratio > best_departure.ratio) {
                    tmp[best_departure.sp_idx] = best_departure;
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
    SpIdx current_sp = SpIdx();
    DateTime last_time = DateTimeUtils::inf;
    void final_step(const SpIdx current_sp, size_t count, const std::vector<Labels> &labels) {
        this->current_sp = current_sp;
        last_time = labels[count].dt_pt(current_sp);
    }
};

// Reparcours l’itinéraire rapidement pour avoir le JPP et la date de départ (si on cherchait l’arrivée au plus tôt)
std::pair<SpIdx, DateTime>
get_final_spidx_and_date(int count, SpIdx sp_idx, bool clockwise, bool disruption_active,
                          const type::AccessibiliteParams & accessibilite_params, const RAPTOR& raptor) {
    VisitorFinalJppAndDate v;
    read_path(v, sp_idx, count, !clockwise, disruption_active, accessibilite_params, raptor);
    return std::make_pair(v.current_sp, v.last_time);
}


struct VisitorWalkingTime : public BasePathVisitor {
    SpIdx departure_sp_idx = SpIdx();

    void final_step(SpIdx current_sp, size_t , const std::vector<Labels> &) {
        departure_sp_idx = current_sp;
    }

};


navitia::time_duration getWalkingTime(int count, SpIdx sp_idx, const std::vector<std::pair<SpIdx, navitia::time_duration> > &departs,
                     const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations,
                     bool clockwise, bool disruption_active, const type::AccessibiliteParams & accessibilite_params,
                     const RAPTOR& raptor) {
    navitia::time_duration walking_time = {};

    //Marche à la fin
    for(auto dest_dist : destinations) {
        if(dest_dist.first == sp_idx) {
            walking_time = dest_dist.second;
            break;
        }
    }

    VisitorWalkingTime v;
    read_path(v, sp_idx, count, !clockwise, disruption_active, accessibilite_params, raptor);
    if (!v.departure_sp_idx.is_valid()) {
        return walking_time;
    }
    //Marche au départ
    for(auto dep_dist : departs) {
        if (dep_dist.first == v.departure_sp_idx) {
            walking_time += dep_dist.second;
            break;
        }
    }

    return walking_time;
}

}}

