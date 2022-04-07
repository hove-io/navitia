/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "utils/flat_enum_map.h"
#include "time_duration.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_config.hpp>

#include <cstdint>
#include <string>
#include <limits>
#include <fstream>
#include <bitset>

namespace boost {
namespace posix_time {
class ptime;
class time_duration;
}  // namespace posix_time
}  // namespace boost

namespace navitia {
typedef uint32_t DateTime;

namespace DateTimeUtils {

const uint32_t SECONDS_PER_DAY = 86400;

constexpr navitia::DateTime inf = std::numeric_limits<uint32_t>::max();

// sometimes we need a not valid datetime which is not semanticaly the max, so we use a different name
constexpr navitia::DateTime not_valid = std::numeric_limits<uint32_t>::max();

constexpr navitia::DateTime min = 0;

inline navitia::DateTime set(int date, int time_of_day) {
    return date * SECONDS_PER_DAY + time_of_day;
}

inline uint32_t hour(const navitia::DateTime datetime) {
    return datetime % SECONDS_PER_DAY;
}

inline uint32_t date(const navitia::DateTime datetime) {
    return datetime / SECONDS_PER_DAY;
}

// return a DateTime shifted in the future (resp past for ! clockwise)
// from the datetime dt. the new hour is hh.
// handle overmidnight cases
inline DateTime shift(navitia::DateTime dt, uint32_t hh, bool clockwise = true) {
    uint32_t this_hour = hour(dt);
    if (hh >= SECONDS_PER_DAY)
        hh -= SECONDS_PER_DAY;
    if (clockwise) {
        dt += (hh >= this_hour ? 0 : SECONDS_PER_DAY) + hh - this_hour;
    } else {
        if (hh <= this_hour)
            dt += hh - this_hour;
        else {
            if (date(dt) > 0)
                dt += hh - this_hour - SECONDS_PER_DAY;
            else
                dt = 0;
        }
    }
    return dt;
}

// from a number of seconds from midnight (can be negative), return the hour in [0; SECONDS_PER_DAY]
inline u_int32_t hour_in_day(int hour) {
    while (hour < 0) {
        hour += SECONDS_PER_DAY;
    }
    return hour % SECONDS_PER_DAY;
}

}  // namespace DateTimeUtils
namespace type {
class Data;
}

// std::ostream & operator<<(std::ostream & os, const DateTime &dt);
std::string str(const DateTime& dt);
std::string iso_string(const DateTime datetime, const type::Data& d);
std::string iso_hour_string(const DateTime datetime, const type::Data& d);

boost::posix_time::ptime to_posix_time(DateTime datetime, const type::Data& d);
DateTime to_datetime(boost::posix_time::ptime ptime, const type::Data& d);

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
    return {ptime.date(), boost::posix_time::seconds(tod.total_seconds())};  // striping fractional seconds
}

inline boost::posix_time::time_duration strip_fractional_second(boost::posix_time::time_duration dur) {
    return boost::posix_time::seconds(dur.total_seconds());
}

template <typename Time>
inline std::string to_iso_string_no_fractional(Time t) {
    return boost::posix_time::to_iso_string(strip_fractional_second(t));
}

static const boost::posix_time::ptime posix_epoch(boost::gregorian::date(1970, boost::gregorian::Jan, 1));

inline uint64_t to_posix_timestamp(boost::posix_time::ptime ptime) {
    if (ptime.is_not_a_date_time()) {
        return 0;
    }
    return (ptime - posix_epoch).total_seconds();
}

inline uint64_t to_posix_timestamp(DateTime datetime, const type::Data& d) {
    return to_posix_timestamp(to_posix_time(datetime, d));
}

boost::posix_time::ptime from_posix_timestamp(uint64_t val);

// date are represented as time stamp to midnight
inline int32_t to_int_date(boost::posix_time::ptime ptime) {
    return (boost::posix_time::ptime(ptime.date()) - posix_epoch).total_seconds();  // todo bound/overflow check ?
}

inline int32_t to_int_date(boost::gregorian::date date) {
    return (boost::posix_time::ptime(date) - posix_epoch).total_seconds();  // todo bound/overflow check ?
}

/**
 * weekday enum (because boost::date_time::weekdays start on sunday and we want monday to be the first day)
 */
enum weekdays { Monday = 0, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday };

/*
 * boost week day start from sunday to saturday
 * and we want a week day to start on monday
 * so here is the conversion
 *
 * */
template <>
struct enum_size_trait<boost::date_time::weekdays> {
    inline static constexpr typename get_enum_type<boost::date_time::weekdays>::type size() { return 7; }
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

/*
 * Split a period starting from 'start' to 'end'
 *
 * the period is split for each day, each day starting from 'beg_of_day' to 'end_of_day'
 */
std::vector<boost::posix_time::time_period> expand_calendar(boost::posix_time::ptime start,
                                                            boost::posix_time::ptime end,
                                                            const boost::posix_time::time_duration& beg_of_day,
                                                            const boost::posix_time::time_duration& end_of_day,
                                                            std::bitset<7> days);
}  // namespace navitia

/*
 * enable simple date construction
 *
 * build it like "20140101"_d
 */
inline boost::gregorian::date operator"" _d(const char* str, size_t s) {
    return boost::gregorian::from_undelimited_string(std::string(str, s));
}

/*
 * enable simple posix time construction
 *
 * build it like "20140101T120000"_dt
 */
inline boost::posix_time::ptime operator"" _dt(const char* str, size_t s) {
    return boost::posix_time::from_iso_string(std::string(str, s));
}

/*
 * enable simple duration construction (in days)
 *
 * build it like 7_days
 */
inline boost::gregorian::days operator"" _days(unsigned long long v) {
    return boost::gregorian::days(v);
}

/*
 * enable simple duration construction (in seconds)
 *
 * build it like 12_s => creates a navitia::time_duration of 12 seconds
 */
inline navitia::time_duration operator"" _s(unsigned long long v) {
    return navitia::seconds(v);
}

/*
 * enable simple duration construction (in minutes)
 *
 * build it like 12_min => creates a navitia::time_duration of 12 minutes
 */
inline navitia::time_duration operator"" _min(unsigned long long v) {
    return navitia::minutes(v);
}

/*
 * enable simple duration construction (in hours)
 *
 * build it like 12_h => creates a navitia::time_duration of 12 hours
 */
inline navitia::time_duration operator"" _h(unsigned long long v) {
    return navitia::hours(v);
}
