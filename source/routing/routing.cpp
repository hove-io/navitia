#include "routing.h"

namespace navitia { namespace routing {



DateTime operator+(DateTime dt, int seconds) {
    if(!(dt == DateTime::infinity())){
        dt.hour += seconds;
        //dt.normalize();
    }
    return dt;
}

std::ostream & operator<<(std::ostream & os, const DateTime & dt){
    os << "D=" << dt.date << " " << dt.hour/(3600) << ":" << (dt.hour%3600)/60;
    return os;
}

std::ostream & operator<<(std::ostream & os, const Path & path) {
    os << "Nombre de correspondances : " << path.nb_changes << std::endl
       << "Durée totale : " << path.duration << std::endl
       << path.percent_visited << "% visited" << std::endl;

    BOOST_FOREACH(PathItem item, path.items) {
        os  << item.said << " à " << item.time << " le " << item.day << " avec "  << item.line_idx <<std::endl;
    }
    return os;
}



}}
