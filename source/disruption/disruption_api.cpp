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

#include "disruption_api.h"
#include "type/pb_converter.h"
#include "ptreferential/ptreferential.h"
#include "disruption.h"

namespace navitia { namespace disruption {

pbnavitia::Response disruptions(const navitia::type::Data &d, const std::string &str_dt,
                                const size_t period,
                                const size_t depth,
                                size_t count,
                                size_t start_page, const std::string &filter,
                                const std::vector<std::string>& forbidden_uris){
    pbnavitia::Response pb_response;
    boost::posix_time::ptime now;
    try{
        now = boost::posix_time::from_iso_string(str_dt);
    }catch(boost::bad_lexical_cast){
           fill_pb_error(pbnavitia::Error::unable_to_parse,
                   "Unable to parse Datetime", pb_response.mutable_error());
           return pb_response;
    }

    auto period_end = boost::posix_time::ptime(now.date() + boost::gregorian::days(period),
                                               boost::posix_time::time_duration(23,59,59));
    auto action_period = boost::posix_time::time_period(now, period_end);
    Disruption result;
    try {
        result.disruptions_list(filter, forbidden_uris, d, action_period, now);
    } catch(const ptref::parsing_error &parse_error) {
        fill_pb_error(pbnavitia::Error::unable_to_parse,
                "Unable to parse filter" + parse_error.more, pb_response.mutable_error());
        return pb_response;
    } catch(const ptref::ptref_error &ptref_error){
        fill_pb_error(pbnavitia::Error::bad_filter,
                "ptref : "  + ptref_error.more, pb_response.mutable_error());
        return pb_response;
    }

    size_t total_result = result.get_disrupts_size();
    std::vector<disrupt> disrupts = paginate(result.get_disrupts(), count, start_page);
    for(disrupt dist: disrupts){
        pbnavitia::Disruptions* pb_disruption = pb_response.add_disruptions();
        pbnavitia::Network* pb_network = pb_disruption->mutable_network();
        navitia::fill_pb_object(d.pt_data->networks[dist.network_idx], d, pb_network, depth, now, action_period);
        for(type::idx_t idx : dist.line_idx){
            pbnavitia::Line* pb_line = pb_disruption->add_lines();
            navitia::fill_pb_object(d.pt_data->lines[idx], d, pb_line, depth-1, now, action_period);
        }
        for(type::idx_t idx : dist.stop_area_idx){
            pbnavitia::StopArea* pb_stop_area = pb_disruption->add_stop_areas();
            navitia::fill_pb_object(d.pt_data->stop_areas[idx], d, pb_stop_area, depth-1, now, action_period);
        }
    }
    auto pagination = pb_response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(pb_response.disruptions_size());

    if (pb_response.disruptions_size() == 0) {
        fill_pb_error(pbnavitia::Error::no_solution, "no solution found for this disruption",
        pb_response.mutable_error());
        pb_response.set_response_type(pbnavitia::NO_SOLUTION);
    }
    return pb_response;
}
}
}
