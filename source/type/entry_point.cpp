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
#include "entry_point.h"

#include "utils/coord_parser.h"
#include "utils/functions.h"
#include "utils/logger.h"

namespace navitia {
namespace type {

EntryPoint::EntryPoint(const Type_e type, const std::string& uri, int access_duration)
    : type(type), uri(uri), access_duration(access_duration) {
    if (type == Type_e::Address) {
        auto vect = split_string(uri, ":");
        if (vect.size() == 3) {
            this->uri = vect[0] + ":" + vect[1];
            this->house_number = str_to_int(vect[2]);
        }
    }
    if (type == Type_e::Coord) {
        try {
            const auto coord = navitia::parse_coordinate(uri);
            this->coordinates.set_lon(coord.first);
            this->coordinates.set_lat(coord.second);
        } catch (const navitia::wrong_coordinate&) {
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("logger"),
                           "uri " << uri << " partialy match coordinate, cannot work");
            this->coordinates.set_lon(0);
            this->coordinates.set_lat(0);
        }
    }
}

bool EntryPoint::is_coord(const std::string& uri) {
    return (uri.size() > 6 && uri.substr(0, 6) == "coord:")
           || (boost::regex_match(uri, navitia::coord_regex) && uri != ";");
}

EntryPoint::EntryPoint(const Type_e type, const std::string& uri) : EntryPoint(type, uri, 0) {}

void StreetNetworkParams::set_filter(const std::string& param_uri) {
    size_t pos = param_uri.find(':');
    if (pos == std::string::npos) {
        type_filter = Type_e::Unknown;
    } else {
        uri_filter = param_uri;
        type_filter = navitia::type::static_data::get()->typeByCaption(param_uri.substr(0, pos));
    }
}

}  // namespace type
}  // namespace navitia
