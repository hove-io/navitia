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

    const type::VehicleJourney* vj = d.pt_data.vehicle_journeys[vj_idx];
    const type::JourneyPattern* jp = vj->journey_pattern;
    const type::Route* route = jp->route;
    const type::Line* line = route->line;

    auto mvj = pb_section->mutable_vehicle_journey();
    auto mroute = mvj->mutable_route();
    auto mline = mroute->mutable_line();

    fill_pb_object(vj, d, mvj, 0, now, action_period);
    fill_pb_object(route, d, mroute, 0, now, action_period);
    fill_pb_object(line, d, mline, 0, now, action_period);
    fill_pb_object(line->network, d, mline->mutable_network(), 0, now, action_period);
    fill_pb_object(line->commercial_mode, d, mline->mutable_commercial_mode(), 0, now, action_period);
    fill_pb_object(vj->physical_mode, d, mvj->mutable_physical_mode(), 0, now, action_period);
}


pbnavitia::Response make_pathes(const std::vector<navitia::routing::Path> &paths, const nt::Data & d, streetnetwork::StreetNetwork & worker,
                                const type::EntryPoint &origin,
                                const type::EntryPoint &destination) {
    pbnavitia::Response pb_response;
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();


    auto temp = worker.get_direct_path();
    if(!paths.empty() || !temp.path_items.empty()) {
        pb_response.set_response_type(pbnavitia::ITINERARY_FOUND);
        if(!temp.path_items.empty()) {
            pbnavitia::Journey * pb_journey = pb_response.add_journeys();
            pb_journey->set_duration(temp.length);
            fill_street_section(origin, temp, d, pb_journey->add_sections(), 1);
        }
        for(Path path : paths) {
            navitia::DateTime departure_time = DateTimeUtils::inf, arrival_time = DateTimeUtils::inf;
            pbnavitia::Journey * pb_journey = pb_response.add_journeys();
            pb_journey->set_nb_transfers(path.nb_changes);
            pb_journey->set_requested_date_time(boost::posix_time::to_iso_string(path.request_time));

            // La marche à pied initiale si on avait donné une coordonnée
            if(path.items.size() > 0 && path.items.front().stop_points.size() > 0){
                const auto temp = worker.get_path(path.items.front().stop_points.front());
                if(temp.path_items.size() > 0) {
                    pbnavitia::Section * pb_section = pb_journey->add_sections();
                    fill_street_section(origin, temp , d, pb_section, 1);
                    departure_time = path.items.front().departure - temp.length/origin.streetnetwork_params.speed;
                    auto arr_time = path.items.front().departure;
                    pb_section->set_end_date_time(iso_string(d, DateTimeUtils::date(arr_time), DateTimeUtils::hour(arr_time)));
                    pb_section->set_begin_date_time(iso_string(d, DateTimeUtils::date(departure_time), DateTimeUtils::hour(departure_time)));
                }
            }
            
            const type::VehicleJourney* vj = nullptr;
            // La partie TC et correspondances
            for(PathItem & item : path.items){

                pbnavitia::Section * pb_section = pb_journey->add_sections();
                if(item.type == public_transport){
                    pb_section->set_type(pbnavitia::PUBLIC_TRANSPORT);
                    
                    boost::posix_time::ptime departure_ptime , arrival_ptime;
                    for(size_t i=0;i<item.stop_points.size();++i){
                        pbnavitia::StopDateTime * stop_time = pb_section->add_stop_date_times();
                        auto arr_time = item.arrivals[i];
                        stop_time->set_arrival_date_time(iso_string(d, DateTimeUtils::date(arr_time), DateTimeUtils::hour(arr_time)));
                        auto dep_time = item.departures[i];
                        stop_time->set_departure_date_time(iso_string(d, DateTimeUtils::date(dep_time), DateTimeUtils::hour(dep_time)));
                        boost::posix_time::time_period action_period(navitia::to_posix_time(dep_time, d), navitia::to_posix_time(arr_time, d));
                        fill_pb_object(d.pt_data.stop_points[item.stop_points[i]], d, stop_time->mutable_stop_point(), 0, now, action_period);
                        if(!pb_section->has_origin())
                            fill_pb_placemark(d.pt_data.stop_points[item.stop_points[i]], d, pb_section->mutable_origin(), 1, now, action_period);
                        fill_pb_placemark(d.pt_data.stop_points[item.stop_points[i]], d, pb_section->mutable_destination(), 1, now, action_period);
                        if (item.vj_idx != type::invalid_idx)
                        {
                            vj = d.pt_data.vehicle_journeys[item.vj_idx];
                            fill_pb_object(vj->stop_time_list[item.orders[i]], d, stop_time, 1, now, action_period);
                        }

                        // L'heure de départ du véhicule au premier stop point
                        if(departure_ptime.is_not_a_date_time())
                            departure_ptime = navitia::to_posix_time(dep_time, d);
                        // L'heure d'arrivée au dernier stop point
                        arrival_ptime = navitia::to_posix_time(arr_time, d);
                    }
                    if( item.vj_idx != type::invalid_idx){ // TODO : réfléchir si ça peut vraiment arriver
                        boost::posix_time::time_period action_period(departure_ptime, arrival_ptime); 
                        fill_section(pb_section, item.vj_idx, d, now, action_period);
                    }
                }

                else {
                    pb_section->set_type(pbnavitia::TRANSFER);
                    switch(item.type) {
                        case extension : pb_section->set_transfer_type(pbnavitia::EXTENSION); break;
                        case guarantee : pb_section->set_transfer_type(pbnavitia::GUARANTEED); break;
                        case waiting : pb_section->set_type(pbnavitia::WAITING); break;
                        default :pb_section->set_transfer_type(pbnavitia::WALKING); break;
                    }

                    boost::posix_time::time_period action_period(navitia::to_posix_time(item.departure, d), navitia::to_posix_time(item.arrival, d));
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points.front()], d, pb_section->mutable_origin(), 1, now, action_period);
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points.back()], d, pb_section->mutable_destination(), 1, now, action_period);
                }
                auto dep_time = item.departure;
                pb_section->set_begin_date_time(iso_string(d, DateTimeUtils::date(dep_time), DateTimeUtils::hour(dep_time)));
                auto arr_time = item.arrival;
                pb_section->set_end_date_time(iso_string(d, DateTimeUtils::date(arr_time), DateTimeUtils::hour(arr_time)));

                pb_section->set_duration(item.arrival - item.departure);
                if(departure_time == DateTimeUtils::inf)
                    departure_time = item.departure;
                arrival_time = item.arrival;
            }
            pb_journey->set_duration(arrival_time - departure_time);

            // La marche à pied finale si on avait donné une coordonnée
            if(path.items.size() > 0 && path.items.back().stop_points.size() > 0){
                auto temp = worker.get_path(path.items.back().stop_points.back(), true);
                if(temp.path_items.size() > 0) {
                    pbnavitia::Section * pb_section = pb_journey->add_sections();
                    fill_street_section(destination, temp, d, pb_section, 1);
                    pb_section->set_begin_date_time(iso_string(d, DateTimeUtils::date(arrival_time), DateTimeUtils::hour(arrival_time)));
                    arrival_time =  arrival_time + temp.length/destination.streetnetwork_params.speed;
                    pb_section->set_end_date_time(iso_string(d, DateTimeUtils::date(arrival_time), DateTimeUtils::hour(arrival_time)));
                }
            }
            pb_journey->set_departure_date_time(iso_string(d, DateTimeUtils::date(departure_time), DateTimeUtils::hour(departure_time)));
            pb_journey->set_arrival_date_time(iso_string(d, DateTimeUtils::date(arrival_time), DateTimeUtils::hour(arrival_time)));
        }
    } else {
        pb_response.set_response_type(pbnavitia::NO_SOLUTION);
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
            for(auto stop_point : data.pt_data.stop_areas[it->second]->stop_point_list) {
                result.push_back(std::make_pair(stop_point->idx, 0));
            }
        } break;
    case type::Type_e::StopPoint:
        it = data.pt_data.stop_points_map.find(ep.uri);
        if(it != data.pt_data.stop_points_map.end()){
            result.push_back(std::make_pair(data.pt_data.stop_points[it->second]->idx, 0));
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
                response.set_response_type(pbnavitia::DATE_OUT_OF_BOUNDS);
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
              const float walking_speed, const int walking_distance, /*const bool wheelchair*/
              const type::AccessibiliteParams & accessibilite_params,
              std::vector<std::string> forbidden,
              streetnetwork::StreetNetwork & worker, uint32_t max_duration, uint32_t max_transfers) {

    pbnavitia::Response response;

    std::vector<boost::posix_time::ptime> datetimes;
    datetimes = parse_datetimes(raptor, datetimes_str, response, clockwise);
    if(response.error() != "" || response.response_type() == pbnavitia::DATE_OUT_OF_BOUNDS) {
        return response;
    }
    worker.init();
    auto departures = get_stop_points(origin, raptor.data, worker);
    auto destinations = get_stop_points(destination, raptor.data, worker, true);
    if(departures.size() == 0 && destinations.size() == 0){
        response.set_response_type(pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT);
        return response;
    }

    if(departures.size() == 0){
        response.set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }

    if(destinations.size() == 0){
        response.set_response_type(pbnavitia::NO_DESTINATION_POINT);
        return response;
    }

    std::vector<Path> result;

    DateTime bound = clockwise ? DateTimeUtils::inf : DateTimeUtils::min;

    for(boost::posix_time::ptime datetime : datetimes){
        int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
        int time = datetime.time_of_day().total_seconds();
        DateTime init_dt = DateTimeUtils::set(day, time);

        if(max_duration!=std::numeric_limits<uint32_t>::max())  {
            bound = clockwise ? init_dt + max_duration : init_dt - max_duration;
        }

        std::vector<Path> tmp = raptor.compute_all(departures, destinations, init_dt, bound, max_transfers, walking_speed, walking_distance, accessibilite_params/*wheelchair*/, forbidden, clockwise);

        // Lorsqu'on demande qu'un seul horaire, on garde tous les résultas
        if(datetimes.size() == 1){
            result = tmp;
            for(auto & path : result){
                path.request_time = datetime;
            }
        } else if(!tmp.empty()) {
            // Lorsqu'on demande plusieurs horaires, on garde que l'arrivée au plus tôt / départ au plus tard
            tmp.back().request_time = datetime;
            result.push_back(tmp.back());
            bound = tmp.back().items.back().arrival;
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
                                   float walking_speed, int walking_distance,  /*bool wheelchair*/
                                   const type::AccessibiliteParams & accessibilite_params,
                                   std::vector<std::string> forbidden,
                                   streetnetwork::StreetNetwork & worker, int max_duration, uint32_t max_transfers) {
    
    pbnavitia::Response response;

    boost::posix_time::ptime datetime;
    auto tmp_datetime = parse_datetimes(raptor, {datetime_str}, response, clockwise);
    if(response.has_error() || tmp_datetime.size() == 0 ||
       response.response_type() == pbnavitia::DATE_OUT_OF_BOUNDS) {
        return response;
    }
    datetime = tmp_datetime.front();
    worker.init();
    auto departures = get_stop_points(origin, raptor.data, worker);

    if(departures.size() == 0){
        response.set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }
    
    
    std::vector<idx_label> tmp;
    int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
    int time = datetime.time_of_day().total_seconds();
    DateTime init_dt = DateTimeUtils::set(day, time);
    DateTime bound = clockwise ? init_dt + max_duration : init_dt - max_duration;

    raptor.isochrone(departures, init_dt, bound, max_transfers,
                           walking_speed, walking_distance, accessibilite_params/*wheelchair*/, forbidden, clockwise);


    for(const type::StopPoint* sp : raptor.data.pt_data.stop_points) {
        DateTime best = bound;
        type::idx_t best_rp = type::invalid_idx;
        for(auto jpp : sp->journey_pattern_point_list) {
            if(raptor.best_labels[jpp->idx] < best) {
                best = raptor.best_labels[jpp->idx];
                best_rp = jpp->idx;
            }
        }

        if(best_rp != type::invalid_idx) {
            auto label = raptor.best_labels[best_rp];
            type::idx_t initial_rp;
            DateTime initial_dt;
            int round = raptor.best_round(best_rp);
            boost::tie(initial_rp, initial_dt) = getFinalRpidAndDate(round, best_rp, clockwise, raptor.labels, raptor.boardings, raptor.boarding_types, raptor.data);

            int duration = ::abs(label - init_dt);

            if(duration <= max_duration) {
                auto pb_journey = response.add_journeys();
                pb_journey->set_arrival_date_time(iso_string(raptor.data, DateTimeUtils::date(label), DateTimeUtils::hour(label)));
                pb_journey->set_departure_date_time(iso_string(raptor.data, DateTimeUtils::date(label), DateTimeUtils::hour(label)));
                pb_journey->set_duration(duration);
                pb_journey->set_nb_transfers(round);
                fill_pb_placemark(raptor.data.pt_data.journey_pattern_points[best_rp]->stop_point, raptor.data, pb_journey->mutable_destination(), 0);
            }
        }
    }

     std::sort(response.mutable_journeys()->begin(), response.mutable_journeys()->end(),
               [](const pbnavitia::Journey & journey1, const pbnavitia::Journey & journey2) {
               return journey1.duration() < journey2.duration();
                });


    return response;
}


}}
