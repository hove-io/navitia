#pragma once

#include "utils/lotus.h"
#include "data.h"
#include "type/meta_data.h"


namespace ed{

struct EdPersistor{

    Lotus lotus;

    EdPersistor(const std::string& connection_string) : lotus(connection_string){}

    void persist(const ed::Data& data, const navitia::type::MetaData& meta);
    void persist_fare(const ed::Data& data, bool empty_table = false);

private:
    void insert_metadata(const navitia::type::MetaData& meta);
    void insert_sa_sp_properties(const ed::Data& data);
    void insert_stop_areas(const std::vector<types::StopArea*>& stop_areas);

    void insert_networks(const std::vector<types::Network*>& networks);
    void insert_commercial_modes(const std::vector<types::CommercialMode*>& commercial_mode);
    void insert_physical_modes(const std::vector<types::PhysicalMode*>& physical_modes);
    void insert_companies(const std::vector<types::Company*>& companies);
    void insert_contributors(const std::vector<types::Contributor*>& contributors);

    void insert_stop_points(const std::vector<types::StopPoint*>& stop_points);
    void insert_lines(const std::vector<types::Line*>& lines);
    void insert_routes(const std::vector<types::Route*>& routes);
    void insert_journey_patterns(const std::vector<types::JourneyPattern*>& journey_pattern);
    void insert_validity_patterns(const std::vector<types::ValidityPattern*>& validity_patterns);
    void insert_vehicle_properties(const std::vector<types::VehicleJourney*>& vehicle_journeys);
    void insert_vehicle_journeys(const std::vector<types::VehicleJourney*>& vehicle_journeys);

    void insert_journey_pattern_point(const std::vector<types::JourneyPatternPoint*>& journey_pattern_points);

    void insert_stop_times(const std::vector<types::StopTime*>& stop_times);

    void insert_stop_point_connections(const std::vector<types::StopPointConnection*>& connections);
    void insert_journey_pattern_point_connections(const std::vector<types::JourneyPatternPointConnection*>& connections);
    void insert_alias(const std::map<std::string, std::string>& alias);
    void insert_synonyms(const std::map<std::string, std::string>& synonyms);

    /// Inserer les données fare
    void insert_transitions(const ed::Data& data);
    void insert_prices(const ed::Data& data);
    void insert_origin_destination(const ed::Data& data);

    /// suppression de l'ensemble des objets chargés par gtfs déja present en base
    void clean_db();
    void build_relation();

};

}
