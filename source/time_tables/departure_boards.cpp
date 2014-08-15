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

#include "departure_boards.h"
#include "request_handle.h"
#include "get_stop_times.h"
#include "type/pb_converter.h"
#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "utils/paginate.h"

namespace pt = boost::posix_time;

namespace navitia { namespace timetables {

std::vector<vector_datetime> make_columuns(const vector_dt_st &stop_times) {
    std::vector<vector_datetime> result;
    DateTime prev_date = DateTimeUtils::inf;

    for(auto & item : stop_times) {
        if(prev_date == DateTimeUtils::inf ||
                (DateTimeUtils::hour(prev_date)/3600) != (DateTimeUtils::hour(item.first)/3600)) {
            //On supprime les doublons
            if(result.size() > 0) {
                auto last = result.back();
                auto it = std::unique(last.begin(), last.end());
                last.resize(std::distance(last.begin(), it));
            }
            result.push_back(vector_datetime());
        }

        result.back().push_back(item.first);
        prev_date = item.first;
    }

    if(result.size() > 0) {
        auto it = std::unique(result.back().begin(), result.back().end());
        result.back().resize(std::distance(result.back().begin(), it));
    }

    return result;
}


pbnavitia::Response
render_v0(const std::map<stop_point_line, vector_dt_st> &map_route_stop_point,
          DateTime datetime, DateTime max_datetime,
          const type::Data &data) {
    pbnavitia::Response response;
    auto current_time = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(datetime, data),
                                  to_posix_time(max_datetime, data));

    auto sort_predicate =
        [&](datetime_stop_time d1, datetime_stop_time d2)->bool {
            auto hour1 = DateTimeUtils::hour(d1.first) % DateTimeUtils::SECONDS_PER_DAY;
            auto hour2 = DateTimeUtils::hour(d2.first) % DateTimeUtils::SECONDS_PER_DAY;
            return std::abs(hour1 - DateTimeUtils::hour(datetime)) <
                   std::abs(hour2 - DateTimeUtils::hour(datetime));
        };

    for(auto id_vec : map_route_stop_point) {

        auto board = response.add_departure_boards();
        fill_pb_object(data.pt_data->stop_points[id_vec.first.first], data,
                       board->mutable_stop_point(), 1,
                       current_time, action_period);
        fill_pb_object(data.pt_data->routes[id_vec.first.second], data,
                       board->mutable_route(), 2, current_time, action_period);

        auto vec_st = id_vec.second;
        std::sort(vec_st.begin(), vec_st.end(), sort_predicate);

        for(auto vec : make_columuns(vec_st)) {
            pbnavitia::BoardItem *item = board->add_board_items();

            for(DateTime dt : vec) {
                if(!item->has_hour()) {
                    auto hours = DateTimeUtils::hour(dt)/3600;
                    item->set_hour(boost::lexical_cast<std::string>(hours));
                }
                auto minutes = (DateTimeUtils::hour(dt)%3600)/60;
                item->add_minutes(boost::lexical_cast<std::string>(minutes));
            }
        }
    }
    return response;
}


pbnavitia::Response
render_v1(const std::map<uint32_t, pbnavitia::ResponseStatus>& response_status,
          const std::map<stop_point_line, vector_dt_st> &map_route_stop_point,
          DateTime datetime, DateTime max_datetime,
          boost::optional<const std::string> calendar_id,
          uint32_t depth, const bool show_codes, const type::Data &data) {
    pbnavitia::Response response;
    auto current_time = pt::second_clock::local_time();
    auto now = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(datetime, data),
                                  to_posix_time(max_datetime, data));

//    bool display_date = ! calendar_id;
    for(auto id_vec : map_route_stop_point) {
        auto schedule = response.add_stop_schedules();
        //Each schedule has a stop_point and a route
        fill_pb_object(data.pt_data->stop_points[id_vec.first.first], data,
                       schedule->mutable_stop_point(), depth,
                       current_time, action_period, show_codes);

        auto m_route = schedule->mutable_route();
        fill_pb_object(data.pt_data->routes[id_vec.first.second], data,
                               m_route, depth, current_time, action_period, show_codes);
        if (data.pt_data->routes[id_vec.first.second]->line != nullptr){
            auto m_line = m_route->mutable_line();
            fill_pb_object(data.pt_data->routes[id_vec.first.second]->line, data,
                                   m_line, 0, current_time, action_period, show_codes);
        }
        auto pt_display_information = schedule->mutable_pt_display_informations();
        navitia::type::idx_t spt_idx = data.pt_data->routes[id_vec.first.second]->main_destination();
        const navitia::type::StopPoint* destination = nullptr;
        if(spt_idx != navitia::type::invalid_idx){
            destination = data.pt_data->stop_points[spt_idx];
        }
        fill_pb_object(data.pt_data->routes[id_vec.first.second], data,
                               pt_display_information, 0, current_time, action_period, destination);
        //Now we fill the date_times
        for(auto dt_st : id_vec.second) {
            auto date_time = schedule->add_date_times();
            fill_pb_object(dt_st.second, data, date_time, 0,
                           now, action_period, dt_st.first, calendar_id, destination);
        }
        const auto& it = response_status.find(id_vec.first.second);
        if(it != response_status.end()){
            schedule->set_response_status(it->second);
        }
    }
    return response;
}


