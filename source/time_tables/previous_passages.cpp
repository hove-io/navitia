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

#include "previous_passages.h"
#include "get_stop_times.h"
#include "request_handle.h"
#include "type/datetime.h"
#include "ptreferential/ptreferential.h"
#include "utils/paginate.h"
#include "routing/dataraptor.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

namespace pt = boost::posix_time;
using navitia::routing::StopEvent;

namespace navitia { namespace timetables {

template<typename Visitor>
pbnavitia::Response
previous_passages(const std::string &request,
              const std::vector<std::string>& forbidden_uris,
              const pt::ptime datetime,
              uint32_t duration, uint32_t nb_stoptimes, const int depth,
              const type::AccessibiliteParams & accessibilite_params,
              const type::Data & data, const type::RTLevel rt_level, Visitor vis, uint32_t count,
              uint32_t start_page, const bool show_codes, const boost::posix_time::ptime current_datetime) {
    RequestHandle handler(request, forbidden_uris, datetime, duration, data, {}, false);

    if(handler.pb_response.has_error()) {
        return handler.pb_response;
    }

    boost::remove_erase_if(handler.journey_pattern_points, vis.predicate);

    const StopEvent stop_event = (vis.api_pb == pbnavitia::PREVIOUS_DEPARTURES) ?
                                 StopEvent::pick_up : StopEvent::drop_off;
    auto passages_dt_st = get_stop_times(stop_event, handler.journey_pattern_points,
                                         handler.date_time, handler.max_datetime, nb_stoptimes,
                                         data, rt_level, accessibilite_params);
    size_t total_result = passages_dt_st.size();
    passages_dt_st = paginate(passages_dt_st, count, start_page);

    for (auto dt_stop_time : passages_dt_st) {
        pbnavitia::Passage* passage;
        if (stop_event == StopEvent::pick_up) {
            passage = handler.pb_response.add_next_departures();
        } else {
            passage = handler.pb_response.add_next_arrivals();
        }
        pt::time_period action_period(navitia::to_posix_time(dt_stop_time.first, data), pt::seconds(1));
        auto departure_date = navitia::to_posix_timestamp(dt_stop_time.first, data);
        auto arrival_date = navitia::to_posix_timestamp(dt_stop_time.first, data);
        passage->mutable_stop_date_time()->set_departure_date_time(departure_date);
        passage->mutable_stop_date_time()->set_arrival_date_time(arrival_date);

        ProtoCreator::fill_pb_object(dt_stop_time.second, data, passage->mutable_stop_date_time()->mutable_properties(),
                       0, current_datetime, action_period);

        ProtoCreator::fill_pb_object(dt_stop_time.second->stop_point, data, passage->mutable_stop_point(),
                depth, current_datetime, action_period);
        const type::VehicleJourney* vj = dt_stop_time.second->vehicle_journey;
        const type::Route* route = vj->route;
        const type::Line* line = route->line;
        const type::PhysicalMode* physical_mode = vj->physical_mode;
        auto m_vj = passage->mutable_vehicle_journey();
        auto m_route = m_vj->mutable_route();
        auto m_physical_mode = m_vj->mutable_journey_pattern()->mutable_physical_mode();
        ProtoCreator::fill_pb_object(vj, data, m_vj, 0, current_datetime, action_period, show_codes);
        ProtoCreator::fill_pb_object(route, data, m_route, 0, current_datetime, action_period, show_codes);
        ProtoCreator::fill_pb_object(line, data, m_route->mutable_line(), 0, current_datetime, action_period, show_codes);
        ProtoCreator::fill_pb_object(physical_mode, data, m_physical_mode, 0, current_datetime, action_period);
        const auto& vj_st = ProtoCreator::VjStopTimes(vj, dt_stop_time.second, nullptr);
        ProtoCreator::fill_pb_object(&vj_st, data, passage->mutable_pt_display_informations(), 1, current_datetime, action_period);
    }
    auto pagination = handler.pb_response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(passages_dt_st.size());
    return handler.pb_response;
}


pbnavitia::Response previous_departures(const std::string &request,
        const std::vector<std::string>& forbidden_uris,
        const pt::ptime datetime, uint32_t duration, uint32_t nb_stoptimes,
        const int depth, const type::AccessibiliteParams & accessibilite_params,
        const type::Data & data, const type::RTLevel rt_level, uint32_t count, uint32_t start_page,
        const bool show_codes, const boost::posix_time::ptime current_datetime) {

    struct vis_previous_departures {
        struct predicate_t {
            const type::Data &data;
            predicate_t(const type::Data& data) : data(data){}
            bool operator()(const routing::JppIdx& jpp_idx) const {
                const auto& jpp = data.dataRaptor->jp_container.get(jpp_idx);
                const auto& jp = data.dataRaptor->jp_container.get(jpp.jp_idx);
                if (jp.jpps.empty()) { return false; }
                return jpp_idx == jp.jpps.back();
            }
        };
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_previous_departures(const type::Data& data) :
            api_pb(pbnavitia::PREVIOUS_DEPARTURES), predicate(data) {}
    };
    vis_previous_departures vis(data);
    return previous_passages(request,
                             forbidden_uris,
                             datetime,
                             duration,
                             nb_stoptimes,
                             depth,
                             accessibilite_params,
                             data,
                             rt_level,
                             vis,
                             count,
                             start_page,
                             show_codes,
                             current_datetime
    );
}


pbnavitia::Response previous_arrivals(const std::string &request,
        const std::vector<std::string>& forbidden_uris,
        const pt::ptime datetime, uint32_t duration, uint32_t nb_stoptimes,
        const int depth, const type::AccessibiliteParams & accessibilite_params,
        const type::Data & data, const type::RTLevel rt_level, uint32_t count, uint32_t start_page,
        const bool show_codes, const boost::posix_time::ptime current_datetime) {

    struct vis_previous_arrivals {
        struct predicate_t {
            const type::Data &data;
            predicate_t(const type::Data& data) : data(data){}
            bool operator()(const routing::JppIdx& jpp_idx) const {
                const auto& jpp = data.dataRaptor->jp_container.get(jpp_idx);
                return jpp.order == 0;
            }
        };
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_previous_arrivals(const type::Data& data) :
            api_pb(pbnavitia::PREVIOUS_ARRIVALS), predicate(data) {}
    };
    vis_previous_arrivals vis(data);
    return previous_passages(request, forbidden_uris, datetime, duration, nb_stoptimes, depth,
            accessibilite_params, data, rt_level, vis,count , start_page, show_codes, current_datetime);
}


} }
