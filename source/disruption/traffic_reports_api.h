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

#include "type/response.pb.h"
#include "type/pb_converter.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace navitia {
namespace disruption {

void traffic_reports(navitia::PbCreator& pb_creator,
                     const navitia::type::Data& d,
                     const size_t depth,
                     size_t count,
                     size_t start_page,
                     const std::string& filter,
                     const std::vector<std::string>& forbidden_uris,
                     const boost::optional<boost::posix_time::ptime>& since,
                     const boost::optional<boost::posix_time::ptime>& until);
}
}  // namespace navitia
