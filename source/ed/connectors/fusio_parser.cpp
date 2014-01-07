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
        ext_code_c = csv.get_pos_col("external_code");
        property_id_c = csv.get_pos_col("property_id");
        comment_id_c =  csv.get_pos_col("comment_id");
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

    auto it_property = gtfs_data.hasProperties_map.find(row[property_id_c]);
    if(it_property != gtfs_data.hasProperties_map.end()){
        if( return_wrapper.first != nullptr){
            return_wrapper.first->set_properties(it_property->second.properties());
        }
        if( return_wrapper.second != nullptr){
            return_wrapper.second->set_properties(it_property->second.properties());
        }
    }

    auto it_comment = gtfs_data.comment_map.find(row[comment_id_c]);
    if(it_comment != gtfs_data.comment_map.end()){
        if( return_wrapper.first != nullptr){
            return_wrapper.first->comment = it_comment->second;
        }
        if( return_wrapper.second != nullptr){
            return_wrapper.second->comment = it_comment->second;
        }
    }

    return return_wrapper;
}

void RouteFusioHandler::init(Data& ) {
    ext_code_c = csv.get_pos_col("external_code");
    route_id_c = csv.get_pos_col("route_id");
    route_name_c = csv.get_pos_col("route_name");
    is_forward_c = csv.get_pos_col("is_forward");
    line_id_c = csv.get_pos_col("line_id");
    comment_id_c = csv.get_pos_col("comment_id");
    contributor_id_c = csv.get_pos_col("contributor_id");
    ignored = 0;
}

void RouteFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(gtfs_data.route_map.find(row[route_id_c]) != gtfs_data.route_map.end()) {
            ignored++;
            LOG4CPLUS_WARN(logger, "dupplicate on route line " + row[route_id_c]);
            return;
     }
    ed::types::Line* ed_line = nullptr;
    if(has_col(line_id_c, row)) {
        auto it_line = gtfs_data.line_map.find(row[line_id_c]);
        if(it_line != gtfs_data.line_map.end()){
            ed_line = it_line->second;
        }
    }
    if( ed_line == nullptr){
        ignored++;
        LOG4CPLUS_WARN(logger, "Route orphan " + row[route_id_c]);
        return;
    }
    ed::types::Route* ed_route = new ed::types::Route();
    ed_route->line = ed_line;
    ed_route->uri = row[route_id_c];
    ed_route->external_code = row[ext_code_c];
    ed_route->name = row[route_name_c];

    if ( has_col(comment_id_c, row) ){
        auto it_comment = gtfs_data.comment_map.find(row[comment_id_c]);
        if(it_comment != gtfs_data.comment_map.end()){
            ed_route->comment = it_comment->second;
        }
    }

    gtfs_data.route_map[row[route_id_c]] = ed_route;
    data.routes.push_back(ed_route);
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
    comment_id_c = csv.get_pos_col("comment_id"),
    trip_propertie_id_c = csv.get_pos_col("trip_property_id"),
    odt_type_c = csv.get_pos_col("odt_type"),
    company_id_c = csv.get_pos_col("company_id"),
    odt_condition_id_c = csv.get_pos_col("odt_condition_id"),
    physical_mode_c = csv.get_pos_col("physical_mode_id"),
    ext_code_c = csv.get_pos_col("external_code");
}

void TripsFusioHandler::clean_and_delete(Data& data, ed::types::VehicleJourney* vj) {
    gtfs_data.vj_map.erase(vj->uri);

    data.vehicle_journeys.pop_back();
    delete vj;
}

ed::types::VehicleJourney* TripsFusioHandler::get_vj(Data& data, const csv_row& row, bool is_first_line){
    auto it = gtfs_data.route_map.find(row[id_c]);
    if (it == gtfs_data.route_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the route " + row[id_c]
                       + " referenced by trip " + row[trip_c]);
        ignored++;
        return nullptr;
    }

    ed::types::Route* route = it->second;

    auto vp_it = gtfs_data.vp_map.find(row[service_c]);
    if(vp_it == gtfs_data.vp_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the service " + row[service_c]
                       + " referenced by trip " + row[trip_c]);
        ignored++;
        return nullptr;
    }
    ed::types::ValidityPattern* vp_xx = vp_it->second;

    auto vj_it = gtfs_data.vj_map.find(row[trip_c]);
    if(vj_it != gtfs_data.vj_map.end()) {
        ignored_vj++;
        return nullptr;
    }
    ed::types::VehicleJourney* vj = new ed::types::VehicleJourney();
    vj->uri = row[trip_c];
    vj->external_code = vj->uri;
    if(has_col(headsign_c, row))
        vj->name = row[headsign_c];
    else
        vj->name = vj->uri;

    vj->validity_pattern = vp_xx;
    vj->adapted_validity_pattern = vp_xx;
    vj->journey_pattern = 0;
    vj->tmp_route = route;
    vj->tmp_line = vj->tmp_route->line;
    if(has_col(block_id_c, row))
        vj->block_id = row[block_id_c];
    else
        vj->block_id = "";

    gtfs_data.vj_map[vj->uri] = vj;

    data.vehicle_journeys.push_back(vj);
    return vj;
}

