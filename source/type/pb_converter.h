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

#pragma once
#include "data.h"
#include "type/type.pb.h"
#include "type/response.pb.h"

//forward declare
namespace navitia{
    namespace routing{
        class PathItem;
    }
    namespace georef{
        class PathItem;
        class Way;
        class POI;
        class Path;
        class POIType;
    }
    namespace fare{
        class results;
        class Ticket;
    }
}
namespace pt = boost::posix_time;

#define null_time_period boost::posix_time::time_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0))

namespace navitia {

struct EnhancedResponse {
    pbnavitia::Response response;
    size_t nb_sections = 0;
    std::map<std::pair<pbnavitia::Journey*, size_t>, std::string> routing_section_map;
    pbnavitia::Ticket* unkown_ticket = nullptr; //we want only one unknown ticket

    const std::string& register_section(pbnavitia::Journey* j, const routing::PathItem& /*routing_item*/, size_t section_idx) {
        routing_section_map[{j, section_idx}] = "section_" + boost::lexical_cast<std::string>(nb_sections++);
        return routing_section_map[{j, section_idx}];
    }

    std::string register_section() {
        // For some section (transfer, waiting, streetnetwork, corwfly) we don't need info
        // about the item
        return "section_" + boost::lexical_cast<std::string>(nb_sections++);
    }

    std::string get_section_id(pbnavitia::Journey* j, size_t section_idx) {
        auto it  = routing_section_map.find({j, section_idx});
        if (it == routing_section_map.end()) {
            LOG4CPLUS_WARN(log4cplus::Logger::getInstance("logger"), "Impossible to find section id for section idx " << section_idx);
            return "";
        }
        return it->second;
    }
};

#define FILL_PB_CONSTRUCTOR(type_name, collection_name)\
    void fill_pb_object(const navitia::type::type_name* item, const navitia::type::Data& data, pbnavitia::type_name *, int max_depth = 0,\
            const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,\
            const boost::posix_time::time_period& action_period = null_time_period, const bool show_codes=false);
    ITERATE_NAVITIA_PT_TYPES(FILL_PB_CONSTRUCTOR)
#undef FILL_PB_CONSTRUCTOR

void fill_pb_object(const navitia::georef::Way* way, const type::Data &data, pbnavitia::Address* address, int house_number,
        const type::GeographicalCoord& coord, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const navitia::type::GeographicalCoord& coord, const type::Data &data, pbnavitia::Address* address,
        int max_depth = 0, const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const navitia::georef::Admin* adm, const type::Data &data, pbnavitia::StopDateTime *stop_time, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const navitia::type::StopTime* st, const type::Data &data, pbnavitia::StopTime *stop_time, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const navitia::type::StopTime* st, const type::Data &data, pbnavitia::StopDateTime * stop_date_time, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_codes(const std::string& type, const std::string& value, pbnavitia::Code* code);

void fill_pb_placemark(const type::StopPoint* stop_point, const type::Data &data,
        pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period,
        const bool show_codes = false);

void fill_pb_placemark(const navitia::georef::Way* way, const type::Data &data, pbnavitia::Place* place, int house_number, type::GeographicalCoord& coord, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_placemark(const type::StopArea* stop_area, const type::Data &data,
        pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period,
        const bool show_codes = false);

void fill_pb_placemark(const navitia::georef::Admin* admin, const type::Data &data, pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_placemark(const navitia::georef::POI* poi, const type::Data &data, pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_placemark(const type::EntryPoint& origin, const type::Data &data, pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period,
        const bool show_codes = false);


void fill_crowfly_section(const type::EntryPoint& origin, const type::EntryPoint& destination, boost::posix_time::ptime time,
                          const type::Data& data, EnhancedResponse &response,  pbnavitia::Journey* pb_journey,
                          const pt::ptime& now, const pt::time_period& action_period);

void fill_fare_section(EnhancedResponse& pb_response, pbnavitia::Journey* pb_journey, const fare::results& fare);

void fill_street_sections(EnhancedResponse& response, const type::EntryPoint &ori_dest, const georef::Path & path, const type::Data &data, pbnavitia::Journey* pb_journey,
        const boost::posix_time::ptime departure, int max_depth = 1,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_message(const boost::shared_ptr<type::Message> message, const type::Data &data, pbnavitia::Message* pb_message, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void add_path_item(pbnavitia::StreetNetwork* sn, const navitia::georef::PathItem& item, const type::EntryPoint &ori_dest,
                   const navitia::type::Data& data);

void fill_pb_object(const georef::POI*, const type::Data &data, pbnavitia::Poi* poi, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const georef::POIType*, const type::Data &data, pbnavitia::PoiType* poi_type, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const navitia::georef::Admin* adm, const type::Data& data, pbnavitia::AdministrativeRegion* admin, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period );

void fill_pb_object(const navitia::type::StopTime* st, const type::Data& data, pbnavitia::ScheduleStopTime* row, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period,
                    const DateTime& date_time = DateTime(), boost::optional<const std::string> calendar_id = boost::optional<const std::string>(),
                    const navitia::type::StopPoint* destination = nullptr);

void fill_pb_object(const type::StopPointConnection* c, const type::Data& data,
                    pbnavitia::Connection* connection, int max_depth,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const type::Route* r, const type::Data& data,
                    pbnavitia::PtDisplayInfo* pt_display_info, int max_depth,
                    const boost::posix_time::ptime& now,
                    const boost::posix_time::time_period& action_period = null_time_period,
                    const navitia::type::StopPoint* destination = nullptr);

void fill_pb_object(const type::VehicleJourney* vj, const type::Data& data,
                    pbnavitia::addInfoVehicleJourney * add_info_vehicle_journey, int max_depth,
                    const boost::posix_time::ptime& now,
                    const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const type::VehicleJourney* vj, const type::Data& data,
                    pbnavitia::PtDisplayInfo* pt_display_info, int max_depth,
                    const boost::posix_time::ptime& now,
                    const boost::posix_time::time_period& action_period);


void fill_pb_error(const pbnavitia::Error::error_id id, const std::string& comment,
                    pbnavitia::Error* error, int max_depth = 0 ,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period  = null_time_period);

void fill_pb_object(const navitia::type::ExceptionDate& exception_date, const type::Data& data,
                    pbnavitia::CalendarException* calendar_exception, int max_depth,
                    const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_period);

void fill_pb_object(const std::string comment, const type::Data& data,
                    pbnavitia::Note* note, int max_depth,
                    const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_period);

void fill_pb_object(const navitia::type::StopTime* st, const type::Data& data,
                    pbnavitia::Properties* properties, int max_depth,
                    const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_period);

pbnavitia::StreetNetworkMode convert(const navitia::type::Mode_e& mode);
}//namespace navitia
