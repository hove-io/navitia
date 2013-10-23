#include "ed_reader.h"

namespace ed{

namespace bg = boost::gregorian;
namespace nt = navitia::type;

void EdReader::fill(navitia::type::Data& data){

    pqxx::work work(*conn, "loading ED");

    this->fill_vector_to_ignore(data, work);
    this->fill_meta(data, work);

    this->fill_networks(data, work);
    this->fill_commercial_modes(data, work);
    this->fill_physical_modes(data, work);
    this->fill_companies(data, work);
    this->fill_contributors(data, work);

    this->fill_stop_areas(data, work);
    this->fill_stop_points(data, work);

    this->fill_lines(data, work);
    this->fill_routes(data, work);

    this->fill_journey_patterns(data, work);
    this->fill_journey_pattern_points(data, work);

    this->fill_validity_patterns(data, work);
    this->fill_vehicle_journeys(data, work);


    this->fill_stop_times(data, work);

    this->fill_admins(data, work);

    //@TODO: les connections ont des doublons, en attendant que ce soit corrigé, on ne les enregistre pas
    this->fill_stop_point_connections(data, work);
    this->fill_journey_pattern_point_connections(data, work);
    this->fill_poi_types(data, work);
    this->fill_pois(data, work);    
    this->fill_ways(data, work);
    this->fill_house_numbers(data, work);
    this->fill_vertex(data, work);
    this->fill_graph(data, work);
//    this->clean_graph(data, work);
    this->fill_graph_vls(data, work);

    //Charger les alias et les synonymes
    this->fill_alias(data, work);
    this->fill_synonyms(data, work);

    /// les relations admin et les autres objets
    this->build_rel_stop_point_admin(data, work);
    this->build_rel_stop_area_admin(data, work);
    this->build_rel_way_admin(data, work);
    this->build_rel_poi_admin(data, work);
    this->build_rel_admin_admin(data, work);
}



void EdReader::fill_admins(navitia::type::Data& nav_data, pqxx::work& work){
    std::string request = "SELECT id, name, uri, comment, post_code, insee, level, ST_X(coord::geometry) as lon, "
        "ST_Y(coord::geometry) as lat "
        "FROM navitia.admin";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::georef::Admin * admin = new navitia::georef::Admin;
        const_it["comment"].to(admin->comment);
        const_it["uri"].to(admin->uri);
        const_it["name"].to(admin->name);
        const_it["insee"].to(admin->insee);
        const_it["level"].to(admin->level);
        const_it["post_code"].to(admin->post_code);
        admin->coord.set_lon(const_it["lon"].as<double>());
        admin->coord.set_lat(const_it["lat"].as<double>());

        admin->idx = nav_data.geo_ref.admins.size();

        nav_data.geo_ref.admins.push_back(admin);
        this->admin_map[const_it["id"].as<idx_t>()] = admin;
    }

}

void EdReader::fill_meta(navitia::type::Data& nav_data, pqxx::work& work){
    std::string request = "SELECT beginning_date, end_date FROM navitia.parameters";
    pqxx::result result = work.exec(request);
    auto const_it = result.begin();
    bg::date begin = bg::from_string(const_it["beginning_date"].as<std::string>());
    //on ajoute un jour car "end" n'est pas inclus dans la période
    bg::date end = bg::from_string(const_it["end_date"].as<std::string>()) + bg::days(1);

    nav_data.meta.production_date = bg::date_period(begin, end);
    request = "SELECT ST_AsText(ST_MakeEnvelope("
              "(select min(ST_X(coord::geometry)) from georef.node),"
              "(select min(ST_Y(coord::geometry)) from georef.node),"
              "(select max(ST_X(coord::geometry)) from georef.node),"
              "(select max(ST_Y(coord::geometry)) from georef.node),"
              "4326)) as shape;";
    result = work.exec(request);
    const_it = result.begin();
    const_it["shape"].to(nav_data.meta.shape);
}

void EdReader::fill_networks(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri, comment FROM navitia.network";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::Network* network = new nt::Network();
        const_it["comment"].to(network->comment);
        const_it["uri"].to(network->uri);
        const_it["name"].to(network->name);

        network->idx = data.pt_data.networks.size();

        data.pt_data.networks.push_back(network);
        this->network_map[const_it["id"].as<idx_t>()] = network;
    }
}

void EdReader::fill_commercial_modes(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri FROM navitia.commercial_mode";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::CommercialMode* mode = new nt::CommercialMode();
        const_it["uri"].to(mode->uri);
        const_it["name"].to(mode->name);

        mode->idx = data.pt_data.commercial_modes.size();

        data.pt_data.commercial_modes.push_back(mode);
        this->commercial_mode_map[const_it["id"].as<idx_t>()] = mode;
    }
}

void EdReader::fill_physical_modes(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri FROM navitia.physical_mode";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::PhysicalMode* mode = new nt::PhysicalMode();
        const_it["uri"].to(mode->uri);
        const_it["name"].to(mode->name);

        mode->idx = data.pt_data.physical_modes.size();

        data.pt_data.physical_modes.push_back(mode);
        this->physical_mode_map[const_it["id"].as<idx_t>()] = mode;
    }
}

