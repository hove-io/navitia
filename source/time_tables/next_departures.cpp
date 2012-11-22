#include "next_departures.h"
#include "type/pb_converter.h"

namespace navitia { namespace timetables {


std::string iso_string(const nt::Data & d, int date, int hour){
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(date));
    date_time += boost::posix_time::seconds(hour);
    return boost::posix_time::to_iso_string(date_time);
}

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

    auto departures_dt_idx = next_departures(route_points, dt, max_dt, nb_departures, data);
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




std::vector<dt_st> next_departures(const std::vector<type::idx_t> &route_points, const routing::DateTime &dt, const routing::DateTime &max_dt, const int nb_departures, type::Data & data) {
   std::vector<dt_st> result;
   std::multiset<dt_st, comp_st> result_temp;
   auto test_add = true;
   auto last_departure = dt;
   std::vector<type::idx_t> rps = route_points; //On veut connaitre poru tous les route points leur premier départ

   while(test_add && last_departure < max_dt &&
         (distance(result_temp.begin(), result_temp.upper_bound(std::make_pair(last_departure, type::invalid_idx))) < nb_departures)) {

       test_add = false;
       //On va chercher le prochain départ >= last_departure + 1 pour tous les route points de la liste
       for(auto rp_idx : rps) {
           const type::RoutePoint & rp = data.pt_data.route_points[rp_idx];
           auto etemp = earliest_trip(data.pt_data.routes[rp.route_idx], rp.order, last_departure + 1, data);
           if(etemp >= 0) {
               auto st = data.pt_data.stop_times[data.pt_data.vehicle_journeys[etemp].stop_time_list[rp.order]];
               auto dt_temp = dt;
               dt_temp.update(st.departure_time);
               result_temp.insert(std::make_pair(dt_temp, st.idx));
               test_add = true;
           }
       }

       //Le prochain départ sera le premier dt >= last_departure
       last_departure = result_temp.upper_bound(std::make_pair(last_departure, type::invalid_idx))->first;
       //On met à jour la liste des routepoints à updater (ceux qui sont >= last_departure
       rps.clear();
       for(auto it_st = result_temp.lower_bound(std::make_pair(last_departure, type::invalid_idx));
                it_st!= result_temp.upper_bound(std::make_pair(last_departure, type::invalid_idx)); ++it_st){
           rps.push_back(data.pt_data.stop_times[it_st->second].route_point_idx);
       }

   }

   int i = 0;
   for(auto it = result_temp.begin(); it != result_temp.end() && i < nb_departures; ++it) {
       if(it->first <= max_dt) {
           result.push_back(*it);
       } else {
           break;
       }
       ++i;
   }

   return result;
}


} }
