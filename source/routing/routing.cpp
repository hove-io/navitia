#include "routing.h"

namespace navitia { namespace routing {

DateTime DateTime::inf = DateTime::infinity();
DateTime DateTime::min = DateTime::minimity();

std::ostream & operator<<(std::ostream & os, const DateTime & dt){
    os << "D=" << dt.date << " " << dt.hour/(3600) << ":" << (dt.hour%3600)/60;
    return os;
}

std::ostream & operator<<(std::ostream & os, const Path & path) {
    os << "Nombre de correspondances : " << path.nb_changes << std::endl
       << "Durée totale : " << path.duration << std::endl
       << path.percent_visited << "% visited" << std::endl;

    BOOST_FOREACH(PathItem item, path.items) {
        os  << item.said << " à " << item.arrival.hour << " le " << item.arrival.date << " avec "  << item.line_idx <<std::endl;
    }
    return os;
}

Path makeItineraire(const Path &path) {
    Path result;
    result.duration = path.duration;
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

    }
    return result;
}

}}
