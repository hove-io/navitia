/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "passages.h"

#include "ptreferential/ptreferential.h"
#include "request_handle.h"
#include "routing/dataraptor.h"
#include "routing/get_stop_times.h"
#include "type/datetime.h"
#include "type/type_utils.h"
#include "utils/paginate.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

namespace pt = boost::posix_time;
using navitia::routing::StopEvent;

namespace navitia {
namespace timetables {

namespace {
struct PassagesVisitor {
    pbnavitia::API api_pb;
    const type::Data& data;
    StopEvent stop_event() const {
        switch (api_pb) {
            case pbnavitia::NEXT_DEPARTURES:
                return StopEvent::pick_up;
            case pbnavitia::NEXT_ARRIVALS:
                return StopEvent::drop_off;
            default:
                throw std::logic_error("bad api_pb in passages");
        }
    }
    // Is this jpp useless for passages (domain specific rules).
    bool is_useless(const routing::JppIdx& jpp_idx) const {
        const auto& jpp = data.dataRaptor->jp_container.get(jpp_idx);
        const auto& jp = data.dataRaptor->jp_container.get(jpp.jp_idx);

        // We don't want to put in a {next,previous} departures
        // (resp. arrivals) the last (resp. first) stop time of the vj
        // even if we can pick up (resp. drop off) because of a vj
        // extention.
        switch (stop_event()) {
            case StopEvent::pick_up:
                if (jp.jpps.empty()) {
                    return false;
                }
                return jpp_idx == jp.jpps.back();
            case StopEvent::drop_off:
                return jpp.order == navitia::routing::RankJourneyPatternPoint(0);
        }
    }
};
struct RoutePoint {
    const type::Route* route;
    const type::StopPoint* stop_point;
    bool operator<(const RoutePoint& other) const {
        if (route->idx != other.route->idx) {
            return route->idx < other.route->idx;
        }
        return stop_point->idx < other.stop_point->idx;
    }
};
std::set<RoutePoint> make_route_points(const std::vector<routing::JppIdx>& jpps, const type::Data& data) {
    std::set<RoutePoint> res;
    for (const auto& jpp_idx : jpps) {
        const auto& jpp = data.dataRaptor->jp_container.get(jpp_idx);
        const auto& jp = data.dataRaptor->jp_container.get(jpp.jp_idx);
        res.insert({data.pt_data->routes[jp.route_idx.val], data.pt_data->stop_points[jpp.sp_idx.val]});
    }
    return res;
}
std::set<RoutePoint> make_route_points(const std::vector<routing::datetime_stop_time>& dtsts) {
    std::set<RoutePoint> res;
    for (const auto& dtst : dtsts) {
        res.insert({dtst.second->vehicle_journey->route, dtst.second->stop_point});
    }
    return res;
}
template <typename T>
void fill_route_point(PbCreator& pb_creator,
                      const int depth,
                      const type::Route* route,
                      const type::StopPoint* stop_point,
                      T* pb) {
    pb_creator.fill(stop_point, pb->mutable_stop_point(), depth);
    const type::Line* line = route->line;
    auto m_route = pb->mutable_route();
    pb_creator.fill(route, m_route, 0);
    auto pm = navitia::ptref_indexes<nt::PhysicalMode, type::Route>(route, *pb_creator.data);
    pb_creator.fill(pm, m_route->mutable_physical_modes(), 0);
    pb_creator.fill(line, m_route->mutable_line(), 0);
}
}  // namespace

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
    const PassagesVisitor vis{api_pb, *pb_creator.data};
    RequestHandle handler(pb_creator, datetime, duration, {}, true);
    handler.init_jpp(request, forbidden_uris);

    if (pb_creator.has_error()) {
        return;
    }

    boost::remove_erase_if(handler.journey_pattern_points,
                           [&](const routing::JppIdx& jpp_idx) { return vis.is_useless(jpp_idx); });

    auto passages_dt_st =
        get_stop_times(vis.stop_event(), handler.journey_pattern_points, handler.date_time, handler.max_datetime,
                       nb_stoptimes, *pb_creator.data, rt_level, accessibilite_params);
    size_t total_result = passages_dt_st.size();
    passages_dt_st = paginate(passages_dt_st, count, start_page);
    auto sort_predicate = [](routing::datetime_stop_time dt1, routing::datetime_stop_time dt2) {
        return dt1.first < dt2.first;
    };
    std::sort(passages_dt_st.begin(), passages_dt_st.end(), sort_predicate);

    for (auto dt_stop_time : passages_dt_st) {
        auto base_ptime = navitia::to_posix_time(dt_stop_time.first, *pb_creator.data);
        pbnavitia::Passage* passage;
        if (vis.stop_event() == StopEvent::pick_up) {
            passage = pb_creator.add_next_departures();
        } else {
            passage = pb_creator.add_next_arrivals();
        }

        // We need one second before because else, on terminus, we may
        // miss something as period exclude the end (disruption period in particular).
        pb_creator.action_period = pt::time_period(base_ptime - pt::seconds(1), pt::seconds(2));

        auto departure_dt = get_date_time(vis.stop_event(), dt_stop_time.second, dt_stop_time.second, base_ptime, true);
        auto arrival_dt = get_date_time(vis.stop_event(), dt_stop_time.second, dt_stop_time.second, base_ptime, false);

        passage->mutable_stop_date_time()->set_departure_date_time(navitia::to_posix_timestamp(departure_dt));
        passage->mutable_stop_date_time()->set_arrival_date_time(navitia::to_posix_timestamp(arrival_dt));

        // find base datetime
        auto base_st = dt_stop_time.second->get_base_stop_time();
        if (base_st != nullptr) {
            auto base_departure_dt = get_date_time(vis.stop_event(), dt_stop_time.second, base_st, base_ptime, true);
            auto base_arrival_dt = get_date_time(vis.stop_event(), dt_stop_time.second, base_st, base_ptime, false);
            passage->mutable_stop_date_time()->set_base_departure_date_time(
                navitia::to_posix_timestamp(base_departure_dt));
            passage->mutable_stop_date_time()->set_base_arrival_date_time(navitia::to_posix_timestamp(base_arrival_dt));
        }
        pb_creator.fill(dt_stop_time.second, passage->mutable_stop_date_time()->mutable_properties(), 0);

        const type::VehicleJourney* vj = dt_stop_time.second->vehicle_journey;
        passage->mutable_stop_date_time()->set_data_freshness(to_pb_realtime_level(vj->realtime_level));
        const auto sts = std::vector<const nt::StopTime*>{dt_stop_time.second};
        const auto& vj_st = navitia::VjStopTimes(vj, sts);
        pb_creator.fill(&vj_st, passage->mutable_pt_display_informations(), 1);
        fill_route_point(pb_creator, depth, vj->route, dt_stop_time.second->stop_point, passage);
    }
    pb_creator.make_paginate(total_result, start_page, count, passages_dt_st.size());

    // filling empty route points
    const auto& all_route_points = make_route_points(handler.journey_pattern_points, *pb_creator.data);
    const auto& filled_route_points = make_route_points(passages_dt_st);
    std::vector<RoutePoint> empty_route_points;
    boost::set_difference(all_route_points, filled_route_points, std::back_inserter(empty_route_points));
    for (const auto& rp : empty_route_points) {
        auto* pb_route_point = pb_creator.add_route_points();
        fill_route_point(pb_creator, depth, rp.route, rp.stop_point, pb_route_point);
        pb_creator.fill(rp.route, pb_route_point->mutable_pt_display_informations(), 1);
    }
}

}  // namespace timetables
}  // namespace navitia
