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

#include "type/message.h"
#include "type/pt_data.h"
#include "utils/logger.h"

#include <boost/format.hpp>

namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia { namespace type { namespace new_disruption {

bool Impact::is_valid(const boost::posix_time::ptime& publication_date, const boost::posix_time::time_period& active_period) const {

    if(publication_date.is_not_a_date_time() || active_period.is_null()){
        return false;
    }

    // we check if we want to publish the impact
    if (! disruption->publication_period.contains(publication_date)) {
        return false;
    }

    //we check if the impact applies on this period
    for (const auto& period: application_periods) {
        if (! period.intersection(active_period).is_null()) {
            return true;
        }
    }
    return false;
}

namespace {
template<typename T>
PtObj transform_pt_object(const std::string& uri,
                          const std::map<std::string, T*>& map,
                          const boost::shared_ptr<Impact>& impact) {
    if (auto o = find_or_default(uri, map)) {
        if (impact) o->add_impact(impact);
        return o;
    } else {
        return UnknownPtObj();
    }
}
}

PtObj make_pt_obj(Type_e type,
                  const std::string& uri,
                  const PT_Data& pt_data,
                  const boost::shared_ptr<Impact>& impact) {
    switch (type) {
    case Type_e::Network: return transform_pt_object(uri, pt_data.networks_map, impact);
    case Type_e::StopArea: return transform_pt_object(uri, pt_data.stop_areas_map, impact);
    case Type_e::Line: return transform_pt_object(uri, pt_data.lines_map, impact);
    case Type_e::Route: return transform_pt_object(uri, pt_data.routes_map, impact);
    default: return UnknownPtObj();
    }
}

}}}//namespace
