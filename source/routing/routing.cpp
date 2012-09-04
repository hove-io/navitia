#include "routing.h"

namespace navitia { namespace routing {

DateTime DateTime::inf = DateTime::infinity();
DateTime DateTime::min = DateTime::minimity();

std::ostream & operator<<(std::ostream & os, const DateTime & dt){
    os << "D=" << dt.date() << " " << dt.hour()/(3600) << ":";
    if((dt.hour()%3600)/60 < 10)
        os << "0" << (dt.hour()%3600)/60;
    else
        os << (dt.hour()%3600)/60;
    return os;
}

std::ostream & operator<<(std::ostream & os, const Path & path) {
    os << "Nombre de correspondances : " << path.nb_changes << std::endl
       << "Durée totale : " << path.duration << std::endl
       << path.percent_visited << "% visited" << std::endl;

    BOOST_FOREACH(PathItem item, path.items) {
      //  os  << item.said << " à " << item.arrival.hour() << " le " << item.arrival.date() << " avec "  << item.line_idx <<std::endl;
    }
    return os;
}

std::string PathItem::print(const navitia::type::PT_Data & data) const {
    std::stringstream ss;

    if(stop_points.size() < 2) {
        ss << "Section avec moins de 2 stop points, hrmmmm \n";
        return ss.str();
    }

    navitia::type::StopArea start = data.stop_areas[data.stop_points[stop_points.front()].stop_area_idx];
    navitia::type::StopArea dest = data.stop_areas[data.stop_points[stop_points.back()].stop_area_idx];
    ss << "Section de type " << (type == public_transport ? "transport en commun" : "marche") << "\n";

    if(type == public_transport && vj_idx != navitia::type::invalid_idx){
        const navitia::type::VehicleJourney & vj = data.vehicle_journeys[vj_idx];
        const navitia::type::Route & route = data.routes[vj.route_idx];
        const navitia::type::Line & line = data.lines[route.line_idx];
        ss << "Ligne : " << line.name  << " (" << line.external_code << " " << line.idx << "), "
           << "Route : " << route.name << " (" << route.external_code << " " << route.idx << "), "
           << "Vehicle journey " << vj_idx << "\n";
    }
    ss << "Départ de " << start.name << "(" << start.external_code << " " << start.idx << ") à " << departure << "\n";
    for(auto sp_idx : stop_points){
        navitia::type::StopPoint sp = data.stop_points[sp_idx];
        ss << "    " << sp.name << " (" << sp.external_code << " " << sp.idx << ")" << "\n";
    }
    ss << "Arrivée à " << dest.name << "(" << dest.external_code << " " << dest.idx << ") à " << arrival << "\n";
    return ss.str();
}

Path makeItineraire(const Path &path) {
    Path result;
  /*  result.duration = path.duration;
    result.nb_changes = path.nb_changes;
    result.percent_visited = path.percent_visited;

    if(path.items.size() > 0) {
        result.items.push_back(PathItem(path.items.front().said, path.items.front().arrival, path.items.front().departure, path.items.front().line_idx));
        PathItem precitem = result.items.front();
        bool eviterpremier = false;
        BOOST_FOREACH(PathItem item, path.items) {
            if(eviterpremier) {
            if(precitem.line_idx != item.line_idx) {
                result.items.push_back(PathItem(precitem.said, precitem.arrival,  precitem.departure, precitem.line_idx));
                result.items.push_back(PathItem(item.said, item.arrival, item.departure, item.line_idx));

            }
            } else {
                eviterpremier = true;
            }
            precitem = item;
        }
        result.items.push_back(PathItem(path.items.back().said, path.items.back().arrival, path.items.back().departure, path.items.back().line_idx));
//        std::reverse(result.items.begin(), result.items.end());

    }*/
    return result;
}

}}
