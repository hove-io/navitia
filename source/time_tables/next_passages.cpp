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

#include "next_passages.h"
#include "get_stop_times.h"
#include "request_handle.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/datetime.h"
#include "ptreferential/ptreferential.h"
#include "utils/paginate.h"


namespace pt = boost::posix_time;

namespace navitia { namespace timetables {

template<typename Visitor>
pbnavitia::Response
next_passages(const std::string &request,
              const std::vector<std::string>& forbidden_uris,
              const std::string &str_dt,
              uint32_t duration, uint32_t nb_stoptimes, const int depth,
              const type::AccessibiliteParams & accessibilite_params,
              type::Data & data, bool disruption_active, Visitor vis, uint32_t count,
              uint32_t start_page, const bool show_codes) {
    RequestHandle handler(vis.api_str, request, forbidden_uris, str_dt, duration, data, {});

    if(handler.pb_response.has_error()) {
        return handler.pb_response;
    }

    std::remove_if(handler.journey_pattern_points.begin(),
                   handler.journey_pattern_points.end(), vis.predicate);

    auto passages_dt_st = get_stop_times(handler.journey_pattern_points,
                            handler.date_time, handler.max_datetime,
                            nb_stoptimes, data, disruption_active, {}, accessibilite_params);
    size_t total_result = passages_dt_st.size();
    passages_dt_st = paginate(passages_dt_st, count, start_page);
    auto now = pt::second_clock::local_time();
    pt::time_period action_period(navitia::to_posix_time(handler.date_time, data),
                                  navitia::to_posix_time(handler.max_datetime, data));

    for(auto dt_stop_time : passages_dt_st) {
        pbnavitia::Passage * passage;
        if(vis.api_str == "NEXT_ARRIVALS")
            passage = handler.pb_response.add_next_arrivals();
        else
            passage = handler.pb_response.add_next_departures();
        auto departure_date = navitia::to_posix_timestamp(dt_stop_time.first, data);
        auto arrival_date = navitia::to_posix_timestamp(dt_stop_time.first, data);
        passage->mutable_stop_date_time()->set_departure_date_time(departure_date);
        passage->mutable_stop_date_time()->set_arrival_date_time(arrival_date);
        const type::JourneyPatternPoint* jpp = dt_stop_time.second->journey_pattern_point;
        fill_pb_object(jpp->stop_point, data, passage->mutable_stop_point(),
                depth, now, action_period);
        const type::VehicleJourney* vj = dt_stop_time.second->vehicle_journey;
        const type::JourneyPattern* jp = vj->journey_pattern;
        const type::Route* route = jp->route;
        const type::Line* line = route->line;
        const type::PhysicalMode* physical_mode = jp->physical_mode;
        auto m_vj = passage->mutable_vehicle_journey();
        auto m_route = m_vj->mutable_route();
        auto m_physical_mode = m_vj->mutable_journey_pattern()->mutable_physical_mode();
        fill_pb_object(vj, data, m_vj, 0, now, action_period, show_codes);
        fill_pb_object(route, data, m_route, 0, now, action_period, show_codes);
        fill_pb_object(line, data, m_route->mutable_line(), 0, now, action_period, show_codes);
        fill_pb_object(physical_mode, data, m_physical_mode, 0, now, action_period);
    }
    auto pagination = handler.pb_response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(passages_dt_st.size());
    return handler.pb_response;
}


pbnavitia::Response next_departures(const std::string &request,
        const std::vector<std::string>& forbidden_uris,
        const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes,
        const int depth, const type::AccessibiliteParams & accessibilite_params,
        type::Data & data, bool disruption_active, uint32_t count, uint32_t start_page,
        const bool show_codes) {

    struct vis_next_departures {
        struct predicate_t {
            type::Data &data;
            predicate_t(type::Data& data) : data(data){}
            bool operator()(const type::idx_t jppidx) const {
                auto jpp = data.pt_data->journey_pattern_points[jppidx];
                auto last_jpp = jpp->journey_pattern->journey_pattern_point_list.back();
                return jpp == last_jpp;
            }
        };
        std::string api_str;
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_next_departures(type::Data& data) :
            api_str("NEXT_DEPARTURES"), api_pb(pbnavitia::NEXT_DEPARTURES), predicate(data) {}
    };
    vis_next_departures vis(data);
    return next_passages(request, forbidden_uris, str_dt, duration, nb_stoptimes, depth,
                         accessibilite_params, data, disruption_active, vis, count, start_page, show_codes);
}


pbnavitia::Response next_arrivals(const std::string &request,
        const std::vector<std::string>& forbidden_uris,
        const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes,
        const int depth, const type::AccessibiliteParams & accessibilite_params,
        type::Data & data, bool disruption_active, uint32_t count, uint32_t start_page,
        const bool show_codes) {

    struct vis_next_arrivals {
        struct predicate_t {
            type::Data &data;
            predicate_t(type::Data& data) : data(data){}
            bool operator()(const type::idx_t jppidx) const{
                return data.pt_data->journey_pattern_points[jppidx]->order == 0;
            }
        };
        std::string api_str;
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_next_arrivals(type::Data& data) :
            api_str("NEXT_ARRIVALS"), api_pb(pbnavitia::NEXT_ARRIVALS), predicate(data) {}
    };
    vis_next_arrivals vis(data);
    return next_passages(request, forbidden_uris, str_dt, duration, nb_stoptimes, depth,
            accessibilite_params, data, disruption_active, vis,count , start_page, show_codes);
}


} }
