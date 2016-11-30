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
#include "routing/get_stop_times.h"
#include "type/pb_converter.h"
#include "routing/dataraptor.h"
#include "utils/paginate.h"
#include "routing/dataraptor.h"

#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/container/flat_set.hpp>

namespace pt = boost::posix_time;

namespace navitia { namespace timetables {

static bool is_last_stoptime(const nt::StopTime* stop_time, const nt::StopPoint* stp){
    return stop_time->vehicle_journey
            && stop_time->vehicle_journey->route
            && stop_time->vehicle_journey->route->destination
            && ! stop_time->vehicle_journey->stop_time_list.empty()
            && stop_time->vehicle_journey->stop_time_list.back().stop_point
            && (stop_time->vehicle_journey->stop_time_list.back().stop_point == stp);
}

static void
render(PbCreator& pb_creator,
          const std::map<stop_point_route, pbnavitia::ResponseStatus>& response_status,
          const std::map<stop_point_route, vector_dt_st>& map_route_stop_point,
          DateTime datetime,
          DateTime max_datetime,
          boost::optional<const std::string> calendar_id,
          uint32_t depth) {
    pb_creator.action_period = pt::time_period(to_posix_time(datetime, pb_creator.data),
                                               to_posix_time(max_datetime, pb_creator.data));

    for(auto id_vec : map_route_stop_point) {
        auto schedule = pb_creator.add_stop_schedules();
        //Each schedule has a stop_point and a route
        auto stp = pb_creator.data.pt_data->stop_points[id_vec.first.first.val];
        pb_creator.fill(pb_creator.data.pt_data->stop_points[id_vec.first.first.val],
                schedule->mutable_stop_point(), depth);

        auto m_route = schedule->mutable_route();
        pb_creator.fill(pb_creator.data.pt_data->routes[id_vec.first.second.val], m_route, depth);
        if (pb_creator.data.pt_data->routes[id_vec.first.second.val]->line != nullptr){
            auto m_line = m_route->mutable_line();
            pb_creator.fill(pb_creator.data.pt_data->routes[id_vec.first.second.val]->line, m_line, 0);
        }
        auto pt_display_information = schedule->mutable_pt_display_informations();

        pb_creator.fill(pb_creator.data.pt_data->routes[id_vec.first.second.val], pt_display_information, 0);

        //Now we fill the date_times
        for(auto dt_st : id_vec.second) {
            const auto& st_calendar = navitia::StopTimeCalandar(dt_st.second, dt_st.first, calendar_id);
            // terminus or partial terminus
            if (is_last_stoptime(st_calendar.stop_time, stp)) {
                continue;
            }
            auto date_time = schedule->add_date_times();
            pb_creator.fill(&st_calendar, date_time, 0);
            if (dt_st.second != nullptr) {
                auto vj = dt_st.second->vehicle_journey;
                if(vj != nullptr) {
                    for (const auto& comment: pb_creator.data.pt_data->comments.get(*vj)) {
                        pb_creator.fill(&comment, date_time->mutable_properties()->add_notes(), 0);
                    }
                }
            }
        }
        const auto& it = response_status.find(id_vec.first);
        if(it != response_status.end()){
            schedule->set_response_status(it->second);
        }
    }    
}

static time_duration to_navitia(const boost::posix_time::time_duration& dur) {
    return navitia::seconds(dur.total_seconds());
}

time_duration length_of_time(const time_duration& duration_1,
                       const time_duration& duration_2) {
    if (duration_1 <= duration_2) {
        return duration_2 - duration_1;
    } else {
        return duration_2 + (hours(24) - duration_1);
    }
}

bool between_opening_and_closing(const time_duration& me,
                                 const time_duration& opening,
                                 const time_duration& closing) {
    if (opening < closing) {
        return (opening <= me && me <= closing);
    } else {
        return (opening <= me || me <= closing);
    }
}

bool line_closed (const time_duration& duration,
             const time_duration& opening,
             const time_duration& closing,
             const pt::ptime& date) {
    const auto begin = to_navitia(date.time_of_day());
    return !between_opening_and_closing(begin, opening, closing)
            && !between_opening_and_closing((begin + duration), opening, closing)
            && duration < (length_of_time(opening, closing) + length_of_time(begin, opening));
}

static bool line_closed (const time_duration& duration,
                  const type::Route* route,
                  const pt::ptime& date) {
    if (route->line->opening_time && route->line->closing_time) {
        const auto opening = to_navitia(*route->line->opening_time);
        const auto closing = to_navitia(*route->line->closing_time);
        return line_closed(duration, opening, closing, date);
    }
    return false;
}

static std::vector<routing::JppIdx> get_jpp_from_route_point(const stop_point_route& sp_route,
                                                             const navitia::routing::dataRAPTOR& data_raptor) {
    const auto& jpps = data_raptor.jpps_from_sp[sp_route.first];
    std::vector<routing::JppIdx> routepoint_jpps;
    for (const auto& jpp_from_sp: jpps) {
        const routing::JppIdx& jpp_idx = jpp_from_sp.idx;
        const auto& jpp = data_raptor.jp_container.get(jpp_idx);
        const auto& jp = data_raptor.jp_container.get(jpp.jp_idx);
        if (jp.route_idx != sp_route.second) { continue; }

        routepoint_jpps.push_back(jpp_idx);
    }
    return routepoint_jpps;
}

void departure_board(PbCreator& pb_creator, const std::string& request,
                boost::optional<const std::string> calendar_id,
                const std::vector<std::string>& forbidden_uris,
                const pt::ptime date,
                uint32_t duration, uint32_t depth,
                int count, int start_page, const type::RTLevel rt_level, const size_t items_per_route_point) {

    RequestHandle handler(pb_creator, request, forbidden_uris, date, duration, calendar_id);

    if (pb_creator.has_error() || (handler.journey_pattern_points.size() == 0)) {
        return;
    }

    if (calendar_id) {
        //check whether that calendar exists, to raise an early error
        if (pb_creator.data.pt_data->calendars_map.find(*calendar_id) == pb_creator.data.pt_data->calendars_map.end()) {
            pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "stop_schedules : calendar does not exist");
            return;
        }
    }
    //  <stop_point_route, status>
    std::map<stop_point_route, pbnavitia::ResponseStatus> response_status;

