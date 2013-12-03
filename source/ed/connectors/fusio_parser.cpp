#include "fusio_parser.h"

namespace ed { namespace connectors {

void StopsFusioHandler::init(Data& data) {
    StopsGtfsHandler::init(data);
    sheltered_c = csv.get_pos_col("sheltered"),
            elevator_c = csv.get_pos_col("elevator"),
            escalator_c = csv.get_pos_col("escalator"),
            bike_accepted_c = csv.get_pos_col("bike_accepted"),
            bike_depot_c = csv.get_pos_col("bike_depot"),
            visual_announcement_c = csv.get_pos_col("visual_announcement"),
            audible_announcement_c = csv.get_pos_col("audible_announcement"),
            appropriate_escort_c = csv.get_pos_col("appropriate_escort"),
            appropriate_signage_c = csv.get_pos_col("appropriate_signage");
}

StopsGtfsHandler::stop_point_and_area StopsFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto return_wrapper = StopsGtfsHandler::handle_line(data, row, is_first_line);

    //the additional data are only for stop area
    if ( return_wrapper.second == nullptr )
        return return_wrapper;

    if(sheltered_c != -1 && row[sheltered_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::SHELTERED);
    if(elevator_c != -1 && row[elevator_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::ELEVATOR);
    if(escalator_c != -1 && row[escalator_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::ESCALATOR);
    if(bike_accepted_c != -1 && row[bike_accepted_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    if(bike_depot_c != -1 && row[bike_depot_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::BIKE_DEPOT);
    if(visual_announcement_c != -1 && row[visual_announcement_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
    if(audible_announcement_c != -1 && row[audible_announcement_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
    if(appropriate_escort_c != -1 && row[appropriate_escort_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
    if(appropriate_signage_c != -1 && row[appropriate_signage_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);

    return return_wrapper;
}

void RouteFusioHandler::init(Data& data) {
    RouteGtfsHandler::init(data);
    commercial_mode_c = csv.get_pos_col("commercial_mode_id");
}

ed::types::Line* RouteFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto ed_line = RouteGtfsHandler::handle_line(data, row, is_first_line);

    if (! ed_line) {
        return nullptr;
    }

    std::unordered_map<std::string, ed::types::CommercialMode*>::iterator it;
    if(commercial_mode_c != -1)
        it = gtfs_data.commercial_mode_map.find(row[commercial_mode_c]);
    else it = gtfs_data.commercial_mode_map.find(row[type_c]);

    if(it != gtfs_data.commercial_mode_map.end())
        ed_line->commercial_mode = it->second;

    return ed_line;
}
void TransfersFusioHandler::init(Data& d) {
    TransfersGtfsHandler::init(d);
    real_time_c = csv.get_pos_col("real_min_transfer_time"),
            wheelchair_c = csv.get_pos_col("wheelchair_boarding"),
            sheltered_c = csv.get_pos_col("sheltered"),
            elevator_c = csv.get_pos_col("elevator"),
            escalator_c = csv.get_pos_col("escalator"),
            bike_accepted_c = csv.get_pos_col("bike_accepted"),
            bike_depot_c = csv.get_pos_col("bike_depot"),
            visual_announcement_c =csv.get_pos_col("visual_announcement"),
            audible_announcement_c = csv.get_pos_col("audible_announcement"),
            appropriate_escort_c = csv.get_pos_col("appropriate_escort"),
            appropriate_signage_c =csv.get_pos_col("appropriate_signage");
}

void TransfersFusioHandler::fill_stop_point_connection(ed::types::StopPointConnection* connection, const csv_row& row) const {
    TransfersGtfsHandler::fill_stop_point_connection(connection, row);

    if(wheelchair_c != -1 && row[wheelchair_c] == "1")
        connection->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
    if(sheltered_c != -1 && row[sheltered_c] == "1")
        connection->set_property(navitia::type::hasProperties::SHELTERED);
    if(elevator_c != -1 && row[elevator_c] == "1")
        connection->set_property(navitia::type::hasProperties::ELEVATOR);
    if(escalator_c != -1 && row[escalator_c] == "1")
        connection->set_property(navitia::type::hasProperties::ESCALATOR);
    if(bike_accepted_c != -1 && row[bike_accepted_c] == "1")
        connection->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    if(bike_depot_c != -1 && row[bike_depot_c] == "1")
        connection->set_property(navitia::type::hasProperties::BIKE_DEPOT);
    if(visual_announcement_c != -1 && row[visual_announcement_c] == "1")
        connection->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
    if(audible_announcement_c != -1 && row[audible_announcement_c] == "1")
        connection->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
    if(appropriate_escort_c != -1 && row[appropriate_escort_c] == "1")
        connection->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
    if(appropriate_signage_c != -1 && row[appropriate_signage_c] == "1")
        connection->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);

    if(real_time_c != -1) {
        try{
            connection->duration = boost::lexical_cast<int>(row[real_time_c]);
        } catch (...) {
            connection->duration = connection->display_duration;
        }
    } else {
        connection->duration = connection->display_duration;
    }
}

void StopTimeFusioHandler::init(Data& data) {
    StopTimeGtfsHandler::init(data);
    itl_c = csv.get_pos_col("stop_times_itl"),
               desc_c = csv.get_pos_col("stop_desc"),
            date_time_estimated_c = csv.get_pos_col("date_time_estimated");
}

ed::types::StopTime* StopTimeFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto stop_time = StopTimeGtfsHandler::handle_line(data, row, is_first_line);

    if (! stop_time) {
        return nullptr;
    }
    if (date_time_estimated_c != -1)
        stop_time->date_time_estimated = (row[date_time_estimated_c] == "1");
    else stop_time->date_time_estimated = false;

    if (desc_c != -1)
        stop_time->comment = row[desc_c];

    if(itl_c != -1){
        int local_traffic_zone = str_to_int(row[itl_c]);
        if (local_traffic_zone > 0)
            stop_time->local_traffic_zone = local_traffic_zone;
        else stop_time->local_traffic_zone = std::numeric_limits<uint32_t>::max();
    }
    else
        stop_time->local_traffic_zone = std::numeric_limits<uint32_t>::max();
    return stop_time;

}

void TripsFusioHandler::init(Data& d) {
    TripsGtfsHandler::init(d);

    trip_desc_c = csv.get_pos_col("trip_desc"),
    bike_accepted_c = csv.get_pos_col("bike_accepted"),
    air_conditioned_c = csv.get_pos_col("air_conditioned"),
    visual_announcement_c =csv.get_pos_col("visual_announcement"),
    audible_announcement_c = csv.get_pos_col("audible_announcement"),
    appropriate_escort_c = csv.get_pos_col("appropriate_escort"),
    appropriate_signage_c =csv.get_pos_col("appropriate_signage"),
    school_vehicle_c = csv.get_pos_col("school_vehicle"),
    odt_type_c = csv.get_pos_col("odt_type"),
    company_id_c = csv.get_pos_col("company_id"),
    condition_c = csv.get_pos_col("trip_condition"),
    physical_mode_c = csv.get_pos_col("physical_mode_id");
}

void TripsFusioHandler::clean_and_delete(Data& data, ed::types::VehicleJourney* vj) {
    gtfs_data.vj_map.erase(vj->uri);

    data.vehicle_journeys.pop_back();
    delete vj;
}

ed::types::VehicleJourney* TripsFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    ed::types::VehicleJourney* vj = TripsGtfsHandler::handle_line(data, row, is_first_line);

    if (vj == nullptr) {
        return nullptr;
    }

    auto it_line = gtfs_data.line_map.find(row[id_c]);
    if (it_line == gtfs_data.line_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the Gtfs route " + row[id_c]
                       + " referenced by trip " + row[trip_c]);
        return nullptr;
    }

    ed::types::Line* line = it_line->second;
    std::unordered_map<std::string, ed::types::PhysicalMode*>::iterator itm;

    if (physical_mode_c != -1) {
        itm = gtfs_data.physical_mode_map.find(row[physical_mode_c]);
    } else itm = gtfs_data.physical_mode_map.find(line->commercial_mode->id);

    if (itm == gtfs_data.physical_mode_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the Gtfs mode " + (physical_mode_c != -1 ? row[physical_mode_c] : line->commercial_mode->id)
                       + " referenced by trip " + row[trip_c]);
        ignored++;
        clean_and_delete(data, vj);
        return nullptr;
    }
    vj->physical_mode = itm->second;

    if (trip_desc_c != -1)
        vj->comment = row[trip_desc_c];

    if (condition_c != -1)
        vj->odt_message = row[condition_c];

    if(bike_accepted_c != -1 && row[bike_accepted_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
    if(air_conditioned_c != -1 && row[air_conditioned_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::AIR_CONDITIONED);
    if(visual_announcement_c != -1 && row[visual_announcement_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::VISUAL_ANNOUNCEMENT);
    if(audible_announcement_c != -1 && row[audible_announcement_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::AUDIBLE_ANNOUNCEMENT);
    if(appropriate_escort_c != -1 && row[appropriate_escort_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_ESCORT);
    if(appropriate_signage_c != -1 && row[appropriate_signage_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_SIGNAGE);
    if(school_vehicle_c != -1 && row[school_vehicle_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::SCHOOL_VEHICLE);

    if(odt_type_c != -1){
        vj->vehicle_journey_type = static_cast<nt::VehicleJourneyType>(boost::lexical_cast<int>(row[odt_type_c]));
    }

    std::string company_s;
    if ((company_id_c != -1) && (!row[company_id_c].empty())) {
        company_s = row[company_id_c];
    }
    auto company_it = gtfs_data.company_map.find(company_s);
    if (company_it != gtfs_data.company_map.end()) {
        vj->company = company_it->second;
    } else {
        auto company_it = gtfs_data.company_map.find("default_company");
        if(company_it != gtfs_data.company_map.end()){
            vj->company = company_it->second;
        }
    }
    return vj;
}

void ContributorFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("contributor_id");
    name_c = csv.get_pos_col("contributor_name");
}

void ContributorFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && id_c == -1) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one contributor and no contributor_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::Contributor * contributor = new ed::types::Contributor();
    if (id_c != -1) {
        contributor->uri = row[id_c];
    } else {
        contributor->uri = "default_contributor";
    }
    contributor->name = row[name_c];
    contributor->idx = data.contributors.size() + 1;
    data.contributors.push_back(contributor);
    gtfs_data.contributor_map[contributor->uri] = contributor;
}

void CompanyFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("company_id"), name_c = csv.get_pos_col("company_name"),
            company_address_name_c = csv.get_pos_col("company_address_name"),
            company_address_number_c = csv.get_pos_col("company_address_number"),
            company_address_type_c = csv.get_pos_col("company_address_type"),
            company_url_c = csv.get_pos_col("company_url"),
            company_mail_c = csv.get_pos_col("company_mail"),
            company_phone_c = csv.get_pos_col("company_phone"),
            company_fax_c = csv.get_pos_col("company_fax");
}

void CompanyFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && id_c == -1) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one company and no company_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::Company * company = new ed::types::Company();
    if(id_c != -1){
        company->uri = row[id_c];
    }else{
        company->uri = "default_company";
    }
    company->name = row[name_c];
    if (company_address_name_c != -1)
        company->address_name = row[company_address_name_c];
    if (company_address_number_c != -1)
        company->address_number = row[company_address_number_c];
    if (company_address_type_c != -1)
        company->address_type_name = row[company_address_type_c];
    if (company_url_c != -1)
        company->website = row[company_url_c];
    if (company_mail_c != -1)
        company->mail = row[company_mail_c];
    if (company_phone_c != -1)
        company->phone_number = row[company_phone_c];
    if (company_fax_c != -1)
        company->fax = row[company_fax_c];
    data.companies.push_back(company);
    gtfs_data.company_map[company->uri] = company;
}


void PhysicalModeFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("physical_mode_id");
    name_c = csv.get_pos_col("physical_mode_name");
}

void PhysicalModeFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && id_c == -1) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one physical mode and no physical_mode_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::PhysicalMode* mode = new ed::types::PhysicalMode();
    mode->id = data.physical_modes.size() + 1;
    mode->name = row[name_c];
    mode->uri = row[id_c];
    data.physical_modes.push_back(mode);
    gtfs_data.physical_mode_map[mode->uri] = mode;
}

void CommercialModeFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("commercial_mode_id");
    name_c = csv.get_pos_col("commercial_mode_name");
}

void CommercialModeFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && id_c == -1) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one commercial mode and no commercial_mode_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::CommercialMode* commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = data.commercial_modes.size() + 1;
    commercial_mode->name = row[name_c];
    commercial_mode->uri = row[id_c];
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->uri] = commercial_mode;
}

void FusioParser::parse_files(Data& data) {
    parse<AgencyGtfsHandler>(data, "agency.txt", true);
    parse<ContributorFusioHandler>(data, "contributor.txt");
    parse<CompanyFusioHandler>(data, "company.txt");
    parse<PhysicalModeFusioHandler>(data, "physical_mode.txt");
    parse<CommercialModeFusioHandler>(data, "commercial_mode.txt");
    parse<StopsFusioHandler>(data, "stops.txt", true);
    parse<RouteFusioHandler>(data, "routes.txt", true);
    parse<TransfersFusioHandler>(data, "transfers.txt");
    parse<CalendarGtfsHandler>(data, "calendar.txt");
    parse<CalendarDatesGtfsHandler>(data, "calendar_dates.txt");
    parse<TripsFusioHandler>(data, "trips.txt", true);
    parse<StopTimeFusioHandler>(data, "stop_times.txt", true);
    parse<FrequenciesGtfsHandler>(data, "frequencies.txt");
}
}
}
