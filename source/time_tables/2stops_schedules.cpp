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

#include "2stops_schedules.h"
#include "type/meta_data.h"

namespace pt = boost::posix_time;
namespace navitia { namespace timetables {


std::vector<pair_dt_st> stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                       const std::vector<std::string>& forbidden_uris,
                                        const DateTime &datetime, const DateTime &max_datetime,
                                        type::Data & data, bool disruption_active) {

    std::vector<pair_dt_st> result;
    //On va chercher tous les journey_pattern points correspondant au deuxieme filtre
    auto departure_journey_pattern_points = ptref::make_query(type::Type_e::JourneyPatternPoint,
                                                    departure_filter, forbidden_uris, data);
    auto arrival_journey_pattern_points = ptref::make_query(type::Type_e::JourneyPatternPoint,
                                                    arrival_filter, forbidden_uris, data);

    std::unordered_map<type::idx_t, size_t> departure_idx_arrival_order;
    for(type::idx_t idx : departure_journey_pattern_points) {
        const auto & jpp = data.pt_data->journey_pattern_points[idx];
        auto it_idx = std::find_if(arrival_journey_pattern_points.begin(), arrival_journey_pattern_points.end(),
                                  [&](type::idx_t idx2)
                                  {return data.pt_data->journey_pattern_points[idx2]->journey_pattern == jpp->journey_pattern;});
        if(it_idx != arrival_journey_pattern_points.end() && jpp->order < data.pt_data->journey_pattern_points[*it_idx]->order)
            departure_idx_arrival_order.insert(std::make_pair(idx, data.pt_data->journey_pattern_points[*it_idx]->order));
    }
    //On ne garde que departure qui sont dans les deux sets, et dont l'ordre est bon.
    std::remove_if(departure_journey_pattern_points.begin(), departure_journey_pattern_points.end(),
               [&](type::idx_t idx){return departure_idx_arrival_order.find(idx) == departure_idx_arrival_order.end(); });

    //On demande tous les next_departures
    auto departure_dt_st = get_stop_times(departure_journey_pattern_points, datetime, max_datetime, std::numeric_limits<int>::max(), data, disruption_active);

    //On va chercher les retours
    for(auto dep_dt_st : departure_dt_st) {
        const type::StopTime* departure_st = dep_dt_st.second;
        const type::VehicleJourney* vj = departure_st->vehicle_journey;
        const uint32_t arrival_order = departure_idx_arrival_order[departure_st->journey_pattern_point->idx];
        const type::StopTime* arrival_st = vj->stop_time_list[arrival_order];
        DateTime arrival_dt = dep_dt_st.first;
        DateTimeUtils::update(arrival_dt, arrival_st->arrival_time);
        result.push_back(std::make_pair(dep_dt_st, std::make_pair(arrival_dt, arrival_st)));
    }

    return result;
}



pbnavitia::Response stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                   const std::vector<std::string>& forbidden_uris,
                                   const std::string &str_dt, uint32_t duration, uint32_t depth, type::Data & data, bool disruption_active) {
    pbnavitia::Response pb_response;

    boost::posix_time::ptime ptime;
    try{
        ptime = boost::posix_time::from_iso_string(str_dt);
    } catch(boost::bad_lexical_cast) {
//        pb_response.set_error("DateTime " + str_dt + " is in a bad format should be YYYYMMddThhmmss ");
          fill_pb_error(pbnavitia::Error::bad_format, "DateTime is in a bad format should be YYYYMMddThhmmss",pb_response.mutable_error());
        return pb_response;
    }

    DateTime dt = DateTimeUtils::set((ptime.date() - data.meta->production_date.begin()).days(), ptime.time_of_day().total_seconds());

    DateTime max_dt;
    max_dt = dt + duration;
    std::vector<pair_dt_st> board;
    try {
        board = stops_schedule(departure_filter, arrival_filter, forbidden_uris, dt, max_dt, data, disruption_active);
    } catch(const ptref::parsing_error &parse_error) {
//        pb_response.set_error(parse_error.more);
        fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse :" + parse_error.more,pb_response.mutable_error());
        return pb_response;
    }

    auto current_time = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(dt, data), to_posix_time(max_dt, data));

    for(auto pair_dt_idx : board) {
        pbnavitia::PairStopTime * pair_stoptime = pb_response.mutable_stops_schedule()->add_board_items();
        auto stoptime = pair_stoptime->mutable_departure();
        const auto &dt_stop_time = pair_dt_idx.first;
        stoptime->set_departure_date_time(to_posix_timestamp(dt_stop_time.first, data));
        stoptime->set_arrival_date_time(to_posix_timestamp(dt_stop_time.first, data));
        const type::JourneyPatternPoint* jpp = dt_stop_time.second->journey_pattern_point;
        fill_pb_object(jpp->stop_point, data, stoptime->mutable_stop_point(), depth, current_time, action_period);

        stoptime = pair_stoptime->mutable_arrival();
        const auto &dt_stop_time2 = pair_dt_idx.second;
        stoptime->set_departure_date_time(to_posix_timestamp(dt_stop_time2.first, data));
        stoptime->set_arrival_date_time(to_posix_timestamp(dt_stop_time2.first, data));
        const type::JourneyPatternPoint* jpp2 = dt_stop_time2.second->journey_pattern_point;
        fill_pb_object(jpp2->stop_point, data, stoptime->mutable_stop_point(), depth, current_time, action_period);
    }
    return pb_response;
}


}}