    std::map<stop_point_route, vector_dt_st> map_route_stop_point;

    //Mapping route/stop_point
    boost::container::flat_set<stop_point_route> sps_routes;
    for(auto jpp_idx : handler.journey_pattern_points) {
        const auto& jpp = pb_creator.data.dataRaptor->jp_container.get(jpp_idx);
        const auto& jp = pb_creator.data.dataRaptor->jp_container.get(jpp.jp_idx);
        stop_point_route key = {jpp.sp_idx, jp.route_idx};
        sps_routes.insert(key);
    }
    size_t total_result = sps_routes.size();
    sps_routes = paginate(sps_routes, count, start_page);
    auto sort_predicate = [](routing::datetime_stop_time dt1, routing::datetime_stop_time dt2) {
                    return dt1.first < dt2.first;
                };
    // we group the stoptime belonging to the same pair (stop_point, route)
    // since we want to display the departures grouped by route
    // the route being a loose commercial direction
    for (const auto& sp_route: sps_routes) {
        const type::StopPoint* stop_point = pb_creator.data.pt_data->stop_points[sp_route.first.val];
        const type::Route* route = pb_creator.data.pt_data->routes[sp_route.second.val];
        const auto routepoint_jpps = get_jpp_from_route_point(sp_route, *pb_creator.data.dataRaptor);

        std::vector<routing::datetime_stop_time> stop_times;
        if (! calendar_id) {
            stop_times = routing::get_stop_times(routing::StopEvent::pick_up, routepoint_jpps, handler.date_time,
                    handler.max_datetime, items_per_route_point, pb_creator.data, rt_level);
            std::sort(stop_times.begin(), stop_times.end(), sort_predicate);
        } else {
            stop_times = routing::get_calendar_stop_times(routepoint_jpps, DateTimeUtils::hour(handler.date_time),
                    DateTimeUtils::hour(handler.max_datetime), pb_creator.data, *calendar_id);
            // for calendar we want the first stop time to start from handler.date_time
            std::sort(stop_times.begin(), stop_times.end(), routing::CalendarScheduleSort(handler.date_time));
            if (stop_times.size() > items_per_route_point) {
                stop_times.resize(items_per_route_point);
            }
        }

        //we compute the route status
        if (stop_point->stop_area == route->destination) {
            response_status[sp_route] = pbnavitia::ResponseStatus::terminus;
        } else {
            bool last_stoptime = false;
            for (auto st : stop_times) {
                auto& jp_idx = pb_creator.data.dataRaptor->jp_container.get_jp_from_vj()[navitia::routing::VjIdx(*st.second->vehicle_journey)];
                const auto& pair_jp = pb_creator.data.dataRaptor->jp_container.get_jps()[jp_idx.val];
                const auto& last_jpp = pb_creator.data.dataRaptor->jp_container.get(pair_jp.second.jpps.back());

                last_stoptime = (last_jpp.sp_idx == sp_route.first);
                if (! last_stoptime) {
                    break;
                }
            }
            if ( last_stoptime ) {
                response_status[sp_route] = pbnavitia::ResponseStatus::partial_terminus;
            }
        }
        //If there is no departure for a request with "RealTime", Test existance of any departure with "base_schedule"
        //If departure with base_schedule is not empty, additional_information = active_disruption
        //Else additional_information = no_departure_this_day
        if (stop_times.empty() && (response_status.find(sp_route) == response_status.end())) {
            auto resp_status = pbnavitia::ResponseStatus::no_departure_this_day;
            if (line_closed(navitia::seconds(duration), route, date)) {
                  resp_status = pbnavitia::ResponseStatus::no_active_circulation_this_day;
            }
            if (rt_level != navitia::type::RTLevel::Base) {
                auto tmp_stop_times = routing::get_stop_times(routing::StopEvent::pick_up, routepoint_jpps, handler.date_time,
                                                              handler.max_datetime, 1, pb_creator.data,
                                                              navitia::type::RTLevel::Base);
                if (!tmp_stop_times.empty()) { resp_status = pbnavitia::ResponseStatus::active_disruption; }
            }
            response_status[sp_route] = resp_status;
        }

        map_route_stop_point[sp_route] = stop_times;
    }

    render(pb_creator, response_status, map_route_stop_point, handler.date_time, handler.max_datetime,
              calendar_id, depth);

    pb_creator.make_paginate(total_result, start_page, count, std::max(pb_creator.departure_boards_size(),
                                                                       pb_creator.stop_schedules_size()));
}
}
}
