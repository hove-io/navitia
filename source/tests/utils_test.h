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

inline u_int32_t operator"" _t(const char* str, size_t s) {
    return boost::posix_time::duration_from_string(std::string(str, s)).total_seconds();
}

inline u_int64_t operator"" _pts(const char* str, size_t s) {
    return navitia::to_posix_timestamp(boost::posix_time::from_iso_string(std::string(str, s)));
}

namespace std {
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    os << "{";
    auto it = s.cbegin(), end = s.cend();
    if (it != end) os << *it++;
    for (; it != end; ++it) os << ", " << *it;
    return os << "}";
}
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[";
    auto it = v.cbegin(), end = v.cend();
    if (it != end) { os << *it++; }
    for (; it != end; ++it) { os << ", " << *it; }
    return os << "]";
}

template<typename T, typename U>
std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& p) {
    return os << "(" << p.first << ", " << p.second << ")";
}


template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m) {
    os << "{";
    auto it = m.cbegin(), end = m.cend();
    if (it != end) { os << it++; }
    for (; it != end; ++it) { os << ", " << *it; }
    return os << "}";
}
}
