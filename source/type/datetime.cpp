/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "type/datetime.h"
#include "type/data.h"
#include "type/time_duration.h"
#include "type/meta_data.h"

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


std::string iso_string(const DateTime datetime, const type::Data &d){
    return boost::posix_time::to_iso_string(to_posix_time(datetime, d));
}

std::string iso_hour_string(const DateTime datetime, const type::Data &d) {
    auto time = to_posix_time(datetime, d);
    static std::locale loc(std::cout.getloc(), new boost::posix_time::time_facet("T%H%M%S")); //note: the destruction of the time_facet is handled by the locale
    std::stringstream ss;
    ss.imbue(loc);
    ss << time;
    return ss.str();
}


boost::posix_time::ptime to_posix_time(DateTime datetime, const type::Data &d){
    boost::posix_time::ptime date_time(d.meta->production_date.begin() + boost::gregorian::days(DateTimeUtils::date(datetime)));
    date_time += boost::posix_time::seconds(DateTimeUtils::hour(datetime));
    return date_time;
}


DateTime to_datetime(boost::posix_time::ptime ptime, const type::Data &d) {
    int day = (ptime.date() - d.meta->production_date.begin()).days();
    int time = ptime.time_of_day().total_seconds();
    return DateTimeUtils::set(day, time);
}

boost::posix_time::ptime from_posix_timestamp(uint32_t val) {
    return posix_epoch + navitia::seconds(val).to_posix();
}

}