void EdReader::fill_contributors(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri FROM navitia.contributor";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::Contributor* contributor = new nt::Contributor();
        const_it["uri"].to(contributor->uri);
        const_it["name"].to(contributor->name);

        contributor->idx = data.pt_data.contributors.size();

        data.pt_data.contributors.push_back(contributor);
        this->contributor_map[const_it["id"].as<idx_t>()] = contributor;
    }
}

void EdReader::fill_companies(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri, comment FROM navitia.company";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::Company* company = new nt::Company();
        const_it["uri"].to(company->uri);
        const_it["name"].to(company->name);
        const_it["comment"].to(company->comment);

        company->idx = data.pt_data.companies.size();

        data.pt_data.companies.push_back(company);
        this->company_map[const_it["id"].as<idx_t>()] = company;
    }
}

void EdReader::fill_stop_areas(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT sa.id as id, sa.name as name, sa.uri as uri, sa.comment as comment,";
    request += "ST_X(sa.coord::geometry) as lon,";
    request += "ST_Y(sa.coord::geometry) as lat,";
    request += "pr.wheelchair_boarding as wheelchair_boarding,";
    request += "pr.sheltered as sheltered,";
    request += "pr.elevator as elevator,";
    request += "pr.escalator as escalator,";
    request += "pr.bike_accepted as bike_accepted,";
    request += "pr.bike_depot as bike_depot,";
    request += "pr.visual_announcement as visual_announcement,";
    request += "pr.audible_announcement as audible_announcement,";
    request += "pr.appropriate_escort as appropriate_escort,";
    request += "pr.appropriate_signage as appropriate_signage ";
    request += "FROM navitia.stop_area as sa, navitia.properties  as pr ";
    request += "where sa.properties_id=pr.id ";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::StopArea* sa = new nt::StopArea();
        const_it["uri"].to(sa->uri);
        const_it["name"].to(sa->name);
        const_it["comment"].to(sa->comment);
        sa->coord.set_lon(const_it["lon"].as<double>());
        sa->coord.set_lat(const_it["lat"].as<double>());
        if (const_it["wheelchair_boarding"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        }
        if (const_it["sheltered"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::SHELTERED);
        }
        if (const_it["elevator"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::ELEVATOR);
        }
        if (const_it["escalator"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::ESCALATOR);
        }
        if (const_it["bike_accepted"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
        }
        if (const_it["bike_depot"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::BIKE_DEPOT);
        }
        if (const_it["visual_announcement"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
        }
        if (const_it["audible_announcement"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
        }
        if (const_it["appropriate_escort"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
        }
        if (const_it["appropriate_signage"].as<bool>()){
            sa->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
        }

        sa->idx = data.pt_data.stop_areas.size();

        data.pt_data.stop_areas.push_back(sa);
        this->stop_area_map[const_it["id"].as<idx_t>()] = sa;
    }
}

void EdReader::fill_stop_points(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT sp.id as id, sp.name as name, sp.uri as uri,";
    request += "sp.comment as comment, ST_X(sp.coord::geometry) as lon, ST_Y(sp.coord::geometry) as lat,";
    request += "sp.fare_zone as fare_zone, sp.stop_area_id as stop_area_id,";
    request += "pr.wheelchair_boarding as wheelchair_boarding,";
    request += "pr.sheltered as sheltered,";
    request += "pr.elevator as elevator,";
    request += "pr.escalator as escalator,";
    request += "pr.bike_accepted as bike_accepted,";
    request += "pr.bike_depot as bike_depot,";
    request += "pr.visual_announcement as visual_announcement,";
    request += "pr.audible_announcement as audible_announcement,";
    request += "pr.appropriate_escort as appropriate_escort,";
    request += "pr.appropriate_signage as appropriate_signage ";
    request += "FROM navitia.stop_point as sp, navitia.properties  as pr ";
    request += "where sp.properties_id=pr.id";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::StopPoint* sp = new nt::StopPoint();
        const_it["uri"].to(sp->uri);
        const_it["name"].to(sp->name);
        const_it["comment"].to(sp->comment);
        const_it["fare_zone"].to(sp->fare_zone);
        sp->coord.set_lon(const_it["lon"].as<double>());
        sp->coord.set_lat(const_it["lat"].as<double>());
        if (const_it["wheelchair_boarding"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        }
        if (const_it["sheltered"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::SHELTERED);
        }
        if (const_it["elevator"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::ELEVATOR);
        }
        if (const_it["escalator"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::ESCALATOR);
        }
        if (const_it["bike_accepted"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
        }
        if (const_it["bike_depot"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::BIKE_DEPOT);
        }
        if (const_it["visual_announcement"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
        }
        if (const_it["audible_announcement"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
        }
        if (const_it["appropriate_escort"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
        }
        if (const_it["appropriate_signage"].as<bool>()){
            sp->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
        }
        sp->stop_area = stop_area_map[const_it["stop_area_id"].as<idx_t>()];
        sp->stop_area->stop_point_list.push_back(sp);

        data.pt_data.stop_points.push_back(sp);
        this->stop_point_map[const_it["id"].as<idx_t>()] = sp;
    }
}

void EdReader::fill_lines(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri, comment, code, color, network_id, commercial_mode_id "
        "FROM navitia.line";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::Line* line = new nt::Line();
        const_it["uri"].to(line->uri);
        const_it["name"].to(line->name);
        const_it["comment"].to(line->comment);
        const_it["code"].to(line->code);
        const_it["color"].to(line->color);

        line->network = network_map[const_it["network_id"].as<idx_t>()];
        line->network->line_list.push_back(line);

        line->commercial_mode = commercial_mode_map[const_it["commercial_mode_id"].as<idx_t>()];
        line->commercial_mode->line_list.push_back(line);

        data.pt_data.lines.push_back(line);
        this->line_map[const_it["id"].as<idx_t>()] = line;
    }
}

void EdReader::fill_routes(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri, comment, line_id "
        "FROM navitia.route";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::Route* route = new nt::Route();
        const_it["uri"].to(route->uri);
        const_it["name"].to(route->name);
        const_it["comment"].to(route->comment);

        route->line = line_map[const_it["line_id"].as<idx_t>()];
        route->line->route_list.push_back(route);

        data.pt_data.routes.push_back(route);
        this->route_map[const_it["id"].as<idx_t>()] = route;
    }
}

void EdReader::fill_journey_patterns(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri, comment, route_id, is_frequence, physical_mode_id "
        "FROM navitia.journey_pattern";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::JourneyPattern* journey_pattern = new nt::JourneyPattern();
        const_it["uri"].to(journey_pattern->uri);
        const_it["name"].to(journey_pattern->name);
        const_it["comment"].to(journey_pattern->comment);
        const_it["is_frequence"].to(journey_pattern->is_frequence);

        journey_pattern->route = route_map[const_it["route_id"].as<idx_t>()];
        journey_pattern->route->journey_pattern_list.push_back(journey_pattern);

        // attach the physical mode with database values
        journey_pattern->physical_mode = physical_mode_map[const_it["physical_mode_id"].as<idx_t>()];
        journey_pattern->physical_mode->journey_pattern_list.push_back(journey_pattern);

        // get commercial mode from the line of the route of the journey_pattern
        journey_pattern->commercial_mode = journey_pattern->route->line->commercial_mode;

        data.pt_data.journey_patterns.push_back(journey_pattern);
        this->journey_pattern_map[const_it["id"].as<idx_t>()] = journey_pattern;
    }
}


void EdReader::fill_journey_pattern_points(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri, comment, \"order\", stop_point_id, journey_pattern_id FROM navitia.journey_pattern_point";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::JourneyPatternPoint* jpp = new nt::JourneyPatternPoint();

        const_it["uri"].to(jpp->uri);
        //const_it["name"].to(jpp->name);
        //const_it["comment"].to(jpp->comment);
        const_it["\"order\""].to(jpp->order);

        jpp->journey_pattern = journey_pattern_map[const_it["journey_pattern_id"].as<idx_t>()];
        jpp->journey_pattern->journey_pattern_point_list.push_back(jpp);

        jpp->stop_point = stop_point_map[const_it["stop_point_id"].as<idx_t>()];

        jpp->idx = data.pt_data.journey_pattern_points.size();

        data.pt_data.journey_pattern_points.push_back(jpp);
        this->journey_pattern_point_map[const_it["id"].as<idx_t>()] = jpp;
    }
}


void EdReader::fill_validity_patterns(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT id, days FROM navitia.validity_pattern";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::ValidityPattern* validity_pattern = NULL;
        validity_pattern = new nt::ValidityPattern(data.meta.production_date.begin(), const_it["days"].as<std::string>());

        validity_pattern->idx = data.pt_data.validity_patterns.size();

        data.pt_data.validity_patterns.push_back(validity_pattern);
        this->validity_pattern_map[const_it["id"].as<idx_t>()] = validity_pattern;
    }
}



void EdReader::fill_stop_point_connections(nt::Data& data, pqxx::work& work){
//    std::string request = "SELECT departure_stop_point_id, destination_stop_point_id, connection_type_id, "
//                          "duration, max_duration FROM navitia.connection";
    std::string request = "SELECT conn.departure_stop_point_id as departure_stop_point_id,";
                request += "conn.destination_stop_point_id as destination_stop_point_id,";
                request += "conn.connection_type_id as connection_type_id,";
                request += "conn.duration as duration, conn.max_duration as max_duration,";
                request += "pr.wheelchair_boarding as wheelchair_boarding,";
                request += "pr.sheltered as sheltered,";
                request += "pr.elevator as elevator,";
                request += "pr.escalator as escalator,";
                request += "pr.bike_accepted as bike_accepted,";
                request += "pr.bike_depot as bike_depot,";
                request += "pr.visual_announcement as visual_announcement,";
                request += "pr.audible_announcement as audible_announcement,";
                request += "pr.appropriate_escort as appropriate_escort,";
                request += "pr.appropriate_signage as appropriate_signage ";
                request += "FROM navitia.connection as conn, navitia.properties  as pr ";
                request += "where conn.properties_id=pr.id ";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        auto it_departure = stop_point_map.find(const_it["departure_stop_point_id"].as<idx_t>());
        auto it_destination = stop_point_map.find(const_it["destination_stop_point_id"].as<idx_t>());
        if(it_departure!=stop_point_map.end() && it_destination!=stop_point_map.end()) {
            auto* stop_point_connection = new nt::StopPointConnection();
            stop_point_connection->departure = it_departure->second;
            stop_point_connection->destination = it_destination->second;
            stop_point_connection->connection_type = static_cast<nt::ConnectionType>(const_it["connection_type_id"].as<int>());
            stop_point_connection->duration = const_it["duration"].as<int>();
            stop_point_connection->max_duration = const_it["max_duration"].as<int>();

            if (const_it["wheelchair_boarding"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
            }
            if (const_it["sheltered"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::SHELTERED);
            }
            if (const_it["elevator"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::ELEVATOR);
            }
            if (const_it["escalator"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::ESCALATOR);
            }
            if (const_it["bike_accepted"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
            }
            if (const_it["bike_depot"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::BIKE_DEPOT);
            }
            if (const_it["visual_announcement"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
            }
            if (const_it["audible_announcement"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
            }
            if (const_it["appropriate_escort"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
            }
            if (const_it["appropriate_signage"].as<bool>()){
                stop_point_connection->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
            }

            data.pt_data.stop_point_connections.push_back(stop_point_connection);
        }
    }
}

void EdReader::fill_journey_pattern_point_connections(nt::Data& data, pqxx::work& work){
    std::string request = "select departure_journey_pattern_point_id,";
                request += "destination_journey_pattern_point_id,";
                request += "connection_kind_id,";
                request += "length ";
                request += "from navitia.journey_pattern_point_connection";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        auto it_departure = journey_pattern_point_map.find(const_it["departure_journey_pattern_point_id"].as<idx_t>());
        auto it_destination = journey_pattern_point_map.find(const_it["destination_journey_pattern_point_id"].as<idx_t>());
        if(it_departure!=journey_pattern_point_map.end() && it_destination!=journey_pattern_point_map.end()) {
            auto* jppc= new nt::JourneyPatternPointConnection();
            jppc->departure = it_departure->second;
            jppc->destination = it_destination->second;
            jppc->connection_type = static_cast<nt::ConnectionType>(const_it["connection_kind_id"].as<int>());
            jppc->duration = const_it["length"].as<int>();
            data.pt_data.journey_pattern_point_connections.push_back(jppc);
        }
    }
}


void EdReader::fill_vehicle_journeys(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT vj.id as id, vj.name as name, vj.uri as uri, vj.comment as comment,";
                request += "vj.company_id as company_id, ";
                request += "vj.journey_pattern_id as journey_pattern_id, vj.validity_pattern_id as validity_pattern_id,";
                request += "vj.adapted_validity_pattern_id as adapted_validity_pattern_id,";
                request += "vj.theoric_vehicle_journey_id as theoric_vehicle_journey_id ,";
                request += "vj.odt_type_id as odt_type_id, vj.odt_message as odt_message,";
                request += "vp.wheelchair_accessible as wheelchair_accessible,";
                request += "vp.bike_accepted as bike_accepted,";
                request += "vp.air_conditioned as air_conditioned,";
                request += "vp.visual_announcement as visual_announcement,";
                request += "vp.audible_announcement as audible_announcement,";
                request += "vp.appropriate_escort as appropriate_escort,";
                request += "vp.appropriate_signage as appropriate_signage,";
                request += "vp.school_vehicle as school_vehicle ";
                request += "FROM navitia.vehicle_journey as vj, navitia.vehicle_properties as vp ";
                request += "where vj.vehicle_properties_id = vp.id ";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::VehicleJourney* vj = new nt::VehicleJourney();

        const_it["uri"].to(vj->uri);
        const_it["name"].to(vj->name);
        const_it["comment"].to(vj->comment);
        const_it["odt_message"].to(vj->odt_message);
        vj->vehicle_journey_type = static_cast<nt::VehicleJourneyType>(const_it["odt_type_id"].as<int>());

        vj->journey_pattern = journey_pattern_map[const_it["journey_pattern_id"].as<idx_t>()];
        vj->journey_pattern->vehicle_journey_list.push_back(vj);

        vj->company = company_map[const_it["company_id"].as<idx_t>()];

        vj->adapted_validity_pattern = validity_pattern_map[const_it["adapted_validity_pattern_id"].as<idx_t>()];

        if(!const_it["validity_pattern_id"].is_null()){
            vj->validity_pattern = validity_pattern_map[const_it["validity_pattern_id"].as<idx_t>()];
        }

        if(!const_it["theoric_vehicle_journey_id"].is_null()){
            vj->theoric_vehicle_journey = vehicle_journey_map[const_it["theoric_vehicle_journey_id"].as<idx_t>()];
        }

        if (const_it["wheelchair_accessible"].as<bool>()){
            vj->set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);
        }
        if (const_it["bike_accepted"].as<bool>()){
            vj->set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
        }
        if (const_it["air_conditioned"].as<bool>()){
            vj->set_vehicle(navitia::type::hasVehicleProperties::AIR_CONDITIONED);
        }
        if (const_it["visual_announcement"].as<bool>()){
            vj->set_vehicle(navitia::type::hasVehicleProperties::VISUAL_ANNOUNCEMENT);
        }
        if (const_it["audible_announcement"].as<bool>()){
            vj->set_vehicle(navitia::type::hasVehicleProperties::AUDIBLE_ANNOUNCEMENT);
        }
        if (const_it["appropriate_escort"].as<bool>()){
            vj->set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_ESCORT);
        }
        if (const_it["appropriate_signage"].as<bool>()){
            vj->set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_SIGNAGE);
        }
        if (const_it["school_vehicle"].as<bool>()){
            vj->set_vehicle(navitia::type::hasVehicleProperties::SCHOOL_VEHICLE);
        }
        data.pt_data.vehicle_journeys.push_back(vj);
        this->vehicle_journey_map[const_it["id"].as<idx_t>()] = vj;
    }
}

void EdReader::fill_stop_times(nt::Data& data, pqxx::work& work){
    std::string request = "SELECT vehicle_journey_id, journey_pattern_point_id, arrival_time, departure_time, " // 0, 1, 2, 3
        "local_traffic_zone, start_time, end_time, headway_sec, odt, pick_up_allowed, " // 4, 5, 6, 7, 8, 9, 10
        "drop_off_allowed, is_frequency, date_time_estimated, comment " // 11, 12
        "FROM navitia.stop_time;";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){       
        nt::StopTime* stop = new nt::StopTime();
        const_it["arrival_time"].to(stop->arrival_time);
        const_it["departure_time"].to(stop->departure_time);
        const_it["local_traffic_zone"].to(stop->local_traffic_zone);
        const_it["start_time"].to(stop->start_time);
        const_it["end_time"].to(stop->end_time);
        const_it["headway_sec"].to(stop->headway_secs);
        const_it["comment"].to(stop->comment);

        stop->set_date_time_estimated(const_it["date_time_estimated"].as<bool>());

        stop->set_odt(const_it["odt"].as<bool>());

        stop->set_pick_up_allowed(const_it["pick_up_allowed"].as<bool>());

        stop->set_drop_off_allowed(const_it["drop_off_allowed"].as<bool>());

        stop->set_is_frequency(const_it["is_frequency"].as<bool>());

        stop->journey_pattern_point = journey_pattern_point_map[const_it["journey_pattern_point_id"].as<idx_t>()];

        //stop->vehicle_journey->stop_time_list.push_back(stop->id);
        auto vj = vehicle_journey_map[const_it["vehicle_journey_id"].as<idx_t>()];
        vj->stop_time_list.push_back(stop);
        stop->vehicle_journey = vj;
        if(stop->departure_time > 24*3600) {
            auto tmp_vp = new nt::ValidityPattern(vj->validity_pattern);
            tmp_vp->days <<= 1;
            auto find_vp_predicate = [&](nt::ValidityPattern* vp1) { return tmp_vp->days == vp1->days;};
            auto it = std::find_if(data.pt_data.validity_patterns.begin(),
                                data.pt_data.validity_patterns.end(), find_vp_predicate);
            if(it != data.pt_data.validity_patterns.end()) {
                stop->departure_validity_pattern = *(it);
            } else {
                tmp_vp->idx = data.pt_data.validity_patterns.size();
                data.pt_data.validity_patterns.push_back(tmp_vp);
                this->validity_pattern_map[tmp_vp->idx] = tmp_vp;
                stop->departure_validity_pattern = tmp_vp;
            }
        } else {
            stop->departure_validity_pattern = vj->validity_pattern;
        }
        if(stop->arrival_time > 24*3600) {
            auto tmp_vp = new nt::ValidityPattern(vj->validity_pattern);
            tmp_vp->days <<= 1;
            auto find_vp_predicate = [&](nt::ValidityPattern* vp1) { return tmp_vp->days == vp1->days;};
            auto it = std::find_if(data.pt_data.validity_patterns.begin(),
                                data.pt_data.validity_patterns.end(), find_vp_predicate);
            if(it != data.pt_data.validity_patterns.end()) {
                stop->arrival_validity_pattern = *(it);
            } else {
                tmp_vp->idx = data.pt_data.validity_patterns.size();
                data.pt_data.validity_patterns.push_back(tmp_vp);
                this->validity_pattern_map[tmp_vp->idx] = tmp_vp;
                stop->arrival_validity_pattern = tmp_vp;
            }
        } else {
            stop->arrival_validity_pattern = vj->validity_pattern;
        }

        data.pt_data.stop_times.push_back(stop);
    }

}

void EdReader::fill_poi_types(navitia::type::Data& data, pqxx::work& work){
    std::string request = "SELECT id, uri, name FROM navitia.poi_type WHERE uri <> 'bicycle_rental';";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::georef::POIType* poi_type = new navitia::georef::POIType();
        const_it["uri"].to(poi_type->uri);
        const_it["name"].to(poi_type->name);
        const_it["id"].to(poi_type->id);
        poi_type->idx = data.geo_ref.poitypes.size();
        data.geo_ref.poitypes.push_back(poi_type);
        this->poi_type_map[const_it["id"].as<idx_t>()] = poi_type;
    }
}

