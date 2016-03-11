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
#include "type/response.pb.h"
#include "type/request.pb.h"
#include "type/pt_data.h"

namespace navitia {

namespace type {
    class Data;
    enum class Type_e;
}

namespace autocomplete {

/** Trouve tous les objets définis par filter dont le nom contient q */
pbnavitia::Response autocomplete(const std::string &q,
                                 const std::vector<navitia::type::Type_e> &filter,
                                 uint32_t depth,
                                 int nbmax,
                                 const std::vector <std::string> &admins,
                                 int search_type,
                                 const type::Data &d,
                                 const boost::posix_time::ptime& current_datetime);
}
}
