/* Copyright © 2001-2019, Canal TP and/or its affiliates. All rights reserved.

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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "speed_parser.h"
#include "utils/functions.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <algorithm>
#include "utils/exception.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/optional/optional_io.hpp>

namespace ed {
namespace connectors {

constexpr float kmh_to_ms(float val) {
    return val / 3.6;
}

template <typename T>
static void update_if_unknown(boost::optional<T>& target, const T& source) {
    if (!target) {
        target = source;
    }
}

// https://www.openstreetmap.org/relation/934933
// default values for France
SpeedsParser SpeedsParser::defaults() {
    const float walking_speed = kmh_to_ms(4);
    const float urban_speed = kmh_to_ms(50);
    const float rural_speed = kmh_to_ms(80);
    const float motorway_speed = kmh_to_ms(130);
    const float trunk_speed = kmh_to_ms(110);
    const float living_street_speed = kmh_to_ms(20);

    SpeedsParser parser;

    parser.implicit_speeds["fr:walk"] = walking_speed;
    parser.implicit_speeds["fr:urban"] = urban_speed;
    parser.implicit_speeds["fr:rural"] = rural_speed;
    parser.implicit_speeds["fr:motorway"] = motorway_speed;

    parser.max_speeds_by_type["motorway"] = motorway_speed;
    parser.max_speeds_by_type["motorway_link"] = urban_speed;
    parser.max_speeds_by_type["trunk"] = trunk_speed;
    parser.max_speeds_by_type["trunk_link"] = urban_speed;
    parser.max_speeds_by_type["primary"] = rural_speed;
    parser.max_speeds_by_type["secondary"] = urban_speed;
    parser.max_speeds_by_type["tertiary"] = urban_speed;
    parser.max_speeds_by_type["primary_link"] = urban_speed;
    parser.max_speeds_by_type["secondary_link"] = urban_speed;
    parser.max_speeds_by_type["tertiary_link"] = urban_speed;
    parser.max_speeds_by_type["unclassified"] = urban_speed;
    parser.max_speeds_by_type["road"] = urban_speed;
    parser.max_speeds_by_type["residential"] = urban_speed;
    parser.max_speeds_by_type["living_street"] = living_street_speed;
    parser.max_speeds_by_type["service"] = urban_speed;
    parser.max_speeds_by_type["track"] = walking_speed;
    parser.max_speeds_by_type["path"] = walking_speed;
    parser.max_speeds_by_type["unclassified"] = urban_speed;

    parser.speed_factor_by_type["motorway"] = 0.9;
    parser.speed_factor_by_type["motorway_link"] = 0.5;
    parser.speed_factor_by_type["trunk"] = 0.8;
    parser.speed_factor_by_type["trunk_link"] = 0.5;
    parser.speed_factor_by_type["primary"] = 0.8;
    parser.speed_factor_by_type["secondary"] = 0.6;
    parser.speed_factor_by_type["tertiary"] = 0.6;
    parser.speed_factor_by_type["primary_link"] = 0.5;
    parser.speed_factor_by_type["secondary_link"] = 0.5;
    parser.speed_factor_by_type["tertiary_link"] = 0.5;
    parser.speed_factor_by_type["unclassified"] = 0.5;
    parser.speed_factor_by_type["road"] = 0.5;
    parser.speed_factor_by_type["residential"] = 0.5;
    parser.speed_factor_by_type["living_street"] = 0.5;
    parser.speed_factor_by_type["service"] = 0.2;
    parser.speed_factor_by_type["track"] = 0.2;
    parser.speed_factor_by_type["path"] = 0.2;
    parser.speed_factor_by_type["unclassified"] = 0.5;

    return parser;
}

boost::optional<float> SpeedsParser::get_speed(const std::string& highway,
                                               const boost::optional<std::string>& max_speed) const {
    boost::optional<float> speed;
    boost::optional<float> factor;

    if(max_speeds_by_type.empty() && speed_factor_by_type.empty()){
        return boost::none;
    }

    if (max_speed) {
        std::string s = boost::algorithm::trim_copy(max_speed.get());
        auto it = implicit_speeds.find(s);
        if (it != implicit_speeds.end()) {
            speed = it->second;
        } else {
            try {
                // todo handle value not in km/h.
                // if there is no unit the value is in km/h else cast will fail
                speed = kmh_to_ms(boost::lexical_cast<float>(s));
            } catch (const std::exception&) {
                std::cerr << "impossible to convert \"" << max_speed << "\" in speed" << std::endl;
            }
        }
    }

    auto it = max_speeds_by_type.find(highway);
    if (it != max_speeds_by_type.end()) {
        update_if_unknown(speed, it->second);
    } else {
        std::cerr << highway << " not handled" << std::endl;
    }

    auto it_factor = speed_factor_by_type.find(highway);
    if (it_factor != speed_factor_by_type.end() && speed) {
        speed = speed.get() * it_factor->second;
    } else {
        std::cerr << highway << " not handled" << std::endl;
    }
    return speed;
}

}  // namespace connectors
}  // namespace ed
