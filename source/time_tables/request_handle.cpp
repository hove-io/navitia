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

#include "request_handle.h"
#include "ptreferential/ptreferential.h"
#include "type/meta_data.h"

namespace navitia { namespace timetables {

RequestHandle::RequestHandle(const std::string& /*api*/, const std::string &request,
                             const std::vector<std::string>& forbidden_uris,
                             const std::string &str_dt, uint32_t duration,
                             const type::Data &data,
                             boost::optional<const std::string> calendar_id) :
    date_time(DateTimeUtils::inf), max_datetime(DateTimeUtils::inf){
    try {
        auto ptime = boost::posix_time::from_iso_string(str_dt);

        if (! calendar_id) {
            //we only have to check the production period if we do not have a calendar,
            // since if we have one we are only interested in the time, not the date
            if(! data.meta->production_date.contains(ptime.date()) ) {
                fill_pb_error(pbnavitia::Error::date_out_of_bounds, "date is out of bound",pb_response.mutable_error());
            } else if( !data.meta->production_date.contains((ptime + boost::posix_time::seconds(duration)).date()) ) {
                 // On regarde si la date + duration ne déborde pas de la période de production
                fill_pb_error(pbnavitia::Error::date_out_of_bounds, "date is not in data production period",pb_response.mutable_error());
            }
        }
        if(! pb_response.has_error()){
            date_time = DateTimeUtils::set((ptime.date() - data.meta->production_date.begin()).days(), ptime.time_of_day().total_seconds());
            max_datetime = date_time + duration;
            const auto jpp_t = type::Type_e::JourneyPatternPoint;
            journey_pattern_points = ptref::make_query(jpp_t, request, forbidden_uris, data);
            total_result = journey_pattern_points.size();
        }
    } catch(const ptref::parsing_error &parse_error) {
        fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse Datetime" + parse_error.more,pb_response.mutable_error());
    } catch(const ptref::ptref_error &ptref_error){
        fill_pb_error(pbnavitia::Error::bad_filter, "ptref : "  + ptref_error.more,pb_response.mutable_error());
    } catch(...) {
        fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse Datetime",pb_response.mutable_error());
  }
}

}}
