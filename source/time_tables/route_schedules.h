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

#include "routing/routing.h"
#include "routing/get_stop_times.h"
#include "type/pb_converter.h"

namespace navitia {
namespace timetables {

using vector_string = std::vector<std::string>;
using vector_date_time = std::pair<DateTime, const type::StopTime*>;

std::vector<std::vector<routing::datetime_stop_time> > get_all_route_stop_times(
    const navitia::type::Route* route,
    const DateTime& date_time,
    const DateTime& max_datetime,
    const size_t max_stop_date_times,
    const type::Data& d,
    const type::RTLevel rt_level,
    const boost::optional<const std::string>& calendar_id);

void route_schedule(PbCreator& pb_creator,
                    const std::string& filter,
                    const boost::optional<const std::string>& calendar_id,
                    const std::vector<std::string>& forbidden_uris,
                    const boost::posix_time::ptime datetime,
                    uint32_t duration,
                    size_t max_stop_date_times,
                    const uint32_t max_depth,
                    int count,
                    int start_page,
                    const type::RTLevel rt_level);

}  // namespace timetables
}  // namespace navitia
