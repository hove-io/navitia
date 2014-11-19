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