pbnavitia::Response
departure_board(const std::string& request,
                boost::optional<const std::string> calendar_id,
                const std::vector<std::string>& forbidden_uris,
                const std::string& date,
                uint32_t duration, uint32_t depth,
                int32_t max_date_times,
                int interface_version,
                int count, int start_page, const type::Data &data, bool disruption_active,
                bool show_codes) {

    RequestHandle handler("DEPARTURE_BOARD", request, forbidden_uris, date,  duration, data, calendar_id);

    if(handler.pb_response.has_error())
        return handler.pb_response;

    if(handler.journey_pattern_points.size() == 0)
        return handler.pb_response;

    if (max_date_times < 0) {
        fill_pb_error(pbnavitia::Error::bad_filter, "stop_schedules : value of max_date_times invalid", handler.pb_response.mutable_error());
        return handler.pb_response;
    }
    if (calendar_id) {
        //check whether that calendar exists, to raise an early error
        if (data.pt_data->calendars_map.find(*calendar_id) == data.pt_data->calendars_map.end()) {
            fill_pb_error(pbnavitia::Error::bad_filter, "stop_schedules : calendar does not exist", handler.pb_response.mutable_error());
            return handler.pb_response;
        }
    }
    //  <idx_route, status>
    std::map<uint32_t, pbnavitia::ResponseStatus> response_status;


    std::map<stop_point_line, vector_dt_st> map_route_stop_point;
    //Mapping route/stop_point
    std::vector<stop_point_line> sps_routes;
    for(auto jpp_idx : handler.journey_pattern_points) {
        auto jpp = data.pt_data->journey_pattern_points[jpp_idx];
        auto route_idx  = jpp->journey_pattern->route->idx;
        auto sp_idx = jpp->stop_point->idx;
        stop_point_line key = stop_point_line(sp_idx, route_idx);
        auto find_predicate = [&](stop_point_line spl) {
            return spl.first == key.first && spl.second == key.second;
        };
        auto it = std::find_if(sps_routes.begin(), sps_routes.end(), find_predicate);
        if(it == sps_routes.end()){
            sps_routes.push_back(key);
        }
    }
    size_t total_result = sps_routes.size();
    sps_routes = paginate(sps_routes, count, start_page);
    //Trie des vecteurs de date_times stop_times
    auto sort_predicate = [](datetime_stop_time dt1, datetime_stop_time dt2) {
                    return dt1.first < dt2.first;
                };
    // On regroupe entre eux les stop_times appartenant
    // au meme couple (stop_point, route)
    // On veut en effet afficher les départs regroupés par route
    // (une route étant une vague direction commerciale
    for(auto sp_route : sps_routes) {
        std::vector<datetime_stop_time> stop_times;
        const type::StopPoint* stop_point = data.pt_data->stop_points[sp_route.first];
        const type::Route* route = data.pt_data->routes[sp_route.second];
        auto jpps = stop_point->journey_pattern_point_list;
        for(auto jpp : jpps) {
            if(jpp->journey_pattern->route != route) {
                continue;
            }
            if(stop_point->idx == jpp->journey_pattern->journey_pattern_point_list.back()->stop_point->idx) { // dans le cas de terminus
                response_status[route->idx] = pbnavitia::ResponseStatus::terminus;
                continue;
            }
            auto tmp = get_stop_times({jpp->idx}, handler.date_time,
                                      handler.max_datetime,
                                      max_date_times, data, disruption_active, calendar_id);
            if (! tmp.empty()) {
                stop_times.insert(stop_times.end(), tmp.begin(), tmp.end());
            }
        }
        std::sort(stop_times.begin(), stop_times.end(), sort_predicate);
        auto to_insert = std::pair<stop_point_line, vector_dt_st>(sp_route, stop_times);
        map_route_stop_point.insert(to_insert);
    }

    //we check if we have at least one departure for each route (and no other error)
    for(auto sp_route : sps_routes) {
        const type::Route* route = data.pt_data->routes[sp_route.second];
        if (map_route_stop_point[sp_route].empty() &&
                response_status[route->idx] == pbnavitia::ResponseStatus::none) {
            response_status[route->idx] = pbnavitia::ResponseStatus::no_departure_this_day;
        }
    }

    if(interface_version == 0) {
        handler.pb_response = render_v0(map_route_stop_point,
                                        handler.date_time,
                                        handler.max_datetime, data);
    } else if(interface_version == 1) {
        handler.pb_response = render_v1(response_status, map_route_stop_point,
                                        handler.date_time,
                                        handler.max_datetime,
                                        calendar_id, depth, show_codes, data);
    }


    auto pagination = handler.pb_response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(std::max(handler.pb_response.departure_boards_size(),
                                         handler.pb_response.stop_schedules_size()));

    return handler.pb_response;
}
}
}