void EdReader::fill_pois(navitia::type::Data& data, pqxx::work& work){
    std::string request = "SELECT poi.id, poi.weight, ST_X(poi.coord::geometry) as lon, ST_Y(poi.coord::geometry) as lat,";
    request += " poi.name, poi.uri, poi.poi_type_id FROM navitia.poi poi, navitia.poi_type poi_type where poi.poi_type_id=poi_type.id and poi_type.uri <> 'bicycle_rental';";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::georef::POI* poi = new navitia::georef::POI();
        const_it["uri"].to(poi->uri);
        const_it["name"].to(poi->name);
        const_it["id"].to(poi->id);
        poi->coord.set_lon(const_it["lon"].as<double>());
        poi->coord.set_lat(const_it["lat"].as<double>());
        const_it["weight"].to(poi->weight);
        navitia::georef::POIType* poi_type = this->poi_type_map[const_it["poi_type_id"].as<idx_t>()];
        if(poi_type != NULL ){
            poi->poitype_idx = poi_type->idx;
        }
        this->poi_map[const_it["id"].as<idx_t>()] = poi;
        poi->idx = data.geo_ref.pois.size();
        data.geo_ref.pois.push_back(poi);
    }
}

void EdReader::fill_ways(navitia::type::Data& data, pqxx::work& work){
    std::string request = "SELECT id, name, uri, type FROM georef.way;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){        
        if(binary_search(this->way_no_ignore.begin(), this->way_no_ignore.end(), const_it["id"].as<idx_t>())){
            navitia::georef::Way* way = new navitia::georef::Way;
            const_it["uri"].to(way->uri);
            const_it["name"].to(way->name);
            const_it["id"].to(way->id);
            way->idx =data.geo_ref.ways.size();

            const_it["type"].to(way->way_type);
            data.geo_ref.ways.push_back(way);
            this->way_map[const_it["id"].as<idx_t>()] = way;
        }
    }    
}

