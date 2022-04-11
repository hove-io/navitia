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

#include "utils/lotus.h"
#include "utils/base64_encode.h"
#include "data.h"
#include "type/meta_data.h"
#include "type/address_from_ntfs.h"
#include "utils/functions.h"

#include <pqxx/pqxx>

namespace ed {

static const int unknown_field = -1;

struct EdPersistor {
    Lotus lotus;
    log4cplus::Logger logger;
    bool parse_pois = true, is_osm_reader;

    std::string poi_source = "";
    std::string street_network_source = "";

    EdPersistor(const std::string& connection_string, const bool is_osm_reader = true);

    std::set<std::string> ignored_uris;

    void persist(const ed::Data& data);
    void persist_fare(const ed::Data& data);
    /// Données Georef
    void persist(const ed::Georef& data);
    /// Poi
    void persist_pois(const ed::Georef& data);
    void clean_georef();
    void clean_poi();
    /// Synonyms
    void clean_synonym();
    void persist_synonym(const std::map<std::string, std::string>& data);

    /// Données Georef
    void insert_admins(const ed::Georef& data);
    void insert_postal_codes(const ed::Georef& data);
    void insert_ways(const ed::Georef& data);
    void insert_nodes(const ed::Georef& data);
    void insert_house_numbers(const ed::Georef& data);
    void insert_edges(const ed::Georef& data);
    void build_relation_way_admin(const ed::Georef& data);

    /// Données POI
    void insert_poi_types(const ed::Georef& data);
    void insert_pois(const ed::Georef& data);
    void insert_poi_properties(const ed::Georef& data);

    void compute_bounding_shape();
    void insert_metadata_georef();

private:
    void insert_feed_info(const std::map<std::string, std::string>& feed_infos);
    void insert_metadata(const navitia::type::MetaData& meta);
    void insert_sa_sp_properties(const ed::Data& data);
    void insert_stop_areas(const std::vector<types::StopArea*>& stop_areas);

    void insert_networks(const std::vector<types::Network*>& networks);
    void insert_commercial_modes(const std::vector<types::CommercialMode*>& commercial_modes);
    void insert_physical_modes(const std::vector<types::PhysicalMode*>& physical_modes);
    void insert_companies(const std::vector<types::Company*>& companies);
    void insert_contributors(const std::vector<types::Contributor*>& contributors);
    void insert_datasets(const std::vector<types::Dataset*>& datasets);

    void insert_stop_points(const std::vector<types::StopPoint*>& stop_points);
    void insert_access_points(const std::vector<types::AccessPoint*>& access_points);
    void insert_pathways(const std::vector<types::PathWay*>& pathways);
    void insert_lines(const std::vector<types::Line*>& lines);
    void insert_line_groups(const std::vector<types::LineGroup*>& groups,
                            const std::vector<types::LineGroupLink>& group_links);
    void insert_routes(const std::vector<types::Route*>& routes);
    void insert_validity_patterns(const std::vector<types::ValidityPattern*>& validity_patterns);
    void insert_vehicle_properties(const std::vector<types::VehicleJourney*>& vehicle_journeys);
    void insert_vehicle_journeys(const std::vector<types::VehicleJourney*>& vehicle_journeys);
    void insert_meta_vj(const std::map<std::string, types::MetaVehicleJourney>& meta_vjs);
    void insert_object_codes(
        const std::map<ed::types::pt_object_header, std::map<std::string, std::vector<std::string>>>& object_codes);
    void insert_shapes(const std::vector<std::shared_ptr<types::Shape>>& shapes);

    void insert_stop_times(const std::vector<types::StopTime*>& stop_times);

    void insert_stop_point_connections(const std::vector<types::StopPointConnection*>& connections);
    void insert_synonyms(const std::map<std::string, std::string>& synonyms);

    void insert_admin_stop_areas(const std::vector<types::AdminStopArea*> admin_stop_areas);

    void insert_object_properties(
        const std::map<ed::types::pt_object_header, std::map<std::string, std::string>>& object_properties);

    void insert_timezones(const navitia::type::TimeZoneHandler&);

    void insert_comments(const Data& data);

    /// Inserer les données fiche horaire par période
    void insert_week_patterns(const std::vector<types::Calendar*>& calendars);
    void insert_calendars(const std::vector<types::Calendar*>& calendars);
    void insert_exception_dates(const std::vector<types::Calendar*>& calendars);
    void insert_periods(const std::vector<types::Calendar*>& calendars);
    void insert_rel_calendar_line(const std::vector<types::Calendar*>& calendars);

    /// Inserer les données fare
    void insert_transitions(const ed::Data& data);
    void insert_prices(const ed::Data& data);
    void insert_origin_destination(const ed::Data& data);

    void insert_addresses_from_ntfs(const std::vector<navitia::type::Address*>& addresses);

    /// suppression de l'ensemble des objets chargés par gtfs déja present en base
    void clean_db();

    std::string to_geographic_point(const navitia::type::GeographicalCoord& coord) const;

    size_t default_timezone_idx = 0;
};

}  // namespace ed
