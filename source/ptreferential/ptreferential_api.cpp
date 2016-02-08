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

namespace navitia{ namespace ptref{

static pbnavitia::Response extract_data(const type::Data& data,
                                        const type::Type_e requested_type,
                                        const std::vector<type::idx_t>& rows,
                                        const int depth,
                                        const bool show_codes,
                                        const boost::posix_time::ptime& current_time) {
    const auto& action_period = data.meta->production_period();
        switch(requested_type){
        case Type_e::ValidityPattern:
            return get_response(data.get_data<nt::ValidityPattern>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::Line:
            return get_response(data.get_data<nt::Line>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::LineGroup:
            return get_response(data.get_data<nt::LineGroup>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::JourneyPattern:{
            navitia::PbCreator pb_creator(data, current_time, action_period, show_codes);
            for(const auto& idx : rows){
                const auto& pair_jp = data.dataRaptor->jp_container.get_jps()[idx];
                auto* pb_jp = pb_creator.add_journey_patterns();
                pb_creator.fill(&pair_jp, pb_jp, depth);
            }
            return pb_creator.get_response();
        }
        case Type_e::StopPoint:
            return get_response(data.get_data<nt::StopPoint>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::StopArea:
            return get_response(data.get_data<nt::StopArea>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::Network:
            return get_response(data.get_data<nt::Network>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::PhysicalMode:
            return get_response(data.get_data<nt::PhysicalMode>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::CommercialMode:
            return get_response(data.get_data<nt::CommercialMode>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::JourneyPatternPoint:{
            navitia::PbCreator pb_creator(data, current_time, action_period, show_codes);
            for(const auto& idx : rows){
                const auto& pair_jpp = data.dataRaptor->jp_container.get_jpps()[idx];
                auto* pb_jpp = pb_creator.add_journey_pattern_points();
                pb_creator.fill(&pair_jpp, pb_jpp, depth);
            }
            return pb_creator.get_response();
        }
        case Type_e::Company:
            return get_response(data.get_data<nt::Company>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::Route:
            return get_response(data.get_data<nt::Route>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::POI:
            return get_response(data.get_data<georef::POI>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::POIType:
            return get_response(data.get_data<georef::POIType>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::Connection:
            return get_response(data.get_data<nt::StopPointConnection>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::VehicleJourney:
            return get_response(data.get_data<nt::VehicleJourney>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::Calendar:
            return get_response(data.get_data<nt::Calendar>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::MetaVehicleJourney:{
            navitia::PbCreator pb_creator(data, current_time, action_period, show_codes);
            for(const auto& idx : rows){
                const auto* meta_vj = data.pt_data->meta_vjs[Idx<type::MetaVehicleJourney>(idx)];
                auto* pb_trip = pb_creator.add_trips();
                pb_creator.fill(meta_vj, pb_trip, depth);
            }
            return pb_creator.get_response();
        }
        case Type_e::Impact:{
            navitia::PbCreator pb_creator(data, current_time, action_period, show_codes);
            for(const auto& idx : rows){
                auto impact = data.pt_data->disruption_holder.get_weak_impacts()[idx].lock();
                pb_creator.fill(impact.get(), depth);

            }
            return pb_creator.get_response();
        }
        case Type_e::Contributor:
            return get_response(data.get_data<nt::Contributor>(rows), data, depth, current_time,
                                action_period, show_codes);
        case Type_e::Frame:
            return get_response(data.get_data<nt::Frame>(rows), data, depth, current_time,
                                action_period, show_codes);
        default: return {};
        }
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
