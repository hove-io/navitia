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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "data.h"
#include "utils/functions.h"
#include "utils/exception.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>
#include <memory>

#include <pqxx/pqxx>

#include <unordered_map>
#include <algorithm>

namespace ed {

struct EdReader {
    std::unique_ptr<pqxx::connection> conn;

    EdReader(const std::string& connection_string) {
        try {
            conn = std::make_unique<pqxx::connection>(connection_string);
        } catch (const pqxx::pqxx_exception& e) {
            throw navitia::exception(e.base().what());
        }
    }

    void fill(navitia::type::Data& data,
              const double min_non_connected_graph_ratio,
              const bool export_georef_edges_geometries);

    // for admin main stop areas, we need this temporary map
    //(we can't use an index since the link is between georef and navitia, and those modules are loaded separatly)
    std::unordered_map<std::string, navitia::georef::Admin*> admin_by_insee_code;
    std::unordered_map<std::string, navitia::type::Address*> address_by_address_id;

private:
    // map d'id en base vers le poiteur de l'objet instancié
    std::unordered_map<idx_t, navitia::type::Network*> network_map;
    std::unordered_map<idx_t, navitia::type::CommercialMode*> commercial_mode_map;
    std::unordered_map<idx_t, navitia::type::PhysicalMode*> physical_mode_map;
    std::unordered_map<idx_t, navitia::type::Company*> company_map;
    std::unordered_map<idx_t, navitia::type::Contributor*> contributor_map;
    std::unordered_map<idx_t, navitia::type::Dataset*> dataset_map;
    std::unordered_map<idx_t, navitia::type::StopArea*> stop_area_map;
    std::unordered_map<std::string, idx_t> uri_to_idx_stop_area;
    std::unordered_map<idx_t, navitia::type::StopPoint*> stop_point_map;
    std::unordered_map<std::string, idx_t> uri_to_idx_stop_point;
    std::unordered_map<std::string, navitia::type::AccessPoint*> access_point_map;
    std::unordered_map<idx_t, navitia::type::Line*> line_map;
    std::unordered_map<idx_t, navitia::type::LineGroup*> line_group_map;
    std::unordered_map<idx_t, navitia::type::Route*> route_map;
    std::unordered_map<idx_t, navitia::type::ValidityPattern*> validity_pattern_map;
    std::unordered_map<idx_t, navitia::type::VehicleJourney*> vehicle_journey_map;
    std::unordered_map<idx_t, const navitia::type::TimeZoneHandler*> timezone_map;
    std::unordered_map<idx_t, boost::shared_ptr<nt::LineString>> shapes_map;

    // stop_times by vj idx
    std::unordered_map<idx_t, std::vector<navitia::type::StopTime>> sts_from_vj;

    // we need a temporary structure to store the comments on the stop times
    std::unordered_map<idx_t, std::vector<nt::Comment>> stop_time_comments;
    std::unordered_map<idx_t, std::vector<nt::Comment>> vehicle_journey_comments;
    std::unordered_map<idx_t, std::string> stop_time_headsigns;
    using StKey = std::pair<idx_t, uint16_t>;  // idx ed vj, order stop time
    std::unordered_map<idx_t, StKey> id_to_stop_time_key;

    // map d'id en base(osmid) vers l'idx de l'objet
    std::unordered_map<idx_t, navitia::georef::Admin*> admin_map;
    std::unordered_map<idx_t, navitia::georef::Way*> way_map;
    std::unordered_map<idx_t, navitia::georef::POI*> poi_map;
    std::unordered_map<idx_t, navitia::georef::POIType*> poi_type_map;
    std::unordered_map<uint64_t, navitia::type::Calendar*> calendar_map;
    std::unordered_map<uint64_t, navitia::type::AssociatedCalendar*> associated_calendar_map;

    std::unordered_map<uint64_t, uint64_t> node_map;

    // ces deux vectors servent pour ne pas charger les graphes secondaires
    std::set<uint64_t> way_to_ignore;  // TODO if bottleneck change to flat_set
    std::unordered_set<uint64_t> node_to_ignore;
    using EdgeId = std::pair<uint64_t, uint64_t>;
    navitia::flat_enum_map<navitia::type::Mode_e, std::set<EdgeId>> edge_to_ignore_by_modes;

