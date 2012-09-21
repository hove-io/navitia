#include "raptor_api.h"
#include "routing.h"
#include "type/pb_converter.h"

namespace navitia { namespace routing { namespace raptor {


pbnavitia::Response make_pathes(const std::vector<navitia::routing::Path> &paths, const nt::Data & d) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PLANNER);

    for(Path path : paths) {
        //navitia::routing::Path itineraire = navitia::routing::makeItineraire(path);
        pbnavitia::PTPath * pb_path = pb_response.mutable_planner()->add_path();
        pb_path->set_duration(path.duration);
        pb_path->set_nb_changes(path.nb_changes);
        for(PathItem & item : path.items){
            pbnavitia::PTPathItem * pb_item = pb_path->add_items();
            pb_item->set_arrival_date(item.arrival.date());
            pb_item->set_arrival_hour(item.arrival.hour());
            pb_item->set_departure_date(item.departure.date());
            pb_item->set_departure_hour(item.departure.hour());
            if(item.type == public_transport)
                pb_item->set_type("Public Transport");
            else
                pb_item->set_type("Walking");
            if(item.type == public_transport && item.vj_idx != type::invalid_idx){
                const type::VehicleJourney & vj = d.pt_data.vehicle_journeys[item.vj_idx];
                const type::Route & route = d.pt_data.routes[vj.route_idx];
                const type::Line & line = d.pt_data.lines[route.line_idx];
                pb_item->set_line_name(line.name);
            }
            for(navitia::type::idx_t stop_point : item.stop_points){
                fill_pb_object<type::Type_e::eStopPoint>(stop_point, d, pb_item->add_stop_points());
            }
            if(item.stop_points.size() >= 2) {
                fill_pb_object<type::Type_e::eStopArea>(d.pt_data.stop_points[item.stop_points.front()].stop_area_idx, d, pb_item->mutable_departure());
                fill_pb_object<type::Type_e::eStopArea>(d.pt_data.stop_points[item.stop_points.back()].stop_area_idx, d, pb_item->mutable_arrival());
            }

        }
    }

    return pb_response;
}

bool checkTime(const int time) {
    return !(time < 0 || time > 86400);
}

std::vector<std::pair<type::idx_t, double> > get_stop_points(const type::EntryPoint &ep, const type::Data & data, streetnetwork::StreetNetworkWorker & worker){
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
        result = worker.find_nearest(ep.coordinates, data.pt_data.stop_point_proximity_list, 300);
    } break;
    default: break;
    }
    return result;
}

pbnavitia::Response make_response(RAPTOR &raptor, const type::EntryPoint &departure, const type::EntryPoint &destination, const int time, const boost::gregorian::date &date, const senscompute sens, streetnetwork::StreetNetworkWorker & worker) {
    pbnavitia::Response response;
    if(!raptor.data.meta.production_date.contains(date)) {
        response.set_error("Date not in the production period");
        return response;
    }
    auto departures = get_stop_points(departure, raptor.data, worker);
    if(departures.size() == 0){
        response.set_error("Departure point not found");
        return response;
    }

    auto destinations = get_stop_points(destination, raptor.data, worker);
    if(destinations.size() == 0){
        response.set_error("Destination point not found");
        return response;
    }

    return make_pathes(raptor.compute_all(departures, destinations, time, (date - raptor.data.meta.production_date.begin()).days(), sens), raptor.data);

}

}}}
