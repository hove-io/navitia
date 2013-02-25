#include "line_schedule.h"
#include "thermometer.h"
#include "parse_request.h"
#include "type/pb_converter.h"
#include "ptreferential/ptreferential.h"


namespace navitia { namespace timetables {

std::vector<vector_stopTime> get_all_stop_times(const vector_idx &routes, const type::DateTime &dateTime, const type::DateTime &max_datetime, type::Data &d) {
    std::vector<vector_stopTime> result;

    //On cherche les premiers route_points de toutes les routes
    std::vector<type::idx_t> first_route_points;
    for(type::idx_t route_idx : routes) {
        first_route_points.push_back(d.pt_data.routes[route_idx].route_point_list.front());
    }

    //On fait un next_departures sur ces route points
    auto first_dt_st = get_stop_times(first_route_points, dateTime, max_datetime, std::numeric_limits<int>::max(), d);

    //On va chercher tous les prochains horaires
    for(auto ho : first_dt_st) {
        result.push_back(vector_stopTime());
        type::DateTime dt = ho.first;
        for(type::idx_t stidx : d.pt_data.vehicle_journeys[d.pt_data.stop_times[ho.second].vehicle_journey_idx].stop_time_list) {
            dt.update(d.pt_data.stop_times[stidx].departure_time);
            result.back().push_back(std::make_pair(dt, stidx));
        }
    }
    return result;
}

std::vector<vector_string> make_matrice(const std::vector<vector_stopTime> & stop_times, Thermometer &thermometer, type::Data &d) {
    std::vector<vector_string> result;

    //On initilise le tableau vide
    for(unsigned int i=0; i<thermometer.get_thermometer().size(); ++i) {
        result.push_back(vector_string());
        result.back().resize(stop_times.size());
    }

    //On remplit le tableau
    int y=0;
    for(vector_stopTime vec : stop_times) {
        std::vector<uint32_t> orders = thermometer.match_route(d.pt_data.routes[d.pt_data.vehicle_journeys[d.pt_data.stop_times[vec.front().second].vehicle_journey_idx].route_idx]);
        int order = 0;
        for(dt_st dt_idx : vec) {
            result[orders[order]][y] = iso_string(dt_idx.first.date(),  dt_idx.first.hour(), d);
            ++order;
        }
        ++y;
    }

    return result;
}


pbnavitia::Response line_schedule(const std::string & filter, const std::string &str_dt, uint32_t duration, const uint32_t max_depth, type::Data &d) {
    request_parser parser("LINE_SCHEDULE", "", str_dt, duration, d);
    parser.pb_response.set_requested_api(pbnavitia::LINE_SCHEDULE);

    if(parser.pb_response.has_error()) {
        return parser.pb_response;
    }
    Thermometer thermometer(d);

    for(type::idx_t line_idx : navitia::ptref::make_query(type::Type_e::eLine, filter, d)) {
        auto schedule = parser.pb_response.mutable_line_schedule()->add_schedules();
        auto routes = d.pt_data.lines[line_idx].route_list;
        //On récupère les stop_times
        auto stop_times = get_all_stop_times(routes, parser.date_time, parser.max_datetime, d);
        std::vector<vector_idx> route_point_routes;
        for(auto route_idx : routes) {
            route_point_routes.push_back(vector_idx());
            for(auto rpidx : d.pt_data.routes[route_idx].route_point_list) {
                route_point_routes.back().push_back(d.pt_data.route_points[rpidx].stop_point_idx);
            }
        }
        thermometer.generate_thermometer(route_point_routes);
        //On remplit l'objet header
        pbnavitia::LineScheduleHeader *header = schedule->mutable_header();
        for(vector_stopTime vec : stop_times) {
            fill_pb_object(line_idx, d, header->add_items()->mutable_line(), max_depth-1);
        }
        //On génère la matrice
        std::vector<vector_string> matrice = make_matrice(stop_times, thermometer, d);

        pbnavitia::Table *table = schedule->mutable_table();
        for(unsigned int i=0; i < thermometer.get_thermometer().size(); ++i) {
            type::idx_t spidx=thermometer.get_thermometer()[i];
            const type::StopPoint & sp = d.pt_data.stop_points[spidx];
            pbnavitia::TableLine * line = table->add_lines();
            line->set_stop_point(sp.name);

            for(unsigned int j=0; j<stop_times.size(); ++j) {
                line->add_stop_times(matrice[i][j]);
            }
        }


    }

    return parser.pb_response;

}
}}
