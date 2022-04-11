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

#include "calendar_api.h"

#include "calendar.h"
#include "ptreferential/ptreferential.h"
#include "type/pb_converter.h"
#include "utils/paginate.h"

namespace navitia {
namespace calendar {

void calendars(navitia::PbCreator& pb_creator,
               const navitia::type::Data& d,
               const std::string& start_date,
               const std::string& end_date,
               const size_t depth,
               size_t count,
               size_t start_page,
               const std::string& filter,
               const std::vector<std::string>& forbidden_uris) {
    navitia::type::Indexes calendar_list;
    boost::gregorian::date start_period(boost::gregorian::not_a_date_time);
    boost::gregorian::date end_period(boost::gregorian::not_a_date_time);
    if ((!start_date.empty()) && (!end_date.empty())) {
        try {
            start_period = boost::gregorian::from_undelimited_string(start_date);
        } catch (const std::exception& e) {
            pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse,
                                     "Unable to parse start_date, " + std::string(e.what()));
            return;
        }

        try {
            end_period = boost::gregorian::from_undelimited_string(end_date);
        } catch (const std::exception& e) {
            pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse,
                                     "Unable to parse end_date, " + std::string(e.what()));
            return;
        }
    }
    auto action_period = boost::gregorian::date_period(start_period, end_period);

    try {
        Calendar calendar;
        calendar_list = calendar.get_calendars(filter, forbidden_uris, d, action_period);
    } catch (const ptref::parsing_error& parse_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse, parse_error.more);
        return;
    } catch (const ptref::ptref_error& ptref_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, ptref_error.more);
        return;
    }
    size_t total_result = calendar_list.size();
    calendar_list = paginate(calendar_list, count, start_page);

    pb_creator.pb_fill(pb_creator.data->get_data<nt::Calendar>(calendar_list), depth);

    pb_creator.make_paginate(total_result, start_page, count, pb_creator.calendars_size());
    if (pb_creator.calendars_size() == 0) {
        pb_creator.fill_pb_error(pbnavitia::Error::no_solution, pbnavitia::NO_SOLUTION,
                                 "no solution found for calendars API");
    }
}
}  // namespace calendar
}  // namespace navitia
