/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "gtfs_parser.h"

/**
 * Read CanalTP custom transportation files
 *
 * The format is based on GTFS but additional data have been added
 *
 * In most case the FusioHandler will inherit from the GTFS handler and add additional data to the created object
 *
 * Note: In case of errors don't forget to clean the created object since
 * it will have been added in the data structure during the GTFSHandler::handle_line
 */
namespace ed {
namespace connectors {

struct FeedInfoFusioHandler : public GenericHandler {
    FeedInfoFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int feed_info_param_c, feed_info_value_c;
    boost::optional<boost::gregorian::date> feed_creation_date;
    boost::optional<boost::posix_time::time_duration> feed_creation_time;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    void finish(Data&);
};

struct AgencyFusioHandler : public AgencyGtfsHandler {
    AgencyFusioHandler(GtfsData& gdata, CsvReader& reader) : AgencyGtfsHandler(gdata, reader) {}
    int ext_code_c, sort_c, agency_url_c;
    void init(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {}; }
    // const std::vector<std::string> required_headers() const { return {"network_id", "network_name", "network_url",
    // "network_timezone"}; }
};

struct StopsFusioHandler : public StopsGtfsHandler {
    StopsFusioHandler(GtfsData& gdata, CsvReader& reader) : StopsGtfsHandler(gdata, reader) {}
    int property_id_c, comment_id_c, visible_c, geometry_id_c, address_id_c;

    void init(Data& data);
    stop_point_and_area handle_line(Data& data, const csv_row& line, bool is_first_line);
    void handle_stop_point_without_area(Data& data);
    const std::vector<std::string> required_headers() const {
        return {"stop_id", "stop_name", "location_type", "stop_lat", "stop_lon"};
    }

    ed::types::StopPoint* build_stop_point(Data& data, const csv_row& line);
    ed::types::StopArea* build_stop_area(Data& data, const csv_row& line);
    ed::types::AccessPoint* build_access_points(Data& data, const csv_row& row);
};

struct PathWayFusioHandler : public PathWayGtfsHandler {
    PathWayFusioHandler(GtfsData& gdata, CsvReader& reader) : PathWayGtfsHandler(gdata, reader) {}
    int ext_code_c, sort_c, agency_url_c;
    void init(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return PathWayGtfsHandler::required_headers(); }
};

struct RouteFusioHandler : public GenericHandler {
    RouteFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int route_id_c, ext_code_c, route_name_c, direction_type_c, line_id_c, comment_id_c, commercial_mode_id_c,
        geometry_id_c, destination_id_c;
    int ignored;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"route_id", "route_name", "line_id"}; }
};

struct TransfersFusioHandler : public TransfersGtfsHandler {
    TransfersFusioHandler(GtfsData& gdata, CsvReader& reader) : TransfersGtfsHandler(gdata, reader) {}
    int real_time_c, property_id_c;
    ~TransfersFusioHandler() override = default;
    void init(Data&);
    void fill_stop_point_connection(ed::types::StopPointConnection* connection, const csv_row& row) const override;
};

struct GeometriesFusioHandler : public GenericHandler {
    GeometriesFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int geometry_id_c = -1, geometry_wkt_c = -1;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    void finish(Data& data);
    const std::vector<std::string> required_headers() const { return {"geometry_id", "geometry_wkt"}; }
};

struct TripsFusioHandler : public GenericHandler {
    TripsFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int route_id_c, service_c, trip_c, trip_headsign_c, block_id_c, comment_id_c, trip_propertie_id_c, odt_type_c,
        company_id_c, odt_condition_id_c, physical_mode_c, ext_code_c, geometry_id_c, contributor_id_c, dataset_id_c,
        trip_short_name_c;

    int ignored = 0;
    int ignored_vj = 0;
    void init(Data&);
    std::vector<ed::types::VehicleJourney*> get_split_vj(Data& data, const csv_row& row, bool is_first_line);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"route_id", "service_id", "trip_id", "physical_mode_id", "company_id"};
    }
    void finish(Data&);
};

struct StopTimeFusioHandler : public StopTimeGtfsHandler {
    StopTimeFusioHandler(GtfsData& gdata, CsvReader& reader)
        : StopTimeGtfsHandler(gdata, reader), is_stop_time_precision(true) {}
    int desc_c, itl_c, date_time_estimated_c, id_c, stop_headsign_c, boarding_duration_c, alighting_duration_c,
        trip_short_name_at_stop_c;
    bool is_stop_time_precision;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
};

struct ContributorFusioHandler : public GenericHandler {
    ContributorFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c, website_c, license_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"contributor_name", "contributor_id"}; }
};

// TODO: remove FrameFusioHandler after complete migration(fusio, ntfs, fusio2ed, ...)
struct FrameFusioHandler : public GenericHandler {
    FrameFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, contributor_c, start_date_c, end_date_c, type_c, desc_c, system_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"frame_id", "contributor_id", "frame_start_date", "frame_end_date"};
    }
};

struct DatasetFusioHandler : public GenericHandler {
    DatasetFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, contributor_c, start_date_c, end_date_c, type_c, desc_c, system_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"dataset_id", "contributor_id", "dataset_start_date", "dataset_end_date"};
    }
};

struct LineFusioHandler : public GenericHandler {
    LineFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c, external_code_c, code_c, forward_name_c, backward_name_c, color_c, sort_c, network_c, comment_c,
        commercial_mode_c, contributor_c, geometry_id_c, opening_c, closing_c, text_color_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"line_id", "line_name", "commercial_mode_id"}; }
};

