#include "fusio_parser.h"

namespace ed { namespace connectors {

void AgencyFusioHandler::init(Data& data) {
    AgencyGtfsHandler::init(data); //don't forget to call the parent init
    ext_code_c = csv.get_pos_col("external_code");
}

ed::types::Network* AgencyFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto network = AgencyGtfsHandler::handle_line(data, row, is_first_line);

    if (! network)
        return nullptr;

    if (has_col(ext_code_c, row)) {
        network->external_code = row[ext_code_c];
    }
    return network;
}

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
            appropriate_signage_c = csv.get_pos_col("appropriate_signage"),
            ext_code_c = csv.get_pos_col("external_code");
}

//in fusio we want to delete all stop points without stop area
void StopsFusioHandler::handle_stop_point_without_area(Data& data) {
    //Deletion of the stop point without stop areas
    std::vector<size_t> erase_sp;
    for (int i = data.stop_points.size()-1; i >=0;--i) {
        if (data.stop_points[i]->stop_area == nullptr) {
            erase_sp.push_back(i);
        }
    }
    int num_elements = data.stop_points.size();
    for (size_t to_erase : erase_sp) {
        gtfs_data.stop_map.erase(data.stop_points[to_erase]->uri);
        delete data.stop_points[to_erase];
        data.stop_points[to_erase] = data.stop_points[num_elements - 1];
        num_elements--;
    }
    data.stop_points.resize(num_elements);
    LOG4CPLUS_INFO(logger, "Deletion of " << erase_sp.size() << " stop_point wihtout stop_area");
}

