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

#include "calendar.h"

#include "ptreferential/ptreferential.h"
#include "type/data.h"

namespace navitia {
namespace calendar {

type::Indexes Calendar::get_calendars(const std::string& filter,
                                      const std::vector<std::string>& forbidden_uris,
                                      const type::Data& d,
                                      const boost::gregorian::date_period filter_period) {
    type::Indexes to_return;
    to_return = ptref::make_query(type::Type_e::Calendar, filter, forbidden_uris, d);
    if (to_return.empty() || (filter_period.begin().is_not_a_date())) {
        return to_return;
    }
    type::Indexes indexes;
    for (type::idx_t idx : to_return) {
        navitia::type::Calendar* cal = d.pt_data->calendars[idx];
        for (const boost::gregorian::date_period& per : cal->active_periods) {
            if (filter_period.begin() == filter_period.end()) {
                if (per.contains(filter_period.begin())) {
                    indexes.insert(cal->idx);
                    break;
                }
            } else {
                if (filter_period.intersects(per)) {
                    indexes.insert(cal->idx);
                    break;
                }
            }
        }
    }
    return indexes;
}
}  // namespace calendar
}  // namespace navitia
