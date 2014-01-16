#include "raptor_api.h"
#include "type/pb_converter.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/datetime.h"
#include <unordered_set>
#include <chrono>


namespace navitia { namespace routing {

void fill_section(pbnavitia::Section *pb_section, navitia::type::idx_t vj_idx,
        const nt::Data & d, boost::posix_time::ptime now,
        boost::posix_time::time_period action_period) {

    const type::VehicleJourney* vj = d.pt_data.vehicle_journeys[vj_idx];
    if (vj->has_boarding()){
        pb_section->set_type(pbnavitia::boarding);
        return;
    }
    if (vj->has_landing()){
        pb_section->set_type(pbnavitia::landing);
        return;
    }
    auto* vj_pt_display_information = pb_section->mutable_pt_display_informations();
    auto* add_info_vehicle_journey = pb_section->mutable_add_info_vehicle_journey();
    fill_pb_object(vj, d, vj_pt_display_information, 0, now, action_period);
    fill_pb_object(vj, d, add_info_vehicle_journey, 0, now, action_period);
}


pbnavitia::Response make_pathes(const std::vector<navitia::routing::Path>& paths,
        const nt::Data& d, georef::StreetNetwork& worker,
        const type::EntryPoint& origin, const type::EntryPoint& destination,
        const std::vector<boost::posix_time::ptime>& datetimes,
                                bool clockwise) {
    pbnavitia::Response pb_response;
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

    auto temp = worker.get_direct_path();
    if(!temp.path_items.empty()) {
        pb_response.set_response_type(pbnavitia::ITINERARY_FOUND);

        //for each date time we add a direct street journey
        for(boost::posix_time::ptime datetime : datetimes) {
            pbnavitia::Journey* pb_journey = pb_response.add_journeys();
            pb_journey->set_duration(temp.duration.total_seconds());

            boost::posix_time::ptime departure;
            if (clockwise) {
                departure = datetime;
            } else {
                departure = datetime - temp.duration;
            }
            fill_street_sections(origin, temp, d, pb_journey, departure);

            const auto str_departure = boost::posix_time::to_iso_string(departure);
            const auto str_arrival = boost::posix_time::to_iso_string(departure + temp.duration);
            pb_journey->set_departure_date_time(str_departure);
            pb_journey->set_arrival_date_time(str_arrival);
        }
    }

    pb_response.set_response_type(pbnavitia::ITINERARY_FOUND);

    for(Path path : paths) {
        navitia::DateTime departure_time = DateTimeUtils::inf,
                          arrival_time = DateTimeUtils::inf;
        pbnavitia::Journey* pb_journey = pb_response.add_journeys();
        pb_journey->set_nb_transfers(path.nb_changes);
        pb_journey->set_requested_date_time(boost::posix_time::to_iso_string(path.request_time));

        // La marche à pied initiale si on avait donné une coordonnée
        if(path.items.size() > 0 && path.items.front().stop_points.size() > 0) {
            auto temp = worker.get_path(path.items.front().stop_points.front());
            if(temp.path_items.size() > 0) {
                //because of projection problem, the walking path might not join exactly the routing one
                nt::GeographicalCoord routing_first_coord =
                        d.pt_data.stop_points[path.items.front().stop_points.front()]->coord;
                if (temp.path_items.back().coordinates.back() != routing_first_coord) {
                    //if it's the case, we artificialy add the missing segment
                    temp.path_items.back().coordinates.push_back(routing_first_coord);
                }

                const auto walking_time = temp.duration;
                departure_time = path.items.front().departure - walking_time;
                fill_street_sections(origin, temp, d, pb_journey, to_posix_time(departure_time, d));
            }
        }

        const type::VehicleJourney* vj = nullptr;
        // La partie TC et correspondances
        for(PathItem & item : path.items) {

            pbnavitia::Section* pb_section = pb_journey->add_sections();
            if(item.type == public_transport) {
                pb_section->set_type(pbnavitia::PUBLIC_TRANSPORT);
                boost::posix_time::ptime departure_ptime , arrival_ptime;
                vj = d.pt_data.vehicle_journeys[item.vj_idx];
                int length = 0;
                for(size_t i=0;i<item.stop_points.size();++i) {
                    if ((!vj->has_boarding()) && (!vj->has_landing())) {
                        pbnavitia::StopDateTime* stop_time = pb_section->add_stop_date_times();
                        auto arr_time = iso_string(item.arrivals[i], d);
                        stop_time->set_arrival_date_time(arr_time);
                        auto dep_time = iso_string(item.departures[i], d);
                        stop_time->set_departure_date_time(dep_time);
                        const auto p_deptime = navitia::to_posix_time(item.departures[i], d);
                        const auto p_arrtime = navitia::to_posix_time(item.arrivals[i], d);
                        boost::posix_time::time_period action_period(p_deptime, p_arrtime);
                        fill_pb_object(d.pt_data.stop_points[item.stop_points[i]], d, stop_time->mutable_stop_point(), 0, now, action_period);

                        if (item.vj_idx != type::invalid_idx) {
                            vj = d.pt_data.vehicle_journeys[item.vj_idx];
                            fill_pb_object(vj->stop_time_list[item.orders[i]], d, stop_time, 1, now, action_period);
                        }

                        // L'heure de départ du véhicule au premier stop point
                        if(departure_ptime.is_not_a_date_time())
                            departure_ptime = p_deptime;
                        // L'heure d'arrivée au dernier stop point
                        arrival_ptime = p_arrtime;
                        if(i>0) {
                            const auto & previous_coord = d.pt_data.stop_points[item.stop_points[i-1]]->coord;
                            const auto & current_coord = d.pt_data.stop_points[item.stop_points[i]]->coord;
                            length += previous_coord.distance_to(current_coord);
                        }
                    }
                }
                if (item.stop_points.size() > 1) {
                    auto arr_time = item.arrivals[0];
                    auto dep_time = item.departures[0];
                    boost::posix_time::time_period action_period(navitia::to_posix_time(dep_time, d), navitia::to_posix_time(arr_time, d));
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points[0]], d, pb_section->mutable_origin(), 1, now, action_period);
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points[item.stop_points.size()-1]], d, pb_section->mutable_destination(), 1, now, action_period);
                }
                pb_section->set_length(length);
                if( item.vj_idx != type::invalid_idx) { // TODO : réfléchir si ça peut vraiment arriver
                    boost::posix_time::time_period action_period(departure_ptime, arrival_ptime);
                    fill_section(pb_section, item.vj_idx, d, now, action_period);
                }
            } else {
                pb_section->set_type(pbnavitia::TRANSFER);
                switch(item.type) {
                    case stay_in : pb_section->set_transfer_type(pbnavitia::stay_in); break;
                    case guarantee : pb_section->set_transfer_type(pbnavitia::guaranteed); break;
                    case waiting : pb_section->set_type(pbnavitia::WAITING); break;
                    default : pb_section->set_transfer_type(pbnavitia::walking); break;
                }

                boost::posix_time::time_period action_period(navitia::to_posix_time(item.departure, d), navitia::to_posix_time(item.arrival, d));
                const auto& origin_sp = d.pt_data.stop_points[item.stop_points.front()];
                const auto& destination_sp = d.pt_data.stop_points[item.stop_points.back()];
                fill_pb_placemark(origin_sp, d, pb_section->mutable_origin(), 1, now, action_period);
                fill_pb_placemark(destination_sp, d, pb_section->mutable_destination(), 1, now, action_period);
                pb_section->set_length(origin_sp->coord.distance_to(destination_sp->coord));
            }
            auto dep_time = iso_string(item.departure, d);
            pb_section->set_begin_date_time(dep_time);
            auto arr_time = iso_string(item.arrival, d);
            pb_section->set_end_date_time(arr_time);

