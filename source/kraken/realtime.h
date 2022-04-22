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

#include "type/data.h"
#include "type/gtfs-realtime.pb.h"

namespace navitia {

/**
 * WARNING: this method can add, remove or transform PT-Ref objects in PT_Data
 * After using it, make sure to rebuild:
 * - RAPTOR (probably through Data.build_raptor)
 * - AUTOCOMPLETE on PT-Ref (probably through PT_Data.build_autocomplete)
 */
void handle_realtime(const std::string& id,
                     const boost::posix_time::ptime& timestamp,
                     const transit_realtime::TripUpdate&,
                     const type::Data&,
                     const bool is_realtime_add_enabled = false,
                     const bool is_realtime_add_trip_enabled = false);
}  // namespace navitia
