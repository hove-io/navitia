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

#pragma once

#include "data.h"
#include "utils/exception.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>
#include <pqxx/pqxx>
#include <unordered_map>
#include <algorithm>
#include "utils/functions.h"

namespace ed{

struct EdReader{

    std::unique_ptr<pqxx::connection> conn;


    EdReader(const std::string& connection_string){
        try{
            conn = std::unique_ptr<pqxx::connection>(new pqxx::connection(connection_string));
        }catch(const pqxx::pqxx_exception& e){
            throw navitia::exception(e.base().what());

        }
    }

    void fill(navitia::type::Data& nav_data, const double min_non_connected_graph_ratio);

private:
    //map d'id en base vers le poiteur de l'objet instancié
    std::unordered_map<idx_t, navitia::type::Network*> network_map;
    std::unordered_map<idx_t, navitia::type::CommercialMode*> commercial_mode_map;
    std::unordered_map<idx_t, navitia::type::PhysicalMode*> physical_mode_map;
    std::unordered_map<idx_t, navitia::type::Company*> company_map;
    std::unordered_map<idx_t, navitia::type::Contributor*> contributor_map;
    std::unordered_map<idx_t, navitia::type::StopArea*> stop_area_map;
    std::unordered_map<idx_t, navitia::type::StopPoint*> stop_point_map;
    std::unordered_map<idx_t, navitia::type::Line*> line_map;
    std::unordered_map<idx_t, navitia::type::Route*> route_map;
    std::unordered_map<idx_t, navitia::type::JourneyPattern*> journey_pattern_map;
    std::unordered_map<idx_t, navitia::type::ValidityPattern*> validity_pattern_map;
    std::unordered_map<idx_t, navitia::type::JourneyPatternPoint*> journey_pattern_point_map;
    std::unordered_map<idx_t, navitia::type::VehicleJourney*> vehicle_journey_map;

    //map d'id en base(osmid) vers l'idx de l'objet
    std::unordered_map<idx_t, navitia::georef::Admin*> admin_map;
    std::unordered_map<idx_t, navitia::georef::Way*> way_map;
    std::unordered_map<idx_t, navitia::georef::POI*> poi_map;
    std::unordered_map<idx_t, navitia::georef::POIType*> poi_type_map;
    std::unordered_map<uint64_t, navitia::type::Calendar*> calendar_map;

    std::unordered_map<uint64_t, uint64_t> node_map;

    //for admin main stop areas, we need this temporary map
    //(we can't use an index since the link is between georef and navitia, and those modules are loaded separatly)
    std::unordered_map<std::string, navitia::georef::Admin*> admin_by_insee_code;


    // ces deux vectors servent pour ne pas charger les graphes secondaires
    std::set<uint64_t> way_to_ignore; //TODO if bottleneck change to flat_set
    std::set<std::pair<uint64_t, uint64_t>> edge_to_ignore;
    std::set<uint64_t> node_to_ignore;

    void fill_meta(navitia::type::Data& data, pqxx::work& work);
    void fill_networks(navitia::type::Data& data, pqxx::work& work);
    void fill_commercial_modes(navitia::type::Data& data, pqxx::work& work);
    void fill_physical_modes(navitia::type::Data& data, pqxx::work& work);
    void fill_companies(navitia::type::Data& data, pqxx::work& work);
    void fill_contributors(nt::Data& data, pqxx::work& work);

    void fill_stop_areas(navitia::type::Data& data, pqxx::work& work);
    void fill_stop_points(navitia::type::Data& data, pqxx::work& work);
    void fill_lines(navitia::type::Data& data, pqxx::work& work);
    void fill_routes(navitia::type::Data& data, pqxx::work& work);
    void fill_journey_patterns(navitia::type::Data& data, pqxx::work& work);
    void fill_validity_patterns(navitia::type::Data& data, pqxx::work& work);

    void fill_journey_pattern_points(navitia::type::Data& data, pqxx::work& work);
    void fill_vehicle_journeys(navitia::type::Data& data, pqxx::work& work);
    void fill_meta_vehicle_journeys(navitia::type::Data& data, pqxx::work& work);

    void fill_stop_times(navitia::type::Data& data, pqxx::work& work);


    void fill_admins(navitia::type::Data& data, pqxx::work& work);
    void fill_admin_stop_areas(navitia::type::Data& data, pqxx::work& work);
    void fill_stop_point_connections(navitia::type::Data& data, pqxx::work& work);
    void fill_poi_types(navitia::type::Data& data, pqxx::work& work);
    void fill_pois(navitia::type::Data& data, pqxx::work& work);
    void fill_poi_properties(navitia::type::Data& data, pqxx::work& work);
    void fill_ways(navitia::type::Data& data, pqxx::work& work);
    void fill_house_numbers(navitia::type::Data& data, pqxx::work& work);
    void fill_vertex(navitia::type::Data& data, pqxx::work& work);
    void fill_graph(navitia::type::Data& data, pqxx::work& work);
    void fill_vector_to_ignore(navitia::type::Data& data, pqxx::work& work, const double percent_delete);
    void fill_graph_vls(navitia::type::Data& data, pqxx::work& work);

    //Synonyms:
    void fill_synonyms(navitia::type::Data& data, pqxx::work& work);

    //les tarifs:
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
};

}
