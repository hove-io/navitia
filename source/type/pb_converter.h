#pragma once
#include "data.h"
#include "type/type.pb.h"
#include "type/response.pb.h"

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

    std::string register_section(const georef::PathItem& /*georef_item*/) {
        //we don't need to register the georef item
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
void fill_pb_object(const nt::type_name* item, const nt::Data& data, pbnavitia::type_name *, int max_depth = 0,\
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,\
        const boost::posix_time::time_period& action_period = null_time_period);
    ITERATE_NAVITIA_PT_TYPES(FILL_PB_CONSTRUCTOR)
#undef FILL_PB_CONSTRUCTOR

void fill_pb_object(navitia::georef::Way* way, const type::Data &data, pbnavitia::Address* address, int house_number,type::GeographicalCoord& coord,
        int max_depth = 0, const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(navitia::georef::Admin* adm, const type::Data &data, pbnavitia::StopDateTime *stop_time, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const navitia::type::StopTime* st, const type::Data &data, pbnavitia::StopTime *stop_time, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const navitia::type::StopTime* st, const type::Data &data, pbnavitia::StopDateTime * stop_date_time, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_placemark(const type::StopPoint* stop_point, const type::Data &data, pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_placemark(navitia::georef::Way* way, const type::Data &data, pbnavitia::Place* place, int house_number, type::GeographicalCoord& coord, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_placemark(const type::StopArea* stop_area, const type::Data &data, pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_placemark(navitia::georef::Admin* admin, const type::Data &data, pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_placemark(navitia::georef::POI* poi, const type::Data &data, pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

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

void fill_pb_object(navitia::georef::Admin* adm, const nt::Data& data, pbnavitia::AdministrativeRegion* admin, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period );

void fill_pb_object(const navitia::type::StopTime* st, const nt::Data& data, pbnavitia::ScheduleStopTime* row, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period,
                    const DateTime& date_time = DateTime());

void fill_pb_object(const type::StopPointConnection* c, const type::Data& data,
                    pbnavitia::Connection* connection, int max_depth,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const nt::Route* r, const nt::Data& data,
                    pbnavitia::PtDisplayInfo* pt_display_info, int max_depth,
                    const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const nt::VehicleJourney* vj, const nt::Data& data,
                    pbnavitia::addInfoVehicleJourney * add_info_vehicle_journey, int max_depth,
                    const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const nt::VehicleJourney* vj, const nt::Data& data,
                    pbnavitia::PtDisplayInfo* pt_display_info, int max_depth,
                    const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_period);


void fill_pb_error(const pbnavitia::Error::error_id id, const std::string& comment,
                    pbnavitia::Error* error, int max_depth = 0 ,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period  = null_time_period);

}//namespace navitia
