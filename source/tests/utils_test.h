#pragma once

#include "type/datetime.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
/*
 * Utilities for tests
 */
namespace navitia {
namespace test {

inline uint64_t to_posix_timestamp(const std::string& str) {
    return navitia::to_posix_timestamp(boost::posix_time::from_iso_string(str));
}

}
}

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
 * build it like 7_day
 */
inline boost::gregorian::days operator"" _day(unsigned long long v) {
    return boost::gregorian::days(v);
}

/*
 * enable simple duration construction (in seconds)
 *
 * build it like 12_s => creates a boost::time_duration of 12 seconds
 */
inline navitia::time_duration operator"" _s(unsigned long long v) {
    return navitia::seconds(v);
}
