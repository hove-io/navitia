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

#pragma once
#include <cstdint>
#include <string>
#include <limits>
#include <fstream>
#include "utils/flat_enum_map.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_config.hpp>

namespace boost{ namespace posix_time{ class ptime; class time_duration; }}

namespace navitia{
typedef uint32_t DateTime;

namespace DateTimeUtils{

    const uint32_t SECONDS_PER_DAY = 86400;

    const navitia::DateTime inf = std::numeric_limits<uint32_t>::max();
    const navitia::DateTime min = 0;

    inline navitia::DateTime set(int date, int hour) {
        return date * SECONDS_PER_DAY + hour;
    }

    inline uint32_t hour(navitia::DateTime datetime) {
        return datetime%SECONDS_PER_DAY;
    }

    inline uint32_t date(navitia::DateTime datetime) {
        return datetime/SECONDS_PER_DAY;
    }

    inline void update(navitia::DateTime & datetime, uint32_t hh, bool clockwise = true) {
        uint32_t this_hour = hour(datetime);
        if(hh>=SECONDS_PER_DAY)
            hh -= SECONDS_PER_DAY;
        if(clockwise){
            datetime += (hh>=this_hour ?0:SECONDS_PER_DAY) + hh - this_hour;
        }
        else {
            if(hh<=this_hour)
                datetime += hh - this_hour;
            else {
                if(date(datetime) > 0)
                    datetime += hh - this_hour - SECONDS_PER_DAY;
                else
                    datetime = 0;
            }
        }
    }

}
namespace type {
class Data;
}

//std::ostream & operator<<(std::ostream & os, const DateTime &dt);
std::string str(const DateTime &dt);
std::string iso_string(const DateTime dt, const type::Data &d);
std::string iso_hour_string(const DateTime dt, const type::Data &d);


boost::posix_time::ptime to_posix_time(DateTime datetime, const type::Data &d);
DateTime to_datetime(boost::posix_time::ptime ptime, const type::Data &d);

template <typename... Args>
inline DateTime operator-(DateTime time, boost::date_time::time_duration<Args...> dur) {
    return time - dur.total_seconds();
}

template <typename... Args>
inline DateTime operator+(DateTime time, boost::date_time::time_duration<Args...> dur) {
    return time + dur.total_seconds();
}

inline boost::posix_time::ptime strip_fractional_second(boost::posix_time::ptime ptime) {
    boost::posix_time::time_duration tod = ptime.time_of_day();
    return boost::posix_time::ptime(ptime.date(), boost::posix_time::seconds(tod.total_seconds())); // striping fractional seconds
}

inline boost::posix_time::time_duration strip_fractional_second(boost::posix_time::time_duration dur) {
    return boost::posix_time::seconds(dur.total_seconds());
}

template <typename Time>
inline std::string to_iso_string_no_fractional(Time t) {
    return boost::posix_time::to_iso_string(strip_fractional_second(t));
}

static const boost::posix_time::ptime posix_epoch(boost::gregorian::date(1970, boost::gregorian::Jan, 1));

inline uint32_t to_posix_timestamp(boost::posix_time::ptime ptime) {
    return (ptime - posix_epoch).total_seconds();//todo bound/overflow check ?
}

inline uint32_t to_posix_timestamp(DateTime datetime, const type::Data &d) {
    return to_posix_timestamp(to_posix_time(datetime, d));
}

// date are represented as time stamp to midnight
inline int32_t to_int_date(boost::posix_time::ptime ptime) {
    return (boost::posix_time::ptime(ptime.date()) - posix_epoch).total_seconds();//todo bound/overflow check ?
}

/**
 * weekday enum (because boost::date_time::weekdays start on sunday and we want monday to be the first day)
 */
enum weekdays {Monday = 0, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday};

/*
 * boost week day start from sunday to saturday
 * and we want a week day to start on monday
 * so here is the conversion
 *
 * */
template <>
struct enum_size_trait<boost::date_time::weekdays> {
    inline static constexpr typename get_enum_type<boost::date_time::weekdays>::type size() {
        return 7;
    }
};

inline const flat_enum_map<boost::date_time::weekdays, weekdays> build_conversion() {
    flat_enum_map<boost::date_time::weekdays, weekdays> map;
    map[boost::date_time::Monday] = weekdays::Monday;
    map[boost::date_time::Tuesday] = weekdays::Tuesday;
    map[boost::date_time::Wednesday] = weekdays::Wednesday;
    map[boost::date_time::Thursday] = weekdays::Thursday;
    map[boost::date_time::Friday] = weekdays::Friday;
    map[boost::date_time::Saturday] = weekdays::Saturday;
    map[boost::date_time::Sunday] = weekdays::Sunday;
    return map;
}

const auto weekday_conversion_ = build_conversion();

inline weekdays get_weekday(const boost::gregorian::date& date) {
    auto boost_week = date.day_of_week();
    return weekday_conversion_[boost_week];
}

}
