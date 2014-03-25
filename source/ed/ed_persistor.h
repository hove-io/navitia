#pragma once

#include "utils/lotus.h"
#include "utils/base64_encode.h"
#include "data.h"
#include "type/meta_data.h"
#include "utils/functions.h"


namespace ed{

struct EdPersistor{
    Lotus lotus;
    log4cplus::Logger logger;

    EdPersistor(const std::string& connection_string) : lotus(connection_string),
                        logger(log4cplus::Logger::getInstance("log")){}

    void persist(const ed::Data& data, const navitia::type::MetaData& meta);
    void persist_fare(const ed::Data& data);
    /// Données Georef
    void persist(const ed::Georef& data);
    void build_ways();
    void clean_georef();

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

    /// suppression de l'ensemble des objets chargés par gtfs déja present en base
    void clean_db();

    /// Données Georef
    void insert_admins(const ed::Georef& data);
    void insert_ways(const ed::Georef& data);
    void insert_nodes(const ed::Georef& data);
    void insert_house_numbers(const ed::Georef& data);
    void insert_edges(const ed::Georef& data);
    void insert_poi_types(const ed::Georef& data);
    void insert_pois(const ed::Georef& data);
    void build_relation_way_admin(const ed::Georef& data);
    void update_boundary();

    std::string to_geographic_point(const navitia::type::GeographicalCoord& coord) const;

};

}
