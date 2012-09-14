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

pbnavitia::Response make_response(RAPTOR &raptor, const type::EntryPoint &departure, const type::EntryPoint &destination, const int time, const boost::gregorian::date &date, const navitia::routing::senscompute sens) {
    pbnavitia::Response response;
    if(!raptor.data.meta.production_date.contains(date)) {
        response.set_error("Date not in the production period");
        return response;
    }


    response = navitia::routing::raptor::make_pathes(raptor.compute_all(departure, destination, time, (date - raptor.data.meta.production_date.begin()).days(), sens), raptor.data);


    return response;
}

}}}
