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

#include "type/type_interfaces.h"

#include <boost/serialization/split_member.hpp>

namespace navitia {
namespace type {

struct StopPoint;

enum class ConnectionType { StopPoint = 0, StopArea, Walking, VJ, Default, stay_in, undefined };

struct StopPointConnection : public Header, hasProperties {
    const static Type_e type = Type_e::Connection;
    StopPoint* departure;
    StopPoint* destination;
    int display_duration;
    int duration;
    int max_duration;
    ConnectionType connection_type;

    StopPointConnection()
        : departure(nullptr), destination(nullptr), display_duration(0), duration(0), max_duration(0) {}

    template <class Archive>
    void save(Archive& ar, const unsigned int) const;
    template <class Archive>
    void load(Archive& ar, const unsigned int);
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    Indexes get(Type_e type, const PT_Data& data) const;

    bool operator<(const StopPointConnection& other) const;
};

}  // namespace type
}  // namespace navitia