            if(departure_time == DateTimeUtils::inf)
                departure_time = item.departure;
            arrival_time = item.arrival;
            pb_section->set_duration(item.arrival - item.departure);
        }
        // La marche à pied finale si on avait donné une coordonnée
        if(path.items.size() > 0 && path.items.back().stop_points.size() > 0) {
            auto temp = worker.get_path(path.items.back().stop_points.back(), true);
            if(temp.path_items.size() > 0) {
               //add a junction between the routing path and the walking one if needed
                nt::GeographicalCoord routing_last_coord = d.pt_data.stop_points[path.items.back().stop_points.back()]->coord;
                if (temp.path_items.front().coordinates.front() != routing_last_coord) {
                    temp.path_items.front().coordinates.push_front(routing_last_coord);
                }

                auto begin_section_time = arrival_time;
                fill_street_sections(destination, temp, d, pb_journey, to_posix_time(begin_section_time, d));
            }
        }
        const auto str_departure = iso_string(departure_time, d);
        const auto str_arrival = iso_string(arrival_time, d);
        pb_journey->set_departure_date_time(str_departure);
        pb_journey->set_arrival_date_time(str_arrival);
        pb_journey->set_duration(arrival_time - departure_time);
    }

	if (pb_response.journeys().size() == 0) {
        fill_pb_error(pbnavitia::Error::no_solution, "no solution found for this journey",
        pb_response.mutable_error());
        pb_response.set_response_type(pbnavitia::NO_SOLUTION);
    }

    return pb_response;
}