void EdReader::fill_house_numbers(navitia::type::Data& , pqxx::work& work){
    std::string request = "SELECT way_id, ST_X(coord::geometry) as lon, ST_Y(coord::geometry) as lat, number, left_side FROM georef.house_number where way_id IS NOT NULL;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){        
        navitia::georef::HouseNumber hn;
        const_it["number"].to(hn.number);
        hn.coord.set_lon(const_it["lon"].as<double>());
        hn.coord.set_lat(const_it["lat"].as<double>());
        navitia::georef::Way* way = this->way_map[const_it["way_id"].as<idx_t>()];
        if (way != NULL){
            way->add_house_number(hn);
        }
    }
}

void EdReader::fill_vector_to_ignore(navitia::type::Data& , pqxx::work& work){
    navitia::georef::GeoRef geo_ref_temp;
    std::unordered_map<idx_t, uint64_t> osmid_idex;
    std::unordered_map<uint64_t, idx_t> node_map_temp;
    // chargement des vertex
    std::string request = "select id, ST_X(coord::geometry) as lon, ST_Y(coord::geometry) as lat from georef.node;";
    pqxx::result result = work.exec(request);
    uint64_t idx = 0;
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::georef::Vertex v;
        v.coord.set_lon(const_it["lon"].as<double>());
        v.coord.set_lat(const_it["lat"].as<double>());
        boost::add_vertex(v, geo_ref_temp.graph);
        node_map_temp[const_it["id"].as<uint64_t>()] = idx;
        osmid_idex[idx] = const_it["id"].as<uint64_t>();
        idx++;
    }
    // construction du graph temporaire
    request = "select e.source_node_id, target_node_id, e.way_id, ST_LENGTH(the_geog) AS leng,";
    request += "e.pedestrian_allowed as map,e.cycles_allowed as bike,e.cars_allowed as car from georef.edge e;";
    result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
            navitia::georef::Edge e;
            const_it["leng"].to(e.length);
            e.way_idx = const_it["way_id"].as<idx_t>();
            uint64_t source = node_map_temp[const_it["source_node_id"].as<uint64_t>()];
            uint64_t target = node_map_temp[const_it["target_node_id"].as<uint64_t>()];
            boost::add_edge(source, target, e, geo_ref_temp.graph);
    }

    std::vector<navitia::georef::vertex_t> component(boost::num_vertices(geo_ref_temp.graph));
    boost::connected_components(geo_ref_temp.graph, &component[0]);
    std::unordered_map<uint64_t, uint64_t> map_count;
    uint64_t principal_component = 0;
    // recherche de la composante principale
    for (navitia::georef::vertex_t i = 0; i != component.size(); ++i){
        int component_ = component[i];
        auto mc = map_count.find(component_);
        if(mc == map_count.end()){
            map_count[component_] = 0;
        }else{
            map_count[component_] = mc->second + 1 ;
            if (principal_component < map_count[component_]){
                principal_component = component_;
            }
        }
    }
    // remplissage de la liste des noeuds et edges à ne pas binariser
    for (navitia::georef::vertex_t i = 0;  i != component.size(); ++i){
        if (component[i] != principal_component){
            bool found = false;
            uint64_t source = osmid_idex[i];
            BOOST_FOREACH(navitia::georef::edge_t e, boost::out_edges(i, geo_ref_temp.graph)){
                uint64_t target = boost::target(e, geo_ref_temp.graph);
                edge_to_ignore.push_back(std::to_string(source) + std::to_string(target));
                node_to_ignore.push_back(source);
                node_to_ignore.push_back(target);
                found = true;
            }
            if (!found){
                node_to_ignore.push_back(source);
            }
        }
    }
    // remplissage de la liste des voies à binariser
    navitia::georef::edge_iterator i, end;
    for (tie(i, end) = boost::edges(geo_ref_temp.graph); i != end; ++i) {
        navitia::georef::vertex_t source_idx = boost::source(*i, geo_ref_temp.graph);
        navitia::georef::vertex_t target_idx = boost::target(*i, geo_ref_temp.graph);

        if ((principal_component == component[source_idx]) || (principal_component == component[target_idx])){
            navitia::georef::edge_t e;
            bool b;
            boost::tie(e,b) = boost::edge(source_idx, target_idx, geo_ref_temp.graph);
            navitia::georef::Edge edge = geo_ref_temp.graph[e];
            way_no_ignore.push_back(edge.way_idx);
        }
    }


    std::vector<uint64_t>::iterator it;
    std::sort(way_no_ignore.begin(), way_no_ignore.end());
    it = std::unique(way_no_ignore.begin(), way_no_ignore.end());
    way_no_ignore.resize( std::distance(way_no_ignore.begin(),it) );

    std::sort(node_to_ignore.begin(), node_to_ignore.end());
    it = std::unique(node_to_ignore.begin(), node_to_ignore.end());
    node_to_ignore.resize( std::distance(node_to_ignore.begin(),it) );

    std::vector<std::string>::iterator itstr;
    std::sort(edge_to_ignore.begin(), edge_to_ignore.end());
    itstr = std::unique(edge_to_ignore.begin(), edge_to_ignore.end());
    edge_to_ignore.resize( std::distance(edge_to_ignore.begin(),itstr) );
}

