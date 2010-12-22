#pragma once
#include "type.h"
#include <boost/unordered_map.hpp>
#include <deque>
#include <boost/serialization/deque.hpp>
#include "unordered_map_serialization.h"

/** Lit les fichiers au format General Transit Feed Specifications
  *
  * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
  */
class GtfsParser {
private:
    std::vector<ValidityPattern> validity_patterns;
    boost::unordered_map<std::string, int> validity_patterns_map;

    std::vector<Line> lines;
    boost::unordered_map<std::string, int> lines_map;

    std::vector<Route> routes;
    boost::unordered_map<std::string, int> routes_map;

    std::vector<VehicleJourney> vehicle_journeys;
    boost::unordered_map<std::string, int> vehicle_journeys_map;

    std::vector<StopPoint> stop_points;
    boost::unordered_map<std::string, int> stop_points_map;

    std::vector<StopArea> stop_areas;
    boost::unordered_map<std::string, int> stop_areas_map;

    std::vector<StopTime> stop_times;
    std::string path;///< Chemin vers les fichiers
    boost::gregorian::date start;///< Premier jour où les données sont valables

public:
    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GtfsParser(const std::string & path, const std::string & start_date);

    /// Constructeur d'une instance vide
    GtfsParser() {}

    /// Parse le fichier calendar_dates.txt
    /// Contient les dates définies par jour (et non par période)
    void parse_calendar_dates();

    /// Parse le fichier routes.txt
    /// Contient les routes
    void parse_routes();

    /// Parse le fichier stops.txt
    /// Contient les points d'arrêt et les zones d'arrêt
    void parse_stops();

    /// Parse le fichier stop_times.txt
    /// Contient les horaires
    void parse_stop_times();

    /// Parse le fichier transfers.txt
    /// Contient les correspondances entre arrêts
    void parse_transfers();

    /// Parse le fichier trips.txt
    /// Contient les VehicleJourney
    void parse_trips();

    void save(const std::string & filename);
    void load(const std::string & filename);
    void save_bin(const std::string & filename);
    void load_bin(const std::string & filename);

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & stop_points & stop_areas & validity_patterns & lines & routes & vehicle_journeys & stop_times
                & stop_points_map & stop_areas_map & validity_patterns & lines_map & routes_map & vehicle_journeys_map
                & path & start;
    }

};

/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
  *
  * Retourne -1 s'il y a eu un problème
  */
int time_to_int(const std::string & time);
