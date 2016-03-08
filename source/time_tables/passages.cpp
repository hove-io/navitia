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

#include "passages.h"
#include "routing/get_stop_times.h"
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

namespace {
struct PassagesVisitor {
    pbnavitia::API api_pb;
    const type::Data &data;
    bool clockwise() const {
        switch (api_pb) {
        case pbnavitia::NEXT_DEPARTURES:
        case pbnavitia::NEXT_ARRIVALS:
            return true;
        case pbnavitia::PREVIOUS_DEPARTURES:
        case pbnavitia::PREVIOUS_ARRIVALS:
            return false;
        default:
            throw std::logic_error("bad api_pb in passages");
        }
    }
    StopEvent stop_event() const {
        switch (api_pb) {
        case pbnavitia::NEXT_DEPARTURES:
        case pbnavitia::PREVIOUS_DEPARTURES:
            return StopEvent::pick_up;
        case pbnavitia::NEXT_ARRIVALS:
        case pbnavitia::PREVIOUS_ARRIVALS:
            return StopEvent::drop_off;
        default:
            throw std::logic_error("bad api_pb in passages");
        }
    }
    bool is_useless(const routing::JppIdx& jpp_idx) const {
        const auto& jpp = data.dataRaptor->jp_container.get(jpp_idx);
        const auto& jp = data.dataRaptor->jp_container.get(jpp.jp_idx);
        switch (stop_event()) {
        case StopEvent::pick_up:
            if (jp.jpps.empty()) { return false; }
            return jpp_idx == jp.jpps.back();
        case StopEvent::drop_off:
            return jpp.order == 0;
        }
    }
};
}


void passages(PbCreator& pb_creator,
              const std::string& request,
              const std::vector<std::string>& forbidden_uris,
              const pt::ptime datetime,
              const uint32_t duration,
              const uint32_t nb_stoptimes,
              const int depth,
              const type::AccessibiliteParams& accessibilite_params,
              const type::RTLevel rt_level,
              const pbnavitia::API api_pb,
              const uint32_t count,
              const uint32_t start_page) {

    const PassagesVisitor vis{api_pb, pb_creator.data};
    RequestHandle handler(pb_creator, request, forbidden_uris,
                          datetime, duration, {}, vis.clockwise());

    if(pb_creator.has_error()) {
        return;
    }

    boost::remove_erase_if(handler.journey_pattern_points,
                           [&](const routing::JppIdx& jpp_idx){return vis.is_useless(jpp_idx);});

    auto passages_dt_st = get_stop_times(vis.stop_event(), handler.journey_pattern_points,
                                         handler.date_time, handler.max_datetime, nb_stoptimes,
                                         pb_creator.data, rt_level, accessibilite_params);
    size_t total_result = passages_dt_st.size();
    passages_dt_st = paginate(passages_dt_st, count, start_page);

    for (auto dt_stop_time : passages_dt_st) {
        pbnavitia::Passage* passage;
        if (vis.stop_event() == StopEvent::pick_up) {
            passage = pb_creator.add_next_departures();
        } else {
            passage = pb_creator.add_next_arrivals();
        }
        pb_creator.action_period = pt::time_period(navitia::to_posix_time(dt_stop_time.first, pb_creator.data),
                                                   pt::seconds(1));
        auto departure_date = navitia::to_posix_timestamp(dt_stop_time.first, pb_creator.data);
        auto arrival_date = navitia::to_posix_timestamp(dt_stop_time.first, pb_creator.data);
        passage->mutable_stop_date_time()->set_departure_date_time(departure_date);
        passage->mutable_stop_date_time()->set_arrival_date_time(arrival_date);

        pb_creator.fill(dt_stop_time.second, passage->mutable_stop_date_time()->mutable_properties(), 0);
        pb_creator.fill(dt_stop_time.second->stop_point, passage->mutable_stop_point(), depth);
        const type::VehicleJourney* vj = dt_stop_time.second->vehicle_journey;
        const type::Route* route = vj->route;
        const type::Line* line = route->line;
        auto m_route = passage->mutable_route();
        pb_creator.fill(route, m_route, 0);
        pb_creator.fill(line, m_route->mutable_line(), 0);

        const auto& vj_st = navitia::VjStopTimes(vj, dt_stop_time.second, nullptr);
        pb_creator.fill(&vj_st, passage->mutable_pt_display_informations(), 1);
    }
    pb_creator.make_paginate(total_result, start_page, count, passages_dt_st.size());
}

}}// namespace
