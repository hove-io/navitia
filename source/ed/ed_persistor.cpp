#include "ed_persistor.h"
#include "ed/connectors/fare_utils.h"

namespace bg = boost::gregorian;

namespace ed{

void EdPersistor::persist(const ed::Georef& data){

    this->lotus.start_transaction();
    LOG4CPLUS_INFO(logger, "Begin: TRUNCATE data!");
    this->clean_georef();
    LOG4CPLUS_INFO(logger, "Begin: add admins data");
    this->insert_admins(data);
    LOG4CPLUS_INFO(logger, "End: add admins data\n"
                           "Begin: add ways data");
    this->insert_ways(data);
    LOG4CPLUS_INFO(logger, "End: add ways data\n"
                           "Begin: add nodes data");
    this->insert_nodes(data);
    LOG4CPLUS_INFO(logger, "End: add nodes data\n"
                           "Begin: add house numbers data");
    this->insert_house_numbers(data);
    LOG4CPLUS_INFO(logger, "End: add house numbers data\n"
                           "Begin: add edges data");
    this->insert_edges(data);
    LOG4CPLUS_INFO(logger, "End: add edges data\n"
                           "Begin: relation admin way");
    this->build_relation_way_admin(data);
    LOG4CPLUS_INFO(logger, "End: relation admin way\n"
                           "Begin: add poitypes data");
    this->insert_poi_types(data);
    LOG4CPLUS_INFO(logger, "End: add poitypes data\n"
                           "Begin: add pois data");
    this->insert_pois(data);
    LOG4CPLUS_INFO(logger, "End: add pois data\n"
                           "Begin: update boundary admins");
    this->update_boundary();
    LOG4CPLUS_INFO(logger, "End: update boundary admins\n"
                           "Begin: Relations stop_area, stop_point et admins");
    this->build_relation();
    LOG4CPLUS_INFO(logger, "End: Relations stop_area, stop_point et admins\n"
                           "Begin: Fusion ways");
    this->build_ways();
    LOG4CPLUS_INFO(logger, "End: Fusion ways\n"
                           "Begin commit");
    this->lotus.commit();
    LOG4CPLUS_INFO(logger, "End: commit");
}

std::string EdPersistor::to_geografic_point(const navitia::type::GeographicalCoord& coord) const{
    std::stringstream geog;
    geog << std::cout.precision(10);
    geog.str("");
    geog <<"POINT("<<coord.lon()<<" "<<coord.lat()<<")";
    return geog.str();
}

std::string EdPersistor::to_geografic_linestring(const navitia::type::GeographicalCoord& source, const navitia::type::GeographicalCoord& target) const{
    std::stringstream geog;
    geog << std::cout.precision(10);
    geog.str("");
    geog << "LINESTRING("<<source.lon()<<" "<<source.lat()<<","<<target.lon()<<" "<<target.lat()<<")";
    return geog.str();
}

void EdPersistor::build_ways(){
    /// Add a name for the admins with an empty name
    PQclear(this->lotus.exec("SELECT georef.add_way_name();", "", PGRES_TUPLES_OK));
    /// Fusion ways by name and admin
    PQclear(this->lotus.exec("SELECT georef.fusion_ways_by_admin_name();", "", PGRES_TUPLES_OK));
    /// Moving admin and way relations, so that the pair (admin, way) is unique
    PQclear(this->lotus.exec("SELECT georef.insert_tmp_rel_way_admin();", "", PGRES_TUPLES_OK));
    /// Update way_id in table edge
    PQclear(this->lotus.exec("SELECT georef.update_edge();", "", PGRES_TUPLES_OK));
    /// Add ways not in fusion table
    PQclear(this->lotus.exec("SELECT georef.complete_fusion_ways();", "", PGRES_TUPLES_OK));
    /// Add ways where admin is nil
    PQclear(this->lotus.exec("SELECT georef.add_fusion_ways();", "", PGRES_TUPLES_OK));
    /// Update data in table 'rel_way_admin' by 'tmp_rel_way_admin'
    PQclear(this->lotus.exec("SELECT georef.insert_rel_way_admin();", "", PGRES_TUPLES_OK));
    /// Remove duplicate data in table of ways
    PQclear(this->lotus.exec("SELECT georef.clean_way();", "", PGRES_TUPLES_OK));
    /// Update way_id in housenumber table
    PQclear(this->lotus.exec("SELECT georef.update_house_number();", "", PGRES_TUPLES_OK));
    /// Update ways name
    PQclear(this->lotus.exec("SELECT georef.clean_way_name();", "", PGRES_TUPLES_OK));
    /// Update of admin cordinates  : Calcul of barycentre
    PQclear(this->lotus.exec("SELECT navitia.update_admin_coord();", "", PGRES_TUPLES_OK));
    /// Relation between admins
    PQclear(this->lotus.exec("SELECT navitia.match_admin_to_admin();", "", PGRES_TUPLES_OK));
}

void EdPersistor::insert_admins(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("navitia.admin", {"id", "name", "post_code", "insee", "level", "coord", "uri"});
    for(const auto& itm : data.admins){
        if(itm.second->is_used){
            this->lotus.insert({std::to_string(itm.second->id), itm.second->name,
                                    itm.second->postcode, itm.second->insee,
                                    itm.second->level, this->to_geografic_point(itm.second->coord), itm.second->insee});
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_ways(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.way", {"id", "name", "uri", "type"});
    for(const auto& itm : data.ways){
        std::vector<std::string> values;
        values.push_back(std::to_string(itm.second->id));
        values.push_back(itm.second->name);
        values.push_back(itm.first);
        values.push_back(itm.second->type);
        this->lotus.insert(values);
     }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_nodes(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.node", {"id","coord"});
    for(const auto& itm : data.nodes){
        if(itm.second->is_used){
            this->lotus.insert({std::to_string(itm.second->id), this->to_geografic_point(itm.second->coord)});
        }

     }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_house_numbers(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.house_number", {"coord", "number", "left_side"});
    for(const auto& itm : data.house_numbers) {
        this->lotus.insert({this->to_geografic_point(itm.second->coord), itm.second->number, std::to_string(str_to_int(itm.second->number) % 2 == 0)});
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::insert_edges(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.edge", {"source_node_id", "target_node_id", "way_id", "the_geog", "pedestrian_allowed",
                                    "cycles_allowed", "cars_allowed"});
    for(const auto& edge : data.edges){
        this->lotus.insert({std::to_string(edge.second->source->id), std::to_string(edge.second->target->id), std::to_string(edge.second->way->id),
                           this->to_geografic_linestring(edge.second->source->coord, edge.second->target->coord),
                           std::to_string(true), std::to_string(true), std::to_string(true)});
        this->lotus.insert({std::to_string(edge.second->target->id), std::to_string(edge.second->source->id), std::to_string(edge.second->way->id),
                           this->to_geografic_linestring(edge.second->target->coord, edge.second->source->coord),
                           std::to_string(true), std::to_string(true), std::to_string(true)});
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::insert_poi_types(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("navitia.poi_type", {"id", "uri", "name"});
    for(const auto& itm : data.poi_types) {
        this->lotus.insert({std::to_string(itm.second->id), itm.first, itm.second->name});
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::insert_pois(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("navitia.poi", {"id", "weight", "coord", "name", "uri", "poi_type_id"});
    for(const auto& itm : data.pois) {
        std::string poi_type("NULL");
        if(itm.second->poi_type != nullptr){
            poi_type = std::to_string(itm.second->poi_type->id);
        }
        this->lotus.insert({std::to_string(itm.second->id), std::to_string(itm.second->weight),
                           this->to_geografic_point(itm.second->coord),
                           itm.second->name, itm.first, poi_type});
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::build_relation_way_admin(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.rel_way_admin", {"admin_id", "way_id"});
    for(const auto& itm : data.ways){
        std::vector<std::string> values;
        values.push_back(std::to_string(itm.second->admin->id));
        values.push_back(std::to_string(itm.second->id));
        this->lotus.insert(values);
     }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::update_boundary(){
    PQclear(this->lotus.exec("SELECT georef.update_boundary(id) from navitia.admin;", "", PGRES_TUPLES_OK));
}

void EdPersistor::persist(const ed::Data& data, const navitia::type::MetaData& meta){

    this->lotus.start_transaction();

    std::cout << "début : vider toutes les tables (TRUNCATE)!" << std::endl;
    this->clean_db();

    std::cout << "début : block insert!" << std::endl;
    this->insert_metadata(meta);
    this->insert_networks(data.networks);
    this->insert_commercial_modes(data.commercial_modes);
    this->insert_physical_modes(data.physical_modes);
    this->insert_companies(data.companies);
    this->insert_contributors(data.contributors);

    this->insert_sa_sp_properties(data);
    this->insert_stop_areas(data.stop_areas);
    this->insert_stop_points(data.stop_points);

    this->insert_lines(data.lines);
    this->insert_routes(data.routes);
    this->insert_journey_patterns(data.journey_patterns);

    this->insert_validity_patterns(data.validity_patterns);
    this->insert_vehicle_properties(data.vehicle_journeys);
    this->insert_vehicle_journeys(data.vehicle_journeys);

    this->insert_journey_pattern_point(data.journey_pattern_points);

    this->insert_stop_times(data.stops);

    //@TODO: les connections ont des doublons, en attendant que ce soit corrigé, on ne les enregistre pas
    this->insert_stop_point_connections(data.stop_point_connections);
    this->insert_journey_pattern_point_connections(data.journey_pattern_point_connections);
    this->insert_alias(data.alias);
    this->insert_synonyms(data.synonymes);

    persist_fare(data);

    std::cout << "fin : block insert!" << std::endl;
    this->build_relation();
    this->lotus.commit();
    std::cout << "fin : commit!" << std::endl;
}


void EdPersistor::persist_fare(const ed::Data& data) {
    std::cout << "TRUNCATE fare tables" << std::endl;
    PQclear(this->lotus.exec("TRUNCATE navitia.origin_destination, navitia.transition, navitia.ticket, navitia.dated_ticket, navitia.od_ticket CASCADE"));
    this->insert_prices(data);
    this->insert_transitions(data);
    this->insert_origin_destination(data);
}

void EdPersistor::insert_metadata(const navitia::type::MetaData& meta){
    std::string request = "insert into navitia.parameters (beginning_date, end_date) VALUES ('" + bg::to_iso_extended_string(meta.production_date.begin()) +
        "', '" + bg::to_iso_extended_string(meta.production_date.end()) + "');";
    PQclear(this->lotus.exec(request));
}

void EdPersistor::build_relation(){
    PQclear(this->lotus.exec("SELECT georef.match_stop_area_to_admin()", "", PGRES_TUPLES_OK));
    PQclear(this->lotus.exec("SELECT georef.match_stop_point_to_admin();", "", PGRES_TUPLES_OK));
    PQclear(this->lotus.exec("SELECT georef.match_poi_to_admin();", "", PGRES_TUPLES_OK));
}

void EdPersistor::clean_georef(){
    PQclear(this->lotus.exec("truncate georef.node, georef.house_number, navitia.admin, georef.way, navitia.poi_type CASCADE;"));
}

void EdPersistor::clean_db(){
    PQclear(this->lotus.exec("TRUNCATE navitia.stop_area, navitia.line, navitia.company, navitia.physical_mode, navitia.contributor, navitia.alias,navitia.synonym,"
                            "navitia.commercial_mode, navitia.vehicle_properties, navitia.properties, navitia.validity_pattern, navitia.network, navitia.parameters, navitia.connection CASCADE"));
}

void EdPersistor::insert_networks(const std::vector<types::Network*>& networks){
    this->lotus.prepare_bulk_insert("navitia.network", {"id", "uri",
            "external_code", "name", "comment"});
    for(types::Network* net : networks){
        std::vector<std::string> values;
        values.push_back(std::to_string(net->idx));
        values.push_back(navitia::base64_encode(net->uri));
        values.push_back(net->external_code);
        values.push_back(net->name);
        values.push_back(net->comment);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_commercial_modes(const std::vector<types::CommercialMode*>& commercial_modes){
    this->lotus.prepare_bulk_insert("navitia.commercial_mode",
            {"id", "uri","name"});
    for(types::CommercialMode* mode : commercial_modes){
        std::vector<std::string> values;
        values.push_back(std::to_string(mode->idx));
        values.push_back(navitia::base64_encode(mode->uri));
        values.push_back(mode->name);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_physical_modes(const std::vector<types::PhysicalMode*>& physical_modes){
    this->lotus.prepare_bulk_insert("navitia.physical_mode",
            {"id", "uri", "name"});
    for(types::PhysicalMode* mode : physical_modes){
        std::vector<std::string> values;
        values.push_back(std::to_string(mode->idx));
        values.push_back(navitia::base64_encode(mode->uri));
        values.push_back(mode->name);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_companies(const std::vector<types::Company*>& companies){
    this->lotus.prepare_bulk_insert("navitia.company", {"id", "uri",
            "name", "comment", "address_name", "address_number",
            "address_type_name", "phone_number","mail", "website", "fax"});
    for(types::Company* company : companies){
        std::vector<std::string> values;
        values.push_back(std::to_string(company->idx));
        values.push_back(navitia::base64_encode(company->uri));
        values.push_back(company->name);
        values.push_back(company->comment);
        values.push_back(company->address_name);
        values.push_back(company->address_number);
        values.push_back(company->address_type_name);
        values.push_back(company->phone_number);
        values.push_back(company->mail);
        values.push_back(company->website);
        values.push_back(company->fax);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_contributors(const std::vector<types::Contributor*>& contributors){
    this->lotus.prepare_bulk_insert("navitia.contributor",
            {"id", "uri", "name"});
    for(types::Contributor* contributor : contributors){
        std::vector<std::string> values;
        values.push_back(std::to_string(contributor->idx));
        values.push_back(navitia::base64_encode(contributor->uri));
        values.push_back(contributor->name);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_sa_sp_properties(const ed::Data& data){
    this->lotus.prepare_bulk_insert("navitia.properties",
            {"id", "wheelchair_boarding", "sheltered", "elevator", "escalator",
             "bike_accepted","bike_depot", "visual_announcement",
             "audible_announcement", "appropriate_escort", "appropriate_signage"});
    std::vector<navitia::type::idx_t> to_insert;
    for(types::StopArea* sa : data.stop_areas){
        navitia::type::idx_t idx = sa->to_ulog();
        if(!binary_search(to_insert.begin(), to_insert.end(), idx)){
            std::vector<std::string> values;
            values.push_back(std::to_string(idx));
            values.push_back(std::to_string(sa->wheelchair_boarding()));
            values.push_back(std::to_string(sa->sheltered()));
            values.push_back(std::to_string(sa->elevator()));
            values.push_back(std::to_string(sa->escalator()));
            values.push_back(std::to_string(sa->bike_accepted()));
            values.push_back(std::to_string(sa->bike_depot()));
            values.push_back(std::to_string(sa->visual_announcement()));
            values.push_back(std::to_string(sa->audible_announcement()));
            values.push_back(std::to_string(sa->appropriate_escort()));
            values.push_back(std::to_string(sa->appropriate_signage()));
            this->lotus.insert(values);
            to_insert.push_back(idx);
            std::sort(to_insert.begin(), to_insert.end());
        }
    }
    for(types::StopPoint* sp : data.stop_points){
        navitia::type::idx_t idx = sp->to_ulog();
        if(!binary_search(to_insert.begin(), to_insert.end(), idx)){
            std::vector<std::string> values;
            values.push_back(std::to_string(idx));
            values.push_back(std::to_string(sp->wheelchair_boarding()));
            values.push_back(std::to_string(sp->sheltered()));
            values.push_back(std::to_string(sp->elevator()));
            values.push_back(std::to_string(sp->escalator()));
            values.push_back(std::to_string(sp->bike_accepted()));
            values.push_back(std::to_string(sp->bike_depot()));
            values.push_back(std::to_string(sp->visual_announcement()));
            values.push_back(std::to_string(sp->audible_announcement()));
            values.push_back(std::to_string(sp->appropriate_escort()));
            values.push_back(std::to_string(sp->appropriate_signage()));
            this->lotus.insert(values);
            to_insert.push_back(idx);
            std::sort(to_insert.begin(), to_insert.end());
        }
    }
    for(types::StopPointConnection* connection : data.stop_point_connections){
        navitia::type::idx_t idx = connection->to_ulog();
        if(!binary_search(to_insert.begin(), to_insert.end(), idx)){
            std::vector<std::string> values;
            values.push_back(std::to_string(idx));
            values.push_back(std::to_string(connection->wheelchair_boarding()));
            values.push_back(std::to_string(connection->sheltered()));
            values.push_back(std::to_string(connection->elevator()));
            values.push_back(std::to_string(connection->escalator()));
            values.push_back(std::to_string(connection->bike_accepted()));
            values.push_back(std::to_string(connection->bike_depot()));
            values.push_back(std::to_string(connection->visual_announcement()));
            values.push_back(std::to_string(connection->audible_announcement()));
            values.push_back(std::to_string(connection->appropriate_escort()));
            values.push_back(std::to_string(connection->appropriate_signage()));
            this->lotus.insert(values);
            to_insert.push_back(idx);
            std::sort(to_insert.begin(), to_insert.end());
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_areas(const std::vector<types::StopArea*>& stop_areas){
    this->lotus.prepare_bulk_insert("navitia.stop_area",
            {"id", "uri", "external_code", "name", "coord", "comment",
            "properties_id"});

    for(types::StopArea* sa : stop_areas){
        std::vector<std::string> values;
        values.push_back(std::to_string(sa->idx));
        values.push_back(navitia::base64_encode(sa->uri));
        values.push_back(sa->external_code);
        values.push_back(sa->name);
        values.push_back("POINT(" + std::to_string(sa->coord.lon()) + " " + std::to_string(sa->coord.lat()) + ")");
        values.push_back(sa->comment);
        values.push_back(std::to_string(sa->to_ulog()));
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_points(const std::vector<types::StopPoint*>& stop_points){
    this->lotus.prepare_bulk_insert("navitia.stop_point",
            {"id", "uri", "external_code", "name", "coord", "comment",
            "fare_zone", "stop_area_id","properties_id"});

    for(types::StopPoint* sp : stop_points){
        std::vector<std::string> values;
        values.push_back(std::to_string(sp->idx));
        values.push_back(navitia::base64_encode(sp->uri));
        values.push_back(sp->external_code);
        values.push_back(sp->name);
        values.push_back("POINT(" + std::to_string(sp->coord.lon()) + " " + std::to_string(sp->coord.lat()) + ")");
        values.push_back(sp->comment);
        values.push_back(std::to_string(sp->fare_zone));
        if(sp->stop_area != NULL){
            values.push_back(std::to_string(sp->stop_area->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(sp->to_ulog()));
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();

}
void EdPersistor::insert_lines(const std::vector<types::Line*>& lines){
    this->lotus.prepare_bulk_insert("navitia.line",
            {"id", "uri", "external_code", "name", "comment", "color", "code",
            "commercial_mode_id", "network_id"});

    for(types::Line* line : lines){
        std::vector<std::string> values;
        values.push_back(std::to_string(line->idx));
        values.push_back(navitia::base64_encode(line->uri));
        values.push_back(line->external_code);
        values.push_back(line->name);
        values.push_back(line->comment);
        values.push_back(line->color);
        values.push_back(line->code);
        if(line->commercial_mode != NULL){
            values.push_back(std::to_string(line->commercial_mode->idx));
        }else{
            values.push_back(lotus.null_value);
        }

        if(line->network != NULL){
            values.push_back(std::to_string(line->network->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();

}

void EdPersistor::insert_stop_point_connections(const std::vector<types::StopPointConnection*>& connections){
    this->lotus.prepare_bulk_insert("navitia.connection",
            {"departure_stop_point_id", "destination_stop_point_id",
            "connection_type_id", "display_duration","duration", "max_duration",
            "properties_id"});

    //@TODO properties!!
    for(types::StopPointConnection* co : connections){
        std::vector<std::string> values;
        values.push_back(std::to_string(co->departure->idx));
        values.push_back(std::to_string(co->destination->idx));
        values.push_back(std::to_string(static_cast<int>(co->connection_kind)));
        // la durée de la correspondance à afficher
        values.push_back(std::to_string(co->display_duration));
        // la durée réelle de la correspondance
        values.push_back(std::to_string(co->duration));
        values.push_back(std::to_string(co->max_duration));
        values.push_back(std::to_string(co->to_ulog()));
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_journey_pattern_point_connections(const std::vector<types::JourneyPatternPointConnection*>& connections){
    this->lotus.prepare_bulk_insert("navitia.journey_pattern_point_connection",
            {"departure_journey_pattern_point_id",
            "destination_journey_pattern_point_id", "connection_kind_id",
            "length"});

    //@TODO properties!!
    for(types::JourneyPatternPointConnection* co : connections){
        std::vector<std::string> values;
        values.push_back(std::to_string(co->departure->idx));
        values.push_back(std::to_string(co->destination->idx));
        values.push_back(std::to_string(static_cast<int>(co->connection_kind)));
        values.push_back(std::to_string(co->length));
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_routes(const std::vector<types::Route*>& routes){
    this->lotus.prepare_bulk_insert("navitia.route",
            {"id", "uri", "external_code", "name", "comment", "line_id"});

    for(types::Route* route : routes){
        std::vector<std::string> values;
        values.push_back(std::to_string(route->idx));
        values.push_back(navitia::base64_encode(route->uri));
        values.push_back(route->external_code);
        values.push_back(route->name);
        values.push_back(route->comment);
        if(route->line != NULL){
            values.push_back(std::to_string(route->line->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();

}

void EdPersistor::insert_journey_patterns(const std::vector<types::JourneyPattern*>& journey_patterns){
    this->lotus.prepare_bulk_insert("navitia.journey_pattern",
            {"id", "uri", "name", "comment", "physical_mode_id",
            "is_frequence", "route_id"});

    for(types::JourneyPattern* jp : journey_patterns){
        std::vector<std::string> values;
        values.push_back(std::to_string(jp->idx));
        values.push_back(navitia::base64_encode(jp->uri));
        values.push_back(jp->name);
        values.push_back(jp->comment);
        if (jp->physical_mode != NULL){
            values.push_back(std::to_string(jp->physical_mode->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(jp->is_frequence));
        if(jp->route != NULL){
            values.push_back(std::to_string(jp->route->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_validity_patterns(const std::vector<types::ValidityPattern*>& validity_patterns){
    this->lotus.prepare_bulk_insert("navitia.validity_pattern", {"id", "days"});

    for(types::ValidityPattern* vp : validity_patterns){
        std::vector<std::string> values;
        values.push_back(std::to_string(vp->idx));
        values.push_back(vp->days.to_string());
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_times(const std::vector<types::StopTime*>& stop_times){
    this->lotus.prepare_bulk_insert("navitia.stop_time",
            {"arrival_time", "departure_time", "local_traffic_zone", "start_time",
             "end_time", "headway_sec", "odt", "pick_up_allowed", "drop_off_allowed",
             "is_frequency", "journey_pattern_point_id", "vehicle_journey_id",
             "comment", "date_time_estimated"});

    for(types::StopTime* stop : stop_times){
        std::vector<std::string> values;
        values.push_back(std::to_string(stop->arrival_time));
        values.push_back(std::to_string(stop->departure_time));
        if(stop->local_traffic_zone != navitia::type::invalid_idx){
            values.push_back(std::to_string(stop->local_traffic_zone));
        }else{
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(stop->start_time));
        values.push_back(std::to_string(stop->end_time));
        values.push_back(std::to_string(stop->headway_secs));
        values.push_back(std::to_string(stop->ODT));
        values.push_back(std::to_string(stop->pick_up_allowed));
        values.push_back(std::to_string(stop->drop_off_allowed));
        values.push_back(std::to_string(stop->is_frequency));

        if(stop->journey_pattern_point != NULL){
            values.push_back(std::to_string(stop->journey_pattern_point->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(stop->vehicle_journey != NULL){
            values.push_back(std::to_string(stop->vehicle_journey->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        values.push_back(stop->comment);
        values.push_back(std::to_string(stop->date_time_estimated));
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_journey_pattern_point(const std::vector<types::JourneyPatternPoint*>& journey_pattern_points){
    this->lotus.prepare_bulk_insert("navitia.journey_pattern_point",
            {"id", "uri", "name", "comment", "\"order\"",
            "stop_point_id", "journey_pattern_id"});

    for(types::JourneyPatternPoint* jpp : journey_pattern_points){
        std::vector<std::string> values;
        values.push_back(std::to_string(jpp->idx));
        values.push_back(navitia::base64_encode(jpp->uri));
        values.push_back(jpp->name);
        values.push_back(jpp->comment);
        values.push_back(std::to_string(jpp->order));

        if(jpp->stop_point != NULL){
            values.push_back(std::to_string(jpp->stop_point->idx));
        }else{
            values.push_back(lotus.null_value);
        }

        if(jpp->journey_pattern != NULL){
            values.push_back(std::to_string(jpp->journey_pattern->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_vehicle_properties(const std::vector<types::VehicleJourney*>& vehicle_journeys){
    this->lotus.prepare_bulk_insert("navitia.vehicle_properties",
            {"id", "wheelchair_accessible", "bike_accepted", "air_conditioned",
            "visual_announcement", "audible_announcement", "appropriate_escort",
            "appropriate_signage", "school_vehicle"});

    std::vector<navitia::type::idx_t> to_insert;
    for(types::VehicleJourney* vj : vehicle_journeys){

        navitia::type::idx_t idx = vj->to_ulog();
        if(!binary_search(to_insert.begin(), to_insert.end(), idx)){
            std::vector<std::string> values;
            values.push_back(std::to_string(idx));
            values.push_back(std::to_string(vj->wheelchair_accessible()));
            values.push_back(std::to_string(vj->bike_accepted()));
            values.push_back(std::to_string(vj->air_conditioned()));
            values.push_back(std::to_string(vj->visual_announcement()));
            values.push_back(std::to_string(vj->audible_announcement()));
            values.push_back(std::to_string(vj->appropriate_escort()));
            values.push_back(std::to_string(vj->appropriate_signage()));
            values.push_back(std::to_string(vj->school_vehicle()));
            this->lotus.insert(values);
            to_insert.push_back(idx);
            std::sort(to_insert.begin(), to_insert.end());
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_vehicle_journeys(const std::vector<types::VehicleJourney*>& vehicle_journeys){
    this->lotus.prepare_bulk_insert("navitia.vehicle_journey",
            {"id", "uri", "external_code", "name", "comment", "validity_pattern_id",
             "adapted_validity_pattern_id", "company_id", "journey_pattern_id",
             "theoric_vehicle_journey_id", "vehicle_properties_id",
             "odt_type_id", "odt_message"});

    for(types::VehicleJourney* vj : vehicle_journeys){
        std::vector<std::string> values;
        values.push_back(std::to_string(vj->idx));
        values.push_back(navitia::base64_encode(vj->uri));
        values.push_back(vj->external_code);
        values.push_back(vj->name);
        values.push_back(vj->comment);
        if(vj->validity_pattern != NULL){
            values.push_back(std::to_string(vj->validity_pattern->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(vj->adapted_validity_pattern != NULL){
            values.push_back(std::to_string(vj->adapted_validity_pattern->idx));
        }else{
            values.push_back(std::to_string(vj->validity_pattern->idx));
        }
        if(vj->company != NULL){
            values.push_back(std::to_string(vj->company->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(vj->journey_pattern != NULL){
            values.push_back(std::to_string(vj->journey_pattern->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(vj->theoric_vehicle_journey != NULL){
            values.push_back(std::to_string(vj->theoric_vehicle_journey->idx));
        }else{
            //@TODO WTF??
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(vj->to_ulog()));
        values.push_back(std::to_string(static_cast<int>(vj->vehicle_journey_type)));
        values.push_back(vj->odt_message);
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_alias(const std::map<std::string, std::string>& alias){
    this->lotus.prepare_bulk_insert("navitia.alias", {"id", "key", "value"});
    int count=1;
    std::map <std::string, std::string> ::const_iterator it = alias.begin();
    while(it != alias.end()){
        std::vector<std::string> values;
        values.push_back(std::to_string(count));
        values.push_back(it->first);
        values.push_back(it->second);
        this->lotus.insert(values);
        count++;
        ++it;
    }
    this->lotus.finish_bulk_insert();

}

void EdPersistor::insert_synonyms(const std::map<std::string, std::string>& synonyms){
    this->lotus.prepare_bulk_insert("navitia.synonym", {"id", "key", "value"});
    int count = 1;
        std::map <std::string, std::string>::const_iterator it = synonyms.begin();
    while(it != synonyms.end()){
        std::vector<std::string> values;
        values.push_back(std::to_string(count));
        values.push_back(it->first);
        values.push_back(it->second);
        this->lotus.insert(values);
        count++;
        ++it;
    }
    this->lotus.finish_bulk_insert();
}

// TODO:
// before adding in bd a transition or an od fare, we should check if object filtering the transition is in ed (the line, the network, ...)

void EdPersistor::insert_transitions(const ed::Data& data) {

    this->lotus.prepare_bulk_insert("navitia.transition",
            {"id", "before_change", "after_change", "start_trip", "end_trip",
            "global_condition", "ticket_id"});
    size_t count = 1;
    std::vector<std::vector<std::string>> null_ticket_vector;
    for (const auto& transition_tuple: data.transitions) {
        const navitia::fare::State& start = std::get<0>(transition_tuple);
        const navitia::fare::State& end = std::get<1>(transition_tuple);

        const navitia::fare::Transition& transition = std::get<2>(transition_tuple);

        std::vector<std::string> values;
        values.push_back(std::to_string(count++));
        values.push_back(ed::connectors::to_string(start));
        values.push_back(ed::connectors::to_string(end));
        std::string start_cond;
        std::string sep = "";
        for (const auto& c: transition.start_conditions) {
            start_cond += c.to_string() + sep;
            sep = "&";
        }
        values.push_back(start_cond);

        std::string end_cond;
        sep = "";
        for (const auto& c: transition.end_conditions) {
            end_cond += c.to_string() + sep;
            sep = "&";
        }
        values.push_back(end_cond);
        values.push_back(ed::connectors::to_string(transition.global_condition));
        if (! transition.ticket_key.empty()) //we do not add empty ticket, to have null in db
            values.push_back(transition.ticket_key);
        else {
            null_ticket_vector.push_back(values);
            continue;
        }

        std::cout << "transition : " << boost::algorithm::join(values, ",") << std::endl;
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();

    //the postgres connector does not handle well null values, so we add the transition without tickets in a separate bulk
    this->lotus.prepare_bulk_insert("navitia.transition",
            {"id", "before_change", "after_change", "start_trip", "end_trip",
            "global_condition",});
    for (const auto& null_ticket: null_ticket_vector) {
        this->lotus.insert(null_ticket);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_prices(const ed::Data& data) {
    this->lotus.prepare_bulk_insert("navitia.ticket",
            {"ticket_key", "ticket_title", "ticket_comment"});
    for (const auto& ticket_it: data.fare_map) {
        const navitia::fare::DateTicket& tickets = ticket_it.second;

        assert(! tickets.tickets.empty()); //by construction there has to be at least one ticket
        std::vector<std::string> values {
            ticket_it.first,
            tickets.tickets.front().ticket.caption,
            tickets.tickets.front().ticket.comment
        };
        std::cout << "ticket : " << boost::algorithm::join(values, ",") << std::endl;
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();

    //we add the dated ticket afterward
    std::cout << "dated ticket : " << std::endl;
    this->lotus.prepare_bulk_insert("navitia.dated_ticket", {"id", "ticket_id", "valid_from", "valid_to", "ticket_price", "comments", "currency"});
    int count = 0;
    for (const auto& ticket_it: data.fare_map) {
        const navitia::fare::DateTicket& tickets = ticket_it.second;

        for (const auto& dated_ticket: tickets.tickets) {
            const auto& start = dated_ticket.validity_period.begin();
            const auto& last = dated_ticket.validity_period.end();
            std::vector<std::string> values {
                std::to_string(count++),
                dated_ticket.ticket.key,
                bg::to_iso_extended_string(start),
                bg::to_iso_extended_string(last),
                std::to_string(dated_ticket.ticket.value.value),
                dated_ticket.ticket.caption,
                dated_ticket.ticket.currency
            };
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_origin_destination(const ed::Data& data) {
    this->lotus.prepare_bulk_insert("navitia.origin_destination",
            {"id", "origin_id", "origin_mode", "destination_id", "destination_mode"});
    size_t cpt(0); //id of the od
    for (const auto& origin_ticket: data.od_tickets) {
        for (const auto& destination_ticket: origin_ticket.second) {
            std::vector<std::string> values {
                std::to_string(cpt++),
                origin_ticket.first.value, //origin
                ed::connectors::to_string(origin_ticket.first.type), //origin mode
                destination_ticket.first.value, //destination
                ed::connectors::to_string(destination_ticket.first.type), //destination mode
            };
            std::cout << "origin_destination : " << boost::algorithm::join(values, ",") << std::endl;
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();

    std::cout << "links to tickets" << std::endl;
    //now we take care of the matching od ticket
    this->lotus.prepare_bulk_insert("navitia.od_ticket", {"id", "od_id", "ticket_id"});
    cpt = 0;
    size_t od_ticket_cpt(0);
    for (const auto& origin_ticket: data.od_tickets) {
        for (const auto& destination_ticket: origin_ticket.second) {
            for (const auto& ticket: destination_ticket.second) {
                std::vector<std::string> values {
                    std::to_string(++od_ticket_cpt),
                    std::to_string(cpt),
                    ticket
                };
                std::cout << "origin_destination : " << boost::algorithm::join(values, ",") << std::endl;
                this->lotus.insert(values);
            }
            cpt++;
        }
    }
    this->lotus.finish_bulk_insert();
}

}//namespace
