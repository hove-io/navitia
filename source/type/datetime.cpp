#include "type/datetime.h"
#include "type/data.h"

namespace navitia {
/*
std::ostream & operator<<(std::ostream & os, const DateTime & dt){
    os << "D=" << DateTimeUtils::date(dt) << " " << DateTimeUtils::hour(dt)/(3600) << ":";
    if((DateTimeUtils::hour(dt)%3600)/60 < 10)
        os << "0" << (DateTimeUtils::hour(dt)%3600)/60;
    else
        os << (DateTimeUtils::hour(dt)%3600)/60;
    return os;
}
*/
std::string str(const DateTime & dt){
    std::stringstream os;
    os << "D=" << DateTimeUtils::date(dt) << " " << DateTimeUtils::hour(dt)/(3600) << ":";
    if((DateTimeUtils::hour(dt)%3600)/60 < 10)
        os << "0" << (DateTimeUtils::hour(dt)%3600)/60;
    else
        os << (DateTimeUtils::hour(dt)%3600)/60;
    return os.str();
}

std::string iso_string(DateTime datetime, const type::Data &d){
    return boost::posix_time::to_iso_string(to_posix_time(datetime, d));
}

std::string iso_string(int date, int hour, const type::Data &d){
    DateTime tmp = DateTimeUtils::set(date, hour);
    return iso_string(tmp, d);
}


boost::posix_time::ptime to_posix_time(DateTime datetime, const type::Data &d){
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(DateTimeUtils::date(datetime)));
    date_time += boost::posix_time::seconds(DateTimeUtils::hour(datetime));
    return date_time;
}
}