    void fill_meta(navitia::type::Data& nav_data, pqxx::work& work);
    void fill_feed_infos(navitia::type::Data& data, pqxx::work& work);
    void fill_timezones(navitia::type::Data& data, pqxx::work& work);
    void fill_networks(navitia::type::Data& data, pqxx::work& work);
    void fill_commercial_modes(navitia::type::Data& data, pqxx::work& work);
    void fill_physical_modes(navitia::type::Data& data, pqxx::work& work);
    void fill_companies(navitia::type::Data& data, pqxx::work& work);
    void fill_contributors(nt::Data& data, pqxx::work& work);
    void fill_datasets(nt::Data& data, pqxx::work& work);

    void fill_stop_areas(navitia::type::Data& data, pqxx::work& work);
    void fill_stop_points(navitia::type::Data& data, pqxx::work& work);
    void fill_access_point_field(navitia::type::AccessPoint* access_point,
                                 const pqxx::result::iterator const_it,
                                 const bool from_access_point,
                                 const std::string& sp_id);
    void fill_access_points(navitia::type::Data& data, pqxx::work& work);
    void fill_ntfs_addresses(pqxx::work& work);
    void fill_lines(navitia::type::Data& data, pqxx::work& work);
    void fill_line_groups(navitia::type::Data& data, pqxx::work& work);

    void fill_routes(navitia::type::Data& data, pqxx::work& work);
    void fill_validity_patterns(navitia::type::Data& data, pqxx::work& work);

    void fill_vehicle_journeys(navitia::type::Data& data, pqxx::work& work);
    void fill_associated_calendar(navitia::type::Data& data, pqxx::work& work);
    void fill_meta_vehicle_journeys(navitia::type::Data& data, pqxx::work& work);

    void fill_shapes(nt::Data& data, pqxx::work& work);
    void fill_stop_times(navitia::type::Data& data, pqxx::work& work);
    void finish_stop_times(navitia::type::Data& data);

    void fill_comments(navitia::type::Data& data, pqxx::work& work);

    void fill_admins(navitia::type::Data& nav_data, pqxx::work& work);
    void fill_admin_stop_areas(navitia::type::Data& data, pqxx::work& work);
    void fill_admins_postal_codes(navitia::type::Data& data, pqxx::work& work);
    void fill_object_codes(navitia::type::Data& data, pqxx::work& work);
    void fill_stop_point_connections(navitia::type::Data& data, pqxx::work& work);
    void fill_poi_types(navitia::type::Data& data, pqxx::work& work);
    void fill_pois(navitia::type::Data& data, pqxx::work& work);
    void fill_poi_properties(navitia::type::Data& data, pqxx::work& work);
    void fill_ways(navitia::type::Data& data, pqxx::work& work);
    void fill_house_numbers(navitia::type::Data& data, pqxx::work& work);
    void fill_vertex(navitia::type::Data& data, pqxx::work& work);
    void fill_graph(navitia::type::Data& data, pqxx::work& work, bool export_georef_edges_geometries);
    boost::optional<navitia::time_res_traits::sec_type> get_duration(nt::Mode_e mode,
                                                                     double len,
                                                                     double speed,
                                                                     uint64_t source,
                                                                     uint64_t target);
    boost::optional<navitia::time_res_traits::sec_type> get_duration(nt::Mode_e mode,
                                                                     double len,
                                                                     uint64_t source,
                                                                     uint64_t target);
    void fill_vector_to_ignore(pqxx::work& work, const double min_non_connected_graph_ratio);
    void fill_graph_bss(navitia::type::Data& data, pqxx::work& work);
    void fill_graph_parking(navitia::type::Data& data, pqxx::work& work);

    // Synonyms:
    void fill_synonyms(navitia::type::Data& data, pqxx::work& work);

    // les tarifs:
    void fill_prices(navitia::type::Data& data, pqxx::work& work);
    void fill_transitions(navitia::type::Data& data, pqxx::work& work);
    void fill_origin_destinations(navitia::type::Data& data, pqxx::work& work);

    // la fiche horaire par période
    void fill_calendars(navitia::type::Data& data, pqxx::work& work);
    void fill_periods(navitia::type::Data& data, pqxx::work& work);
    void fill_exception_dates(navitia::type::Data& data, pqxx::work& work);
    void fill_rel_calendars_lines(navitia::type::Data& data, pqxx::work& work);

    /// les relations admin et les autres objets
    void build_rel_way_admin(navitia::type::Data& data, pqxx::work& work);
    void build_rel_admin_admin(navitia::type::Data& data, pqxx::work& work);

    /// coherence check for logging purpose
    void check_coherence(navitia::type::Data& data) const;
    log4cplus::Logger log = log4cplus::Logger::getInstance("log");
};

}  // namespace ed
