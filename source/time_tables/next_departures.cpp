#include "next_departures.h"
#include "get_stop_times.h"


namespace navitia { namespace timetables {


pbnavitia::Response next_departures(const std::string &request, const std::string &str_dt, const std::string &str_max_dt, const int nb_departures, const int depth, type::Data & data) {
    pbnavitia::Response pb_response;
    std::vector<type::idx_t> route_points;

    boost::posix_time::ptime ptime;
    ptime = boost::posix_time::from_iso_string(str_dt);
    routing::DateTime dt((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());

    routing::DateTime max_dt;
    if(str_max_dt != "") {
        ptime = boost::posix_time::from_iso_string(str_max_dt);
        max_dt = routing::DateTime((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());
    } else if(nb_departures == std::numeric_limits<int>::max()) {
        pb_response.set_error("NextDepartures : Un des deux champs nb_departures ou max_datetime doit être renseigné");
        return pb_response;
    }

    try {
        route_points = navitia::ptref::make_query(type::Type_e::eRoutePoint, request, data);
    } catch(ptref::ptref_parsing_error parse_error) {
        switch(parse_error.type){
        case ptref::ptref_parsing_error::error_type::partial_error: pb_response.set_error("NextDepartures / PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + parse_error.more + "<<"); break;
        case ptref::ptref_parsing_error::error_type::global_error: pb_response.set_error("NextDepartures / PTReferential : Impossible de parser la requête");
        case ptref::ptref_parsing_error::error_type::unknown_object: pb_response.set_error("Objet NAViTiA inconnu : " + parse_error.more);
        }
        return pb_response;
    }

    auto departures_dt_idx = get_stop_times(route_points, dt, max_dt, nb_departures, data);
    pb_response.set_requested_api(pbnavitia::NEXT_DEPARTURES);

    for(auto dt_idx : departures_dt_idx) {
        pbnavitia::StopTime * stoptime = pb_response.mutable_nextdepartures()->add_departure();
        stoptime->set_departure_date_time(iso_string(data, dt_idx.first.date(),  dt_idx.first.hour()));
        stoptime->set_arrival_date_time(iso_string(data, dt_idx.first.date(),  dt_idx.first.hour()));
        const auto &rp = data.pt_data.route_points[data.pt_data.stop_times[dt_idx.second].route_point_idx];
        fill_pb_object(rp.stop_point_idx, data, stoptime->mutable_stop_point(), depth);
        fill_pb_object(data.pt_data.routes[rp.route_idx].line_idx, data, stoptime->mutable_line(), depth);
    }
    return pb_response;
}


} }
