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

#include "raptor_path.h"
#include "raptor_solutions.h"
#include "raptor.h"
#include "raptor_path_defs.h"


namespace navitia { namespace routing {


std::vector<Path>
makePathes(const std::vector<std::pair<type::idx_t, navitia::time_duration> > &departures,
           const std::vector<std::pair<type::idx_t, navitia::time_duration> > &destinations,
           const type::AccessibiliteParams & accessibilite_params, const RAPTOR &raptor_,
           bool clockwise, bool disruption_active) {
    std::vector<Path> result;
    auto solutions = get_solutions(departures, destinations, !clockwise, accessibilite_params, disruption_active, raptor_);
    for(Solution solution : solutions) {
        result.push_back(makePath(solution.jpp_idx, solution.count, clockwise, disruption_active, accessibilite_params, raptor_));
    }

    return result;
}

std::pair<const type::StopTime*, uint32_t>
get_current_stidx_gap(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels,
                      const type::AccessibiliteParams & accessibilite_params, bool clockwise,  const navitia::type::Data &data, bool disruption_active) {
    const auto& label = labels[count][journey_pattern_point];
    if(label.type == boarding_type::vj || label.type == boarding_type::connection_stay_in) {
        const type::JourneyPatternPoint* jpp = data.pt_data->journey_pattern_points[journey_pattern_point];
        return best_stop_time(jpp, label.dt, accessibilite_params.vehicle_properties, clockwise, disruption_active, data, true);
    }
    return std::make_pair(nullptr, std::numeric_limits<uint32_t>::max());
}

Path
makePath(type::idx_t destination_idx, size_t countb, bool clockwise, bool disruption_active,
         const type::AccessibiliteParams & accessibilite_params,
         const RAPTOR &raptor_) {
    struct Visitor: public BasePathVisitor {
        Path result;
        void connection(type::StopPoint* departure, type::StopPoint* destination,
                    boost::posix_time::ptime dep_time, boost::posix_time::ptime arr_time,
                    type::StopPointConnection* stop_point_connection) {
            PathItem item(dep_time, arr_time);
            item.stop_points.push_back(departure);
            item.stop_points.push_back(destination);
            item.connection = stop_point_connection;
            item.type = walking;
            result.items.push_back(item);
        }

        void init_vj() {
            PathItem item;
            item.type = public_transport;
            result.items.push_back(item);
        }

        void loop_vj(const type::StopTime* st, boost::posix_time::ptime departure, boost::posix_time::ptime arrival) {
            auto& item = result.items.back();
            item.stop_points.push_back(st->journey_pattern_point->stop_point);
            item.stop_times.push_back(st);
            item.arrivals.push_back(arrival);
            item.departures.push_back(departure);
            BOOST_ASSERT(item.arrival >= item.departure);
        }

        void finish_vj(bool clockwise) {
            auto& item = result.items.back();
            if(clockwise) {
                item.arrival = item.arrivals.front();
                item.departure = item.departures.back();
            } else {
                item.arrival = item.arrivals.back();
                item.departure = item.departures.front();
            }
        }

        void change_vj(const type::StopTime* prev_st, const type::StopTime* current_st,
                       boost::posix_time::ptime prev_dt, boost::posix_time::ptime current_dt,
                       bool clockwise) {
            PathItem item(prev_dt, current_dt);
            item.type = stay_in;
            item.stop_points.push_back(prev_st->journey_pattern_point->stop_point);
            item.stop_points.push_back(current_st->journey_pattern_point->stop_point);
            finish_vj(clockwise);
            result.items.push_back(item);
            init_vj();
        }
    };
    Visitor v;
    read_path(v, destination_idx, countb, clockwise, disruption_active, accessibilite_params, raptor_);
    if(clockwise){
        std::reverse(v.result.items.begin(), v.result.items.end());
        for(auto & item : v.result.items) {
            std::reverse(item.stop_points.begin(), item.stop_points.end());
            std::reverse(item.arrivals.begin(), item.arrivals.end());
            std::reverse(item.departures.begin(), item.departures.end());
        }
    }

    if(v.result.items.size() > 0)
        v.result.duration = navitia::time_duration::from_boost_duration(v.result.items.back().arrival - v.result.items.front().departure);
    else
        v.result.duration = navitia::seconds(0);

    v.result.nb_changes = 0;
    if(v.result.items.size() > 2) {
        for(unsigned int i = 1; i <= (v.result.items.size()-2); ++i) {
            if(v.result.items[i].type == walking)
                ++ v.result.nb_changes;
        }
    }
    patch_datetimes(v.result);
    return v.result;
}


void patch_datetimes(Path &path){

    for(auto &item : path.items) {
        //if the vehicle journeys of a public transport section isn't of type regular
        //We keep only the first and the last stop time
        if(item.type == public_transport && item.stop_times.size() > 2
                && item.stop_times.front()->vehicle_journey->is_odt()) {
            item.stop_times.erase(item.stop_times.begin()+1, item.stop_times.end()-1);
            item.stop_points.erase(item.stop_points.begin()+1, item.stop_points.end()-1);
            item.arrivals.erase(item.arrivals.begin()+1, item.arrivals.end()-1);
            item.departures.erase(item.departures.begin()+1, item.departures.end()-1);
        }
    }

    PathItem previous_item = *path.items.begin();
    // We take the last transit path item to inspect interling cases.
    PathItem last_transit = *path.items.begin();
    std::vector<std::pair<int, PathItem>> to_insert;
    for(auto item = path.items.begin() + 1; item!= path.items.end(); ++item) {
        if(previous_item.departure != boost::posix_time::pos_infin) {
            if(item->type == walking || item->type == stay_in || item->type == guarantee) {
                auto duration = item->arrival - item->departure;
                item->departure = previous_item.arrival;
                item->arrival = item->departure + duration;
            } else {
                if(previous_item.type != stay_in){
                    if(last_transit.type == public_transport && last_transit.get_vj()->next_vj == (*item).get_vj()){
                        //Interlining do not add wait items
                        if (previous_item.type == waiting || previous_item.type == walking || previous_item.type == guarantee){
                            previous_item.type = stay_in;
                            -- path.nb_changes;
                        }
                    }else{
                        PathItem waitingItem=PathItem();
                        waitingItem.departure = previous_item.arrival;
                        waitingItem.arrival = item->departure;
                        waitingItem.type = waiting;
                        waitingItem.stop_points.push_back(previous_item.stop_points.front());
                        to_insert.push_back(std::make_pair(item-path.items.begin(), waitingItem));
                        BOOST_ASSERT(previous_item.arrival <= waitingItem.departure);
                        BOOST_ASSERT(waitingItem.arrival <= item->departure);
                        BOOST_ASSERT(previous_item.arrival <= item->departure);
                    }
                }
            }
        }
        if (item->type == public_transport){
            last_transit = *item;
        }
        previous_item = *item;
    }

    std::reverse(to_insert.begin(), to_insert.end());
    for(auto pos_value : to_insert) {
        path.items.insert(path.items.begin()+pos_value.first, pos_value.second);
    }

    //Deletion of waiting items when departure = destination, and update of next waiting item
    auto previous_it = path.items.begin();
    std::vector<size_t> to_delete;
    for(auto item = path.items.begin() + 1; item!= path.items.end(); ++item) {
        if(previous_it->departure != boost::posix_time::pos_infin) {
            if((previous_it->type == walking || previous_it->type == guarantee)
                    && (previous_it->stop_points.front() == previous_it->stop_points.back())) {
                item->departure = previous_it->departure;
                to_delete.push_back(previous_it-path.items.begin());
            }
        }
        previous_it = item;
    }
    std::reverse(to_delete.begin(), to_delete.end());
    for(auto pos_value : to_delete) {
        path.items.erase(path.items.begin() + pos_value);
    }
}


Path
makePathreverse(unsigned int destination_idx, unsigned int countb,
                const type::AccessibiliteParams & accessibilite_params,
                const RAPTOR &raptor_, bool disruption_active) {
    return makePath(destination_idx, countb, disruption_active, false, accessibilite_params, raptor_);
}


}}
