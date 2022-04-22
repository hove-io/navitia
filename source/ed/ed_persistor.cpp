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

#include "ed_persistor.h"

#include "ed/connectors/fare_utils.h"

#include <boost/geometry.hpp>
#include <memory>

namespace bg = boost::gregorian;

namespace ed {

EdPersistor::EdPersistor(const std::string& connection_string, const bool is_osm_reader)
    : lotus(connection_string), logger(log4cplus::Logger::getInstance("log")), is_osm_reader(is_osm_reader) {
    if (!is_osm_reader) {
        parse_pois = true;
        return;
    }
    std::unique_ptr<pqxx::connection> conn;
    try {
        conn = std::make_unique<pqxx::connection>(connection_string);
    } catch (const pqxx::pqxx_exception& e) {
        throw navitia::exception(e.base().what());
    }

    pqxx::work work(*conn, "loading params");
    const auto request = "SELECT parse_pois_from_osm FROM navitia.parameters";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        const_it["parse_pois_from_osm"].to(parse_pois);
    }
}

void EdPersistor::persist_pois(const ed::Georef& data) {
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
    this->insert_metadata_georef();
    this->lotus.commit();
    LOG4CPLUS_INFO(logger, "End: commit");
}

void EdPersistor::persist(const ed::Georef& data) {
    this->lotus.start_transaction();
    LOG4CPLUS_INFO(logger, "Begin: TRUNCATE data!");
    this->clean_georef();
    LOG4CPLUS_INFO(logger, "Begin: add admins data");
    this->insert_admins(data);
    LOG4CPLUS_INFO(logger, "End: add admins data");

    LOG4CPLUS_INFO(logger, "Begin: add postal codes data");
    this->insert_postal_codes(data);
    LOG4CPLUS_INFO(logger, "End: add postal codes data");

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

    LOG4CPLUS_INFO(logger, "Begin: compute bounding shape");
    this->compute_bounding_shape();
    LOG4CPLUS_INFO(logger, "End: compute bounding shape");
    this->insert_metadata_georef();

    LOG4CPLUS_INFO(logger, "Begin commit");
    this->lotus.commit();
    LOG4CPLUS_INFO(logger, "End: commit");
}

std::string EdPersistor::to_geographic_point(const navitia::type::GeographicalCoord& coord) const {
    std::stringstream geog;
    geog << std::setprecision(10) << "POINT(" << coord.lon() << " " << coord.lat() << ")";
    return geog.str();
}

