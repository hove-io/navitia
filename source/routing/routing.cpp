#include "routing.h"

namespace navitia { namespace routing {

std::string PathItem::print(const navitia::type::PT_Data & data) const {
    std::stringstream ss;

    if(stop_points.size() < 2) {
        ss << "Section avec moins de 2 stop points, hrmmmm \n";
        return ss.str();
    }

    navitia::type::StopArea start = data.stop_areas[data.stop_points[stop_points.front()].stop_area_idx];
    navitia::type::StopArea dest = data.stop_areas[data.stop_points[stop_points.back()].stop_area_idx];
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
        const navitia::type::VehicleJourney & vj = data.vehicle_journeys[vj_idx];
        const navitia::type::JourneyPattern & journey_pattern = data.journey_patterns[vj.journey_pattern_idx];
        const navitia::type::Route & route = data.routes[journey_pattern.route_idx];
        const navitia::type::Line & line = data.lines[route.line_idx];
        ss << "Ligne : " << line.name  << " (" << line.uri << " " << line.idx << "), "
           << "Route : " << route.name  << " (" << route.uri << " " << route.idx << "), "
           << "JourneyPattern : " << journey_pattern.name << " (" << journey_pattern.uri << " " << journey_pattern.idx << "), "
           << "Vehicle journey " << vj_idx << "\n";
    }
    ss << "Départ de " << start.name << "(" << start.uri << " " << start.idx << ") à " << departure << "\n";
    for(auto sp_idx : stop_points){
        navitia::type::StopPoint sp = data.stop_points[sp_idx];
        ss << "    " << sp.name << " (" << sp.uri << " " << sp.idx << ")" << "\n";
    }
    ss << "Arrivée à " << dest.name << "(" << dest.uri << " " << dest.idx << ") à " << arrival << "\n";
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
            if(!data.validity_patterns[data.vehicle_journeys[item.vj_idx].validity_pattern_idx].check(item.departure.date())) {
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
            const navitia::type::VehicleJourney & vj = data.vehicle_journeys[item.vj_idx];

            for(auto spidx : item.stop_points) {
                if(std::find_if(vj.stop_time_list.begin(), vj.stop_time_list.end(),
                                [&](int stidx){ return (data.journey_pattern_points[data.stop_times[stidx].journey_pattern_point_idx].stop_point_idx == spidx);}) == vj.stop_time_list.end()) {
                    std::cout << "Le stop point : " << spidx << " n'appartient pas au vj : " << item.vj_idx << std::endl;
                    return false;
                }
            }
        }
    }
    return true;
}

bool Verification::check_correspondances(Path path) {
    std::vector<navitia::type::Connection> stop_point_list;

    PathItem precitem;
    for(PathItem item : path.items) {
        navitia::type::Connection conn;
        if(item.type == walking) {
            conn.departure_idx = item.stop_points.front();
            conn.destination_idx =  item.stop_points.back();
            conn.duration = item.arrival - item.departure;
            stop_point_list.push_back(conn);
        }
        if(precitem.arrival != navitia::type::DateTime::inf) {
            if(precitem.type == public_transport && item.type == public_transport) {
                conn.departure_idx = precitem.stop_points.back();
                conn.destination_idx =  item.stop_points.front();
                conn.duration = item.departure - precitem.arrival;
                stop_point_list.push_back(conn);
            }
        }

        precitem = item;
    }

    for(navitia::type::Connection conn: data.connections) {
        auto it = std::find_if(stop_point_list.begin(), stop_point_list.end(),
                               [&](navitia::type::Connection p){return (p.departure_idx == conn.departure_idx && p.destination_idx == conn.destination_idx)
                               ||(p.destination_idx == conn.departure_idx && p.departure_idx == conn.destination_idx);});


    if(it != stop_point_list.end()) {
        if(it->duration != conn.duration) {
            std::cout << "Le temps de correspondance de la connection " << data.stop_points[it->departure_idx].name << "(" << it->departure_idx << ") -> "
                      << data.stop_points[it->destination_idx].name << "(" << it->destination_idx << ") ne correspond pas aux données : "
                      <<  it->duration << " != " <<  conn.duration << std::endl;
            return false;
        } else {
            stop_point_list.erase(it);
        }
    }

    if(stop_point_list.size() == 0)
        return true;
}
bool toreturn = true;
for(auto psp : stop_point_list) {
    if(data.stop_points[psp.departure_idx].stop_area_idx != data.stop_points[psp.destination_idx].stop_area_idx) {
        std::cout << "La correspondance " << psp.departure_idx << " => " << psp.destination_idx << " n'existe pas " << std::endl;
        toreturn = false;
    }

}
return toreturn;
}



                  }}
