#include "line_schedule.h"
#include "thermometer.h"
#include "parse_request.h"
#include "type/pb_converter.h"
#include "ptreferential/ptreferential.h"


namespace navitia { namespace timetables {

std::vector<vector_stopTime> get_all_stop_times(const std::string &filter, const routing::DateTime &dateTime, const routing::DateTime &max_datetime, type::Data &d) {
    std::vector<vector_stopTime> result;
    //On cherche les premiers route_points de toutes les routes

    std::vector<type::idx_t> first_route_points;
    for(type::idx_t route_idx : ptref::make_query(navitia::type::Type_e::eRoute, filter, d)) {
        first_route_points.push_back(d.pt_data.routes[route_idx].route_point_list.front());
    }

    //On fait un next_departures sur ces route points
    auto first_dt_st = get_stop_times(first_route_points, dateTime, max_datetime, std::numeric_limits<int>::max(), d);

    //On va chercher tous les prochains horaires
    for(auto ho : first_dt_st) {
        result.push_back(vector_stopTime());
        routing::DateTime dt = ho.first;
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
            result[orders[order]][y] = iso_string(d, dt_idx.first.date(),  dt_idx.first.hour());
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

    //On récupère les stop_times
    auto stop_times = get_all_stop_times(filter, parser.date_time, parser.max_datetime, d);

    std::vector<vector_idx> vec_stop_points;

    for(auto vec_st : stop_times) {
        vec_stop_points.push_back(vector_idx());
        for(auto pairst : vec_st) {
            vec_stop_points.back().push_back(d.pt_data.route_points[d.pt_data.stop_times[pairst.second].route_point_idx].stop_point_idx);
        }
    }

    Thermometer thermometer(d);
    thermometer.get_thermometer(filter);



    //On remplit l'objet header
    pbnavitia::LineScheduleHeader *header = parser.pb_response.mutable_line_schedule()->mutable_header();
    for(vector_stopTime vec : stop_times) {
        auto dt_idx = vec.front();
        fill_pb_object(d.pt_data.routes[d.pt_data.route_points[d.pt_data.stop_times[dt_idx.second].route_point_idx].route_idx].line_idx, d, header->add_item()->mutable_line(), max_depth-1);
    }

    //On génère la matrice
    std::vector<vector_string> matrice = make_matrice(stop_times, thermometer, d);

    pbnavitia::Table *table = parser.pb_response.mutable_line_schedule()->mutable_table();
    for(unsigned int i=0; i < thermometer.get_thermometer().size(); ++i) {
        type::idx_t spidx=thermometer.get_thermometer()[i];
        const type::StopPoint & sp = d.pt_data.stop_points[spidx];
        pbnavitia::TableLine * line = table->add_line();
        line->set_stop_point(sp.name);

        for(unsigned int j=0; j<stop_times.size(); ++j) {
            line->add_stop_time(matrice[i][j]);
        }
    }

    return parser.pb_response;

}
}}