void EdPersistor::insert_admins(const ed::Georef& data) {
    this->lotus.prepare_bulk_insert("georef.admin", {"id", "name", "insee", "level", "coord", "uri"});
    for (const auto& itm : data.admins) {
        if (itm.second->is_used) {
            this->lotus.insert({std::to_string(itm.second->id), itm.second->name, itm.second->insee, itm.second->level,
                                this->to_geographic_point(itm.second->coord), "admin:fr:" + itm.second->insee});
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_postal_codes(const ed::Georef& data) {
    this->lotus.prepare_bulk_insert("georef.postal_codes", {"admin_id", "postal_code"});
    for (const auto& itm : data.admins) {
        if (itm.second->is_used) {
            for (const auto& postal_code : itm.second->postal_codes) {
                this->lotus.insert({std::to_string(itm.second->id), postal_code});
            }
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_ways(const ed::Georef& data) {
    this->lotus.prepare_bulk_insert("georef.way", {"id", "name", "uri", "type", "visible"});
    for (const auto& itm : data.ways) {
        if (itm.second->is_used) {
            std::vector<std::string> values;
            values.push_back(std::to_string(itm.second->id));
            values.push_back(itm.second->name);
            values.push_back(itm.first);
            values.push_back(itm.second->type);
            values.emplace_back(itm.second->visible ? "true" : "false");
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_nodes(const ed::Georef& data) {
    this->lotus.prepare_bulk_insert("georef.node", {"id", "coord"});
    for (const auto& itm : data.nodes) {
        if (itm.second->is_used) {
            this->lotus.insert({std::to_string(itm.second->id), this->to_geographic_point(itm.second->coord)});
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_house_numbers(const ed::Georef& data) {
    this->lotus.prepare_bulk_insert("georef.house_number", {"coord", "number", "left_side", "way_id"});

    for (const auto& itm : data.house_numbers) {
        std::string way_id(lotus.null_value);
        if (itm.second.way != nullptr) {
            way_id = std::to_string(itm.second.way->id);
        }
        this->lotus.insert({this->to_geographic_point(itm.second.coord), itm.second.number,
                            std::to_string(str_to_int(itm.second.number) % 2 == 0), way_id});
    }
    lotus.finish_bulk_insert();
}

static std::string coord_to_string(const navitia::type::GeographicalCoord& coord) {
    std::stringstream geog;
    geog << std::setprecision(10) << coord.lon() << " " << coord.lat();
    return geog.str();
}

void EdPersistor::insert_edges(const ed::Georef& data) {
    this->lotus.prepare_bulk_insert("georef.edge", {"source_node_id", "target_node_id", "way_id", "the_geog",
                                                    "pedestrian_allowed", "cycles_allowed", "cars_allowed"});
    const auto bool_str = std::to_string(true);
    size_t to_insert_count = 0;
    size_t all_count = data.edges.size();
    for (const auto& edge : data.edges) {
        const auto source_str = std::to_string(edge.second->source->id);
        const auto target_str = std::to_string(edge.second->target->id);
        const auto way_str = std::to_string(edge.second->way->id);
        const auto source_coord = coord_to_string(edge.second->source->coord);
        const auto target_coord = coord_to_string(edge.second->target->coord);

        this->lotus.insert({source_str, target_str, way_str,
                            std::string("LINESTRING(" + source_coord + ",").append(target_coord + ")"), bool_str,
                            bool_str, bool_str});
        this->lotus.insert({target_str, source_str, way_str,
                            std::string("LINESTRING(" + target_coord + ",").append(source_coord + ")"), bool_str,
                            bool_str, bool_str});
        ++to_insert_count;
        if (to_insert_count % 150000 == 0) {
            lotus.finish_bulk_insert();
            LOG4CPLUS_INFO(logger, to_insert_count << "/" << all_count << " edges inserées");
            this->lotus.prepare_bulk_insert("georef.edge", {"source_node_id", "target_node_id", "way_id", "the_geog",
                                                            "pedestrian_allowed", "cycles_allowed", "cars_allowed"});
        }
    }
    lotus.finish_bulk_insert();
    LOG4CPLUS_INFO(logger, to_insert_count << "/" << all_count << " edges inserées");
}

void EdPersistor::insert_poi_types(const Georef& data) {
    if (!parse_pois) {
        return;
    }
    this->lotus.prepare_bulk_insert("georef.poi_type", {"id", "uri", "name"});
    for (const auto& itm : data.poi_types) {
        this->lotus.insert({std::to_string(itm.second->id), "poi_type:" + itm.first, itm.second->name});
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::insert_pois(const Georef& data) {
    if (!parse_pois) {
        return;
    }
    this->lotus.prepare_bulk_insert("georef.poi", {"id", "weight", "coord", "name", "uri", "poi_type_id", "visible",
                                                   "address_number", "address_name"});
    for (const auto& itm : data.pois) {
        std::string poi_type(lotus.null_value);
        if (itm.second.poi_type != nullptr) {
            poi_type = std::to_string(itm.second.poi_type->id);
        }
        this->lotus.insert({
            std::to_string(itm.second.id),                // id
            std::to_string(itm.second.weight),            // weight
            this->to_geographic_point(itm.second.coord),  // coord
            itm.second.name,                              // name
            itm.second.uri,                               // uri
            poi_type,                                     // poi_type_id
            std::to_string(itm.second.visible),           // visible
            itm.second.address_number,                    // address_number
            itm.second.address_name                       // /address_name
        });
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::insert_poi_properties(const Georef& data) {
    if (!parse_pois) {
        return;
    }
    this->lotus.prepare_bulk_insert("georef.poi_properties", {"poi_id", "key", "value"});
    for (const auto& itm : data.pois) {
        for (auto property : itm.second.properties) {
            this->lotus.insert({std::to_string(itm.second.id), property.first, property.second});
        }
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::build_relation_way_admin(const ed::Georef& data) {
    this->lotus.prepare_bulk_insert("georef.rel_way_admin", {"admin_id", "way_id"});
    for (const auto& itm : data.ways) {
        if (itm.second->is_used) {
            std::vector<std::string> values;
            if (itm.second->admin) {
                values.push_back(std::to_string(itm.second->admin->id));
                values.push_back(std::to_string(itm.second->id));
                this->lotus.insert(values);
            }
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::compute_bounding_shape() {
    try {
        this->lotus.exec("select georef.update_bounding_shape();", "", PGRES_TUPLES_OK);
    } catch (LotusException& e) {
        LOG4CPLUS_ERROR(logger, "Failure in EdPersistor::compute_bounding_shape: " << e.what());
    }
}

void EdPersistor::persist(const ed::Data& data) {
    this->lotus.start_transaction();

    LOG4CPLUS_INFO(logger, "Begin: clean db");
    this->clean_db();
    LOG4CPLUS_INFO(logger, "End: clean db");
    LOG4CPLUS_INFO(logger, "Begin: insert metadata");
    this->insert_metadata(data.meta);
    LOG4CPLUS_INFO(logger, "End: insert metadata");
    LOG4CPLUS_INFO(logger, "Begin: insert feed_infos");
    this->insert_feed_info(data.feed_infos);
    LOG4CPLUS_INFO(logger, "End: insert feed_info");
    LOG4CPLUS_INFO(logger, "Begin: insert timezone");
    this->insert_timezones(data.tz_wrapper.tz_handler);
    LOG4CPLUS_INFO(logger, "End: insert timezone");
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
    LOG4CPLUS_INFO(logger, "Begin: insert datasets");
    this->insert_datasets(data.datasets);
    LOG4CPLUS_INFO(logger, "End: insert datasets");
    LOG4CPLUS_INFO(logger, "Begin: insert stops properties");
    this->insert_sa_sp_properties(data);
    LOG4CPLUS_INFO(logger, "End: insert stops properties");
    LOG4CPLUS_INFO(logger, "Begin: insert stop areas");
    this->insert_stop_areas(data.stop_areas);
    LOG4CPLUS_INFO(logger, "End: insert stop areas");
    LOG4CPLUS_INFO(logger, "Begin: insert stop points");
    this->insert_stop_points(data.stop_points);
    LOG4CPLUS_INFO(logger, "End: insert stop points");
    LOG4CPLUS_INFO(logger, "Begin: access points exits");
    this->insert_access_points(data.access_points);
    LOG4CPLUS_INFO(logger, "End: insert access points");
    LOG4CPLUS_INFO(logger, "Begin: insert pathways");
    this->insert_pathways(data.pathways);
    LOG4CPLUS_INFO(logger, "End: insert pathways");
    LOG4CPLUS_INFO(logger, "Begin: insert addresses from ntfs");
    this->insert_addresses_from_ntfs(data.addresses_from_ntfs);
    LOG4CPLUS_INFO(logger, "End: insert addresses from ntfs");
    LOG4CPLUS_INFO(logger, "Begin: insert lines");
    this->insert_lines(data.lines);
    LOG4CPLUS_INFO(logger, "End: insert lines");
    LOG4CPLUS_INFO(logger, "Begin: insert line groups");
    this->insert_line_groups(data.line_groups, data.line_group_links);
    LOG4CPLUS_INFO(logger, "End: insert line groups");
    LOG4CPLUS_INFO(logger, "Begin: insert routes");
    this->insert_routes(data.routes);
    LOG4CPLUS_INFO(logger, "End: insert routes");
    LOG4CPLUS_INFO(logger, "Begin: insert validity patterns");
    this->insert_validity_patterns(data.validity_patterns);
    LOG4CPLUS_INFO(logger, "End: insert validity patterns");
    LOG4CPLUS_INFO(logger, "Begin: insert vehicle properties");
    this->insert_vehicle_properties(data.vehicle_journeys);
    LOG4CPLUS_INFO(logger, "End: insert vehicle properties");
    LOG4CPLUS_INFO(logger, "Begin: insert vehicle journeys");
    this->insert_vehicle_journeys(data.vehicle_journeys);
    LOG4CPLUS_INFO(logger, "End: insert vehicle journeys");

    LOG4CPLUS_INFO(logger, "Begin: insert shapes");
    this->insert_shapes(data.shapes_from_prev);
    LOG4CPLUS_INFO(logger, "End: insert shapes");

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

    LOG4CPLUS_INFO(logger, "Begin: insert comments");
    this->insert_comments(data);
    LOG4CPLUS_INFO(logger, "End: insert comments");

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

    LOG4CPLUS_INFO(logger, "Begin: insert meta vehicle journeys");
    this->insert_meta_vj(data.meta_vj_map);
    LOG4CPLUS_INFO(logger, "End: insert meta vehicle journeys");

    LOG4CPLUS_INFO(logger, "Begin: insert object codes");
    this->insert_object_codes(data.object_codes);
    LOG4CPLUS_INFO(logger, "End: insert object codes");

    LOG4CPLUS_INFO(logger, "Begin: insert object properties");
    this->insert_object_properties(data.object_properties);
    LOG4CPLUS_INFO(logger, "End: insert object properties");

    LOG4CPLUS_INFO(logger, "Begin: commit");
    this->lotus.commit();
    LOG4CPLUS_INFO(logger, "End: commit");
}

void EdPersistor::persist_synonym(const std::map<std::string, std::string>& data) {
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
    this->lotus.exec(
        "TRUNCATE navitia.origin_destination, navitia.transition, "
        "navitia.ticket, navitia.dated_ticket, navitia.od_ticket CASCADE");
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

void EdPersistor::insert_metadata(const navitia::type::MetaData& meta) {
    auto beg = bg::to_iso_extended_string(meta.production_date.begin());
    auto last = bg::to_iso_extended_string(meta.production_date.last());
    // metadata consist of only one line, we either have to update it or create it
    this->lotus.exec(Lotus::make_upsert_string("navitia.parameters", {{"beginning_date", beg}, {"end_date", last}}));
}

void EdPersistor::insert_metadata_georef() {
    // If we do one poi2ed, we don't want to read pois from OSM anymore
    std::vector<std::pair<std::string, std::string>> values = {
        {"parse_pois_from_osm", (is_osm_reader && parse_pois) ? "t" : "f"}};

    if (!street_network_source.empty()) {
        values.emplace_back("street_network_source", street_network_source);
    }
    if (parse_pois && !poi_source.empty()) {
        values.emplace_back("poi_source", poi_source);
    }
    this->lotus.exec(Lotus::make_upsert_string("navitia.parameters", values));
}

void EdPersistor::clean_georef() {
    this->lotus.exec(
        "TRUNCATE georef.node, georef.house_number, georef.postal_codes, georef.admin, "
        "georef.way CASCADE;");
}

void EdPersistor::clean_poi() {
    if (!parse_pois) {
        return;
    }
    this->lotus.exec("TRUNCATE georef.poi_type, georef.poi CASCADE;");
}

void EdPersistor::clean_synonym() {
    this->lotus.exec("TRUNCATE georef.synonym");
}

void EdPersistor::clean_db() {
    this->lotus.exec(
        "TRUNCATE navitia.stop_area, navitia.line, navitia.company, "
        "navitia.physical_mode, navitia.contributor, "
        "navitia.commercial_mode, navitia.timezone, navitia.tz_dst, "
        "navitia.vehicle_properties, navitia.properties, "
        "navitia.validity_pattern, navitia.network, "
        "navitia.connection, navitia.calendar, navitia.period, "
        "navitia.week_pattern, "
        "navitia.meta_vj, navitia.object_properties, navitia.object_code, "
        "navitia.comments, navitia.ptobject_comments, navitia.feed_info, "
        "navitia.line_group, navitia.line_group_link, navitia.shape, navitia.access_point,"
        "navitia.pathway"
        " CASCADE");
    // we remove the parameters (but we do not truncate the table since the shape might have been updated with fusio2ed)
    this->lotus.exec(
        "update navitia.parameters set"
        " beginning_date = null"
        ", end_date = null;");
}

void EdPersistor::insert_feed_info(const std::map<std::string, std::string>& feed_infos) {
    this->lotus.prepare_bulk_insert("navitia.feed_info", {"key", "value"});
    for (const auto& feed_info : feed_infos) {
        std::vector<std::string> values;
        values.push_back(feed_info.first);
        values.push_back(feed_info.second);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_timezones(const navitia::type::TimeZoneHandler& tz_handler) {
    lotus.prepare_bulk_insert("navitia.timezone", {"id", "name"});
    // in the ED part there can be only one TZ by construction
    lotus.insert({std::to_string(default_timezone_idx), tz_handler.tz_name});
    lotus.finish_bulk_insert();

    lotus.prepare_bulk_insert("navitia.tz_dst", {"id", "tz_id", "beginning_date", "end_date", "utc_offset"});
    size_t idx = 0;
    for (const auto& dst : tz_handler.get_periods_and_shift()) {
        for (const auto& period : dst.second) {
            lotus.insert({std::to_string(idx), std::to_string(default_timezone_idx),
                          bg::to_iso_extended_string(period.begin()), bg::to_iso_extended_string(period.last()),
                          std::to_string(dst.first)});
            ++idx;
        }
    }
    lotus.finish_bulk_insert();
}

void EdPersistor::insert_networks(const std::vector<types::Network*>& networks) {
    this->lotus.prepare_bulk_insert("navitia.network", {"id", "uri", "name", "sort", "website"});
    for (types::Network* net : networks) {
        std::vector<std::string> values;
        values.push_back(std::to_string(net->idx));
        values.push_back(navitia::encode_uri(net->uri));
        values.push_back(net->name);
        values.push_back(std::to_string(net->sort));
        values.push_back(net->website);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_commercial_modes(const std::vector<types::CommercialMode*>& commercial_modes) {
    this->lotus.prepare_bulk_insert("navitia.commercial_mode", {"id", "uri", "name"});
    for (types::CommercialMode* mode : commercial_modes) {
        std::vector<std::string> values;
        values.push_back(std::to_string(mode->idx));
        values.push_back(navitia::encode_uri(mode->uri));
        values.push_back(mode->name);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_physical_modes(const std::vector<types::PhysicalMode*>& physical_modes) {
    this->lotus.prepare_bulk_insert("navitia.physical_mode", {"id", "uri", "name", "co2_emission"});
    for (types::PhysicalMode* mode : physical_modes) {
        std::vector<std::string> values;
        values.push_back(std::to_string(mode->idx));
        values.push_back(navitia::encode_uri(mode->uri));
        values.push_back(mode->name);
        if (mode->co2_emission) {
            values.push_back(std::to_string(*mode->co2_emission));
        } else {
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_companies(const std::vector<types::Company*>& companies) {
    this->lotus.prepare_bulk_insert("navitia.company", {"id", "uri", "name", "address_name", "address_number",
                                                        "address_type_name", "phone_number", "mail", "website", "fax"});
    for (types::Company* company : companies) {
        std::vector<std::string> values;
        values.push_back(std::to_string(company->idx));
        values.push_back(navitia::encode_uri(company->uri));
        values.push_back(company->name);
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

void EdPersistor::insert_contributors(const std::vector<types::Contributor*>& contributors) {
    this->lotus.prepare_bulk_insert("navitia.contributor", {"id", "uri", "name", "website", "license"});
    for (types::Contributor* contributor : contributors) {
        std::vector<std::string> values;
        values.push_back(std::to_string(contributor->idx));
        values.push_back(navitia::encode_uri(contributor->uri));
        values.push_back(contributor->name);
        values.push_back(contributor->website);
        values.push_back(contributor->license);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_datasets(const std::vector<types::Dataset*>& datasets) {
    this->lotus.prepare_bulk_insert("navitia.dataset",
                                    {"id", "uri", "description", "system", "start_date", "end_date", "contributor_id"});
    for (types::Dataset* dataset : datasets) {
        std::vector<std::string> values;
        values.push_back(std::to_string(dataset->idx));
        values.push_back(navitia::encode_uri(dataset->uri));
        values.push_back(dataset->desc);
        values.push_back(dataset->system);
        values.push_back(bg::to_iso_extended_string(dataset->validation_period.begin()));
        values.push_back(bg::to_iso_extended_string(dataset->validation_period.end()));
        values.push_back(std::to_string(dataset->contributor->idx));
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_sa_sp_properties(const ed::Data& data) {
    this->lotus.prepare_bulk_insert(
        "navitia.properties",
        {"id", "wheelchair_boarding", "sheltered", "elevator", "escalator", "bike_accepted", "bike_depot",
         "visual_announcement", "audible_announcement", "appropriate_escort", "appropriate_signage"});
    std::vector<navitia::type::idx_t> to_insert;
    for (types::StopArea* sa : data.stop_areas) {
        navitia::type::idx_t idx = sa->to_ulog();
        if (!binary_search(to_insert.begin(), to_insert.end(), idx)) {
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
    for (types::StopPoint* sp : data.stop_points) {
        navitia::type::idx_t idx = sp->to_ulog();
        if (!binary_search(to_insert.begin(), to_insert.end(), idx)) {
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
    for (types::StopPointConnection* connection : data.stop_point_connections) {
        navitia::type::idx_t idx = connection->to_ulog();
        if (!binary_search(to_insert.begin(), to_insert.end(), idx)) {
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

void EdPersistor::insert_stop_areas(const std::vector<types::StopArea*>& stop_areas) {
    this->lotus.prepare_bulk_insert("navitia.stop_area",
                                    {"id", "uri", "name", "coord", "properties_id", "visible", "timezone"});

    for (types::StopArea* sa : stop_areas) {
        std::vector<std::string> values;
        values.push_back(std::to_string(sa->idx));
        values.push_back(navitia::encode_uri(sa->uri));
        values.push_back(sa->name);
        values.push_back("POINT(" + std::to_string(sa->coord.lon()) + " " + std::to_string(sa->coord.lat()) + ")");
        values.push_back(std::to_string(sa->to_ulog()));
        values.push_back(std::to_string(sa->visible));
        values.push_back(sa->time_zone_with_name.first);
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_points(const std::vector<types::StopPoint*>& stop_points) {
    this->lotus.prepare_bulk_insert("navitia.stop_point",
                                    {"id", "uri", "name", "coord", "fare_zone", "stop_area_id", "properties_id",
                                     "platform_code", "is_zonal", "area", "address_id"});

    for (types::StopPoint* sp : stop_points) {
        std::vector<std::string> values;
        values.push_back(std::to_string(sp->idx));
        values.push_back(navitia::encode_uri(sp->uri));
        values.push_back(sp->name);
        values.push_back("POINT(" + std::to_string(sp->coord.lon()) + " " + std::to_string(sp->coord.lat()) + ")");
        values.push_back(sp->fare_zone);
        if (sp->stop_area != nullptr) {
            values.push_back(std::to_string(sp->stop_area->idx));
        } else {
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(sp->to_ulog()));
        values.push_back(sp->platform_code);
        values.push_back(std::to_string(sp->is_zonal));
        std::stringstream area;
        if (sp->area) {
            area << std::setprecision(16) << boost::geometry::wkt(*sp->area);
        } else {
            area << lotus.null_value;
        }
        values.push_back(area.str());
        values.push_back(sp->address_id);
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_access_points(const std::vector<types::AccessPoint*>& access_points) {
    this->lotus.prepare_bulk_insert("navitia.access_point",
                                    {"id", "uri", "name", "coord", "stop_code", "parent_station"});

    uint idx = 0;
    for (const auto* ap : access_points) {
        std::vector<std::string> values;
        values.push_back(std::to_string(idx));
        values.push_back(navitia::encode_uri(ap->uri));
        values.push_back(ap->name);
        values.push_back("POINT(" + std::to_string(ap->coord.lon()) + " " + std::to_string(ap->coord.lat()) + ")");
        values.push_back(ap->stop_code);
        values.push_back(ap->parent_station);
        this->lotus.insert(values);
        idx++;
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_pathways(const std::vector<types::PathWay*>& pathways) {
    this->lotus.prepare_bulk_insert(
        "navitia.pathway",
        {"id", "uri", "name", "from_stop_id", "to_stop_id", "pathway_mode", "is_bidirectional", "length",
         "traversal_time", "stair_count", "max_slope", "min_width", "signposted_as", "reversed_signposted_as"});

    uint idx = 0;
    for (const auto* pw : pathways) {
        std::vector<std::string> values;
        values.push_back(std::to_string(idx));
        values.push_back(navitia::encode_uri(pw->uri));
        values.push_back(pw->name);
        values.push_back(pw->from_stop_id);
        values.push_back(pw->to_stop_id);
        values.push_back(std::to_string(pw->pathway_mode));
        values.push_back(std::to_string(pw->is_bidirectional));
        if (pw->length != unknown_field) {
            values.push_back(std::to_string(pw->length));
        } else {
            values.push_back(lotus.null_value);
        }
        if (pw->traversal_time != unknown_field) {
            values.push_back(std::to_string(pw->traversal_time));
        } else {
            values.push_back(lotus.null_value);
        }
        if (pw->stair_count != unknown_field) {
            values.push_back(std::to_string(pw->stair_count));
        } else {
            values.push_back(lotus.null_value);
        }
        if (pw->max_slope != unknown_field) {
            values.push_back(std::to_string(pw->max_slope));
        } else {
            values.push_back(lotus.null_value);
        }
        if (pw->min_width != unknown_field) {
            values.push_back(std::to_string(pw->min_width));
        } else {
            values.push_back(lotus.null_value);
        }
        if (pw->signposted_as != "") {
            values.push_back(pw->signposted_as);
        } else {
            values.push_back(lotus.null_value);
        }
        if (pw->reversed_signposted_as != "") {
            values.push_back(pw->reversed_signposted_as);
        } else {
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
        idx++;
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_addresses_from_ntfs(const std::vector<navitia::type::Address*>& addresses) {
    this->lotus.prepare_bulk_insert("navitia.address", {"id", "house_number", "street_name"});

    for (navitia::type::Address* addr : addresses) {
        std::vector<std::string> values;
        values.push_back(addr->id);
        values.push_back(addr->house_number);
        values.push_back(addr->street_name);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_lines(const std::vector<types::Line*>& lines) {
    this->lotus.prepare_bulk_insert(
        "navitia.line", {"id", "uri", "name", "color", "code", "commercial_mode_id", "network_id", "sort", "shape",
                         "opening_time", "closing_time", "text_color"});

    for (types::Line* line : lines) {
        std::vector<std::string> values;
        values.push_back(std::to_string(line->idx));
        values.push_back(navitia::encode_uri(line->uri));
        values.push_back(line->name);
        values.push_back(line->color);
        values.push_back(line->code);
        if (line->commercial_mode != nullptr) {
            values.push_back(std::to_string(line->commercial_mode->idx));
        } else {
            values.push_back(lotus.null_value);
        }

        if (line->network != nullptr) {
            values.push_back(std::to_string(line->network->idx));
        } else {
            LOG4CPLUS_INFO(logger, "Line " + line->uri + " ignored because it doesn't "
                    "have any network");
            continue;
        }

        values.push_back(std::to_string(line->sort));

        std::stringstream shape;
        if (line->shape.empty()) {
            shape << lotus.null_value;
        } else {
            shape << std::setprecision(16) << boost::geometry::wkt(line->shape);
        }
        values.push_back(shape.str());

        values.push_back(line->opening_time ? boost::posix_time::to_simple_string(*line->opening_time)
                                            : lotus.null_value);
        values.push_back(line->closing_time ? boost::posix_time::to_simple_string(*line->closing_time)
                                            : lotus.null_value);

        values.push_back(line->text_color);

        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_line_groups(const std::vector<types::LineGroup*>& groups,
                                     const std::vector<types::LineGroupLink>& group_links) {
    this->lotus.prepare_bulk_insert("navitia.line_group", {"id", "uri", "name", "main_line_id"});
    for (const auto& line_group : groups) {
        this->lotus.insert({std::to_string(line_group->idx), navitia::encode_uri(line_group->uri), line_group->name,
                            std::to_string(line_group->main_line->idx)});
    }
    this->lotus.finish_bulk_insert();

    this->lotus.prepare_bulk_insert("navitia.line_group_link", {"group_id", "line_id"});
    for (const auto& group_link : group_links) {
        const auto& line_group = std::find(groups.begin(), groups.end(), group_link.line_group);
        if (line_group == groups.end()) {
            LOG4CPLUS_ERROR(
                logger, "Group " << group_link.line_group->idx << " not found when trying to insert line_group_link");
            break;
        }
        this->lotus.insert({std::to_string(group_link.line_group->idx), std::to_string(group_link.line->idx)});
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_point_connections(const std::vector<types::StopPointConnection*>& connections) {
    this->lotus.prepare_bulk_insert("navitia.connection",
                                    {"departure_stop_point_id", "destination_stop_point_id", "connection_type_id",
                                     "display_duration", "duration", "max_duration", "properties_id"});

    //@TODO properties!!
    for (types::StopPointConnection* co : connections) {
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

void EdPersistor::insert_routes(const std::vector<types::Route*>& routes) {
    this->lotus.prepare_bulk_insert(
        "navitia.route", {"id", "uri", "name", "line_id", "destination_stop_area_id", "shape", "direction_type"});

    for (types::Route* route : routes) {
        std::vector<std::string> values;
        values.push_back(std::to_string(route->idx));
        values.push_back(navitia::encode_uri(route->uri));
        values.push_back(route->name);
        if (route->line != nullptr) {
            values.push_back(std::to_string(route->line->idx));
        } else {
            values.push_back(lotus.null_value);
        }
        if (route->destination != nullptr) {
            values.push_back(std::to_string(route->destination->idx));
        } else {
            values.push_back(lotus.null_value);
        }
        std::stringstream shape;
        if (route->shape.empty()) {
            shape << lotus.null_value;
        } else {
            shape << std::setprecision(16) << boost::geometry::wkt(route->shape);
        }
        values.push_back(shape.str());

        values.push_back(route->direction_type);
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_validity_patterns(const std::vector<types::ValidityPattern*>& validity_patterns) {
    this->lotus.prepare_bulk_insert("navitia.validity_pattern", {"id", "days"});

    for (types::ValidityPattern* vp : validity_patterns) {
        std::vector<std::string> values;
        values.push_back(std::to_string(vp->idx));
        values.push_back(vp->days.to_string());
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_shapes(const std::vector<std::shared_ptr<types::Shape>>& shapes) {
    std::vector<std::string> columns = {"id", "geom"};

    this->lotus.prepare_bulk_insert("navitia.shape", columns);
    size_t inserted_count = 0;
    std::vector<std::string> values;
    for (const auto& shape : shapes) {
        values.clear();
        values.push_back(std::to_string(shape->idx));
        std::stringstream ss;
        ss << std::setprecision(16) << boost::geometry::wkt(shape->geom);
        values.push_back(ss.str());
        this->lotus.insert(values);
        ++inserted_count;
    }
    this->lotus.finish_bulk_insert();
    LOG4CPLUS_INFO(logger, "inserted " << inserted_count << "shapes");
}

void EdPersistor::insert_stop_times(const std::vector<types::StopTime*>& stop_times) {
    std::vector<std::string> columns = {"id",
                                        "arrival_time",
                                        "departure_time",
                                        "local_traffic_zone",
                                        "odt",
                                        "pick_up_allowed",
                                        "drop_off_allowed",
                                        "skipped_stop",
                                        "is_frequency",
                                        "\"order\"",
                                        "stop_point_id",
                                        "shape_from_prev_id",
                                        "vehicle_journey_id",
                                        "date_time_estimated",
                                        "headsign",
                                        "boarding_time",
                                        "alighting_time"};

    this->lotus.prepare_bulk_insert("navitia.stop_time", columns);
    size_t inserted_count = 0;
    size_t size_st = stop_times.size();
    std::vector<std::string> values;
    for (types::StopTime* stop : stop_times) {
        values.clear();
        values.push_back(std::to_string(stop->idx));
        values.push_back(std::to_string(stop->arrival_time));
        values.push_back(std::to_string(stop->departure_time));
        if (stop->local_traffic_zone != std::numeric_limits<uint16_t>::max()) {
            values.push_back(std::to_string(stop->local_traffic_zone));
        } else {
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(stop->ODT));
        values.push_back(std::to_string(stop->pick_up_allowed));
        values.push_back(std::to_string(stop->drop_off_allowed));
        values.push_back(std::to_string(stop->skipped_stop));
        values.push_back(std::to_string(stop->is_frequency));

        values.push_back(std::to_string(stop->order));
        values.push_back(std::to_string(stop->stop_point->idx));
        if (!stop->shape_from_prev) {
            values.push_back(lotus.null_value);
        } else {
            values.push_back(std::to_string(stop->shape_from_prev->idx));
        }

        if (stop->vehicle_journey != nullptr) {
            values.push_back(std::to_string(stop->vehicle_journey->idx));
        } else {
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(stop->date_time_estimated));
        values.push_back(stop->headsign);
        values.push_back(std::to_string(stop->boarding_time));
        values.push_back(std::to_string(stop->alighting_time));

        this->lotus.insert(values);
        ++inserted_count;
        if (inserted_count % 150000 == 0) {
            lotus.finish_bulk_insert();
            LOG4CPLUS_INFO(logger, inserted_count << "/" << size_st << " inserted stop times");
            this->lotus.prepare_bulk_insert("navitia.stop_time", columns);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_vehicle_properties(const std::vector<types::VehicleJourney*>& vehicle_journeys) {
    this->lotus.prepare_bulk_insert(
        "navitia.vehicle_properties",
        {"id", "wheelchair_accessible", "bike_accepted", "air_conditioned", "visual_announcement",
         "audible_announcement", "appropriate_escort", "appropriate_signage", "school_vehicle"});

    std::vector<navitia::type::idx_t> to_insert;
    for (types::VehicleJourney* vj : vehicle_journeys) {
        navitia::type::idx_t idx = vj->to_ulog();
        if (!binary_search(to_insert.begin(), to_insert.end(), idx)) {
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

void EdPersistor::insert_vehicle_journeys(const std::vector<types::VehicleJourney*>& vehicle_journeys) {
    this->lotus.prepare_bulk_insert("navitia.vehicle_journey", {"id",
                                                                "uri",
                                                                "name",
                                                                "validity_pattern_id",
                                                                "dataset_id",
                                                                "start_time",
                                                                "end_time",
                                                                "headway_sec",
                                                                "adapted_validity_pattern_id",
                                                                "company_id",
                                                                "route_id",
                                                                "physical_mode_id",
                                                                "theoric_vehicle_journey_id",
                                                                "vehicle_properties_id",
                                                                "odt_type_id",
                                                                "odt_message",
                                                                "is_frequency",
                                                                "meta_vj_name",
                                                                "vj_class",
                                                                "headsign"});

    for (types::VehicleJourney* vj : vehicle_journeys) {
        std::vector<std::string> values;
        values.push_back(std::to_string(vj->idx));
        values.push_back(navitia::encode_uri(vj->uri));
        values.push_back(vj->name);

        if (vj->validity_pattern != nullptr) {
            values.push_back(std::to_string(vj->validity_pattern->idx));
        } else {
            values.push_back(lotus.null_value);
        }
        if (vj->dataset != nullptr) {
            values.push_back(std::to_string(vj->dataset->idx));
        } else {
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(vj->start_time));
        values.push_back(std::to_string(vj->end_time));
        values.push_back(std::to_string(vj->headway_secs));

        if (vj->adapted_validity_pattern != nullptr) {
            values.push_back(std::to_string(vj->adapted_validity_pattern->idx));
        } else if (vj->validity_pattern != nullptr) {
            values.push_back(std::to_string(vj->validity_pattern->idx));
        }
        if (vj->company != nullptr) {
            values.push_back(std::to_string(vj->company->idx));
        } else {
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(vj->route->idx));
        values.push_back(std::to_string(vj->physical_mode->idx));

        if (vj->theoric_vehicle_journey != nullptr) {
            values.push_back(std::to_string(vj->theoric_vehicle_journey->idx));
        } else {
            //@TODO WTF??
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(vj->to_ulog()));
        values.push_back(std::to_string(static_cast<int>(vj->vehicle_journey_type)));
        values.push_back(vj->odt_message);

        bool is_frequency = vj->start_time != std::numeric_limits<int>::max();
        values.push_back(std::to_string(is_frequency));

        // meta_vj's name is the same as the one of vj
        values.push_back(vj->meta_vj_name);
        // vj_class
        values.push_back(navitia::type::get_string_from_rt_level(vj->realtime_level));
        // headsign
        values.push_back(vj->headsign);

        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
    for (types::VehicleJourney* vj : vehicle_journeys) {
        std::string values;
        if (vj->prev_vj) {
            values = "previous_vehicle_journey_id = " + std::to_string(vj->prev_vj->idx);
        }
        if (vj->next_vj) {
            if (!values.empty()) {
                values += ", ";
            }
            values += "next_vehicle_journey_id = " + std::to_string(vj->next_vj->idx);
        }
        if (!values.empty()) {
            std::string query = "UPDATE navitia.vehicle_journey SET ";
            query += values;
            query += " WHERE id = ";
            query += std::to_string(vj->idx);
            query += ";";
            LOG4CPLUS_TRACE(logger, "query : " << query);
            this->lotus.exec(query, "", PGRES_COMMAND_OK);
        }
    }
}

void EdPersistor::insert_meta_vj(const std::map<std::string, types::MetaVehicleJourney>& meta_vjs) {
    // we insert first the meta vj
    this->lotus.prepare_bulk_insert("navitia.meta_vj", {"id", "name", "timezone"});
    size_t cpt(0);
    for (const auto& meta_vj_pair : meta_vjs) {
        this->lotus.insert({std::to_string(cpt), meta_vj_pair.first, std::to_string(default_timezone_idx)});
        cpt++;
    }
    this->lotus.finish_bulk_insert();

    // then we insert the associated calendar
    this->lotus.prepare_bulk_insert("navitia.associated_calendar", {"id", "calendar_id", "name"});
    cpt = 0;
    for (const auto& meta_vj_pair : meta_vjs) {
        const types::MetaVehicleJourney& meta_vj = meta_vj_pair.second;
        for (auto map : meta_vj.associated_calendars) {
            std::string calendar_name = map.first;
            types::AssociatedCalendar* associated_calendar = map.second;
            this->lotus.insert({std::to_string(associated_calendar->idx),
                                std::to_string(associated_calendar->calendar->idx), calendar_name});
        }
        cpt++;
    }
    this->lotus.finish_bulk_insert();

    // then the exception_date relatives to associated calendar
    this->lotus.prepare_bulk_insert("navitia.associated_exception_date",
                                    {"id", "datetime", "type_ex", "associated_calendar_id"});
    cpt = 0;
    int id = 0;
    for (const auto& meta_vj_pair : meta_vjs) {
        const types::MetaVehicleJourney& meta_vj = meta_vj_pair.second;
        for (auto map : meta_vj.associated_calendars) {
            types::AssociatedCalendar* associated_calendar = map.second;
            for (const auto& except : associated_calendar->exceptions) {
                this->lotus.insert({std::to_string(id++), bg::to_iso_extended_string(except.date),
                                    navitia::type::to_string(except.type), std::to_string(associated_calendar->idx)});
            }
        }
        cpt++;
    }
    this->lotus.finish_bulk_insert();

    // then the links between meta vj and associated calendar
    this->lotus.prepare_bulk_insert("navitia.rel_metavj_associated_calendar", {"meta_vj_id", "associated_calendar_id"});
    cpt = 0;
    for (const auto& meta_vj_pair : meta_vjs) {
        const types::MetaVehicleJourney& meta_vj = meta_vj_pair.second;
        for (auto map : meta_vj.associated_calendars) {
            types::AssociatedCalendar* associated_calendar = map.second;
            this->lotus.insert({
                std::to_string(cpt),
                std::to_string(associated_calendar->idx),
            });
        }
        cpt++;
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_object_codes(
    const std::map<ed::types::pt_object_header, std::map<std::string, std::vector<std::string>>>& object_codes) {
    size_t count = 0;
    this->lotus.prepare_bulk_insert("navitia.object_code", {"object_id", "object_type_id", "key", "value"});
    for (const auto& object_code_map : object_codes) {
        for (const auto& object_code : object_code_map.second) {
            if (object_code_map.first.pt_object->idx == nt::invalid_idx) {
                ++count;
            } else {
                for (const auto& object_code_value : object_code.second) {
                    std::vector<std::string> values{std::to_string(object_code_map.first.pt_object->idx),
                                                    std::to_string(static_cast<int>(object_code_map.first.type)),
                                                    object_code.first, object_code_value};
                    this->lotus.insert(values);
                }
            }
        }
    }
    this->lotus.finish_bulk_insert();
    if (count > 0) {
        LOG4CPLUS_INFO(logger, count << "/" << object_codes.size() << " object codes ignored.");
    }
}

// temporary, but for the moment we need to transform the enum back to a string in the db
static std::string to_gtfs_string(nt::Type_e enum_type) {
    switch (enum_type) {
        case nt::Type_e::StopArea:
            return "stop_area";
        case nt::Type_e::StopPoint:
            return "stop_point";
        case nt::Type_e::Line:
            return "line";
        case nt::Type_e::Route:
            return "route";
        case nt::Type_e::VehicleJourney:
            return "trip";
        case nt::Type_e::StopTime:
            return "stop_time";
        default:
            throw navitia::exception("unhandled case");
    }
}

void EdPersistor::insert_object_properties(
    const std::map<ed::types::pt_object_header, std::map<std::string, std::string>>& object_properties) {
    this->lotus.prepare_bulk_insert("navitia.object_properties",
                                    {"object_id", "object_type", "property_name", "property_value"});
    for (const auto& pt_property : object_properties) {
        for (const auto& property : pt_property.second) {
            this->lotus.insert({std::to_string(pt_property.first.pt_object->idx),
                                to_gtfs_string(pt_property.first.type), property.first, property.second});
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_admin_stop_areas(const std::vector<types::AdminStopArea*> admin_stop_areas) {
    this->lotus.prepare_bulk_insert("navitia.admin_stop_area", {"admin_id", "stop_area_id"});

    for (const types::AdminStopArea* asa : admin_stop_areas) {
        for (const types::StopArea* sa : asa->stop_area) {
            std::vector<std::string> values{asa->admin, std::to_string(sa->idx)};

            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_comments(const Data& data) {
    this->lotus.prepare_bulk_insert("navitia.comments", {"id", "comment", "type"});
    // we store the db id's
    std::map<std::string, unsigned int> comment_bd_ids;
    unsigned int cpt = 0;
    for (const auto& comment : data.comment_by_id) {
        lotus.insert({std::to_string(cpt), comment.second.value, comment.second.type});

        comment_bd_ids[comment.first] = cpt++;
    }

    this->lotus.finish_bulk_insert();

    this->lotus.prepare_bulk_insert("navitia.ptobject_comments", {"object_type", "object_id", "comment_id"});

    for (const auto& pt_obj_com : data.comments) {
        for (const auto& comment : pt_obj_com.second) {
            lotus.insert({to_gtfs_string(pt_obj_com.first.type), std::to_string(pt_obj_com.first.pt_object->idx),
                          std::to_string(comment_bd_ids[comment])});
        }
    }
    // we then insert the stoptimes comments
    for (const auto& st_com : data.stoptime_comments) {
        for (const auto& comment : st_com.second) {
            lotus.insert({"stop_time", std::to_string(st_com.first->idx), std::to_string(comment_bd_ids[comment])});
        }
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_week_patterns(const std::vector<types::Calendar*>& calendars) {
    this->lotus.prepare_bulk_insert(
        "navitia.week_pattern", {"id", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"});
    std::vector<navitia::type::idx_t> to_insert;
    for (const types::Calendar* cal : calendars) {
        auto id = cal->week_pattern.to_ulong();
        if (!binary_search(to_insert.begin(), to_insert.end(), id)) {
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

void EdPersistor::insert_calendars(const std::vector<types::Calendar*>& calendars) {
    this->lotus.prepare_bulk_insert("navitia.calendar", {"id", "uri", "name", "week_pattern_id"});

    for (const types::Calendar* cal : calendars) {
        auto id = cal->week_pattern.to_ulong();
        std::vector<std::string> values;
        values.push_back(std::to_string(cal->idx));
        values.push_back(navitia::base64_encode(cal->uri));
        values.push_back(cal->name);
        values.push_back(std::to_string(id));
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_exception_dates(const std::vector<types::Calendar*>& calendars) {
    this->lotus.prepare_bulk_insert("navitia.exception_date", {"datetime", "type_ex", "calendar_id"});

    for (const types::Calendar* cal : calendars) {
        for (const auto& except : cal->exceptions) {
            std::vector<std::string> values;
            values.push_back(bg::to_iso_extended_string(except.date));
            values.push_back(navitia::type::to_string(except.type));
            values.push_back(std::to_string(cal->idx));
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_periods(const std::vector<types::Calendar*>& calendars) {
    this->lotus.prepare_bulk_insert("navitia.period", {"id", "calendar_id", "begin_date", "end_date"});
    size_t idx(0);
    for (const types::Calendar* cal : calendars) {
        for (const boost::gregorian::date_period& period : cal->period_list) {
            std::vector<std::string> values{std::to_string(idx++), std::to_string(cal->idx),
                                            bg::to_iso_extended_string(period.begin()),
                                            bg::to_iso_extended_string(period.end())};
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_rel_calendar_line(const std::vector<types::Calendar*>& calendars) {
    this->lotus.prepare_bulk_insert("navitia.rel_calendar_line", {"calendar_id", "line_id"});
    for (const types::Calendar* cal : calendars) {
        for (const types::Line* line : cal->line_list) {
            std::vector<std::string> values;
            values.push_back(std::to_string(cal->idx));
            values.push_back(std::to_string(line->idx));
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_synonyms(const std::map<std::string, std::string>& synonyms) {
    this->lotus.prepare_bulk_insert("georef.synonym", {"id", "key", "value"});
    int count = 1;
    auto it = synonyms.begin();
    while (it != synonyms.end()) {
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
    this->lotus.prepare_bulk_insert("navitia.transition", {"id", "before_change", "after_change", "start_trip",
                                                           "end_trip", "global_condition", "ticket_id"});
    size_t count = 1;
    std::vector<std::vector<std::string>> null_ticket_vector;
    for (const auto& transition_tuple : data.transitions) {
        const navitia::fare::State& start = std::get<0>(transition_tuple);
        const navitia::fare::State& end = std::get<1>(transition_tuple);

        const navitia::fare::Transition& transition = std::get<2>(transition_tuple);

        std::vector<std::string> values;
        values.push_back(std::to_string(count++));
        values.push_back(ed::connectors::to_string(start));
        values.push_back(ed::connectors::to_string(end));
        std::stringstream start_cond;
        std::string sep;
        for (const auto& c : transition.start_conditions) {
            start_cond << sep << c.to_string();
            sep = "&";
        }
        values.push_back(start_cond.str());

        std::stringstream end_cond;
        sep = "";
        for (const auto& c : transition.end_conditions) {
            end_cond << sep << c.to_string();
            sep = "&";
        }
        values.push_back(end_cond.str());
        values.push_back(ed::connectors::to_string(transition.global_condition));
        if (!transition.ticket_key.empty()) {  // we do not add empty ticket, to have null in db
            values.push_back(transition.ticket_key);
        } else {
            null_ticket_vector.push_back(values);
            continue;
        }

        LOG4CPLUS_TRACE(logger, "transition : " << boost::algorithm::join(values, ";"));
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();

    // the postgres connector does not handle well null values,
    // so we add the transition without tickets in a separate bulk
    this->lotus.prepare_bulk_insert("navitia.transition", {
                                                              "id",
                                                              "before_change",
                                                              "after_change",
                                                              "start_trip",
                                                              "end_trip",
                                                              "global_condition",
                                                          });
    for (const auto& null_ticket : null_ticket_vector) {
        this->lotus.insert(null_ticket);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_prices(const ed::Data& data) {
    this->lotus.prepare_bulk_insert("navitia.ticket", {"ticket_key", "ticket_title", "ticket_comment"});
    for (const auto& ticket_it : data.fare_map) {
        const navitia::fare::DateTicket& tickets = ticket_it.second;

        assert(!tickets.tickets.empty());  // by construction there has to be at least one ticket
        std::vector<std::string> values{ticket_it.first, tickets.tickets.front().ticket.caption,
                                        tickets.tickets.front().ticket.comment};
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();

    // we add the dated ticket afterward
    LOG4CPLUS_INFO(logger, "dated ticket");
    this->lotus.prepare_bulk_insert(
        "navitia.dated_ticket", {"id", "ticket_id", "valid_from", "valid_to", "ticket_price", "comments", "currency"});
    int count = 0;
    for (const auto& ticket_it : data.fare_map) {
        const navitia::fare::DateTicket& tickets = ticket_it.second;

        for (const auto& dated_ticket : tickets.tickets) {
            const auto& start = dated_ticket.validity_period.begin();
            const auto& last = dated_ticket.validity_period.end();
            std::vector<std::string> values{std::to_string(count++),
                                            dated_ticket.ticket.key,
                                            bg::to_iso_extended_string(start),
                                            bg::to_iso_extended_string(last),
                                            std::to_string(dated_ticket.ticket.value.value),
                                            dated_ticket.ticket.caption,
                                            dated_ticket.ticket.currency};
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_origin_destination(const ed::Data& data) {
    this->lotus.prepare_bulk_insert("navitia.origin_destination",
                                    {"id", "origin_id", "origin_mode", "destination_id", "destination_mode"});
    size_t cpt(0);  // id of the od
    for (const auto& origin_ticket : data.od_tickets) {
        for (const auto& destination_ticket : origin_ticket.second) {
            std::vector<std::string> values{
                std::to_string(cpt++),
                origin_ticket.first.value,                                 // origin
                ed::connectors::to_string(origin_ticket.first.type),       // origin mode
                destination_ticket.first.value,                            // destination
                ed::connectors::to_string(destination_ticket.first.type),  // destination mode
            };
            this->lotus.insert(values);
        }
    }
    this->lotus.finish_bulk_insert();

    // now we take care of the matching od ticket
    this->lotus.prepare_bulk_insert("navitia.od_ticket", {"id", "od_id", "ticket_id"});
    cpt = 0;
    size_t od_ticket_cpt(0);
    for (const auto& origin_ticket : data.od_tickets) {
        for (const auto& destination_ticket : origin_ticket.second) {
            for (const auto& ticket : destination_ticket.second) {
                std::vector<std::string> values{std::to_string(++od_ticket_cpt), std::to_string(cpt), ticket};
                this->lotus.insert(values);
            }
            cpt++;
        }
    }
    this->lotus.finish_bulk_insert();
}

}  // namespace ed
