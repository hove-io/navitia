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
#include "type/type.pb.h"
#include "type/request.pb.h"
#include "type/response.pb.h"
#include "type/pt_data.h"
#include "type/pb_converter.h"

namespace navitia {

// Forward declare
namespace type {
class Data;
enum class Type_e;
struct GeographicalCoord;
using idx_t = uint32_t;
}  // namespace type

namespace proximitylist {

void find(navitia::PbCreator& pb_creator,
          const type::GeographicalCoord& coord,
          const double distance,
          const std::vector<type::Type_e>& types,
          const std::vector<std::string>& forbidden_uris,
          const std::string& filter,
          const uint32_t depth,
          const uint32_t count,
          const uint32_t start_page,
          const type::Data& data,
          const double stop_points_nearby_radius = 0);
}  // namespace proximitylist
}  // namespace navitia
