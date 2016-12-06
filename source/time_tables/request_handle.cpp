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

RequestHandle::RequestHandle(PbCreator& pb_creator,
                             const std::string &request,
                             const std::vector<std::string>& forbidden_uris,
                             const pt::ptime datetime, uint32_t duration,
                             boost::optional<const std::string> calendar_id,
                             const bool clockwise) :
    pb_creator(pb_creator),
    date_time(DateTimeUtils::inf), max_datetime(DateTimeUtils::inf){    
    if (! calendar_id) {
        //we only have to check the production period if we do not have a calendar,
        // since if we have one we are only interested in the time, not the date
        if(! pb_creator.data->meta->production_date.contains(datetime.date()) ) {
            pb_creator.fill_pb_error(pbnavitia::Error::date_out_of_bounds, "date is out of bound");
        } else if( !pb_creator.data->meta->production_date.contains((datetime + boost::posix_time::seconds(duration)).date()) ) {
             // On regarde si la date + duration ne déborde pas de la période de production
            pb_creator.fill_pb_error(pbnavitia::Error::date_out_of_bounds,
                                     "date is not in data production period");
        }
    }

    if(! pb_creator.has_error()){
        date_time = DateTimeUtils::set((datetime.date() - pb_creator.data->meta->production_date.begin()).days(),
                                       datetime.time_of_day().total_seconds());
        max_datetime = date_time + duration * (clockwise ? 1 : -1);
        const auto jpp_t = type::Type_e::JourneyPatternPoint;

        try {
            const auto& jpps = ptref::make_query(jpp_t, request, forbidden_uris, *pb_creator.data);
            for (const auto idx: jpps) { journey_pattern_points.push_back(routing::JppIdx(idx)); }
            total_result = journey_pattern_points.size();
        } catch(const ptref::ptref_error &ptref_error){
            pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : "  + ptref_error.more);
        }
    }

}

}}
