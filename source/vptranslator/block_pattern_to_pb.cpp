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

#include "block_pattern_to_pb.h"
#include "type/pb_converter.h"

namespace navitia {
namespace vptranslator {

void fill_pb_object(const BlockPattern& bp,
                    const type::Data& data,
                    pbnavitia::Calendar* pb_cal,
                    int max_depth,
                    const boost::posix_time::ptime& now,
                    const boost::posix_time::time_period& action_period) {
    auto week = pb_cal->mutable_week_pattern();
    week->set_monday(bp.week[navitia::Monday]);
    week->set_tuesday(bp.week[navitia::Tuesday]);
    week->set_wednesday(bp.week[navitia::Wednesday]);
    week->set_thursday(bp.week[navitia::Thursday]);
    week->set_friday(bp.week[navitia::Friday]);
    week->set_saturday(bp.week[navitia::Saturday]);
    week->set_sunday(bp.week[navitia::Sunday]);

    for (const auto& p: bp.validity_periods) {
        auto pb_period = pb_cal->add_active_periods();
        pb_period->set_begin(boost::gregorian::to_iso_string(p.begin()));
        pb_period->set_end(boost::gregorian::to_iso_string(p.end()));
    }

    for (const auto& excep: bp.exceptions) {
        ::navitia::fill_pb_object(excep, data, pb_cal->add_exceptions(), max_depth, now, action_period);
    }

    // TODO
}

} // namespace vptranslator
} // namespace navitia
