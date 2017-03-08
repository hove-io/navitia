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

#include "type/type.h"
#include "type/type.pb.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace navitia {

const type::StopTime* get_base_stop_time(const type::StopTime* st_orig);

/**
 * Compute base passage from amended passage, knowing amended and base stop-times
 */
boost::posix_time::ptime get_base_dt(const type::StopTime* st_orig, const type::StopTime* st_base,
                             const boost::posix_time::ptime& dt_orig, bool is_departure);

const type::StopTime& earliest_stop_time(const std::vector<type::StopTime>& sts);

pbnavitia::RTLevel to_pb_realtime_level(const navitia::type::RTLevel realtime_level);

}
