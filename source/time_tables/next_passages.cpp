#include "next_passages.h"
#include "get_stop_times.h"
#include "request_handle.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/datetime.h"
namespace pt = boost::posix_time;

namespace navitia { namespace timetables {

template<typename Visitor>
pbnavitia::Response 
next_passages(const std::string &request, const std::string &str_dt,
              uint32_t duration, uint32_t nb_stoptimes, const int depth,
              /*const bool wheelchair*/
              const type::AccessibiliteParams & accessibilite_params, type::Data & data, Visitor vis,
              uint32_t count, uint32_t start_page) {
    RequestHandle handler(vis.api_str, request, str_dt, duration,data, count, start_page);

    if(handler.pb_response.has_error()) {
        return handler.pb_response;
    }

    std::remove_if(handler.journey_pattern_points.begin(), handler.journey_pattern_points.end(), vis.predicate);

    auto departures_dt_stop_times = get_stop_times(handler.journey_pattern_points, handler.date_time, handler.max_datetime, nb_stoptimes, data, accessibilite_params/*wheelchair*/);

    auto now = pt::second_clock::local_time();
    pt::time_period action_period(navitia::to_posix_time(handler.date_time, data), navitia::to_posix_time(handler.max_datetime, data));
    
    for(auto dt_stop_time : departures_dt_stop_times) {
        pbnavitia::Passage * passage;
        if(vis.api_str == "NEXT_ARRIVALS")
            passage = handler.pb_response.add_next_arrivals();
        else
            passage = handler.pb_response.add_next_departures();
        passage->mutable_stop_date_time()->set_departure_date_time(navitia::iso_string(DateTimeUtils::date(dt_stop_time.first), DateTimeUtils::hour(dt_stop_time.first), data));
        passage->mutable_stop_date_time()->set_arrival_date_time(navitia::iso_string(DateTimeUtils::date(dt_stop_time.first), DateTimeUtils::hour(dt_stop_time.first), data));
        const type::JourneyPatternPoint* jpp = dt_stop_time.second->journey_pattern_point;
        fill_pb_object(jpp->stop_point, data, passage->mutable_stop_point(), depth, now, action_period);
        const type::VehicleJourney* vj = dt_stop_time.second->vehicle_journey;
        const type::JourneyPattern* jp = vj->journey_pattern;
        const type::Route* route = jp->route;
        const type::Line* line = route->line;
        const type::PhysicalMode* physical_mode = vj->physical_mode;
        fill_pb_object(vj, data, passage->mutable_vehicle_journey(), 0, now, action_period);
        fill_pb_object(route, data, passage->mutable_vehicle_journey()->mutable_route(), 0, now, action_period);
        fill_pb_object(line, data, passage->mutable_vehicle_journey()->mutable_route()->mutable_line(), 0, now, action_period);
        fill_pb_object(physical_mode, data, passage->mutable_vehicle_journey()->mutable_physical_mode(), 0, now, action_period);
    }
    auto pagination = handler.pb_response.mutable_pagination();
    pagination->set_totalresult(handler.total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(handler.pb_response.route_schedules_size());
    return handler.pb_response;
}


pbnavitia::Response next_departures(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth,
                                    /*const bool wheelchair*/const type::AccessibiliteParams & accessibilite_params, type::Data & data,
                                    uint32_t count, uint32_t start_page) {

    struct vis_next_departures {
        struct predicate_t {
            type::Data &data;
            predicate_t(type::Data& data) : data(data){}
            bool operator()(const type::idx_t jppidx) const{
                return data.pt_data.journey_pattern_points[jppidx]->order == (int)(data.pt_data.journey_pattern_points[jppidx]->journey_pattern->journey_pattern_point_list.size()-1);
            }
        };
        std::string api_str;
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_next_departures(type::Data& data) : api_str("NEXT_DEPARTURES"), api_pb(pbnavitia::NEXT_DEPARTURES), predicate(data) {}
    };
    vis_next_departures vis(data);
    return next_passages(request, str_dt, duration, nb_stoptimes, depth, accessibilite_params/*wheelchair*/, data, vis, count, start_page);
}


pbnavitia::Response next_arrivals(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth,
                                  /*const bool wheelchair*/const type::AccessibiliteParams & accessibilite_params, type::Data & data,
                                  uint32_t count, uint32_t start_page) {

    struct vis_next_arrivals {
        struct predicate_t {
            type::Data &data;
            predicate_t(type::Data& data) : data(data){}
            bool operator()(const type::idx_t jppidx) const{
                return data.pt_data.journey_pattern_points[jppidx]->order == 0;
            }
        };
        std::string api_str;
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_next_arrivals(type::Data& data) : api_str("NEXT_ARRIVALS"), api_pb(pbnavitia::NEXT_ARRIVALS), predicate(data) {}
    };
    vis_next_arrivals vis(data);
    return next_passages(request, str_dt, duration, nb_stoptimes, depth, accessibilite_params/*wheelchair*/, data, vis,count , start_page);
}


} }
