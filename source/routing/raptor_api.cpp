#include "raptor_api.h"
#include "type/pb_converter.h"
#include "boost/date_time/posix_time/posix_time.hpp"

//#include "street_network/street_network_api.h"

namespace navitia { namespace routing { namespace raptor {

std::string iso_string(const nt::Data & d, int date, int hour){
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(date));
    date_time += boost::posix_time::seconds(hour);
    return boost::posix_time::to_iso_string(date_time);
}

pbnavitia::Response make_pathes(const std::vector<navitia::routing::Path> &paths, const nt::Data & d, streetnetwork::StreetNetwork & worker) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PLANNER);


    auto temp = worker.get_direct_path();
    pbnavitia::Planner * planner = pb_response.mutable_planner();
    if(paths.size() > 0 || temp.path_items.size() > 0) {
        planner->set_response_type(pbnavitia::ITINERARY_FOUND);
        if(temp.path_items.size() > 0) {
            pbnavitia::Journey * pb_journey = planner->add_journeys();
            pb_journey->set_duration(temp.length);
            fill_street_section(temp, d, pb_journey->add_sections(), 1);
        }
        for(Path path : paths) {
            DateTime departure_time = DateTime::inf, arrival_time = DateTime::inf;
            pbnavitia::Journey * pb_journey = planner->add_journeys();
            pb_journey->set_nb_transfers(path.nb_changes);
            pb_journey->set_requested_date_time(boost::posix_time::to_iso_string(path.request_time));

            // La marche à pied initiale si on avait donné une coordonnée
            if(path.items.size() > 0 && path.items.front().stop_points.size() > 0 && path.items.front().stop_points.size() > 0){
                const auto temp = worker.get_path(path.items.front().stop_points.front());
                if(temp.path_items.size() > 0) {
                    fill_street_section(temp , d, pb_journey->add_sections(), 1);
                    departure_time = path.items.front().departure - temp.length/1.38;
                }
            }

            // La partie TC et correspondances
            for(PathItem & item : path.items){
                pbnavitia::Section * pb_section = pb_journey->add_sections();
                if(item.type == public_transport){
                    pb_section->set_type(pbnavitia::PUBLIC_TRANSPORT);
                    if( item.vj_idx != type::invalid_idx){ // TODO : réfléchir si ça peut vraiment arriver
                        const type::VehicleJourney & vj = d.pt_data.vehicle_journeys[item.vj_idx];
                        const type::Route & route = d.pt_data.routes[vj.route_idx];
                        const type::Line & line = d.pt_data.lines[route.line_idx];
                        if(line.network_idx != type::invalid_idx)
                            pb_section->set_network(d.pt_data.networks[line.network_idx].name );
                        else
                            pb_section->set_network("");
                        if(vj.physical_mode_idx != type::invalid_idx)
                            pb_section->set_mode(d.pt_data.physical_modes[vj.physical_mode_idx].name);
                        pb_section->set_code(line.code);
                        pb_section->set_headsign(vj.name);
                        pb_section->set_direction(route.name);
                        fill_pb_object(line.idx, d, pb_section->mutable_line());
                    }
                    for(size_t i=0;i<item.stop_points.size();++i){
                        pbnavitia::StopDateTime * stop_time = pb_section->add_stop_date_times();
                        auto arr_time = item.arrivals[i];
                        stop_time->set_arrival_date_time(iso_string(d, arr_time.date(), arr_time.hour()));
                        auto dep_time = item.departures[i];
                        stop_time->set_departure_date_time(iso_string(d, dep_time.date(), dep_time.hour()));
                        fill_pb_object(item.stop_points[i], d, stop_time->mutable_stop_point(), 0);

                    }

                    if(item.stop_points.size() >= 2) {
                        fill_pb_placemark(d.pt_data.stop_points[item.stop_points.front()], d, pb_section->mutable_origin(), 1);
                        fill_pb_placemark(d.pt_data.stop_points[item.stop_points.back()], d, pb_section->mutable_destination(), 1);
                    }
                }

                else {
                    pb_section->set_type(pbnavitia::TRANSFER);
                    pb_section->set_duration(item.departure - item.arrival);
                    switch(item.type) {
                    case extension : pb_section->set_transfer_type(pbnavitia::EXTENSION); break;
                    case guarantee : pb_section->set_transfer_type(pbnavitia::GUARANTEED); break;
                    default :pb_section->set_transfer_type(pbnavitia::WALKING); break;
                    }
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points.front()], d, pb_section->mutable_origin(), 1);
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points.back()], d, pb_section->mutable_destination(), 1);
                }
                pb_section->set_duration(item.arrival - item.departure);
                if(departure_time == DateTime::inf)
                    departure_time = item.departure;
                arrival_time = item.arrival;
                pb_journey->set_duration(arrival_time - departure_time);
            }



            // La marche à pied finale si on avait donné une coordonnée
            if(path.items.size() > 0 && path.items.back().stop_points.size() > 0 && path.items.back().stop_points.size()>0){
                auto temp = worker.get_path(path.items.back().stop_points.back(), true);
                if(temp.path_items.size() > 0) {
                    fill_street_section(temp, d, pb_journey->add_sections(), 1);
                    arrival_time =  arrival_time + temp.length/1.38;
                }
            }
            pb_journey->set_departure_date_time(iso_string(d, departure_time.date(), departure_time.hour()));
            pb_journey->set_arrival_date_time(iso_string(d, arrival_time.date(), arrival_time.hour()));
        }
    } else {
        planner->set_response_type(pbnavitia::NO_SOLUTION);
    }

    return pb_response;
}

