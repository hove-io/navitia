#include "datetime.h"
#include "type/data.h"

namespace navitia { namespace type {

DateTime DateTime::inf = DateTime();
DateTime DateTime::min = DateTime(0,0);

std::ostream & operator<<(std::ostream & os, const DateTime & dt){
    os << "D=" << dt.date() << " " << dt.hour()/(3600) << ":";
    if((dt.hour()%3600)/60 < 10)
        os << "0" << (dt.hour()%3600)/60;
    else
        os << (dt.hour()%3600)/60;
    return os;
}


std::string iso_string(int date, int hour, const Data & d){
    auto tmp = DateTime(date, hour);
    return iso_string(tmp, d);
}

std::string iso_string(DateTime &datetime, const Data &d){
    return boost::posix_time::to_iso_string(to_posix_time(datetime, d));
}


boost::posix_time::ptime to_posix_time(DateTime &datetime, const Data &d) {
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(datetime.date()));
    date_time += boost::posix_time::seconds(datetime.hour());
    return date_time;
}


} }
