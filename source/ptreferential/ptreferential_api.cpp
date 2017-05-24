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
/// build the protobuf response of a pt ref query

static void extract_data(navitia::PbCreator& pb_creator,
                         const type::Data& data,
                         const type::Type_e requested_type,
                         const type::Indexes& rows,
                         const int depth) {
    pb_creator.action_period = data.meta->production_period();
    switch(requested_type){
    case Type_e::ValidityPattern:
        pb_creator.pb_fill(data.get_data<nt::ValidityPattern>(rows), depth);
        return;
    case Type_e::Line:
        pb_creator.pb_fill(data.get_data<nt::Line>(rows), depth,
                {DumpMessage::Yes, DumpLineSectionMessage::Yes});
        return;
    case Type_e::LineGroup:
        pb_creator.pb_fill(data.get_data<nt::LineGroup>(rows), depth);
        return;
    case Type_e::JourneyPattern:{
        for(const auto& idx : rows){
            const auto& pair_jp = data.dataRaptor->jp_container.get_jps()[idx];
            auto* pb_jp = pb_creator.add_journey_patterns();
            pb_creator.fill(&pair_jp, pb_jp, depth);
        }
        return;
        }
    case Type_e::StopPoint:
        pb_creator.pb_fill(data.get_data<nt::StopPoint>(rows), depth);
        return;
    case Type_e::StopArea:
        pb_creator.pb_fill(data.get_data<nt::StopArea>(rows), depth);
        return;
    case Type_e::Network:
        pb_creator.pb_fill(data.get_data<nt::Network>(rows), depth);
        return;
    case Type_e::PhysicalMode:
        pb_creator.pb_fill(data.get_data<nt::PhysicalMode>(rows), depth);
        return;
    case Type_e::CommercialMode:
        pb_creator.pb_fill(data.get_data<nt::CommercialMode>(rows), depth);
        return;
    case Type_e::JourneyPatternPoint:{
        for(const auto& idx : rows){
            const auto& pair_jpp = data.dataRaptor->jp_container.get_jpps()[idx];
            auto* pb_jpp = pb_creator.add_journey_pattern_points();
            pb_creator.fill(&pair_jpp, pb_jpp, depth);
        }
        return;
    }
    case Type_e::Company:
        pb_creator.pb_fill(data.get_data<nt::Company>(rows), depth);
        return;
    case Type_e::Route:
        pb_creator.pb_fill(data.get_data<nt::Route>(rows), depth);
        return;
    case Type_e::POI:
        pb_creator.pb_fill(data.get_data<georef::POI>(rows), depth);
        return;
    case Type_e::POIType:
        pb_creator.pb_fill(data.get_data<georef::POIType>(rows), depth);
        return;
    case Type_e::Connection:
        pb_creator.pb_fill(data.get_data<nt::StopPointConnection>(rows), depth);
        return;
    case Type_e::VehicleJourney:
        pb_creator.pb_fill(data.get_data<nt::VehicleJourney>(rows), depth);
        return;
    case Type_e::Calendar:
        pb_creator.pb_fill(data.get_data<nt::Calendar>(rows), depth);
        return;
    case Type_e::MetaVehicleJourney:{
        for(const auto& idx : rows){
            const auto* meta_vj = data.pt_data->meta_vjs[Idx<type::MetaVehicleJourney>(idx)];
            auto* pb_trip = pb_creator.add_trips();
            pb_creator.fill(meta_vj, pb_trip, depth);
        }
        return;
    }
    case Type_e::Impact:{
        for(const auto& idx : rows){
            auto impact = data.pt_data->disruption_holder.get_weak_impacts()[idx].lock();
            auto* pb_impact = pb_creator.add_impacts();
            pb_creator.fill(impact.get(), pb_impact, depth);
        }
        return;
    }
    case Type_e::Contributor:
        pb_creator.pb_fill(data.get_data<nt::Contributor>(rows), depth);
        return;
    case Type_e::Dataset:
        pb_creator.pb_fill(data.get_data<nt::Dataset>(rows), depth);
        return;
    default: return;
    }
}


void query_pb(navitia::PbCreator& pb_creator,
                             const type::Type_e requested_type,
                             const std::string& request,
                             const std::vector<std::string>& forbidden_uris,
                             const type::OdtLevel_e odt_level,
                             const int depth,
                             const int startPage,
                             const int count,
                             const boost::optional<boost::posix_time::ptime>& since,
                             const boost::optional<boost::posix_time::ptime>& until,
                             const type::Data& data) {
    type::Indexes final_indexes;
    int total_result;
    try {
        final_indexes = make_query(requested_type, request, forbidden_uris, odt_level, since, until, data);
    } catch(const parsing_error &parse_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse :" + parse_error.more);
        return;
    } catch(const ptref_error &pt_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + pt_error.more);
        return;
    }
    total_result = final_indexes.size();
    final_indexes = paginate(final_indexes, count, startPage);

    extract_data(pb_creator, data, requested_type, final_indexes, depth);
    auto pagination = pb_creator.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(startPage);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(final_indexes.size());    
}


}}