void EdReader::fill_vertex(navitia::type::Data& data, pqxx::work& work){
    std::string request = "select id, ST_X(coord::geometry) as lon, ST_Y(coord::geometry) as lat from georef.node;";
    pqxx::result result = work.exec(request);
    uint64_t idx = 0;
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        this->node_map[const_it["id"].as<uint64_t>()] = std::numeric_limits<uint64_t>::max();
        if(! binary_search(this->node_to_ignore.begin(), this->node_to_ignore.end(), const_it["id"].as<uint64_t>())){
            navitia::georef::Vertex v;
            v.coord.set_lon(const_it["lon"].as<double>());
            v.coord.set_lat(const_it["lat"].as<double>());
            boost::add_vertex(v, data.geo_ref.graph);
            this->node_map[const_it["id"].as<uint64_t>()] = idx;
            idx++;
        }
    }
    navitia::georef::vertex_t Conunt_v = boost::num_vertices(data.geo_ref.graph);    
    data.geo_ref.init_offset(Conunt_v);
}

void EdReader::fill_graph(navitia::type::Data& data, pqxx::work& work){
    std::string request = "select e.source_node_id, target_node_id, e.way_id, ST_LENGTH(the_geog) AS leng,";
                request += "e.pedestrian_allowed as map,e.cycles_allowed as bike,e.cars_allowed as car from georef.edge e;";
    pqxx::result result = work.exec(request);    
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::georef::Way* way = this->way_map[const_it["way_id"].as<uint64_t>()];
        std::string source, target;
        const_it["source_node_id"].to(source);
        const_it["target_node_id"].to(target);
        if ((way != NULL) && (! binary_search(this->edge_to_ignore.begin(), this->edge_to_ignore.end(), source+target))){
            navitia::georef::Edge e;
            const_it["leng"].to(e.length);
            e.way_idx = way->idx;
            uint64_t source = this->node_map[const_it["source_node_id"].as<uint64_t>()];
            uint64_t target = this->node_map[const_it["target_node_id"].as<uint64_t>()];
            if ((source != std::numeric_limits<uint64_t>::max()) && (target != std::numeric_limits<uint64_t>::max())){
                data.geo_ref.ways[way->idx]->edges.push_back(std::make_pair(source, target));
                boost::add_edge(source, target, e, data.geo_ref.graph);
                if (const_it["bike"].as<bool>()){
                    boost::add_edge(data.geo_ref.bike_offset + source, data.geo_ref.bike_offset + target, e, data.geo_ref.graph);
                }
                if (const_it["car"].as<bool>()){
                    boost::add_edge(data.geo_ref.car_offset + source, data.geo_ref.car_offset + target, e, data.geo_ref.graph);
                }
            }
        }
    }
}

