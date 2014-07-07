#pragma once

#include "type/datetime.h"
/*
 * Utilities for tests
 */
namespace navitia {
namespace test {

inline uint32_t to_posix_timestamp(const std::string& str) {
    return navitia::to_posix_timestamp(boost::posix_time::from_iso_string(str));
}

}
}
