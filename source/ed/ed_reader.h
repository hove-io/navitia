#pragma once

#include "data.h"
#include <pqxx/pqxx>
#include "utils/exception.h"
#include <unordered_map>

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>
#include <algorithm>

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

    void fill(navitia::type::Data& nav_data);

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

//    std::unordered_map<uint64_t, idx_t> admin_map;
    std::unordered_map<uint64_t, idx_t> node_map;

    // ces deux vectors servent pour ne pas charger les graphes secondaires
    std::vector<uint64_t> way_no_ignore;
    std::vector<std::string> edge_to_ignore;
    std::vector<uint64_t> node_to_ignore;

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

    void fill_stop_times(navitia::type::Data& data, pqxx::work& work);


    void fill_admins(navitia::type::Data& data, pqxx::work& work);
    void fill_stop_point_connections(navitia::type::Data& data, pqxx::work& work);
    //void insert_journey_pattern_point_connections(const std::vector<types::JourneyPatternPointConnection*>& connections);
    void fill_poi_types(navitia::type::Data& data, pqxx::work& work);
    void fill_pois(navitia::type::Data& data, pqxx::work& work);
    void fill_ways(navitia::type::Data& data, pqxx::work& work);
    void fill_house_numbers(navitia::type::Data& data, pqxx::work& work);
    void fill_vertex(navitia::type::Data& data, pqxx::work& work);
    void fill_graph(navitia::type::Data& data, pqxx::work& work);
    void fill_vector_to_ignore(navitia::type::Data& data, pqxx::work& work);
    void fill_graph_vls(navitia::type::Data& data, pqxx::work& work);

    /// les relations admin et les autres objets
    void build_rel_stop_point_admin(navitia::type::Data& data, pqxx::work& work);
    void build_rel_stop_area_admin(navitia::type::Data& data, pqxx::work& work);
    void build_rel_way_admin(navitia::type::Data& data, pqxx::work& work);
    void build_rel_poi_admin(navitia::type::Data& data, pqxx::work& work);
    void build_rel_admin_admin(navitia::type::Data& data, pqxx::work& work);
};

}