void EdReader::fill_graph_vls(navitia::type::Data& data, pqxx::work& work){
    data.geo_ref.build_proximity_list();
    std::string request = "SELECT poi.id as id, ST_X(poi.coord::geometry) as lon,";
                request += "ST_Y(poi.coord::geometry) as lat";
                request += " FROM navitia.poi poi, navitia.poi_type poi_type";
                request += " where poi.poi_type_id=poi_type.id";
                request += " and poi_type.uri = 'bicycle_rental'";

    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::type::GeographicalCoord coord;
        coord.set_lon(const_it["lon"].as<double>());
        coord.set_lat(const_it["lat"].as<double>());
        try{
            navitia::georef::vertex_t v = data.geo_ref.nearest_vertex(coord, data.geo_ref.pl);
            navitia::georef::edge_t e = data.geo_ref.nearest_edge(coord, v);
            navitia::georef::Edge edge;
            edge.length = 0;
            edge.way_idx = data.geo_ref.graph[e].way_idx;
            boost::add_edge(v + data.geo_ref.vls_offset, v + data.geo_ref.bike_offset, edge, data.geo_ref.graph);
            boost::add_edge(v + data.geo_ref.bike_offset, v + data.geo_ref.vls_offset, edge, data.geo_ref.graph);
        }catch(...){
            std::cout<<"Impossible de trouver le noued le plus proche à la station vls poi_id = "<<const_it["id"].as<std::string>()<<std::endl;
        }
    }
}

