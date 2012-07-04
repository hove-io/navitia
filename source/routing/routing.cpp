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



}}
