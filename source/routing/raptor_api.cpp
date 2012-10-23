#include "raptor_api.h"
#include "type/pb_converter.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "street_network/street_network_api.h"

namespace navitia { namespace routing { namespace raptor {

std::string iso_string(const nt::Data & d, int date, int hour){
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(date));
    date_time += boost::posix_time::seconds(hour);
    return boost::posix_time::to_iso_string(date_time);
}

pbnavitia::Response make_pathes(const std::vector<navitia::routing::Path> &paths, const nt::Data & d, georef::StreetNetworkWorker & worker, bool origin_coord, bool destination_coord) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PLANNER);

    pbnavitia::Planner * planner = pb_response.mutable_planner();
    planner->set_response_type(pbnavitia::ITINERARY_FOUND);
    for(Path path : paths) {
        pbnavitia::Journey * pb_journey = planner->add_journey();
        pb_journey->set_duration(path.duration);
        pb_journey->set_nb_transfers(path.nb_changes);
        pb_journey->set_requested_date_time(boost::posix_time::to_iso_string(path.request_time));

        // La marche à pied initiale si on avait donné une coordonnée
        if(origin_coord && path.items.size() > 0 && path.items.front().stop_points.size() > 0){
            fill_road_section(worker.get_path(path.items.front().stop_points.front()), d, pb_journey->add_section(), 1);
        }

        // La partie TC et correspondances
        for(PathItem & item : path.items){
            pbnavitia::Section * pb_section = pb_journey->add_section();
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
                    if(vj.mode_idx != type::invalid_idx)
                        pb_section->set_mode(d.pt_data.modes[vj.mode_idx].name);
                    pb_section->set_code(line.code);
                    pb_section->set_headsign(vj.name);
                    pb_section->set_direction(route.name);
                    fill_pb_object(line.idx, d, pb_section->mutable_line());
                }
                for(size_t i=0;i<item.stop_points.size();++i){
                    pbnavitia::StopTime * stop_time = pb_section->add_stop_time();
                    auto arr_time = item.arrivals[i];
                    stop_time->set_arrival_date_time(iso_string(d, arr_time.date(), arr_time.hour()));
                    auto dep_time = item.departures[i];
                    stop_time->set_departure_date_time(iso_string(d, dep_time.date(), dep_time.hour()));
                    fill_pb_object(item.stop_points[i], d, stop_time->mutable_stop_point(), 1);
                }

                if(item.stop_points.size() >= 2) {
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points.front()], d, pb_section->mutable_origin());
                    fill_pb_placemark(d.pt_data.stop_points[item.stop_points.back()], d, pb_section->mutable_destination());
                }
            }

            else
                pb_section->set_type(pbnavitia::TRANSFER);
        }

        // La marche à pied finale si on avait donné une coordonnée
        if(destination_coord && path.items.size() > 0 && path.items.back().stop_points.size() > 0){
            fill_road_section(worker.get_path(path.items.back().stop_points.back(), true), d, pb_journey->add_section(), 1);
        }
    }

    return pb_response;
}

std::vector<std::pair<type::idx_t, double> > get_stop_points(const type::EntryPoint &ep, const type::Data & data, georef::StreetNetworkWorker & worker, bool use_second = false){
    std::vector<std::pair<type::idx_t, double> > result;

    switch(ep.type) {
    case navitia::type::Type_e::eStopArea:
    {
        auto it = data.pt_data.stop_area_map.find(ep.external_code);
        if(it!= data.pt_data.stop_area_map.end()) {
            for(auto spidx : data.pt_data.stop_areas[it->second].stop_point_list) {
                result.push_back(std::make_pair(spidx, 0));
            }
        }
    } break;
    case type::Type_e::eStopPoint: {
        auto it = data.pt_data.stop_point_map.find(ep.external_code);
        if(it != data.pt_data.stop_point_map.end()){
            result.push_back(std::make_pair(data.pt_data.stop_points[it->second].idx, 0));
        }
    } break;
    case type::Type_e::eCoord: {
        result = worker.find_nearest_stop_points(ep.coordinates, data.pt_data.stop_point_proximity_list, 1000, use_second);
    } break;
    default: break;
    }
    return result;
}


pbnavitia::Response make_response(RAPTOR &raptor, const type::EntryPoint &origin, const type::EntryPoint &destination,
                                  const std::vector<std::string> &datetimes_str, bool clockwise,
                                  std::multimap<std::string, std::string> forbidden,
                                  georef::StreetNetworkWorker & worker) {
    pbnavitia::Response response;
    response.set_requested_api(pbnavitia::PLANNER);

    std::vector<boost::posix_time::ptime> datetimes;
    for(std::string datetime: datetimes_str){
        try {
            boost::posix_time::ptime ptime;
            ptime = boost::posix_time::from_iso_string(datetime);
            if(!raptor.data.meta.production_date.contains(ptime.date())) {
                response.mutable_planner()->set_response_type(pbnavitia::DATE_OUT_OF_BOUNDS);
                response.set_info("Example of invalid date: " + datetime);
                return response;
            }
            datetimes.push_back(ptime);
        } catch(...){
            response.set_error("Impossible to parse date " + datetime);
            return response;
        }
    }
    if(clockwise)
        std::sort(datetimes.begin(), datetimes.end(), 
                  [](boost::posix_time::ptime dt1, boost::posix_time::ptime dt2){return dt1 > dt2;});
    else
        std::sort(datetimes.begin(), datetimes.end());

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
            tmp = raptor.compute_all(departures, destinations, DateTime(day, time), borne, forbidden);
        else
            tmp = raptor.compute_reverse_all(departures, destinations, DateTime(day, time), borne, forbidden);

        // Lorsqu'on demande qu'un seul horaire, on garde tous les résultas
        if(datetimes.size() == 1){
            result = tmp;
            if(result.size() == 0){
                response.mutable_planner()->set_response_type(pbnavitia::NO_SOLUTION);
                return response;
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

    return make_pathes(result, raptor.data, worker, origin.type == nt::Type_e::eCoord, destination.type == nt::Type_e::eCoord);
}

}}}
