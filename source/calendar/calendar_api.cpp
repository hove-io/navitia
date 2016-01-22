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

#include "calendar_api.h"
#include "type/pb_converter.h"
#include "calendar.h"
#include "ptreferential/ptreferential.h"

namespace navitia { namespace calendar {

pbnavitia::Response calendars(const navitia::type::Data &d,
    const std::string &start_date,
    const std::string &end_date,
    const size_t depth,
    size_t count,
    size_t start_page,
    const std::string &filter,
    const std::vector<std::string>& forbidden_uris) {
    pbnavitia::Response pb_response;
    std::vector<type::idx_t> calendar_list;
    auto now = boost::posix_time::second_clock::universal_time();
    boost::gregorian::date start_period(boost::gregorian::not_a_date_time);
    boost::gregorian::date end_period(boost::gregorian::not_a_date_time);
    if((!start_date.empty()) && (!end_date.empty())) {
        try{
            start_period = boost::gregorian::from_undelimited_string(start_date);
        }catch(const std::exception& e) {
           fill_pb_error(pbnavitia::Error::unable_to_parse,
                         "Unable to parse start_date, "+ std::string(e.what()), pb_response.mutable_error());
           return pb_response;
        }

        try{
            end_period = boost::gregorian::from_undelimited_string(end_date);
        }catch(const std::exception& e) {
           fill_pb_error(pbnavitia::Error::unable_to_parse,
                   "Unable to parse end_date, "+ std::string(e.what()), pb_response.mutable_error());
           return pb_response;
        }
    }
    auto period = boost::gregorian::date_period(start_period, end_period);

    try{
        Calendar calendar;
        calendar_list = calendar.get_calendars(filter, forbidden_uris, d, period);
    } catch(const ptref::parsing_error &parse_error) {
        fill_pb_error(pbnavitia::Error::unable_to_parse, parse_error.more, pb_response.mutable_error());
        return pb_response;
    } catch(const ptref::ptref_error &ptref_error){
        fill_pb_error(pbnavitia::Error::bad_filter, ptref_error.more, pb_response.mutable_error());
        return pb_response;
    }
    size_t total_result = calendar_list.size();
    calendar_list = paginate(calendar_list, count, start_page);

    for(type::idx_t cal_idx : calendar_list) {
        type::Calendar* cal = d.pt_data->calendars[cal_idx];
        navitia::fill_pb_object(cal, d,pb_response.add_calendars(), depth, now);
    }

    auto pagination = pb_response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(pb_response.calendars_size());

    if (pb_response.calendars_size() == 0) {
        fill_pb_error(pbnavitia::Error::no_solution, "no solution found for calendars API",
        pb_response.mutable_error());
        pb_response.set_response_type(pbnavitia::NO_SOLUTION);
    }
    return pb_response;
}
}
}