StopsGtfsHandler::stop_point_and_area StopsFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto return_wrapper = StopsGtfsHandler::handle_line(data, row, is_first_line);

    if (has_col(ext_code_c, row)) {
        if ( return_wrapper.second != nullptr )
            return_wrapper.second->external_code = row[ext_code_c];
        else if ( return_wrapper.first != nullptr )
                return_wrapper.first->external_code = row[ext_code_c];
    }
    //the additional data are only for stop area
    if ( return_wrapper.second == nullptr )
        return return_wrapper;

    if(has_col(sheltered_c, row) && row[sheltered_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::SHELTERED);
    if(has_col(elevator_c, row) && row[elevator_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::ELEVATOR);
    if(has_col(escalator_c, row) && row[escalator_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::ESCALATOR);
    if(has_col(bike_accepted_c, row) && row[bike_accepted_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    if(has_col(bike_depot_c, row) && row[bike_depot_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::BIKE_DEPOT);
    if(has_col(visual_announcement_c, row) && row[visual_announcement_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
    if(has_col(audible_announcement_c, row) && row[audible_announcement_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
    if(has_col(appropriate_escort_c, row) && row[appropriate_escort_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
    if(has_col(appropriate_signage_c, row) && row[appropriate_signage_c] == "1")
        return_wrapper.second->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);

    return return_wrapper;
}

void RouteFusioHandler::init(Data& data) {
    RouteGtfsHandler::init(data);
    commercial_mode_c = csv.get_pos_col("commercial_mode_id"),
            ext_code_c = csv.get_pos_col("external_code");
}

ed::types::Line* RouteFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto ed_line = RouteGtfsHandler::handle_line(data, row, is_first_line);

    if (! ed_line) {
        return nullptr;
    }

    if (has_col(ext_code_c, row))
        ed_line->external_code = row[ext_code_c];

    //if we have a commercial_mode column we update the value
    if(has_col(commercial_mode_c, row)) {
        std::unordered_map<std::string, ed::types::CommercialMode*>::iterator it;
        it = gtfs_data.commercial_mode_map.find(row[commercial_mode_c]);

        if(it != gtfs_data.commercial_mode_map.end())
            ed_line->commercial_mode = it->second;
    }

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

    if(has_col(wheelchair_c, row) && row[wheelchair_c] == "1")
        connection->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
    if(has_col(sheltered_c, row) && row[sheltered_c] == "1")
        connection->set_property(navitia::type::hasProperties::SHELTERED);
    if(has_col(elevator_c, row) && row[elevator_c] == "1")
        connection->set_property(navitia::type::hasProperties::ELEVATOR);
    if(has_col(escalator_c, row) && row[escalator_c] == "1")
        connection->set_property(navitia::type::hasProperties::ESCALATOR);
    if(has_col(bike_accepted_c, row) && row[bike_accepted_c] == "1")
        connection->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    if(has_col(bike_depot_c, row) && row[bike_depot_c] == "1")
        connection->set_property(navitia::type::hasProperties::BIKE_DEPOT);
    if(has_col(visual_announcement_c, row) && row[visual_announcement_c] == "1")
        connection->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
    if(has_col(audible_announcement_c, row) && row[audible_announcement_c] == "1")
        connection->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
    if(has_col(appropriate_escort_c, row) && row[appropriate_escort_c] == "1")
        connection->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
    if(has_col(appropriate_signage_c, row) && row[appropriate_signage_c] == "1")
        connection->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);

    if(has_col(real_time_c, row)) {
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
    if (has_col(date_time_estimated_c, row))
        stop_time->date_time_estimated = (row[date_time_estimated_c] == "1");
    else
        stop_time->date_time_estimated = false;

    if (has_col(desc_c, row))
        stop_time->comment = row[desc_c];

    if(has_col(itl_c, row)){
        int local_traffic_zone = str_to_int(row[itl_c]);
        if (local_traffic_zone > 0)
            stop_time->local_traffic_zone = local_traffic_zone;
        else
            stop_time->local_traffic_zone = std::numeric_limits<uint32_t>::max();
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
    physical_mode_c = csv.get_pos_col("physical_mode_id"),
    ext_code_c = csv.get_pos_col("external_code");
}

void TripsFusioHandler::clean_and_delete(Data& data, ed::types::VehicleJourney* vj) {
    gtfs_data.vj_map.erase(vj->uri);

    data.vehicle_journeys.pop_back();
    delete vj;
}

ed::types::VehicleJourney* TripsFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    ed::types::VehicleJourney* vj = TripsGtfsHandler::handle_line(data, row, is_first_line);

    if (! vj)
        return nullptr;

    if (has_col(ext_code_c, row))
        vj->external_code = row[ext_code_c];

    //if a physical_mode is given we override the value
    if (has_col(physical_mode_c, row)) {
        auto itm = gtfs_data.physical_mode_map.find(row[physical_mode_c]);

        if (itm == gtfs_data.physical_mode_map.end()) {
            LOG4CPLUS_WARN(logger, "TripsFusioHandler : Impossible to find the Gtfs mode " << row[physical_mode_c]
                           << " referenced by trip " << row[trip_c]);
            ignored++;
            clean_and_delete(data, vj);
            return nullptr;
        }
        vj->physical_mode = itm->second;
    }

    if (has_col(trip_desc_c, row))
        vj->comment = row[trip_desc_c];

    if (has_col(condition_c, row))
        vj->odt_message = row[condition_c];

    if(has_col(bike_accepted_c, row) && row[bike_accepted_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
    if(has_col(air_conditioned_c, row) && row[air_conditioned_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::AIR_CONDITIONED);
    if(has_col(visual_announcement_c, row) && row[visual_announcement_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::VISUAL_ANNOUNCEMENT);
    if(has_col(audible_announcement_c, row) && row[audible_announcement_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::AUDIBLE_ANNOUNCEMENT);
    if(has_col(appropriate_escort_c, row) && row[appropriate_escort_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_ESCORT);
    if(has_col(appropriate_signage_c, row) && row[appropriate_signage_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_SIGNAGE);
    if(has_col(school_vehicle_c, row) && row[school_vehicle_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::SCHOOL_VEHICLE);

    if(has_col(odt_type_c, row)){
        vj->vehicle_journey_type = static_cast<nt::VehicleJourneyType>(boost::lexical_cast<int>(row[odt_type_c]));
    }

    std::string company_s;
    if ((has_col(company_id_c, row)) && (!row[company_id_c].empty()))
        company_s = row[company_id_c];

    auto company_it = gtfs_data.company_map.find(company_s);
    if (company_it != gtfs_data.company_map.end())
        vj->company = company_it->second;

    return vj;
}

void ContributorFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("contributor_id");
    name_c = csv.get_pos_col("contributor_name");
}

void ContributorFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one contributor and no contributor_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::Contributor * contributor = new ed::types::Contributor();
    if (has_col(id_c, row)) {
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
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one company and no company_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::Company * company = new ed::types::Company();
    if(has_col(id_c, row)){
        company->uri = row[id_c];
    }else{
        company->uri = "default_company";
    }
    company->name = row[name_c];
    if (has_col(company_address_name_c, row))
        company->address_name = row[company_address_name_c];
    if (has_col(company_address_number_c, row))
        company->address_number = row[company_address_number_c];
    if (has_col(company_address_type_c, row))
        company->address_type_name = row[company_address_type_c];
    if (has_col(company_url_c, row))
        company->website = row[company_url_c];
    if (has_col(company_mail_c, row))
        company->mail = row[company_mail_c];
    if (has_col(company_phone_c, row))
        company->phone_number = row[company_phone_c];
    if (has_col(company_fax_c, row))
        company->fax = row[company_fax_c];
    data.companies.push_back(company);
    gtfs_data.company_map[company->uri] = company;
}

void PhysicalModeFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("physical_mode_id");
    name_c = csv.get_pos_col("physical_mode_name");
}

void PhysicalModeFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one physical mode and no physical_mode_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::PhysicalMode* mode = new ed::types::PhysicalMode();
    mode->id = row[id_c];
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
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one commercial mode and no commercial_mode_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::CommercialMode* commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = row[id_c];
    commercial_mode->name = row[name_c];
    commercial_mode->uri = row[id_c];
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->uri] = commercial_mode;
}

void FusioParser::parse_files(Data& data) {
    parse<AgencyGtfsHandler>(data, "agency.txt", true);
    parse<ContributorFusioHandler>(data, "contributors.txt");
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
