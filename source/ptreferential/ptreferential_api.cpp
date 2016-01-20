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

#include "ptreferential.h"
#include "ptreferential_api.h"
#include "type/pb_converter.h"
#include "type/data.h"
#include "type/pt_data.h"
#include "type/meta_data.h"
#include "routing/dataraptor.h"

namespace pt = boost::posix_time;
namespace pc = ProtoCreator;

namespace navitia{ namespace ptref{

static pbnavitia::Response extract_data(const type::Data& data,
                                        const type::Type_e requested_type,
                                        const std::vector<type::idx_t>& rows,
                                        const int depth,
                                        const bool show_codes,
                                        const boost::posix_time::ptime& current_time) {
    pbnavitia::Response result;
    const auto& action_period = data.meta->production_period();
    for(auto idx : rows){
        switch(requested_type){
        case Type_e::ValidityPattern:
            pc::fill_pb_object(data.pt_data->validity_patterns[idx], data,
                           result.add_validity_patterns(), depth, current_time, action_period);
            break;
        case Type_e::Line:
            pc::fill_pb_object(data.pt_data->lines[idx], data, result.add_lines(),
                           depth, current_time, action_period, show_codes);
            break;
        case Type_e::LineGroup:
            pc::fill_pb_object(data.pt_data->line_groups[idx], data, result.add_line_groups(),
                           depth, current_time, action_period, show_codes);
            break;
        case Type_e::JourneyPattern:{
            const auto& pair_jp = data.dataRaptor->jp_container.get_jps()[idx];
            pc::fill_pb_object(&pair_jp, data,
                           result.add_journey_patterns(), depth, current_time, action_period);
            break;
        }
        case Type_e::StopPoint:
            pc::fill_pb_object(data.pt_data->stop_points[idx], data,
                           result.add_stop_points(), depth, current_time, action_period, show_codes);
            break;
        case Type_e::StopArea:
            pc::fill_pb_object(data.pt_data->stop_areas[idx], data,
                           result.add_stop_areas(), depth, current_time, action_period, show_codes);
            break;
        case Type_e::Network:
            pc::fill_pb_object(data.pt_data->networks[idx], data,
                           result.add_networks(), depth, current_time, action_period, show_codes);
            break;
        case Type_e::PhysicalMode:
            pc::fill_pb_object(data.pt_data->physical_modes[idx], data,
                           result.add_physical_modes(), depth, current_time, action_period);
            break;
        case Type_e::CommercialMode:
            pc::fill_pb_object(data.pt_data->commercial_modes[idx], data,
                           result.add_commercial_modes(), depth, current_time, action_period);
            break;
        case Type_e::JourneyPatternPoint:{
            const auto& pair_jppp = data.dataRaptor->jp_container.get_jpps()[idx];
            pc::fill_pb_object(&pair_jppp, data,
                           result.add_journey_pattern_points(), depth, current_time, action_period);
            break;
        }

        case Type_e::Company:
            pc::fill_pb_object(data.pt_data->companies[idx], data,
                           result.add_companies(), depth, current_time, action_period);
            break;
        case Type_e::Route:
            pc::fill_pb_object(data.pt_data->routes[idx], data,
                    result.add_routes(), depth, current_time, action_period, show_codes);
            break;
        case Type_e::POI:
            pc::fill_pb_object(data.geo_ref->pois[idx], data,
                           result.add_pois(), depth, current_time, action_period);
            break;
        case Type_e::POIType:
            pc::fill_pb_object(data.geo_ref->poitypes[idx], data,
                           result.add_poi_types(), depth, current_time, action_period);
            break;
        case Type_e::Connection:
            pc::fill_pb_object(data.pt_data->stop_point_connections[idx], data,
                           result.add_connections(), depth, current_time, action_period);
            break;
        case Type_e::VehicleJourney:
            pc::fill_pb_object(data.pt_data->vehicle_journeys[idx], data,
                           result.add_vehicle_journeys(), depth, current_time, action_period, show_codes);
            break;
        case Type_e::Calendar:
            pc::fill_pb_object(data.pt_data->calendars[idx], data,
                           result.add_calendars(), depth, current_time, action_period, show_codes);
            break;
        case Type_e::MetaVehicleJourney:
            pc::fill_pb_object(data.pt_data->meta_vjs[Idx<type::MetaVehicleJourney>(idx)], data,
                           result.add_trips(), depth, current_time, action_period, show_codes);
            break;
        case Type_e::Impact:{
            auto impact = data.pt_data->disruption_holder.get_weak_impacts()[idx].lock();
            pc::fill_pb_object(impact.get(), data, &result, depth, current_time, action_period, show_codes);
            break;
        }
        case Type_e::Contributor:
            pc::fill_pb_object(data.pt_data->contributors[idx], data,
                           result.add_contributors(), depth, current_time, action_period);
            break;
        case Type_e::Frame:
            pc::fill_pb_object(data.pt_data->frames[idx], data,
                           result.add_frames(), depth, current_time, action_period);
            break;
        default: break;
        }
    }
    return result;
}


pbnavitia::Response query_pb(const type::Type_e requested_type,
                             const std::string& request,
                             const std::vector<std::string>& forbidden_uris,
                             const type::OdtLevel_e odt_level,
                             const int depth,
                             const bool show_codes,
                             const int startPage,
                             const int count,
                             const boost::optional<boost::posix_time::ptime>& since,
                             const boost::optional<boost::posix_time::ptime>& until,
                             const type::Data& data,
                             const boost::posix_time::ptime& current_time) {
    std::vector<type::idx_t> final_indexes;
    pbnavitia::Response pb_response;
    int total_result;
    try {
        final_indexes = make_query(requested_type, request, forbidden_uris, odt_level, since, until, data);
    } catch(const parsing_error &parse_error) {
        fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse :" + parse_error.more, pb_response.mutable_error());
        return pb_response;
    } catch(const ptref_error &pt_error) {
        fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + pt_error.more, pb_response.mutable_error());
        return pb_response;
    }
    total_result = final_indexes.size();
    final_indexes = paginate(final_indexes, count, startPage);

    pb_response = extract_data(data, requested_type, final_indexes, depth, show_codes, current_time);
    auto pagination = pb_response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(startPage);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(final_indexes.size());
    return pb_response;
}


}}