std::vector<std::pair<type::idx_t, double> > 
get_stop_points(const type::EntryPoint &ep, const type::Data & data,
                streetnetwork::StreetNetwork & worker, bool use_second = false,
                const int walking_distance = 1000){
    std::vector<std::pair<type::idx_t, double> > result;

    switch(ep.type) {
    case navitia::type::Type_e::eStopArea:
    {
        auto it = data.pt_data.stop_area_map.find(ep.uri);
        if(it!= data.pt_data.stop_area_map.end()) {
            for(auto spidx : data.pt_data.stop_areas[it->second].stop_point_list) {
                result.push_back(std::make_pair(spidx, 0));
            }
        }
    } break;
    case type::Type_e::eStopPoint: {
        auto it = data.pt_data.stop_point_map.find(ep.uri);
        if(it != data.pt_data.stop_point_map.end()){
            result.push_back(std::make_pair(data.pt_data.stop_points[it->second].idx, 0));
        }
    } break;
        // AA gestion des adresses
    case type::Type_e::eAddress:
    case type::Type_e::eCoord: {
        result = worker.find_nearest_stop_points(ep.coordinates, data.pt_data.stop_point_proximity_list, walking_distance, use_second);
    } break;
    default: break;
    }
    return result;
}


std::vector<boost::posix_time::ptime> 
parse_datetimes(RAPTOR &raptor,const std::vector<std::string> &datetimes_str, 
                pbnavitia::Response &response, bool clockwise) {
    std::vector<boost::posix_time::ptime> datetimes;

    for(std::string datetime: datetimes_str){
        try {
            boost::posix_time::ptime ptime;
            ptime = boost::posix_time::from_iso_string(datetime);
            if(!raptor.data.meta.production_date.contains(ptime.date())) {
                if(response.requested_api() == pbnavitia::PLANNER)
                    response.mutable_planner()->set_response_type(pbnavitia::DATE_OUT_OF_BOUNDS);
                else if(response.requested_api() == pbnavitia::ISOCHRONE)
                    response.mutable_isochrone()->set_response_type(pbnavitia::DATE_OUT_OF_BOUNDS);
            }
            datetimes.push_back(ptime);
        } catch(...){
            response.set_error("Impossible to parse date " + datetime);
            response.set_info("Example of invalid date: " + datetime);
        }
    }
    if(clockwise)
        std::sort(datetimes.begin(), datetimes.end(), 
                  [](boost::posix_time::ptime dt1, boost::posix_time::ptime dt2){return dt1 > dt2;});
    else
        std::sort(datetimes.begin(), datetimes.end());

    return datetimes;
}



pbnavitia::Response 
make_response(RAPTOR &raptor, const type::EntryPoint &origin,
              const type::EntryPoint &destination, 
              const std::vector<std::string> &datetimes_str, bool clockwise,
              const float walking_speed, const int walking_distance, const bool wheelchair,
              std::multimap<std::string, std::string> forbidden,
              streetnetwork::StreetNetwork & worker) {

    pbnavitia::Response response;
    response.set_requested_api(pbnavitia::PLANNER);

    std::vector<boost::posix_time::ptime> datetimes;
    datetimes = parse_datetimes(raptor, datetimes_str, response, clockwise);
    if(response.error() != "" || response.planner().response_type() == pbnavitia::DATE_OUT_OF_BOUNDS) {
        return response;
    }
    worker.init();
    auto departures = get_stop_points(origin, raptor.data, worker);
    auto destinations = get_stop_points(destination, raptor.data, worker, true);
    if(departures.size() == 0 && destinations.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT);
        return response;
    }

    if(departures.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }

    if(destinations.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_DESTINATION_POINT);
        return response;
    }


    std::vector<Path> result;

    DateTime borne;
    if(!clockwise)
        borne = DateTime::min;
    else {
//        std::vector<DateTime> dts;
//        for(boost::posix_time::ptime datetime : datetimes){
//            int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
//            int time = datetime.time_of_day().total_seconds();
//            dts.push_back(DateTime(day, time));
//        }

//        return make_pathes(raptor.compute_all(departures, destinations, dts, borne), raptor.data, worker);
        borne = DateTime::inf;
    }

    for(boost::posix_time::ptime datetime : datetimes){
        std::vector<Path> tmp;
        int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
        int time = datetime.time_of_day().total_seconds();

        if(clockwise)
            tmp = raptor.compute_all(departures, destinations, DateTime(day, time), borne, walking_speed, walking_distance, wheelchair, forbidden);
        else
            tmp = raptor.compute_reverse_all(departures, destinations, DateTime(day, time), borne, walking_speed, walking_distance, wheelchair, forbidden);

        // Lorsqu'on demande qu'un seul horaire, on garde tous les résultas
        if(datetimes.size() == 1){
            result = tmp;
            for(auto & path : result){
                path.request_time = datetime;
            }
        } else if(tmp.size() > 0) {
            // Lorsqu'on demande plusieurs horaires, on garde que l'arrivée au plus tôt / départ au plus tard
            tmp.back().request_time = datetime;
            result.push_back(tmp.back());
            borne = tmp.back().items.back().arrival;
        } else // Lorsqu'on demande plusieurs horaires, et qu'il n'y a pas de résultat, on retourne un itinéraire vide
            result.push_back(Path());
    }
    if(clockwise)
        std::reverse(result.begin(), result.end());

    return make_pathes(result, raptor.data, worker);
}