ed::types::VehicleJourney* TripsFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    ed::types::VehicleJourney* vj = TripsFusioHandler::get_vj(data, row, is_first_line);

    if (! vj)
        return nullptr;

    if (has_col(ext_code_c, row))
        vj->external_code = row[ext_code_c];

    //if a physical_mode is given we override the value
    if (has_col(physical_mode_c, row)) {
        auto itm = gtfs_data.physical_mode_map.find(row[physical_mode_c]);

        if (itm == gtfs_data.physical_mode_map.end()) {
            LOG4CPLUS_WARN(logger, "TripsFusioHandler : Impossible to find the physical mode " << row[physical_mode_c]
                           << " referenced by trip " << row[trip_c]);
            ignored++;
            clean_and_delete(data, vj);
            return nullptr;
        }
        vj->physical_mode = itm->second;
    }

    if (has_col(odt_condition_id_c, row)){
        auto it_odt_condition = gtfs_data.odt_conditions_map.find(row[odt_condition_id_c]);
        if(it_odt_condition != gtfs_data.odt_conditions_map.end()){
            vj->odt_message = it_odt_condition->second;
        }
    }

    if (has_col(trip_propertie_id_c, row)) {
        auto it_property = gtfs_data.hasVehicleProperties_map.find(row[trip_propertie_id_c]);
        if(it_property != gtfs_data.hasVehicleProperties_map.end()){
            vj->set_vehicles(it_property->second.vehicles());
        }
    }

    if (has_col(comment_id_c, row)) {
        auto it_comment = gtfs_data.comment_map.find(row[comment_id_c]);
        if(it_comment != gtfs_data.comment_map.end()){
            vj->comment = it_comment->second;
        }
    }

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

void CommentFusioHandler::init(Data&){
    id_c = csv.get_pos_col("comment_id");
    comment_c = csv.get_pos_col("comment_name");
}

void CommentFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one comment and no comment_id column");
        throw InvalidHeaders(csv.filename);
    }
    gtfs_data.comment_map[row[id_c]] = row[comment_c];
}


void OdtConditionsFusioHandler::init(Data&){
    odt_condition_id_c = csv.get_pos_col("odt_condition_id");
    odt_condition_c = csv.get_pos_col("odt_condition");
}

void OdtConditionsFusioHandler::handle_line(Data& , const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(odt_condition_id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one condition and no odt_condition_id_c column");
        throw InvalidHeaders(csv.filename);ctors/fusio_parser.cpp.REMOTE.22949.cpp
    }
    gtfs_data.odt_conditions_map[row[odt_condition_id_c]] = row[odt_condition_c];
}

void StopPropertiesFusioHandler::init(Data&){
    id_c = csv.get_pos_col("property_id");
    wheelchair_boarding_c = csv.get_pos_col("wheelchair_boarding");
    sheltered_c = csv.get_pos_col("sheltered");
    elevator_c = csv.get_pos_col("elevator");
    escalator_c = csv.get_pos_col("escalator");
    bike_accepted_c = csv.get_pos_col("bike_accepted");
    bike_depot_c = csv.get_pos_col("bike_depot");
    visual_announcement_c = csv.get_pos_col("visual_announcement");
    audible_announcement_c = csv.get_pos_col("audible_announcement");
    appropriate_escort_c = csv.get_pos_col("appropriate_escort");
    appropriate_signage_c = csv.get_pos_col("appropriate_signage");
}

void StopPropertiesFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one stop_properties and no stop_propertie_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto itm = gtfs_data.hasProperties_map.find(row[id_c]);
    if(itm == gtfs_data.hasProperties_map.end()){
        navitia::type::hasProperties has_properties;
        if(has_col(wheelchair_boarding_c, row) && row[wheelchair_boarding_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        if(has_col(sheltered_c, row) && row[sheltered_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::SHELTERED);
        if(has_col(elevator_c, row) && row[elevator_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::ELEVATOR);
        if(has_col(escalator_c, row) && row[escalator_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::ESCALATOR);
        if(has_col(bike_accepted_c, row) && row[bike_accepted_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
        if(has_col(bike_depot_c, row) && row[bike_depot_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::BIKE_DEPOT);
        if(has_col(visual_announcement_c, row) && row[visual_announcement_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
        if(has_col(audible_announcement_c, row) && row[audible_announcement_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
        if(has_col(appropriate_escort_c, row) && row[appropriate_escort_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
        if(has_col(appropriate_signage_c, row) && row[appropriate_signage_c] == "1")
            has_properties.set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
        gtfs_data.hasProperties_map[row[id_c]] = has_properties;
    }
}
void TripPropertiesFusioHandler::init(Data &){
    id_c = csv.get_pos_col("trip_property_id");
    wheelchair_accessible_c = csv.get_pos_col("wheelchair_accessible");
    bike_accepted_c = csv.get_pos_col("bike_accepted");
    air_conditioned_c = csv.get_pos_col("air_conditioned");
    visual_announcement_c = csv.get_pos_col("visual_announcement");
    audible_announcement_c = csv.get_pos_col("audible_announcement");
    appropriate_escort_c = csv.get_pos_col("appropriate_escort");
    appropriate_signage_c = csv.get_pos_col("appropriate_signage");
    school_vehicle_c = csv.get_pos_col("school_vehicle");
}

void TripPropertiesFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one stop_properties and no stop_propertie_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto itm = gtfs_data.hasVehicleProperties_map.find(row[id_c]);
    if(itm == gtfs_data.hasVehicleProperties_map.end()){
        navitia::type::hasVehicleProperties has_properties;
        if(has_col(wheelchair_accessible_c, row) && row[wheelchair_accessible_c] == "1")
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);
        if(has_col(bike_accepted_c, row) && row[bike_accepted_c] == "1")
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
        if(has_col(air_conditioned_c, row) && row[air_conditioned_c] == "1")
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::AIR_CONDITIONED);
        if(has_col(visual_announcement_c, row) && row[visual_announcement_c] == "1")
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::VISUAL_ANNOUNCEMENT);
        if(has_col(audible_announcement_c, row) && row[audible_announcement_c] == "1")
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::AUDIBLE_ANNOUNCEMENT);
        if(has_col(appropriate_escort_c, row) && row[appropriate_escort_c] == "1")
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_ESCORT);
        if(has_col(appropriate_signage_c, row) && row[appropriate_signage_c] == "1")
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_SIGNAGE);
        if(has_col(school_vehicle_c, row) && row[school_vehicle_c] == "1")
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::SCHOOL_VEHICLE);
        gtfs_data.hasVehicleProperties_map[row[id_c]] = has_properties;
    }
}

void FusioParser::parse_files(Data& data) {

    fill_default_company(data);
    fill_default_network(data);
    parse<AgencyGtfsHandler>(data, "agency.txt", true);
    parse<ContributorFusioHandler>(data, "contributors.txt");
    parse<CompanyFusioHandler>(data, "company.txt");
    parse<PhysicalModeFusioHandler>(data, "physical_modes.txt");
    parse<CommercialModeFusioHandler>(data, "commercial_modes.txt");
    parse<CommentFusioHandler>(data, "comments.txt");
    parse<LineFusioHandler>(data, "lines.txt");
    parse<StopPropertiesFusioHandler>(data, "stop_properties.txt");
    parse<StopsFusioHandler>(data, "stops.txt", true);
    parse<RouteFusioHandler>(data, "routes.txt", true);
    parse<TransfersFusioHandler>(data, "transfers.txt");
    parse<CalendarGtfsHandler>(data, "calendar.txt");
    parse<CalendarDatesGtfsHandler>(data, "calendar_dates.txt");
    parse<TripPropertiesFusioHandler>(data, "trip_properties.txt");
    parse<OdtConditionsFusioHandler>(data, "odt_conditions.txt");
    parse<TripsFusioHandler>(data, "trips.txt", true);
    parse<StopTimeFusioHandler>(data, "stop_times.txt", true);
    parse<FrequenciesGtfsHandler>(data, "frequencies.txt");
}
}
}
