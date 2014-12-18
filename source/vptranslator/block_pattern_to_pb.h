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

#include "vptranslator/vptranslator.h"

#include "boost/date_time/posix_time/posix_time.hpp"

namespace pbnavitia { class Calendar; }

namespace navitia {
namespace vptranslator {

#define null_time_period boost::posix_time::time_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0))

template<typename T>
void fill_pb_object(const std::vector<BlockPattern>& bps,
                    const type::Data& data,
                    T* o,
                    int max_depth = 0 ,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period  = null_time_period) {
    for (const auto& bp: bps) {
        fill_pb_object(bp, data, o->add_calendars(), max_depth, now, action_period);
    }
}

void fill_pb_object(const BlockPattern&,
                    const type::Data& data,
                    pbnavitia::Calendar*,
                    int max_depth = 0 ,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period  = null_time_period);

} // namespace vptranslator
} // namespace navitia