struct LineGroupFusioHandler : public GenericHandler {
    LineGroupFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c, main_line_id_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"line_group_id", "line_group_name", "main_line_id"};
    }
};

struct LineGroupLinksFusioHandler : public GenericHandler {
    LineGroupLinksFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int line_group_id_c, line_id_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"line_group_id", "line_id"}; }
};

struct CompanyFusioHandler : public GenericHandler {
    CompanyFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c, company_address_name_c, company_address_number_c, company_address_type_c, company_url_c,
        company_mail_c, company_phone_c, company_fax_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"company_name", "company_id"}; }
};

struct PhysicalModeFusioHandler : public GenericHandler {
    PhysicalModeFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c, co2_emission_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"physical_mode_id", "physical_mode_name"}; }
};

struct CommercialModeFusioHandler : public GenericHandler {
    CommercialModeFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"commercial_mode_id", "commercial_mode_name"}; }
};

struct CommentFusioHandler : public GenericHandler {
    CommentFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, comment_c, type_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"comment_id", "comment_name"}; }
};

struct OdtConditionsFusioHandler : public GenericHandler {
    OdtConditionsFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int odt_condition_id_c, odt_condition_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"odt_condition_id", "odt_condition"}; }
};

struct StopPropertiesFusioHandler : public GenericHandler {
    StopPropertiesFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, wheelchair_boarding_c, sheltered_c, elevator_c, escalator_c, bike_accepted_c, bike_depot_c,
        visual_announcement_c, audible_announcement_c, appropriate_escort_c, appropriate_signage_c;
    void init(Data&);
    void handle_line(Data&, const csv_row& row, bool is_first_line);
};

struct ObjectPropertiesFusioHandler : public GenericHandler {
    ObjectPropertiesFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int object_id_c, object_type_c, property_name_c, property_value_c;
    void init(Data&);
    void handle_line(Data&, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"object_id", "object_type", "object_property_name", "object_property_value"};
    }
};

struct TripPropertiesFusioHandler : public GenericHandler {
    TripPropertiesFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, wheelchair_accessible_c, bike_accepted_c, air_conditioned_c, visual_announcement_c,
        audible_announcement_c, appropriate_escort_c, appropriate_signage_c, school_vehicle_c;
    void init(Data&);
    void handle_line(Data&, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"trip_property_id"}; }
};

namespace grid_calendar {
struct PeriodFusioHandler : public GenericHandler {
    PeriodFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, start_c, end_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"end_date"}; }
};

struct GridCalendarFusioHandler : public GenericHandler {
    GridCalendarFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c, monday_c, tuesday_c, wednesday_c, thursday_c, friday_c, saturday_c, sunday_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"name", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"};
    }
};

struct ExceptionDatesFusioHandler : public GenericHandler {
    ExceptionDatesFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, datetime_c, type_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"date", "type"}; }
};

struct CalendarLineFusioHandler : public GenericHandler {
    CalendarLineFusioHandler(GtfsData& gdata, CsvReader& reader)
        : GenericHandler(gdata, reader), line_id_is_present(true) {}
    int id_c, line_c;
    bool line_id_is_present;
    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {}; }
};

struct CalendarTripFusioHandler : public GenericHandler {
    CalendarTripFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int calendar_c, trip_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"calendar_id", "trip_external_code"}; }
};

struct GridCalendarTripExceptionDatesFusioHandler : public GenericHandler {
    GridCalendarTripExceptionDatesFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int calendar_c, trip_c, date_c, type_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"calendar_id", "trip_external_code", "date", "type"};
    }
};
}  // namespace grid_calendar

struct AdminStopAreaFusioHandler : public GenericHandler {
    AdminStopAreaFusioHandler(GtfsData& gdata, CsvReader& reader)
        : GenericHandler(gdata, reader), stop_id_is_present(true) {}
    int admin_c, stop_area_c;
    bool stop_id_is_present;

    // temporaty map to have a StopArea by it's external code
    std::unordered_map<std::string, ed::types::StopArea*> tmp_stop_area_map;

    std::unordered_map<std::string, ed::types::AdminStopArea*> admin_stop_area_map;
    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    // TODO : "stop_id" will becomme required, after the data team update (NAVP-1285)
    const std::vector<std::string> required_headers() const { return {"admin_id"}; }
};

struct CommentLinksFusioHandler : public GenericHandler {
    CommentLinksFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int object_id_c, object_type_c, comment_id_c;

    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"object_id", "object_type", "comment_id"}; }
};

struct ObjectCodesFusioHandler : public GenericHandler {
    ObjectCodesFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int object_uri_c, object_type_c, code_c, object_system_c;

    void init(Data&);
    void handle_line(Data& data, const csv_row& row, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"object_id", "object_type", "object_code", "object_system"};
    }
};

struct AddressesFusioHandler : public GenericHandler {
    AddressesFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int address_id_c, street_name_c, house_number_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"address_id", "street_name"}; }
};

/**
 * custom parser
 * simply define the list of elemental parsers to use
 */
struct FusioParser : public GenericGtfsParser {
    void parse_files(Data&, const std::string& beginning_date = "") override;
    FusioParser(const std::string& path) : GenericGtfsParser(path) {}
};
}  // namespace connectors
}  // namespace ed
