#include "2stops_schedule.h"

namespace navitia { namespace timetables {

std::unordered_map<type::idx_t, uint32_t>  get_arrival_order(const std::vector<type::idx_t> &departure_routepoint, const std::string &arrival_filter, type::Data & data) {
    std::unordered_map<type::idx_t, uint32_t> result;
    std::unordered_map<type::idx_t, type::idx_t> departure_route_routepoint;
    //On les ajoute dans un map ayant pour clé la route
    for(type::idx_t rpidx : departure_routepoint) {
        auto rp = data.pt_data.route_points[rpidx];
        departure_route_routepoint.insert(std::make_pair(rp.route_idx, rp.idx));
    }


    //On va chercher tous les route points correspondant au deuxieme filtre
    std::vector<type::idx_t > route_points = navitia::ptref::make_query(type::Type_e::eRoutePoint, arrival_filter, data);

    //On parcourt tous les route points du deuxieme filtre
    for(type::idx_t rpidx : route_points) {

        auto it_rp = departure_route_routepoint.find(data.pt_data.route_points[rpidx].route_idx);
        //Si un route point du premier filtre a la meme route, et que son ordre est inferieur à l'ordre de celui du deuxieme filtre alors on ajoute au résultat
        if((it_rp != departure_route_routepoint.end()) && (data.pt_data.route_points[it_rp->second].order < data.pt_data.route_points[rpidx].order)) {
            result.insert(std::make_pair(data.pt_data.route_points[it_rp->second].idx, data.pt_data.route_points[rpidx].order));
        }
    }
    //On retourne les pairs obtenues
    return result;
}


std::vector<pair_dt_st> stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                        const routing::DateTime &datetime, const routing::DateTime &max_datetime, const int nb_stoptimes,
                                        type::Data & data) {

    std::vector<pair_dt_st> result;
    //On va chercher tous les route points correspondant au deuxieme filtre
    std::vector<type::idx_t > departure_route_points_tmp = navitia::ptref::make_query(type::Type_e::eRoutePoint, departure_filter, data);

    //On récupère maintenant l'intersection des deux filtre, on obtient un map entre un route point de départ, et order d'arrivée
    auto departure_idx_arrival_order = get_arrival_order(departure_route_points_tmp, arrival_filter, data);

    std::vector<type::idx_t> departure_route_points;
    for(auto dep_order : departure_idx_arrival_order)
        departure_route_points.push_back(dep_order.first);

    //On demande tous les next_departures
    auto departure_dt_st = get_stop_times(departure_route_points, datetime, max_datetime, nb_stoptimes, data);


    //On va chercher les retours
    for(auto dep_dt_st : departure_dt_st) {
        const type::StopTime &departure_st = data.pt_data.stop_times[dep_dt_st.second];
        const type::VehicleJourney vj = data.pt_data.vehicle_journeys[departure_st.vehicle_journey_idx];
        const uint32_t arrival_order = departure_idx_arrival_order[departure_st.route_point_idx];
        const type::StopTime &arrival_st = data.pt_data.stop_times[vj.stop_time_list[arrival_order]];
        routing::DateTime arrival_dt = dep_dt_st.first;
        arrival_dt.update(arrival_st.arrival_time);
        result.push_back(std::make_pair(dep_dt_st, std::make_pair(arrival_dt, arrival_st.idx)));
    }

    return result;
}



pbnavitia::Response stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                    const std::string &str_dt, uint32_t duration, 
                                    uint32_t nb_stoptimes, uint32_t depth, type::Data & data) {
    pbnavitia::Response pb_response;

    boost::posix_time::ptime ptime;
    ptime = boost::posix_time::from_iso_string(str_dt);
    routing::DateTime dt((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());

    routing::DateTime max_dt;
    max_dt = dt + duration;
    std::vector<pair_dt_st> board;
    try {
        board = stops_schedule(departure_filter, arrival_filter, dt, max_dt, nb_stoptimes, data);
    } catch(ptref::ptref_parsing_error parse_error) {
        switch(parse_error.type){
        case ptref::ptref_parsing_error::error_type::partial_error: pb_response.set_error("DepartureBoard / PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + parse_error.more + "<<"); break;
        case ptref::ptref_parsing_error::error_type::global_error: pb_response.set_error("DepartureBoard / PTReferential : Impossible de parser la requête");
        case ptref::ptref_parsing_error::error_type::unknown_object: pb_response.set_error("Objet NAViTiA inconnu : " + parse_error.more);
        }
        return pb_response;
    }

    pb_response.set_requested_api(pbnavitia::DEPARTURE_BOARD);
    for(auto pair_dt_idx : board) {
        pbnavitia::PairStopTime * pair_stoptime = pb_response.mutable_stops_schedule()->add_board_items();
        auto stoptime = pair_stoptime->mutable_first();
        const auto &dt_idx = pair_dt_idx.first;
        stoptime->set_departure_date_time(iso_string(data, dt_idx.first.date(),  dt_idx.first.hour()));
        stoptime->set_arrival_date_time(iso_string(data, dt_idx.first.date(),  dt_idx.first.hour()));
        const auto &rp = data.pt_data.route_points[data.pt_data.stop_times[dt_idx.second].route_point_idx];
        fill_pb_object(rp.stop_point_idx, data, stoptime->mutable_stop_point(), depth);
        fill_pb_object(data.pt_data.routes[rp.route_idx].line_idx, data, stoptime->mutable_line(), depth);

        stoptime = pair_stoptime->mutable_second();
        const auto &dt_idx2 = pair_dt_idx.second;
        stoptime->set_departure_date_time(iso_string(data, dt_idx2.first.date(),  dt_idx2.first.hour()));
        stoptime->set_arrival_date_time(iso_string(data, dt_idx2.first.date(),  dt_idx2.first.hour()));
        const auto &rp2 = data.pt_data.route_points[data.pt_data.stop_times[dt_idx2.second].route_point_idx];
        fill_pb_object(rp2.stop_point_idx, data, stoptime->mutable_stop_point(), depth);
        fill_pb_object(data.pt_data.routes[rp2.route_idx].line_idx, data, stoptime->mutable_line(), depth);
    }
    return pb_response;
}


}}