pbnavitia::Response make_isochrone(RAPTOR &raptor,
                                   type::EntryPoint origin,
                                   const std::string &datetime_str,bool clockwise,
                                   float walking_speed, int walking_distance,  bool wheelchair,
                                   std::multimap<std::string, std::string> forbidden,
                                   streetnetwork::StreetNetwork & worker, int max_duration) {
    
    pbnavitia::Response response;
    response.set_requested_api(pbnavitia::ISOCHRONE);

    boost::posix_time::ptime datetime;
    auto tmp_datetime = parse_datetimes(raptor, {datetime_str}, response, clockwise);
    if(response.has_error() || tmp_datetime.size() == 0 ||
       response.planner().response_type() == pbnavitia::DATE_OUT_OF_BOUNDS) {
        return response;
    }
    datetime = tmp_datetime.front();
    worker.init();
    auto departures = get_stop_points(origin, raptor.data, worker);

    if(departures.size() == 0){
        response.mutable_isochrone()->set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }
    
    DateTime bound = clockwise ? DateTime::inf : DateTime::min;
    
    std::vector<idx_label> tmp;
    int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
    int time = datetime.time_of_day().total_seconds();
    DateTime init_dt = DateTime(day, time);

    raptor.isochrone(departures, init_dt, bound,
                           walking_speed, walking_distance, wheelchair, forbidden, clockwise);


    for(const type::StopPoint &sp : raptor.data.pt_data.stop_points) {
        DateTime best = bound;
        type::idx_t best_rp = type::invalid_idx;
        for(type::idx_t rpidx : sp.route_point_list) {
            if(raptor.best_labels[rpidx].arrival < best) {
                best = raptor.best_labels[rpidx].arrival;
                best_rp = rpidx;
            }
        }

        if(best_rp != type::invalid_idx) {
            auto label = raptor.best_labels[best_rp];
            type::idx_t initial_rp;
            DateTime initial_dt;
            int round = raptor.best_round(best_rp);
            boost::tie(initial_rp, initial_dt) = init::getFinalRpidAndDate(round, best_rp, raptor.labels, clockwise, raptor.data);

            int duration = ::abs(label.arrival - init_dt);

            if(origin.type == type::Type_e::eCoord) {
                auto temp = worker.get_path(raptor.data.pt_data.route_points[initial_rp].stop_point_idx);
                if(temp.path_items.size() > 0) {
                    duration += temp.length/walking_speed;
                }
            }

            if(duration <= max_duration) {
                auto pb_stop_time = response.mutable_isochrone()->add_stop_date_times();
                pb_stop_time->set_arrival_date_time(iso_string(raptor.data, label.arrival.date(), label.arrival.hour()));
                pb_stop_time->set_departure_date_time(iso_string(raptor.data, label.departure.date(), label.departure.hour()));
                pb_stop_time->set_duration(duration);
                pb_stop_time->set_nb_changes(round);
                fill_pb_object(raptor.data.pt_data.route_points[best_rp].stop_point_idx, raptor.data, pb_stop_time->mutable_stop_point(), 0);
            }
        }
    }

     std::sort(response.mutable_isochrone()->mutable_stop_date_times()->begin(), response.mutable_isochrone()->mutable_stop_date_times()->end(),
               [](const pbnavitia::StopDateTime & stop_time1, const pbnavitia::StopDateTime & stop_time2) {
               return stop_time1.duration() < stop_time2.duration();
                });


    return response;
}


}}}
