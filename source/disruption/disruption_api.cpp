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

namespace bt = boost::posix_time;

namespace navitia { namespace disruption {

pbnavitia::Response traffic_reports(const navitia::type::Data& d,
                                uint64_t posix_now_dt,
                                const size_t depth,
                                size_t count,
                                size_t start_page,
                                const std::string& filter,
                                const std::vector<std::string>& forbidden_uris) {
    pbnavitia::Response pb_response;

    bt::ptime now_dt = bt::from_time_t(posix_now_dt);
    auto action_period = bt::time_period(now_dt, bt::seconds(1));

    Disruption result;
    try {
        result.disruptions_list(filter, forbidden_uris, d, now_dt);
    } catch(const ptref::parsing_error& parse_error) {
        fill_pb_error(pbnavitia::Error::unable_to_parse,
                "Unable to parse filter" + parse_error.more, pb_response.mutable_error());
        return pb_response;
    } catch(const ptref::ptref_error& ptref_error) {
        fill_pb_error(pbnavitia::Error::bad_filter,
                "ptref : "  + ptref_error.more, pb_response.mutable_error());
        return pb_response;
    }

    size_t total_result = result.get_disrupts_size();
    std::vector<Disrupt> disrupts = paginate(result.get_disrupts(), count, start_page);
    for (const Disrupt& dist: disrupts) {
        pbnavitia::Disruptions* pb_disruption = pb_response.add_disruptions();
        pbnavitia::Network* pb_network = pb_disruption->mutable_network();
        for(const auto& impact: dist.network_disruptions){
            fill_message(*impact, d, pb_network, depth-1, now_dt, action_period);
        }
        navitia::fill_pb_object(dist.network, d, pb_network, depth, bt::not_a_date_time, action_period, false);
        for (const auto& line_item: dist.lines) {
            pbnavitia::Line* pb_line = pb_disruption->add_lines();
            navitia::fill_pb_object(line_item.first, d, pb_line, depth-1, bt::not_a_date_time, action_period, false);
            for(const auto& impact: line_item.second){
                fill_message(*impact, d, pb_line, depth-1, now_dt, action_period);
            }
        }
        for (const auto& sa_item: dist.stop_areas) {
            pbnavitia::StopArea* pb_stop_area = pb_disruption->add_stop_areas();
            navitia::fill_pb_object(sa_item.first, d, pb_stop_area, depth-1, bt::not_a_date_time, action_period, false);
            for(const auto& impact: sa_item.second){
                fill_message(*impact, d, pb_stop_area, depth-1, now_dt, action_period);
            }
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

}}//namespace
