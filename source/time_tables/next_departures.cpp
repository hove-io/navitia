#include "next_departures.h"
#include "type/pb_converter.h"

namespace navitia { namespace timetables {




pbnavitia::Response next_departures(std::string request, const std::string &str_dt, const uint32_t nb_departures, type::Data & data, routing::raptor::RAPTOR &raptor) {
    pbnavitia::Response pb_response;
    std::vector<type::idx_t> route_points;

    boost::posix_time::ptime ptime;
    ptime = boost::posix_time::from_iso_string(str_dt);
    routing::DateTime dt((ptime.date() - raptor.data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());

    try {
        route_points = navitia::ptref::make_query(type::Type_e::eRoutePoint, request, data);
    } catch(ptref::ptref_parsing_error parse_error) {
        if(parse_error.type == ptref::ptref_parsing_error::error_type::partial_error) {
            pb_response.set_error("PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + parse_error.more + "<<");
        } else {
            pb_response.set_error("PTReferential : Impossible de parser la requête");
        }
        return pb_response;
    } catch(ptref::ptref_unknown_object unknown_obj_error) {
        pb_response.set_error("Objet NAViTiA inconnu : " + unknown_obj_error.more);
        return pb_response;
    }


    auto departures_idx = next_departures(route_points, dt, nb_departures, raptor);
    return ptref::extract_data(data, type::Type_e::eStopTime, departures_idx);
}




std::vector<type::idx_t> next_departures(const std::vector<type::idx_t> &route_points, const routing::DateTime &dt, const uint32_t nb_departures, routing::raptor::RAPTOR &raptor) {
   std::vector<type::idx_t> result;



   std::multiset<dt_st, comp_st> result_temp;
   for(auto rp_idx : route_points) {
       const type::RoutePoint & rp = raptor.data.pt_data.route_points[rp_idx];
       auto etemp = raptor.earliest_trip(raptor.data.pt_data.routes[rp.route_idx], rp.order, dt);
       if(etemp >= 0) {
           auto st = raptor.data.pt_data.stop_times[raptor.data.pt_data.vehicle_journeys[etemp].stop_time_list[rp.order]];
           auto dt_temp = dt;
           dt_temp.update(st.departure_time);
           result_temp.insert(std::make_pair(dt_temp, st.idx));
       }
   }

   auto last_departure = result_temp.upper_bound(std::make_pair(dt, type::invalid_idx))->first;

   auto test_add = true;
   while((distance(result_temp.upper_bound(std::make_pair(dt, type::invalid_idx)), result_temp.begin()) < nb_departures) && test_add) {
       std::vector<type::idx_t> rps;
       for(auto it_st = result_temp.lower_bound(std::make_pair(last_departure, type::invalid_idx));
                it_st!= result_temp.upper_bound(std::make_pair(last_departure, type::invalid_idx)); ++it_st){
           rps.push_back(raptor.data.pt_data.stop_times[it_st->second].route_point_idx);
       }
       test_add = false;
       for(auto rp_idx : rps) {
           const type::RoutePoint & rp = raptor.data.pt_data.route_points[rp_idx];
           auto etemp = raptor.earliest_trip(raptor.data.pt_data.routes[rp.route_idx], rp.order, dt + 1);
           if(etemp >= 0) {
               auto st = raptor.data.pt_data.stop_times[raptor.data.pt_data.vehicle_journeys[etemp].stop_time_list[rp.order]];
               auto dt_temp = dt;
               dt_temp.update(st.departure_time);
               result_temp.insert(std::make_pair(dt_temp, st.idx));
               test_add = true;
           }
       }
       last_departure = result_temp.upper_bound(std::make_pair(last_departure, type::invalid_idx))->first;
   }

   uint32_t i = 0;
   for(auto it = result_temp.begin(); it != result_temp.end() && i < nb_departures; ++it) {
       result.push_back(it->second);
       ++i;
   }

   return result;
}


} }
