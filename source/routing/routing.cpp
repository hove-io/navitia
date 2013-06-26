#include "routing.h"

namespace navitia { namespace routing {

std::string PathItem::print(const navitia::type::PT_Data & data) const {
    std::stringstream ss;

    if(stop_points.size() < 2) {
        ss << "Section avec moins de 2 stop points, hrmmmm \n";
        return ss.str();
    }

    const navitia::type::StopArea* start = data.stop_points[stop_points.front()]->stop_area;
    const navitia::type::StopArea* dest = data.stop_points[stop_points.back()]->stop_area;
    ss << "Section de type ";
    if(type == public_transport)
        ss << "transport en commun";
    else if(type == walking)
        ss << "marche";
    else if(type == extension)
        ss << "prolongement de service";
    else if(type ==guarantee)
        ss << "corresopondance garantie";
    ss << "\n";


    if(type == public_transport && vj_idx != navitia::type::invalid_idx){
        const navitia::type::VehicleJourney* vj = data.vehicle_journeys[vj_idx];
        const navitia::type::JourneyPattern* journey_pattern = vj->journey_pattern;
        const navitia::type::Route* route = journey_pattern->route;
        const navitia::type::Line* line = route->line;
        ss << "Ligne : " << line->name  << " (" << line->uri << " " << line->idx << "), "
           << "Route : " << route->name  << " (" << route->uri << " " << route->idx << "), "
           << "JourneyPattern : " << journey_pattern->name << " (" << journey_pattern->uri << " " << journey_pattern->idx << "), "
           << "Vehicle journey " << vj_idx << "\n";
    }
    ss << "Départ de " << start->name << "(" << start->uri << " " << start->idx << ") à " << departure << "\n";
    for(auto sp_idx : stop_points){
        const navitia::type::StopPoint* sp = data.stop_points[sp_idx];
        ss << "    " << sp->name << " (" << sp->uri << " " << sp->idx << ")" << "\n";
    }
    ss << "Arrivée à " << dest->name << "(" << dest->uri << " " << dest->idx << ") à " << arrival << "\n";
    return ss.str();
}

bool Verification::verif(Path path) {
    return  croissance(path) && vj_valides(path) && appartenance_rp(path) && check_correspondances(path);
}

bool Verification::croissance(Path path) {
    navitia::type::DateTime precdt = navitia::type::DateTime::min;
    for(PathItem item : path.items) {
        if(precdt > item.departure) {
            std::cout << "Erreur dans la vérification de la croissance des horaires : " << precdt  << " >  " << item.departure << std::endl;
            return false;
        }
        if(item.departure > item.arrival) {
            std::cout << "Erreur dans la vérification de la croissance des horaires : " << item.departure<< " >  "   << item.arrival  << std::endl;
            return false;
        }
        precdt = item.arrival;
    }
    return true;
}

bool Verification::vj_valides(Path path) {
    for(PathItem item : path.items) {
        if(item.type == public_transport) {
            if(!data.vehicle_journeys[item.vj_idx]->validity_pattern->check(item.departure.date())) {
                std::cout << " le vj : " << item.vj_idx << " n'est pas valide le jour : " << item.departure.date() << std::endl;
                return false;
            }
        }

    }
    return true;
}

bool Verification::appartenance_rp(Path path) {
    for(PathItem item : path.items) {

        if(item.type == public_transport) {
            const navitia::type::VehicleJourney* vj = data.vehicle_journeys[item.vj_idx];

            for(auto spidx : item.stop_points) {
                if(std::find_if(vj->stop_time_list.begin(), vj->stop_time_list.end(),
                                [&](const nt::StopTime* stop_time){ return (stop_time->journey_pattern_point->stop_point->idx == spidx);}) == vj->stop_time_list.end()) {
                    std::cout << "Le stop point : " << spidx << " n'appartient pas au vj : " << item.vj_idx << std::endl;
                    return false;
                }
            }
        }
    }
    return true;
}

bool Verification::check_correspondances(Path path) {
    std::vector<navitia::type::StopPointConnection> connection_list;

    PathItem precitem;
    for(PathItem item : path.items) {
        navitia::type::StopPointConnection conn;
        if(item.type == walking) {
            conn.departure = data.stop_points[item.stop_points.front()];
            conn.destination =  data.stop_points[item.stop_points.back()];
            conn.duration = item.arrival - item.departure;
            connection_list.push_back(conn);
        }
        if(precitem.arrival != navitia::type::DateTime::inf) {
            if(precitem.type == public_transport && item.type == public_transport) {
                conn.departure = data.stop_points[precitem.stop_points.back()];
                conn.destination =  data.stop_points[item.stop_points.front()];
                conn.duration = item.departure - precitem.arrival;
                connection_list.push_back(conn);
            }
        }

        precitem = item;
    }

    for(const navitia::type::StopPointConnection* conn: data.stop_point_connections) {
        auto it = std::find_if(connection_list.begin(), connection_list.end(),
                               [&](navitia::type::StopPointConnection p){return (p.departure == conn->departure && p.destination == conn->destination)
                               ||(p.destination == conn->departure && p.departure == conn->destination);});


    if(it != connection_list.end()) {
        if(it->duration != conn->duration) {
            std::cout << "Le temps de correspondance de la connection " << it->departure->name << "(" << it->departure->idx << ") -> "
                      << it->destination->name << "(" << it->destination->idx << ") ne correspond pas aux données : "
                      <<  it->duration << " != " <<  conn->duration << std::endl;
            return false;
        } else {
            connection_list.erase(it);
        }
    }

    if(connection_list.empty())
        return true;
}
bool toreturn = true;
for(auto psp : connection_list) {
    if(psp.departure->stop_area != psp.destination->stop_area) {
        std::cout << "La correspondance " << psp.departure->idx << " => " << psp.destination->idx << " n'existe pas " << std::endl;
        toreturn = false;
    }

}
return toreturn;
}

}}
