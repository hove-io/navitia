#pragma once
#include "data.h"
#include "type/type.pb.h"
#include "type/response.pb.h"

#define null_time_period boost::posix_time::time_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0))

namespace navitia{
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

void fill_pb_placemark(const type::StopPoint* stop_point, const type::Data &data, pbnavitia::Place* place, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_street_section(const type::EntryPoint &ori_dest, const georef::Path & path, const type::Data &data, pbnavitia::Section* section, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_message(const type::Message & message, const type::Data &data, pbnavitia::Message* pb_message, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void create_pb(const type::EntryPoint &ori_dest, const navitia::georef::Path& path, const navitia::type::Data& data, pbnavitia::StreetNetwork* sn,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const georef::POI*, const type::Data &data, pbnavitia::Poi* poi, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const georef::POIType*, const type::Data &data, pbnavitia::PoiType* poi_type, int max_depth = 0,
        const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
        const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(navitia::georef::Admin* adm, const nt::Data& data, pbnavitia::AdministrativeRegion* admin, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period );

void fill_pb_object(const navitia::type::StopTime* st, const nt::Data& data, pbnavitia::RouteScheduleRow* row, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period,
                    const type::DateTime& date_time = type::DateTime());

void fill_pb_object(const navitia::type::StopTime* st, const nt::Data& data, pbnavitia::Uris* uris, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period,
                    const type::DateTime& date_time = type::DateTime());

void fill_pb_object(const type::Route* route, const nt::Data& data, pbnavitia::PtDisplayInfo* pt_display_info, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const type::Route* route, const nt::Data& data, pbnavitia::Uris* uris, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period);

void fill_pb_object(const type::VehicleJourney* vehicle_journey, const nt::Data& data, pbnavitia::PtDisplayInfo* header, int max_depth = 0,
                    const boost::posix_time::ptime& now = boost::posix_time::not_a_date_time,
                    const boost::posix_time::time_period& action_period = null_time_period);

}//namespace navitia