void EdReader::fill_alias(navitia::type::Data& data, pqxx::work& work){
    std::string key, value;
    std::string request = "SELECT key, value FROM navitia.alias;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        const_it["key"].to(key);
        const_it["value"].to(value);
        data.geo_ref.alias[key]=value;
    }
}

void EdReader::fill_synonyms(navitia::type::Data& data, pqxx::work& work){
    std::string key, value;
    std::string request = "SELECT key, value FROM navitia.synonym;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        const_it["key"].to(key);
        const_it["value"].to(value);
        data.geo_ref.synonymes[key]=value;
    }
}

void EdReader::build_rel_stop_point_admin(navitia::type::Data& , pqxx::work& work){
    std::string request = "select admin_id, stop_point_id from navitia.rel_stop_point_admin;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::StopPoint* sp = this->stop_point_map[const_it["stop_point_id"].as<idx_t>()];
        navitia::georef::Admin* admin = this->admin_map[const_it["admin_id"].as<idx_t>()];
        if (admin != NULL){
            sp->admin_list.push_back(admin);
        }
    }
}

void EdReader::build_rel_stop_area_admin(navitia::type::Data& , pqxx::work& work){
    std::string request = "select admin_id, stop_area_id from navitia.rel_stop_area_admin;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        nt::StopArea* sa = this->stop_area_map[const_it["stop_area_id"].as<idx_t>()];
        navitia::georef::Admin* admin = this->admin_map[const_it["admin_id"].as<idx_t>()];
        if (admin!= NULL){
            sa->admin_list.push_back(admin);
        }
    }
}
void EdReader::build_rel_way_admin(navitia::type::Data&, pqxx::work& work){
    std::string request = "select admin_id, way_id from georef.rel_way_admin;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::georef::Way* way = this->way_map[const_it["way_id"].as<idx_t>()];
        if (way != NULL){
            navitia::georef::Admin* admin = this->admin_map[const_it["admin_id"].as<idx_t>()];
            if (admin != NULL){
                way->admin_list.push_back(admin);
            }
        }
    }
}

void EdReader::build_rel_poi_admin(navitia::type::Data&, pqxx::work& work){
    std::string request = "select admin_id, poi_id from navitia.rel_poi_admin;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::georef::POI* poi = this->poi_map[const_it["poi_id"].as<idx_t>()];
        if (poi != NULL){
            navitia::georef::Admin* admin = this->admin_map[const_it["admin_id"].as<idx_t>()];
            if ((admin != NULL)){
                poi->admin_list.push_back(admin);
            }
        }
    }
}

void EdReader::build_rel_admin_admin(navitia::type::Data&, pqxx::work& work){
    std::string request = "select master_admin_id, admin_id from navitia.rel_admin_admin;";
    pqxx::result result = work.exec(request);
    for(auto const_it = result.begin(); const_it != result.end(); ++const_it){
        navitia::georef::Admin* admin_master = this->admin_map[const_it["master_admin_id"].as<idx_t>()];
        if (admin_master != NULL){
            navitia::georef::Admin* admin = this->admin_map[const_it["admin_id"].as<idx_t>()];
            if ((admin != NULL)){
                admin_master->admin_list.push_back(admin);
            }
        }
    }
}

}//namespace