std::vector<std::pair<type::idx_t, bt::time_duration> >
get_stop_points( const type::EntryPoint &ep, const type::PT_Data & pt_data,
        georef::StreetNetwork & worker, bool use_second = false){
    std::vector<std::pair<type::idx_t, bt::time_duration> > result;
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "calcul des stop points pour l'entry point : [" << ep.coordinates.lat()
              << "," << ep.coordinates.lon() << "]");
    if(ep.type == type::Type_e::StopArea){
        auto it = pt_data.stop_areas_map.find(ep.uri);
        if(it!= pt_data.stop_areas_map.end()) {
            for(auto stop_point : it->second->stop_point_list) {
                result.push_back({stop_point->idx, {}});
            }
        }
    }else if(ep.type == type::Type_e::StopPoint){
        auto it = pt_data.stop_points_map.find(ep.uri);
        if(it != pt_data.stop_points_map.end()){
            result.push_back({it->second->idx, {}});
        }
    }else if(ep.type == type::Type_e::Address
                || ep.type == type::Type_e::Coord || ep.type == type::Type_e::Admin){
        result = worker.find_nearest_stop_points(
                    ep.streetnetwork_params.max_duration,
                    pt_data.stop_point_proximity_list,
                    use_second);
    }
    //On va chercher tous les stop_points en correspondance
    //avec ceux déjà trouvés.
    std::vector<std::pair<type::idx_t, bt::time_duration> > tmp_result;
    std::unordered_set<type::idx_t> visited_stop_points;
    for (const auto & sp_idx_distance : result) { visited_stop_points.insert(sp_idx_distance.first); }

    for(const auto & sp_idx_distance : result) {
        const auto sp_idx = sp_idx_distance.first;
        const auto stop_point = pt_data.stop_points[sp_idx];
        const auto connections_idx = stop_point->get(type::Type_e::StopPointConnection, pt_data);
        for(const auto connection_idx : connections_idx) {
            const auto* connection = pt_data.stop_point_connections[connection_idx];

            const auto destination = connection->destination;
            if (visited_stop_points.find(destination->idx) != visited_stop_points.end()) {
                continue;
            }
            visited_stop_points.insert(destination->idx);

            auto duration = worker.get_distance(destination->idx, use_second);
            /*
             * On mettra ce traitement quand on aura trouvé un moyen de refaire path...
             * if(distance == std::numeric_limits<double>::max()) {
            //Fallback si le stop point n'est pas rattaché au filaire de voirie
            //On recalcule une distance en pensant qu'on marche à 1.68 m/s
                distance = connection.duration * 1.68
            }*/
            if (duration == bt::pos_infin)
                continue;

            tmp_result.push_back({destination->idx, duration});
        }
    }

    result.insert(result.end(), tmp_result.begin(), tmp_result.end());
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
                fill_pb_error(pbnavitia::Error::date_out_of_bounds,
                                "date is not in data production period",
                                response.mutable_error());
                response.set_response_type(pbnavitia::DATE_OUT_OF_BOUNDS);
            }
            datetimes.push_back(ptime);
        } catch(...){
//            response.set_error("Impossible to parse date " + datetime);
            fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse Datetime",response.mutable_error());
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
              const type::AccessibiliteParams & accessibilite_params,
              std::vector<std::string> forbidden,
              georef::StreetNetwork & worker, uint32_t max_duration, uint32_t max_transfers) {

    pbnavitia::Response response;

    std::vector<boost::posix_time::ptime> datetimes;
    datetimes = parse_datetimes(raptor, datetimes_str, response, clockwise);
    if(response.has_error() || response.response_type() == pbnavitia::DATE_OUT_OF_BOUNDS) {
        return response;
    }
    worker.init(origin, {destination});
    auto departures = get_stop_points(origin, raptor.data.pt_data, worker);
    auto destinations = get_stop_points(destination, raptor.data.pt_data, worker, true);
    if(departures.size() == 0 && destinations.size() == 0){
        fill_pb_error(pbnavitia::Error::no_origin_nor_destionation, "no origin point nor destination point",response.mutable_error());
        response.set_response_type(pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT);
        return response;
    }

    if(departures.size() == 0){
        fill_pb_error(pbnavitia::Error::no_origin, "no origin point",response.mutable_error());
        response.set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }

    if(destinations.size() == 0){
        fill_pb_error(pbnavitia::Error::no_destination, "no destination point",response.mutable_error());
        response.set_response_type(pbnavitia::NO_DESTINATION_POINT);
        return response;
    }

    std::vector<Path> result;

    DateTime bound = clockwise ? DateTimeUtils::inf : DateTimeUtils::min;

    for(boost::posix_time::ptime datetime : datetimes) {
        int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
        int time = datetime.time_of_day().total_seconds();
        DateTime init_dt = DateTimeUtils::set(day, time);

        if(max_duration!=std::numeric_limits<uint32_t>::max()) {
            bound = clockwise ? init_dt + max_duration : init_dt - max_duration;
        }

        std::vector<Path> tmp = raptor.compute_all(departures, destinations, init_dt, bound, max_transfers, accessibilite_params/*wheelchair*/, forbidden, clockwise);

        // Lorsqu'on demande qu'un seul horaire, on garde tous les résultas
        if(datetimes.size() == 1) {
            result = tmp;
            for(auto & path : result) {
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

    return make_pathes(result, raptor.data, worker, origin, destination, datetimes, clockwise);
}


pbnavitia::Response make_isochrone(RAPTOR &raptor,
                                   type::EntryPoint origin,
                                   const std::string &datetime_str,bool clockwise,
                                   const type::AccessibiliteParams & accessibilite_params,
                                   std::vector<std::string> forbidden,
                                   georef::StreetNetwork & worker, int max_duration, uint32_t max_transfers) {
    pbnavitia::Response response;

    boost::posix_time::ptime datetime;
    auto tmp_datetime = parse_datetimes(raptor, {datetime_str}, response, clockwise);
    if(response.has_error() || tmp_datetime.size() == 0 ||
       response.response_type() == pbnavitia::DATE_OUT_OF_BOUNDS) {
        return response;
    }
    datetime = tmp_datetime.front();
    worker.init(origin);
    auto departures = get_stop_points(origin, raptor.data.pt_data, worker);

    if(departures.size() == 0){
        response.set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }

    int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
    int time = datetime.time_of_day().total_seconds();
    DateTime init_dt = DateTimeUtils::set(day, time);
    DateTime bound = clockwise ? init_dt + max_duration : init_dt - max_duration;

    raptor.isochrone(departures, init_dt, bound, max_transfers,
                           accessibilite_params/*wheelchair*/, forbidden, clockwise);


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
            boost::tie(initial_rp, initial_dt) = getFinalRpidAndDate(round, best_rp, clockwise, raptor.labels);

            int duration = ::abs(label - init_dt);

            if(duration <= max_duration) {
                auto pb_journey = response.add_journeys();
                const auto str_departure = iso_string(label, raptor.data);
                const auto str_arrival = iso_string(label, raptor.data);
                const auto str_requested = iso_string(init_dt, raptor.data);
                pb_journey->set_arrival_date_time(str_arrival);
                pb_journey->set_departure_date_time(str_departure);
                pb_journey->set_requested_date_time(str_requested);
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
