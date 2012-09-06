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

bool Verification::verif(Path path) {
    return  croissance(path) && vj_valides(path) && appartenance_rp(path) && check_correspondances(path);
}

bool Verification::croissance(Path path) {
    DateTime precdt = DateTime::min;
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
            navitia::type::VehicleJourney & vj = data.vehicle_journeys[item.vj_idx];

            unsigned int first = 0;

            for(;first < vj.stop_time_list.size(); ++first) {
                if(data.route_points[data.stop_times[vj.stop_time_list[first]].route_point_idx].stop_point_idx == item.stop_points.front())
                    break;
            }

            if(first >=vj.stop_time_list.size()) {
                std::cout << "le stop point : " << data.stop_points[item.stop_points.front()].name << " ("  << item.stop_points.front() << ") n'appartient pas au vj : " << item.vj_idx << std::endl;
                return false;
            }

            for(unsigned int i = 0; i < item.stop_points.size(); ++i) {
                if(data.route_points[data.stop_times[vj.stop_time_list[first + i]].route_point_idx].stop_point_idx != item.stop_points[i]) {
                    std::cout << "Le stop point : " << item.stop_points[i] << " n'appartient pas au vj : " << item.vj_idx << std::endl;
                    return false;
                }
            }
        }
    }
    return true;
}

bool Verification::check_correspondances(Path path) {
    std::vector<std::pair<navitia::type::idx_t, navitia::type::idx_t>> stop_point_list;

    for(PathItem item : path.items) {
        if(item.type == walking) {
            if(data.stop_points[item.stop_points.front()].stop_area_idx != data.stop_points[item.stop_points.back()].stop_area_idx) {
                stop_point_list.push_back(std::make_pair(item.stop_points.front(), item.stop_points.back()));
            }
        }
    }

    for(navitia::type::Connection conn: data.connections) {
        auto it = stop_point_list.begin();

        for(;it!= stop_point_list.end(); ++it) {
            if(conn.departure_stop_point_idx == it->first && conn.destination_stop_point_idx == it->second)
                break;
        }

        if(it != stop_point_list.end())
            stop_point_list.erase(it);

        if(stop_point_list.size() == 0)
            return true;
    }

    for(auto psp : stop_point_list) {
        std::cout << "La correspondance " << psp.first << " => " << psp.second << " n'existe pas " << std::endl;
    }
    return false;


    return true;
}



}}
