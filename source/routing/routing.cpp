#include "routing.h"

namespace navitia { namespace routing {

std::string PathItem::print() const {
    std::stringstream ss;

    if(stop_points.size() < 2) {
        ss << "Section avec moins de 2 stop points, hrmmmm \n";
        return ss.str();
    }

    const navitia::type::StopArea* start = stop_points.front()->stop_area;
    const navitia::type::StopArea* dest = stop_points.back()->stop_area;
    ss << "Section de type ";
    if(type == public_transport)
        ss << "transport en commun";
    else if(type == walking)
        ss << "marche";
    else if(type == stay_in)
        ss << "prolongement de service";
    else if(type ==guarantee)
        ss << "corresopondance garantie";
    ss << "\n";


    if(type == public_transport && ! stop_times.empty()){
        const navitia::type::StopTime* st = stop_times.front();
        const navitia::type::VehicleJourney* vj = st->vehicle_journey;
        const navitia::type::JourneyPattern* journey_pattern = vj->journey_pattern;
        const navitia::type::Route* route = journey_pattern->route;
        const navitia::type::Line* line = route->line;
        ss << "Ligne : " << line->name  << " (" << line->uri << " " << line->idx << "), "
           << "Route : " << route->name  << " (" << route->uri << " " << route->idx << "), "
           << "JourneyPattern : " << journey_pattern->name << " (" << journey_pattern->uri << " " << journey_pattern->idx << "), "
           << "Vehicle journey " << vj->idx << "\n";
    }
    ss << "Départ de " << start->name << "(" << start->uri << " " << start->idx << ") à " << departure << "\n";
    for(auto sp : stop_points){
        ss << "    " << sp->name << " (" << sp->uri << " " << sp->idx << ")" << "\n";
    }
    ss << "Arrivée à " << dest->name << "(" << dest->uri << " " << dest->idx << ") à " << arrival << "\n";
    return ss.str();
}


}}
