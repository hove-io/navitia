#include "next_passages.h"
#include "get_stop_times.h"
#include "request_handle.h"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace pt = boost::posix_time;

namespace navitia { namespace timetables {

template<typename Visitor>
pbnavitia::Response 
next_passages(const std::string &request, const std::string &str_dt,
              uint32_t duration, uint32_t nb_stoptimes, const int depth,
              const bool wheelchair, type::Data & data, Visitor vis) {
    RequestHandle handler(vis.api_str, request, str_dt, duration,data);

    if(handler.pb_response.has_error()) {
        return handler.pb_response;
    }

    std::remove_if(handler.journey_pattern_points.begin(), handler.journey_pattern_points.end(), vis.predicate);

    auto departures_dt_idx = get_stop_times(handler.journey_pattern_points, handler.date_time, handler.max_datetime, nb_stoptimes, data, wheelchair);

    auto now = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(handler.date_time, data), to_posix_time(handler.max_datetime, data));
    
    for(auto dt_idx : departures_dt_idx) {
        pbnavitia::Passage * passage;
        if(vis.api_str == "NEXT_ARRIVALS")
            passage = handler.pb_response.add_next_arrivals();
        else
            passage = handler.pb_response.add_next_departures();
        passage->mutable_stop_date_time()->set_departure_date_time(type::iso_string(dt_idx.first.date(),  dt_idx.first.hour(), data));
        passage->mutable_stop_date_time()->set_arrival_date_time(type::iso_string(dt_idx.first.date(),  dt_idx.first.hour(), data));
        const auto &rp = data.pt_data.journey_pattern_points[data.pt_data.stop_times[dt_idx.second].journey_pattern_point_idx];
        fill_pb_object(rp.stop_point_idx, data, passage->mutable_stop_point(), depth, now, action_period);
        const type::VehicleJourney & vj = data.pt_data.vehicle_journeys[data.pt_data.stop_times[dt_idx.second].vehicle_journey_idx];
        const type::JourneyPattern & jp = data.pt_data.journey_patterns[vj.journey_pattern_idx];
        const type::Route & route = data.pt_data.routes[jp.route_idx];
        const type::Line & line = data.pt_data.lines[route.line_idx];
        const type::PhysicalMode & physical_mode = data.pt_data.physical_modes[vj.physical_mode_idx];
        fill_pb_object(vj.idx, data, passage->mutable_vehicle_journey(), 0, now, action_period);
        fill_pb_object(route.idx, data, passage->mutable_vehicle_journey()->mutable_route(), 0, now, action_period);
        fill_pb_object(line.idx, data, passage->mutable_vehicle_journey()->mutable_route()->mutable_line(), 0, now, action_period);
        fill_pb_object(physical_mode.idx, data, passage->mutable_vehicle_journey()->mutable_physical_mode(), 0, now, action_period);
    }
    return handler.pb_response;
}


pbnavitia::Response next_departures(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth, const bool wheelchair, type::Data & data) {

    struct vis_next_departures {
        struct predicate_t {
            type::Data &data;
            predicate_t(type::Data& data) : data(data){}
            bool operator()(const type::idx_t rpidx) const{
                return data.pt_data.journey_pattern_points[rpidx].order == (int)(data.pt_data.journey_patterns[data.pt_data.journey_pattern_points[rpidx].journey_pattern_idx].journey_pattern_point_list.size()-1);
            }
        };
        std::string api_str;
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_next_departures(type::Data& data) : api_str("NEXT_DEPARTURES"), api_pb(pbnavitia::NEXT_DEPARTURES), predicate(data) {}
    };
    vis_next_departures vis(data);
    return next_passages(request, str_dt, duration, nb_stoptimes, depth, wheelchair, data, vis);
}


pbnavitia::Response next_arrivals(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth, const bool wheelchair, type::Data & data) {

    struct vis_next_arrivals {
        struct predicate_t {
            type::Data &data;
            predicate_t(type::Data& data) : data(data){}
            bool operator()(const type::idx_t rpidx) const{
                return data.pt_data.journey_pattern_points[rpidx].order == 0;
            }
        };
        std::string api_str;
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_next_arrivals(type::Data& data) : api_str("NEXT_ARRIVALS"), api_pb(pbnavitia::NEXT_ARRIVALS), predicate(data) {}
    };
    vis_next_arrivals vis(data);
    return next_passages(request, str_dt, duration, nb_stoptimes, depth, wheelchair, data, vis);
}


} }
