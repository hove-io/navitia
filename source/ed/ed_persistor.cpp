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

#include "ed_persistor.h"
#include "ed/connectors/fare_utils.h"

namespace bg = boost::gregorian;

namespace ed{

void EdPersistor::persist_pois(const ed::Georef& data){
    this->lotus.start_transaction();
    LOG4CPLUS_INFO(logger, "Begin: TRUNCATE data!");
    this->clean_poi();
    LOG4CPLUS_INFO(logger, "Begin: add poitypes data");
    this->insert_poi_types(data);
    LOG4CPLUS_INFO(logger, "End: add poitypes data");
    LOG4CPLUS_INFO(logger, "Begin: add pois data");
    this->insert_pois(data);
    LOG4CPLUS_INFO(logger, "End: add pois data");
    LOG4CPLUS_INFO(logger, "Begin: add pois data");
    this->insert_poi_properties(data);
    LOG4CPLUS_INFO(logger, "End: add pois data");
    LOG4CPLUS_INFO(logger, "Begin commit");
    this->lotus.commit();
    LOG4CPLUS_INFO(logger, "End: commit");
}

void EdPersistor::persist(const ed::Georef& data){

    this->lotus.start_transaction();
    LOG4CPLUS_INFO(logger, "Begin: TRUNCATE data!");
    this->clean_georef();
    LOG4CPLUS_INFO(logger, "Begin: add admins data");
    this->insert_admins(data);
    LOG4CPLUS_INFO(logger, "End: add admins data");
    LOG4CPLUS_INFO(logger, "Begin: add ways data");
    this->insert_ways(data);
    LOG4CPLUS_INFO(logger, "End: add ways data");
    LOG4CPLUS_INFO(logger, "Begin: add nodes data");
    this->insert_nodes(data);
    LOG4CPLUS_INFO(logger, "End: add nodes data");
    LOG4CPLUS_INFO(logger, "Begin: add house numbers data");
    this->insert_house_numbers(data);
    LOG4CPLUS_INFO(logger, "End: add house numbers data");
    LOG4CPLUS_INFO(logger, "Begin: add edges data");
    this->insert_edges(data);
    LOG4CPLUS_INFO(logger, "End: add edges data");
    LOG4CPLUS_INFO(logger, "Begin: relation admin way");
    this->build_relation_way_admin(data);
    LOG4CPLUS_INFO(logger, "End: relation admin way");
    LOG4CPLUS_INFO(logger, "Begin commit");
    this->lotus.commit();
    LOG4CPLUS_INFO(logger, "End: commit");
}

std::string EdPersistor::to_geographic_point(const navitia::type::GeographicalCoord& coord) const{
    std::stringstream geog;
    geog << std::setprecision(10)<<"POINT("<<coord.lon()<<" "<<coord.lat()<<")";
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
    PQclear(this->lotus.exec("SELECT georef.update_admin_coord();", "", PGRES_TUPLES_OK));
    /// Relation between admins
    PQclear(this->lotus.exec("SELECT georef.match_admin_to_admin();", "", PGRES_TUPLES_OK));
}

void EdPersistor::insert_admins(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.admin",
            {"id", "name", "post_code", "insee", "level", "coord", "uri"});
    for(const auto& itm : data.admins){
        if(itm.second->is_used){
            this->lotus.insert({std::to_string(itm.second->id), itm.second->name,
                                    itm.second->postcode, itm.second->insee,
                               itm.second->level, this->to_geographic_point(itm.second->coord),"admin:" + itm.second->insee});
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_ways(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.way", {"id", "name", "uri", "type"});
    for(const auto& itm : data.ways){
        if(itm.second->is_used){
            std::vector<std::string> values;
            values.push_back(std::to_string(itm.second->id));
            values.push_back(itm.second->name);
            values.push_back(itm.first);
            values.push_back(itm.second->type);
            this->lotus.insert(values);
        }
     }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_nodes(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.node", {"id","coord"});
    for(const auto& itm : data.nodes){
        if(itm.second->is_used){
            this->lotus.insert({std::to_string(itm.second->id), this->to_geographic_point(itm.second->coord)});
        }

     }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_house_numbers(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.house_number", {"coord", "number", "left_side","way_id"});

    for(const auto& itm : data.house_numbers) {
        std::string way_id("NULL");
        if(itm.second->way != nullptr){
            way_id = std::to_string(itm.second->way->id);
        }
        this->lotus.insert({this->to_geographic_point(itm.second->coord),
                itm.second->number,
                std::to_string(str_to_int(itm.second->number) % 2 == 0), way_id});
    }
    lotus.finish_bulk_insert();
}

std::string coord_to_string(const navitia::type::GeographicalCoord& coord){
    std::stringstream geog;
    geog << std::setprecision(10) << coord.lon() << " " << coord.lat();
    return geog.str();
}

void EdPersistor::insert_edges(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.edge",
            {"source_node_id", "target_node_id", "way_id", "the_geog",
            "pedestrian_allowed", "cycles_allowed", "cars_allowed"});
    const auto bool_str = std::to_string(true);
    size_t to_insert_count = 0;
    size_t all_count = data.edges.size();
    for(const auto& edge : data.edges){
        const auto source_str = std::to_string(edge.second->source->id);
        const auto target_str = std::to_string(edge.second->target->id);
        const auto way_str = std::to_string(edge.second->way->id);
        const auto source_coord = coord_to_string(edge.second->source->coord);
        const auto target_coord = coord_to_string(edge.second->target->coord);

        this->lotus.insert({source_str, target_str, way_str,
                           "LINESTRING(" + source_coord + "," + target_coord + ")",
                            bool_str, bool_str, bool_str});
        this->lotus.insert({target_str, source_str, way_str,
                    "LINESTRING(" + target_coord + "," + source_coord + ")",
                    bool_str, bool_str, bool_str});
        ++to_insert_count;
        if(to_insert_count % 150000 == 0) {
            lotus.finish_bulk_insert();
             LOG4CPLUS_INFO(logger, to_insert_count<<"/"<<all_count<<" edges inserées");
            this->lotus.prepare_bulk_insert("georef.edge",
                    {"source_node_id", "target_node_id", "way_id", "the_geog",
                    "pedestrian_allowed", "cycles_allowed", "cars_allowed"});
        }
    }
    lotus.finish_bulk_insert();
    LOG4CPLUS_INFO(logger, to_insert_count<<"/"<<all_count<<" edges inserées");
}

void EdPersistor::insert_poi_types(const Georef &data){
    this->lotus.prepare_bulk_insert("georef.poi_type", {"id", "uri", "name"});
    for(const auto& itm : data.poi_types) {
        this->lotus.insert({std::to_string(itm.second->id), "poi_type:" + itm.first, itm.second->name});
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::insert_pois(const Georef &data){
    this->lotus.prepare_bulk_insert("georef.poi",
    {"id", "weight", "coord", "name", "uri", "poi_type_id", "visible", "address_number", "address_name"});
    for(const auto& itm : data.pois) {
        std::string poi_type("NULL");
        if(itm.second->poi_type != nullptr){
            poi_type = std::to_string(itm.second->poi_type->id);
        }
        this->lotus.insert({std::to_string(itm.second->id),
                std::to_string(itm.second->weight),
                this->to_geographic_point(itm.second->coord),
                itm.second->name, "poi:" + itm.first, poi_type, std::to_string(itm.second->visible),
                itm.second->address_number, itm.second->address_name});
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::insert_poi_properties(const Georef &data){
    this->lotus.prepare_bulk_insert("georef.poi_properties", {"poi_id","key","value"});
    for(const auto& itm : data.pois){
        for(auto property : itm.second->properties){
            this->lotus.insert({std::to_string(itm.second->id),property.first, property.second});
        }
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::build_relation_way_admin(const ed::Georef& data){
    this->lotus.prepare_bulk_insert("georef.rel_way_admin", {"admin_id", "way_id"});
    for(const auto& itm : data.ways){
        if(itm.second->is_used){
            std::vector<std::string> values;
            values.push_back(std::to_string(itm.second->admin->id));
            values.push_back(std::to_string(itm.second->id));
            this->lotus.insert(values);
        }
     }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::persist(const ed::Data& data, const navitia::type::MetaData& meta){

    this->lotus.start_transaction();

    LOG4CPLUS_INFO(logger, "Begin: clean db");
    this->clean_db();
    LOG4CPLUS_INFO(logger, "End: clean db");
    LOG4CPLUS_INFO(logger, "Begin: insert metadata");
    this->insert_metadata(meta);
    LOG4CPLUS_INFO(logger, "End: insert metadata");
    LOG4CPLUS_INFO(logger, "Begin: insert networks");
    this->insert_networks(data.networks);
    LOG4CPLUS_INFO(logger, "End: insert networks");
    LOG4CPLUS_INFO(logger, "Begin: insert commercial_modes");
    this->insert_commercial_modes(data.commercial_modes);
    LOG4CPLUS_INFO(logger, "End: insert commercial_modes");
    LOG4CPLUS_INFO(logger, "Begin: insert physical_modes");
    this->insert_physical_modes(data.physical_modes);
    LOG4CPLUS_INFO(logger, "End: insert physical_modes");
    LOG4CPLUS_INFO(logger, "Begin: insert companies");
    this->insert_companies(data.companies);
    LOG4CPLUS_INFO(logger, "End: insert companies");
    LOG4CPLUS_INFO(logger, "Begin: insert contributors");
    this->insert_contributors(data.contributors);
    LOG4CPLUS_INFO(logger, "End: insert contributors");
    LOG4CPLUS_INFO(logger, "Begin: insert stops properties");
    this->insert_sa_sp_properties(data);
    LOG4CPLUS_INFO(logger, "End: insert stops properties");
    LOG4CPLUS_INFO(logger, "Begin: insert stop areas");
    this->insert_stop_areas(data.stop_areas);
    LOG4CPLUS_INFO(logger, "End: insert stop areas");
    LOG4CPLUS_INFO(logger, "Begin: insert stop points");
    this->insert_stop_points(data.stop_points);
    LOG4CPLUS_INFO(logger, "End: insert stop points");
    LOG4CPLUS_INFO(logger, "Begin: insert lines");
    this->insert_lines(data.lines);
    LOG4CPLUS_INFO(logger, "End: insert lines");
    LOG4CPLUS_INFO(logger, "Begin: insert routes");
    this->insert_routes(data.routes);
    LOG4CPLUS_INFO(logger, "End: insert routes");
    LOG4CPLUS_INFO(logger, "Begin: insert journey patterns");
    this->insert_journey_patterns(data.journey_patterns);
    LOG4CPLUS_INFO(logger, "End: insert journey patterns");
    LOG4CPLUS_INFO(logger, "Begin: insert validity patterns");
    this->insert_validity_patterns(data.validity_patterns);
    LOG4CPLUS_INFO(logger, "End: insert validity patterns");
    LOG4CPLUS_INFO(logger, "Begin: insert vehicle properties");
    this->insert_vehicle_properties(data.vehicle_journeys);
    LOG4CPLUS_INFO(logger, "End: insert vehicle properties");
    LOG4CPLUS_INFO(logger, "Begin: insert vehicle journeys");
    this->insert_vehicle_journeys(data.vehicle_journeys);
    LOG4CPLUS_INFO(logger, "End: insert vehicle journeys");
    LOG4CPLUS_INFO(logger, "Begin: insert meta vehicle journeys");
    this->insert_meta_vj(data.meta_vj_map);
    LOG4CPLUS_INFO(logger, "End: insert meta vehicle journeys");

    LOG4CPLUS_INFO(logger, "Begin: insert journey pattern points");
    this->insert_journey_pattern_point(data.journey_pattern_points);
    LOG4CPLUS_INFO(logger, "End: insert journey pattern points");
    LOG4CPLUS_INFO(logger, "Begin: insert stop times");
    this->insert_stop_times(data.stops);
    LOG4CPLUS_INFO(logger, "End: insert stop times");
    //@TODO: les connections ont des doublons, en attendant que ce soit corrigé, on ne les enregistre pas
    LOG4CPLUS_INFO(logger, "Begin: insert stop point connections");
    this->insert_stop_point_connections(data.stop_point_connections);
    LOG4CPLUS_INFO(logger, "End: insert stop point connections");
    LOG4CPLUS_INFO(logger, "Begin: insert admin stop area");
    this->insert_admin_stop_areas(data.admin_stop_areas);
    LOG4CPLUS_INFO(logger, "End: insert admin stop area");

    LOG4CPLUS_INFO(logger, "Begin: insert fares");
    persist_fare(data);
    LOG4CPLUS_INFO(logger, "End: insert fares");
    LOG4CPLUS_INFO(logger, "Begin: insert week patterns");
    this->insert_week_patterns(data.calendars);
    LOG4CPLUS_INFO(logger, "End: insert week patterns");
    LOG4CPLUS_INFO(logger, "Begin: insert calendars");
    this->insert_calendars(data.calendars);
    LOG4CPLUS_INFO(logger, "End: insert week calendars");
    LOG4CPLUS_INFO(logger, "Begin: insert exception dates");
    this->insert_exception_dates(data.calendars);
    LOG4CPLUS_INFO(logger, "End: insert exception dates");
    LOG4CPLUS_INFO(logger, "Begin: insert periods");
    this->insert_periods(data.calendars);
    LOG4CPLUS_INFO(logger, "End: insert periods");
    LOG4CPLUS_INFO(logger, "Begin: insert relation calendar line");
    this->insert_rel_calendar_line(data.calendars);
    LOG4CPLUS_INFO(logger, "End: insert relation calendar line");
    LOG4CPLUS_INFO(logger, "Begin: commit");
    this->lotus.commit();
    LOG4CPLUS_INFO(logger, "End: commit");
}

void EdPersistor::persist_synonym(const std::map<std::string, std::string>& data){
    this->lotus.start_transaction();
    this->clean_synonym();
    LOG4CPLUS_INFO(logger, "Begin: insert synonyms");
    this->insert_synonyms(data);
    LOG4CPLUS_INFO(logger, "Begin: commit");
    this->lotus.commit();
    LOG4CPLUS_INFO(logger, "End: commit");
}

void EdPersistor::persist_fare(const ed::Data& data) {
    LOG4CPLUS_INFO(logger, "Begin: truncate fare tables");
    PQclear(this->lotus.exec(
                "TRUNCATE navitia.origin_destination, navitia.transition, "
                "navitia.ticket, navitia.dated_ticket, navitia.od_ticket CASCADE"));
    LOG4CPLUS_INFO(logger, "End: truncate fare tables");
    LOG4CPLUS_INFO(logger, "Begin: insert prices");
    this->insert_prices(data);
    LOG4CPLUS_INFO(logger, "End: insert prices");
    LOG4CPLUS_INFO(logger, "Begin: insert transitions");
    this->insert_transitions(data);
    LOG4CPLUS_INFO(logger, "End: insert transitions");
    LOG4CPLUS_INFO(logger, "Begin: insert origin destinations");
    this->insert_origin_destination(data);
    LOG4CPLUS_INFO(logger, "End: insert origin destinations");
}

void EdPersistor::insert_metadata(const navitia::type::MetaData& meta){
    std::string request = "insert into navitia.parameters (beginning_date, end_date, timezone) "
                          "VALUES ('" + bg::to_iso_extended_string(meta.production_date.begin()) +
                          "', '" + bg::to_iso_extended_string(meta.production_date.end()) +
                         "', '" + meta.timezone + "');";
    PQclear(this->lotus.exec(request));
}

void EdPersistor::clean_georef(){
    PQclear(this->lotus.exec(
                "TRUNCATE georef.node, georef.house_number, georef.admin, "
                "georef.way CASCADE;"));
}

void EdPersistor::clean_poi(){
    PQclear(this->lotus.exec(
                "TRUNCATE  georef.poi_type, georef.poi CASCADE;"));
}

void EdPersistor::clean_synonym(){
    PQclear(this->lotus.exec("TRUNCATE georef.synonym"));
}

void EdPersistor::clean_db(){
    PQclear(this->lotus.exec(
                "TRUNCATE navitia.stop_area, navitia.line, navitia.company, "
                "navitia.physical_mode, navitia.contributor, "
                "navitia.commercial_mode, "
                "navitia.vehicle_properties, navitia.properties, "
                "navitia.validity_pattern, navitia.network, navitia.parameters, "
                "navitia.connection, navitia.calendar, navitia.period, "
                "navitia.week_pattern, "
                "navitia.meta_vj, navitia.rel_metavj_vj"
                " CASCADE"));
}

void EdPersistor::insert_networks(const std::vector<types::Network*>& networks){
    this->lotus.prepare_bulk_insert("navitia.network", {"id", "uri",
                                    "external_code", "name", "comment", "sort", "website"});
    for(types::Network* net : networks){
        std::vector<std::string> values;
        values.push_back(std::to_string(net->idx));
        values.push_back(navitia::encode_uri(net->uri));
        values.push_back(net->external_code);
        values.push_back(net->name);
        values.push_back(net->comment);
        values.push_back(std::to_string(net->sort));
        values.push_back(net->website);
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
        values.push_back(navitia::encode_uri(mode->uri));
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
        values.push_back(navitia::encode_uri(mode->uri));
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
        values.push_back(navitia::encode_uri(company->uri));
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
        values.push_back(navitia::encode_uri(contributor->uri));
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
             "properties_id", "visible", "timezone"});

    for(types::StopArea* sa : stop_areas){
        std::vector<std::string> values;
        values.push_back(std::to_string(sa->idx));
        values.push_back(navitia::encode_uri(sa->uri));
        values.push_back(sa->external_code);
        values.push_back(sa->name);
        values.push_back("POINT(" + std::to_string(sa->coord.lon()) + " " + std::to_string(sa->coord.lat()) + ")");
        values.push_back(sa->comment);
        values.push_back(std::to_string(sa->to_ulog()));
        values.push_back(std::to_string(sa->visible));
        values.push_back(sa->time_zone_with_name.first);
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_points(const std::vector<types::StopPoint*>& stop_points){
    this->lotus.prepare_bulk_insert("navitia.stop_point",
            {"id", "uri", "external_code", "name", "coord", "comment",
            "fare_zone", "stop_area_id","properties_id","platform_code"});

    for(types::StopPoint* sp : stop_points){
        std::vector<std::string> values;
        values.push_back(std::to_string(sp->idx));
        values.push_back(navitia::encode_uri(sp->uri));
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
        values.push_back(sp->platform_code);
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();

}
void EdPersistor::insert_lines(const std::vector<types::Line*>& lines){
    this->lotus.prepare_bulk_insert("navitia.line",
            {"id", "uri", "external_code", "name", "comment", "color", "code",
            "commercial_mode_id", "network_id", "sort"});

    for(types::Line* line : lines){
        std::vector<std::string> values;
        values.push_back(std::to_string(line->idx));
        values.push_back(navitia::encode_uri(line->uri));
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
        } else {
            LOG4CPLUS_INFO(logger, "Line " + line->uri + " ignored because it doesn't "
                    "have any network");
            continue;
        }
        values.push_back(std::to_string(line->sort));
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


void EdPersistor::insert_routes(const std::vector<types::Route*>& routes){
    this->lotus.prepare_bulk_insert("navitia.route",
            {"id", "uri", "external_code", "name", "comment", "line_id"});

    for(types::Route* route : routes){
        std::vector<std::string> values;
        values.push_back(std::to_string(route->idx));
        values.push_back(navitia::encode_uri(route->uri));
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
        values.push_back(navitia::encode_uri(jp->uri));
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
            {"arrival_time", "departure_time", "local_traffic_zone", "odt",
             "pick_up_allowed", "drop_off_allowed",
             "is_frequency", "journey_pattern_point_id", "vehicle_journey_id",
             "comment", "date_time_estimated"});
    size_t inserted_count = 0;
    size_t size_st = stop_times.size();
    for(types::StopTime* stop : stop_times){
        std::vector<std::string> values;
        values.push_back(std::to_string(stop->arrival_time));
        values.push_back(std::to_string(stop->departure_time));
        if(stop->local_traffic_zone != std::numeric_limits<uint16_t>::max()){
            values.push_back(std::to_string(stop->local_traffic_zone));
        }else{
            values.push_back(lotus.null_value);
        }
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
        ++inserted_count;
        if(inserted_count % 150000 == 0) {
            lotus.finish_bulk_insert();
            LOG4CPLUS_INFO(logger, inserted_count<<"/"<< size_st <<" inserted stop times");
            this->lotus.prepare_bulk_insert("navitia.stop_time",
            {"arrival_time", "departure_time", "local_traffic_zone", "odt", "pick_up_allowed", "drop_off_allowed",
             "is_frequency", "journey_pattern_point_id", "vehicle_journey_id", "comment", "date_time_estimated"});
        }
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
        values.push_back(navitia::encode_uri(jpp->uri));
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
             "start_time", "end_time", "headway_sec",
             "adapted_validity_pattern_id", "company_id", "journey_pattern_id",
             "theoric_vehicle_journey_id", "vehicle_properties_id",
             "odt_type_id", "odt_message"});

    for(types::VehicleJourney* vj : vehicle_journeys){
        std::vector<std::string> values;
        values.push_back(std::to_string(vj->idx));
        values.push_back(navitia::encode_uri(vj->uri));
        values.push_back(vj->external_code);
        values.push_back(vj->name);
        values.push_back(vj->comment);

        if(vj->validity_pattern != NULL){
            values.push_back(std::to_string(vj->validity_pattern->idx));
        }else{
            values.push_back(lotus.null_value);
        }

        values.push_back(std::to_string(vj->start_time));
        values.push_back(std::to_string(vj->end_time));
        values.push_back(std::to_string(vj->headway_secs));

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
    for(types::VehicleJourney* vj : vehicle_journeys) {
        std::string values = "";
        if(vj->prev_vj) {
            values = "previous_vehicle_journey_id = " + boost::lexical_cast<std::string>(vj->prev_vj->idx);
        }
        if(vj->next_vj) {
            if(!values.empty()) {
                values += ", ";
            }
            values += "next_vehicle_journey_id = "+boost::lexical_cast<std::string>(vj->next_vj->idx);
        }
        if(!values.empty()) {
            std::string query = "UPDATE navitia.vehicle_journey SET ";
            query += values;
            query += " WHERE id = ";
            query += boost::lexical_cast<std::string>(vj->idx);
            query += ";";
            LOG4CPLUS_TRACE(logger, "query : " << query);
            PQclear(this->lotus.exec(query, "", PGRES_COMMAND_OK));
        }
    }
}

void EdPersistor::insert_meta_vj(const std::map<std::string, types::MetaVehicleJourney>& meta_vjs) {
    //we insert first the meta vj
    this->lotus.prepare_bulk_insert("navitia.meta_vj", {"id", "name"});
    size_t cpt(0);
    for (const auto& meta_vj_pair: meta_vjs) {
        this->lotus.insert({std::to_string(cpt), meta_vj_pair.first});
        cpt++;
    }
    this->lotus.finish_bulk_insert();

    //then the links
    this->lotus.prepare_bulk_insert("navitia.rel_metavj_vj", {"meta_vj", "vehicle_journey", "vj_class"});
    cpt = 0;
    for (const auto& meta_vj_pair: meta_vjs) {
        const types::MetaVehicleJourney& meta_vj = meta_vj_pair.second;
        const std::vector<std::pair<std::string, const std::vector<types::VehicleJourney*>& > > list_vj = {
            {"Theoric", meta_vj.theoric_vj},
            {"Adapted", meta_vj.adapted_vj},
            {"RealTime", meta_vj.real_time_vj}
        };

        for (const auto& name_list: list_vj) {
            for (const types::VehicleJourney* vj: name_list.second) {

                this->lotus.insert({std::to_string(cpt),
                                    std::to_string(vj->idx),
                                    name_list.first
                                   });
            }
        }
        cpt++;
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_admin_stop_areas(const std::vector<types::AdminStopArea*> admin_stop_areas) {
    this->lotus.prepare_bulk_insert("navitia.admin_stop_area", {"admin_id", "stop_area_id"});

    for(const types::AdminStopArea* asa : admin_stop_areas) {
        for(const types::StopArea* sa: asa->stop_area) {
            std::vector<std::string> values {
                asa->admin,
                std::to_string(sa->idx)
            };

            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_week_patterns(const std::vector<types::Calendar*>& calendars){
    this->lotus.prepare_bulk_insert("navitia.week_pattern", {"id", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"});
    std::vector<navitia::type::idx_t> to_insert;
    for(const types::Calendar* cal : calendars){
        auto id = cal->week_pattern.to_ulong();
        if(!binary_search(to_insert.begin(), to_insert.end(), id)){
            std::vector<std::string> values;
            values.push_back(std::to_string(id));
            values.push_back(std::to_string(cal->week_pattern[navitia::Monday]));
            values.push_back(std::to_string(cal->week_pattern[navitia::Tuesday]));
            values.push_back(std::to_string(cal->week_pattern[navitia::Wednesday]));
            values.push_back(std::to_string(cal->week_pattern[navitia::Thursday]));
            values.push_back(std::to_string(cal->week_pattern[navitia::Friday]));
            values.push_back(std::to_string(cal->week_pattern[navitia::Saturday]));
            values.push_back(std::to_string(cal->week_pattern[navitia::Sunday]));
            this->lotus.insert(values);
            to_insert.push_back(id);
            std::sort(to_insert.begin(), to_insert.end());
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_calendars(const std::vector<types::Calendar*>& calendars){
    this->lotus.prepare_bulk_insert("navitia.calendar", {"id", "uri", "external_code", "name", "week_pattern_id"});

    for(const types::Calendar* cal : calendars){
        auto id = cal->week_pattern.to_ulong();
        std::vector<std::string> values;
        values.push_back(std::to_string(cal->idx));
        values.push_back(navitia::base64_encode(cal->uri));
        values.push_back(cal->external_code);
        values.push_back(cal->name);
        values.push_back(std::to_string(id));
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_exception_dates(const std::vector<types::Calendar*>& calendars){
    this->lotus.prepare_bulk_insert("navitia.exception_date", {"datetime", "type_ex", "calendar_id"});

    for(const types::Calendar* cal : calendars){
        for( const auto except : cal->exceptions){
            std::vector<std::string> values;
            values.push_back(bg::to_iso_extended_string(except.date));
            values.push_back(navitia::type::to_string(except.type));
            values.push_back(std::to_string(cal->idx));
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_periods(const std::vector<types::Calendar*>& calendars){
    this->lotus.prepare_bulk_insert("navitia.period", {"id", "calendar_id", "begin_date", "end_date"});
    size_t idx(0);
    for (const types::Calendar* cal : calendars) {
        for (const boost::gregorian::date_period period : cal->period_list) {
            std::vector<std::string> values {
                std::to_string(idx++),
                std::to_string(cal->idx),
                bg::to_iso_extended_string(period.begin()),
                bg::to_iso_extended_string(period.end())
            };
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_rel_calendar_line(const std::vector<types::Calendar*>& calendars){
    this->lotus.prepare_bulk_insert("navitia.rel_calendar_line", {"calendar_id", "line_id"});
    for(const types::Calendar* cal : calendars){
        for(const types::Line* line : cal->line_list){
            std::vector<std::string> values;
            values.push_back(std::to_string(cal->idx));
            values.push_back(std::to_string(line->idx));
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_synonyms(const std::map<std::string, std::string>& synonyms){
    this->lotus.prepare_bulk_insert("georef.synonym", {"id", "key", "value"});
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
// before adding in bd a transition or an od fare,
// we should check if object filtering the transition is in ed (the line, the network, ...)

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

        LOG4CPLUS_INFO(logger, "transition : " << boost::algorithm::join(values, ","));
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();

    // the postgres connector does not handle well null values,
    // so we add the transition without tickets in a separate bulk
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
        LOG4CPLUS_INFO(logger, "ticket : " << boost::algorithm::join(values, ","));
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();

    //we add the dated ticket afterward
    LOG4CPLUS_INFO(logger, "dated ticket");
    this->lotus.prepare_bulk_insert("navitia.dated_ticket",
            {"id", "ticket_id", "valid_from", "valid_to", "ticket_price",
            "comments", "currency"});
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
                LOG4CPLUS_INFO(logger, "origin_destination : " << boost::algorithm::join(values, ","));
                this->lotus.insert(values);
            }
            cpt++;
        }
    }
    this->lotus.finish_bulk_insert();
}

}//namespace
