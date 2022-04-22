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
#include "utils/exception.h"

#include <boost/algorithm/string.hpp>

#include <map>
#include <ostream>

namespace navitia {

namespace type {
enum class RTLevel : char {
    Base = 0,  // base schedule (theoretical, from the GTFS)
    Adapted,   // adapted schedule (planned maintenance, strike, ...)
    RealTime
};

inline RTLevel get_rt_level_from_string(const std::string& level_str) {
    auto lower_str = boost::algorithm::to_lower_copy(level_str);
    if (lower_str == "theoric") {
        return RTLevel::Base;
    }
    if (lower_str == "adapted") {
        return RTLevel::Adapted;
    }
    if (lower_str == "realtime") {
        return RTLevel::RealTime;
    }
    throw navitia::exception("technical error, vj class for meta vj should be either Theoric, Adapted or RealTime");
}

inline std::string get_string_from_rt_level(RTLevel level) {
    if (level == RTLevel::Base) {
        return "Theoric";
    }
    if (level == RTLevel::Adapted) {
        return "Adapted";
    }
    if (level == RTLevel::RealTime) {
        return "RealTime";
    }
    throw navitia::exception("technical error, vj class for meta vj should be either Base, Adapted or RealTime");
}
}  // namespace type

template <>
struct enum_size_trait<type::RTLevel> {
    static constexpr typename get_enum_type<type::RTLevel>::type size() { return 3; }
};

}  // namespace navitia

inline std::ostream& operator<<(std::ostream& ss, navitia::type::RTLevel l) {
    switch (l) {
        case navitia::type::RTLevel::Base:
            return ss << "Base";
        case navitia::type::RTLevel::Adapted:
            return ss << "Adapted";
        case navitia::type::RTLevel::RealTime:
            return ss << "RealTime";
        default:
            return ss;
    }
}
