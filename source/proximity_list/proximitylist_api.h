/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "type/type.pb.h"
#include "type/request.pb.h"
#include "type/response.pb.h"
#include "type/pt_data.h"

namespace navitia {

// Forward declare
namespace type {
class Data;
enum class Type_e;
struct GeographicalCoord;
typedef uint32_t idx_t;
}

namespace proximitylist {

typedef std::vector<std::pair<type::idx_t, type::GeographicalCoord>> vector_idx_coord;
pbnavitia::Response find(const type::GeographicalCoord& coord, const double distance,
                         const std::vector<type::Type_e>& types, const std::string& filter,
                         const uint32_t depth, const uint32_t count, const uint32_t start_page,
                         const type::Data& data, const boost::posix_time::ptime& current_time);
}} // namespace navitia::proximitylist
