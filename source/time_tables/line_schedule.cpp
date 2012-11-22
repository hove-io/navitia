#include "line_schedule.h"
#include "thermometer.h"
#include "type/pb_converter.h"


namespace navitia { namespace timetables {

std::vector<vector_stopTime> get_all_stop_times(const type::idx_t line_idx, const routing::DateTime &dateTime, const routing::DateTime &max_datetime, const uint32_t nb_departures, type::Data &d) {
    std::vector<vector_stopTime> result;
    //On cherche les premiers route_points de toutes les routes

    std::vector<type::idx_t> first_route_points;

    for(type::Route route : d.pt_data.routes) {
        if(route.line_idx == line_idx) {
            first_route_points.push_back(route.route_point_list.front());
        }
    }
    //On fait un next_departures sur ces route points
    auto first_dt_st = get_stop_times(first_route_points, dateTime, max_datetime, nb_departures, d);

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


pbnavitia::Response line_schedule(const std::string & line_externalcode, const std::string &str_dt, const std::string &str_max_dt, const int nb_departures, const uint32_t max_depth, type::Data &d) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::LINE_SCHEDULE);

    boost::posix_time::ptime ptime;
    ptime = boost::posix_time::from_iso_string(str_dt);
    if((ptime.date() - d.meta.production_date.begin()).days() < 0) {
        pb_response.set_error("La date demandée est avant la date de production");
        return pb_response;
    }
    routing::DateTime dt((ptime.date() - d.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());

    routing::DateTime max_dt;
    if(str_max_dt != "") {
        ptime = boost::posix_time::from_iso_string(str_max_dt);
        max_dt = routing::DateTime((ptime.date() - d.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());
    } else if(nb_departures == std::numeric_limits<int>::max()) {
        pb_response.set_error("NextDepartures : Un des deux champs nb_departures ou max_datetime doit être renseigné");
        return pb_response;
    }

    type::idx_t line_idx = d.pt_data.line_map[line_externalcode];


    Thermometer thermometer(d);
    thermometer.get_thermometer(line_idx);

    //On récupère les stop_times
    auto stop_times = get_all_stop_times(line_idx, dt, max_dt, nb_departures, d);

    //On remplit l'objet header
    pbnavitia::LineScheduleHeader *header = pb_response.mutable_line_schedule()->mutable_header();
    for(vector_stopTime vec : stop_times) {
        auto dt_idx = vec.front();
        fill_pb_object(d.pt_data.routes[d.pt_data.route_points[d.pt_data.stop_times[dt_idx.second].route_point_idx].route_idx].line_idx, d, header->add_item()->mutable_line(), max_depth-1);
    }

    //On génère la matrice
    std::vector<vector_string> matrice = make_matrice(stop_times, thermometer, d);

    pbnavitia::Table *table = pb_response.mutable_line_schedule()->mutable_table();
    for(unsigned int i=0; i < thermometer.get_thermometer().size(); ++i) {
        type::idx_t spidx=thermometer.get_thermometer()[i];
        const type::StopPoint & sp = d.pt_data.stop_points[spidx];
        pbnavitia::TableLine * line = table->add_line();
        line->set_stop_point(sp.name);

        for(unsigned int j=0; j<stop_times.size(); ++j) {
            line->add_stop_time(matrice[i][j]);
        }
    }

    return pb_response;

}
}}
