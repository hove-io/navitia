#include "next_stop_times.h"
#include "get_stop_times.h"
#include "parse_request.h"


namespace navitia { namespace timetables {

template<typename Visitor>
pbnavitia::Response next_stop_times(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth, const bool wheelchair, type::Data & data, Visitor vis) {
    request_parser parser(vis.api_str, request, str_dt, duration,data);

    parser.pb_response.set_requested_api(vis.api_pb);
    if(parser.pb_response.has_error()) {
        return parser.pb_response;
    }

    std::remove_if(parser.route_points.begin(), parser.route_points.end(), vis.predicate);

    auto departures_dt_idx = get_stop_times(parser.route_points, parser.date_time, parser.max_datetime, nb_stoptimes, data, wheelchair);

    for(auto dt_idx : departures_dt_idx) {
        pbnavitia::StopDateTime * stoptime = parser.pb_response.mutable_nextstoptimes()->add_stop_date_times();
        stoptime->set_departure_date_time(iso_string(data, dt_idx.first.date(),  dt_idx.first.hour()));
        stoptime->set_arrival_date_time(iso_string(data, dt_idx.first.date(),  dt_idx.first.hour()));
        const auto &rp = data.pt_data.route_points[data.pt_data.stop_times[dt_idx.second].route_point_idx];
        stoptime->set_is_adapted(data.pt_data.stop_times[dt_idx.second].is_adapted());
        fill_pb_object(rp.stop_point_idx, data, stoptime->mutable_stop_point(), depth);
        fill_pb_object(data.pt_data.routes[rp.route_idx].line_idx, data, stoptime->mutable_line(), depth);
    }
    return parser.pb_response;
}


pbnavitia::Response next_departures(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth, const bool wheelchair, type::Data & data) {

    struct vis_next_departures {
        struct predicate_t {
            type::Data &data;
            predicate_t(type::Data& data) : data(data){}
            bool operator()(const type::idx_t rpidx) const{
                return data.pt_data.route_points[rpidx].order == (int)(data.pt_data.routes[data.pt_data.route_points[rpidx].route_idx].route_point_list.size()-1);
            }
        };
        std::string api_str;
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_next_departures(type::Data& data) : api_str("NEXT_DEPARTURES"), api_pb(pbnavitia::NEXT_DEPARTURES), predicate(data) {}
    };
    vis_next_departures vis(data);
    return next_stop_times(request, str_dt, duration, nb_stoptimes, depth, wheelchair, data, vis);
}


pbnavitia::Response next_arrivals(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth, const bool wheelchair, type::Data & data) {

    struct vis_next_arrivals {
        struct predicate_t {
            type::Data &data;
            predicate_t(type::Data& data) : data(data){}
            bool operator()(const type::idx_t rpidx) const{
                return data.pt_data.route_points[rpidx].order == 0;
            }
        };
        std::string api_str;
        pbnavitia::API api_pb;
        predicate_t predicate;
        vis_next_arrivals(type::Data& data) : api_str("NEXT_ARRIVALS"), api_pb(pbnavitia::NEXT_ARRIVALS), predicate(data) {}
    };
    vis_next_arrivals vis(data);
    return next_stop_times(request, str_dt, duration, nb_stoptimes, depth, wheelchair, data, vis);
}


} }
