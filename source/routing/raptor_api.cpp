#include "raptor_api.h"
#include "type/pb_converter.h"
#include "boost/date_time/posix_time/posix_time.hpp"

//#include "street_network/street_network_api.h"

namespace navitia { namespace routing {

std::string iso_string(const nt::Data & d, int date, int hour){
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(date));
    date_time += boost::posix_time::seconds(hour);
    return boost::posix_time::to_iso_string(date_time);
}


void fill_section(pbnavitia::Section *pb_section, navitia::type::idx_t vj_idx,
        const nt::Data & d, boost::posix_time::ptime now, boost::posix_time::time_period action_period) {

    const type::VehicleJourney & vj = d.pt_data.vehicle_journeys[vj_idx];
    const type::JourneyPattern & jp = d.pt_data.journey_patterns[vj.journey_pattern_idx];
    const type::Route & route = d.pt_data.routes[jp.route_idx];
    const type::Line & line = d.pt_data.lines[route.line_idx];

    auto mvj = pb_section->mutable_vehicle_journey();
    auto mroute = mvj->mutable_route();
    auto mline = mroute->mutable_line();

    fill_pb_object(vj_idx, d, mvj, 0, now, action_period);
    fill_pb_object(route.idx, d, mroute, 0, now, action_period);
    fill_pb_object(line.idx, d, mline, 0, now, action_period);
    fill_pb_object(line.network_idx, d, mline->mutable_network(), 0, now, action_period);
    fill_pb_object(line.commercial_mode_idx, d, mline->mutable_commercial_mode(), 0, now, action_period);
    fill_pb_object(vj.physical_mode_idx, d, mvj->mutable_physical_mode(), 0, now, action_period);
}


pbnavitia::Response make_pathes(const std::vector<navitia::routing::Path> &paths, const nt::Data & d, streetnetwork::StreetNetwork & worker,
                                const type::EntryPoint &origin,
                                const type::EntryPoint &destination) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PLANNER);
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();


    auto temp = worker.get_direct_path();
    pbnavitia::Planner * planner = pb_response.mutable_planner();
    if(paths.size() > 0 || temp.path_items.size() > 0) {
        planner->set_response_type(pbnavitia::ITINERARY_FOUND);
        if(temp.path_items.size() > 0) {
            pbnavitia::Journey * pb_journey = planner->add_journeys();
            pb_journey->set_duration(temp.length);
            fill_street_section(origin, temp, d, pb_journey->add_sections(), 1);
        }
        for(Path path : paths) {
            navitia::type::DateTime departure_time = navitia::type::DateTime::inf, arrival_time = navitia::type::DateTime::inf;
            pbnavitia::Journey * pb_journey = planner->add_journeys();
            pb_journey->set_nb_transfers(path.nb_changes);
            pb_journey->set_requested_date_time(boost::posix_time::to_iso_string(path.request_time));

            // La marche à pied initiale si on avait donné une coordonnée
            if(path.items.size() > 0 && path.items.front().stop_points.size() > 0 && path.items.front().stop_points.size() > 0){
                const auto temp = worker.get_path(path.items.front().stop_points.front());
                if(temp.path_items.size() > 0) {
                    fill_street_section(origin, temp , d, pb_journey->add_sections(), 1);
                    departure_time = path.items.front().departure - temp.length/1.38;
                }
            }
            

            // La partie TC et correspondances
            for(PathItem & item : path.items){
                pbnavitia::Section * pb_section = pb_journey->add_sections();
                if(item.type == public_transport){
                    pb_section->set_type(pbnavitia::PUBLIC_TRANSPORT);
                    
                    boost::posix_time::ptime departure_ptime , arrival_ptime;
                    for(size_t i=0;i<item.stop_points.size();++i){
                        pbnavitia::StopDateTime * stop_time = pb_section->add_stop_date_times();
                        auto arr_time = item.arrivals[i];
                        stop_time->set_arrival_date_time(iso_string(d, arr_time.date(), arr_time.hour()));
                        auto dep_time = item.departures[i];
                        stop_time->set_departure_date_time(iso_string(d, dep_time.date(), dep_time.hour()));
                        boost::posix_time::time_period action_period(to_posix_time(dep_time, d), to_posix_time(arr_time, d));
                        fill_pb_object(item.stop_points[i], d, stop_time->mutable_stop_point(), 0, now, action_period);
                        if(!pb_section->has_origin())
                            fill_pb_placemark(d.pt_data.stop_points[item.stop_points[i]], d, pb_section->mutable_origin(), 1, now, action_period);
                        fill_pb_placemark(d.pt_data.stop_points[item.stop_points[i]], d, pb_section->mutable_destination(), 1, now, action_period);


                        // L'heure de départ du véhicule au premier stop point
                        if(departure_ptime.is_not_a_date_time())
                            departure_ptime = to_posix_time(dep_time, d);
                        // L'heure d'arrivée au dernier stop point
                        arrival_ptime = to_posix_time(arr_time, d);
                    }
                    if( item.vj_idx != type::invalid_idx){ // TODO : réfléchir si ça peut vraiment arriver
                        boost::posix_time::time_period action_period(departure_ptime, arrival_ptime); 
                        fill_section(pb_section, item.vj_idx, d, now, action_period);
                    }
                }

                else {
                    pb_section->set_type(pbnavitia::TRANSFER);
                    pb_section->set_duration(item.departure - item.arrival);
                    switch(item.type) {
                        case extension : pb_section->set_transfer_type(pbnavitia::EXTENSION); break;
                        case guarantee : pb_section->set_transfer_type(pbnavitia::GUARANTEED); break;
                        case waiting : pb_section->set_type(pbnavitia::WAITING); break;
                        default :pb_section->set_transfer_type(pbnavitia::WALKING); break;
                    }

                    boost::posix_time::time_period action_period(to_posix_time(item.departure, d), to_posix_time(item.arrival, d));
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points.front()], d, pb_section->mutable_origin(), 1, now, action_period);
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points.back()], d, pb_section->mutable_destination(), 1, now, action_period);
                }
                auto dep_time = item.departure;
                pb_section->set_begin_date_time(iso_string(d, dep_time.date(), dep_time.hour()));
                auto arr_time = item.arrival;
                pb_section->set_end_date_time(iso_string(d, arr_time.date(), arr_time.hour()));

                pb_section->set_duration(item.arrival - item.departure);
                if(departure_time == navitia::type::DateTime::inf)
                    departure_time = item.departure;
                arrival_time = item.arrival;
            }
            pb_journey->set_duration(arrival_time - departure_time);



            // La marche à pied finale si on avait donné une coordonnée
            if(path.items.size() > 0 && path.items.back().stop_points.size() > 0 && path.items.back().stop_points.size()>0){
                auto temp = worker.get_path(path.items.back().stop_points.back(), true);
                if(temp.path_items.size() > 0) {
                    fill_street_section(destination, temp, d, pb_journey->add_sections(), 1);
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
get_stop_points( const type::EntryPoint &ep, const type::Data & data,
                streetnetwork::StreetNetwork & worker, bool use_second = false/*,
                const int walking_distance = 1000*/){
    std::vector<std::pair<type::idx_t, double> > result;

    std::map<std::string, type::idx_t>::const_iterator it;
    switch(ep.type) {
    case navitia::type::Type_e::StopArea:
        it = data.pt_data.stop_areas_map.find(ep.uri);
        if(it!= data.pt_data.stop_areas_map.end()) {
            for(auto spidx : data.pt_data.stop_areas[it->second].stop_point_list) {
                result.push_back(std::make_pair(spidx, 0));
            }
        } break;
    case type::Type_e::StopPoint:
        it = data.pt_data.stop_points_map.find(ep.uri);
        if(it != data.pt_data.stop_points_map.end()){
            result.push_back(std::make_pair(data.pt_data.stop_points[it->second].idx, 0));
        } break;
    case type::Type_e::Address:
    case type::Type_e::Coord:
        result = worker.find_nearest_stop_points(ep.coordinates, data.pt_data.stop_point_proximity_list, ep.streetnetwork_params.distance, use_second, ep.streetnetwork_params.offset); break;
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
              std::vector<std::string> forbidden,
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

    navitia::type::DateTime borne;
    if(!clockwise)
        borne = navitia::type::DateTime::min;
    else {
//        std::vector<navitia::type::DateTime> dts;
//        for(boost::posix_time::ptime datetime : datetimes){
//            int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
//            int time = datetime.time_of_day().total_seconds();
//            dts.push_back(navitia::type::DateTime(day, time));
//        }

//        return make_pathes(raptor.compute_all(departures, destinations, dts, borne), raptor.data, worker);
        borne = navitia::type::DateTime::inf;
    }

    for(boost::posix_time::ptime datetime : datetimes){
        int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
        int time = datetime.time_of_day().total_seconds();

        std::vector<Path> tmp = raptor.compute_all(departures, destinations, navitia::type::DateTime(day, time), borne, walking_speed, walking_distance, wheelchair, forbidden, clockwise);

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

    return make_pathes(result, raptor.data, worker, origin, destination);
}


pbnavitia::Response make_isochrone(RAPTOR &raptor,
                                   type::EntryPoint origin,
                                   const std::string &datetime_str,bool clockwise,
                                   float walking_speed, int walking_distance,  bool wheelchair,
                                   std::vector<std::string> forbidden,
                                   streetnetwork::StreetNetwork & worker, int max_duration) {
    
    pbnavitia::Response response;
    response.set_requested_api(pbnavitia::ISOCHRONE);

    boost::posix_time::ptime datetime;
    auto tmp_datetime = parse_datetimes(raptor, {datetime_str}, response, clockwise);
    if(response.has_error() || tmp_datetime.size() == 0 ||
       response.isochrone().response_type() == pbnavitia::DATE_OUT_OF_BOUNDS) {
        return response;
    }
    datetime = tmp_datetime.front();
    worker.init();
    auto departures = get_stop_points(origin, raptor.data, worker);

    if(departures.size() == 0){
        response.mutable_isochrone()->set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }
    
    
    std::vector<idx_label> tmp;
    int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
    int time = datetime.time_of_day().total_seconds();
    navitia::type::DateTime init_dt = navitia::type::DateTime(day, time);
    navitia::type::DateTime bound = clockwise ? init_dt + max_duration : init_dt - max_duration;

    raptor.isochrone(departures, init_dt, bound,
                           walking_speed, walking_distance, wheelchair, forbidden, clockwise);


    for(const type::StopPoint &sp : raptor.data.pt_data.stop_points) {
        navitia::type::DateTime best = bound;
        type::idx_t best_rp = type::invalid_idx;
        for(type::idx_t rpidx : sp.journey_pattern_point_list) {
            if(raptor.best_labels[rpidx] < best) {
                best = raptor.best_labels[rpidx];
                best_rp = rpidx;
            }
        }

        if(best_rp != type::invalid_idx) {
            auto label = raptor.best_labels[best_rp];
            type::idx_t initial_rp;
            type::DateTime initial_dt;
            int round = raptor.best_round(best_rp);
            boost::tie(initial_rp, initial_dt) = getFinalRpidAndDate(round, best_rp, clockwise, raptor.labels, raptor.boardings, raptor.data);

            int duration = ::abs(label - init_dt);

            if(duration <= max_duration) {
                auto pb_item = response.mutable_isochrone()->add_items();
                auto pb_stop_date_time = pb_item->mutable_stop_date_time();
                pb_stop_date_time->set_arrival_date_time(iso_string(raptor.data, label.date(), label.hour()));
                pb_stop_date_time->set_departure_date_time(iso_string(raptor.data, label.date(), label.hour()));
                pb_item->set_duration(duration);
                pb_item->set_nb_changes(round);
                fill_pb_object(raptor.data.pt_data.journey_pattern_points[best_rp].stop_point_idx, raptor.data, pb_stop_date_time->mutable_stop_point(), 0);
            }
        }
    }

     std::sort(response.mutable_isochrone()->mutable_items()->begin(), response.mutable_isochrone()->mutable_items()->end(),
               [](const pbnavitia::IsochroneItem & stop_time1, const pbnavitia::IsochroneItem & stop_time2) {
               return stop_time1.duration() < stop_time2.duration();
                });


    return response;
}


}}
