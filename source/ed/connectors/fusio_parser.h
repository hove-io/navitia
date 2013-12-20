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
namespace ed { namespace connectors {

struct AgencyFusioHandler : public AgencyGtfsHandler {
    AgencyFusioHandler(GtfsData& gdata, CsvReader& reader) : AgencyGtfsHandler(gdata, reader) {}
    int ext_code_c;
    void init(Data& data);
    ed::types::Network* handle_line(Data& data, const csv_row& line, bool is_first_line);
};

struct StopsFusioHandler : public StopsGtfsHandler {
    StopsFusioHandler(GtfsData& gdata, CsvReader& reader) : StopsGtfsHandler(gdata, reader) {}
    int sheltered_c,
    elevator_c,
    escalator_c,
    bike_accepted_c,
    bike_depot_c,
    visual_announcement_c,
    audible_announcement_c,
    appropriate_escort_c,
    appropriate_signage_c,
    ext_code_c;

    void init(Data& data);
    stop_point_and_area handle_line(Data& data, const csv_row& line, bool is_first_line);
    void handle_stop_point_without_area(Data& data);
};

struct RouteFusioHandler : public RouteGtfsHandler {
    RouteFusioHandler(GtfsData& gdata, CsvReader& reader) : RouteGtfsHandler(gdata, reader) {}
    int commercial_mode_c,
    ext_code_c;
    void init(Data&);
    ed::types::Line* handle_line(Data& data, const csv_row& line, bool is_first_line);
};

struct TransfersFusioHandler : public TransfersGtfsHandler {
    TransfersFusioHandler(GtfsData& gdata, CsvReader& reader) : TransfersGtfsHandler(gdata, reader) {}
    int real_time_c,
    wheelchair_c,
    sheltered_c, elevator_c,
    escalator_c, bike_accepted_c,
    bike_depot_c, visual_announcement_c,
    audible_announcement_c, appropriate_escort_c,
    appropriate_signage_c;
    void init(Data&);
    virtual void fill_stop_point_connection(ed::types::StopPointConnection* connection, const csv_row& row) const;
};

struct TripsFusioHandler : public TripsGtfsHandler {
    TripsFusioHandler(GtfsData& gdata, CsvReader& reader) : TripsGtfsHandler(gdata, reader) {}
    int trip_desc_c,
    bike_accepted_c,
    air_conditioned_c,
    visual_announcement_c,
    audible_announcement_c,
    appropriate_escort_c,
    appropriate_signage_c,
    school_vehicle_c,
    odt_type_c,
    company_id_c,
    condition_c,
    physical_mode_c,
    ext_code_c;
    void init(Data&);
    void clean_and_delete(Data&, ed::types::VehicleJourney*);
    ed::types::VehicleJourney* handle_line(Data& data, const csv_row& line, bool is_first_line);
};

struct StopTimeFusioHandler : public StopTimeGtfsHandler {
    StopTimeFusioHandler(GtfsData& gdata, CsvReader& reader) : StopTimeGtfsHandler(gdata, reader) {}
    int desc_c, itl_c, date_time_estimated_c;
    void init(Data&);
    ed::types::StopTime* handle_line(Data& data, const csv_row& line, bool is_first_line);
};

struct ContributorFusioHandler : public GenericHandler {
    ContributorFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"contributor_name", "contributor_id"}; }
};

struct CompanyFusioHandler : public GenericHandler {
    CompanyFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c,
    name_c,
    company_address_name_c,
    company_address_number_c,
    company_address_type_c,
    company_url_c,
    company_mail_c,
    company_phone_c,
    company_fax_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"company_name", "company_id"}; }
};

struct PhysicalModeFusioHandler : public GenericHandler {
    PhysicalModeFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"physical_mode_id", "physical_mode_name"};
    }
};

struct CommercialModeFusioHandler : public GenericHandler {
    CommercialModeFusioHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"commercial_mode_id", "commercial_mode_name"};
    }
};

/**
 * custom parser
 * simply define the list of elemental parsers to use
 */
struct FusioParser : public GenericGtfsParser {
    void parse_files(Data&);
    FusioParser(const std::string & path) : GenericGtfsParser(path, false) {}
};
}
}
