#pragma once
#include "type.h"
#include <map>
#include <deque>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include "data.h"

/** Lit les fichiers au format General Transit Feed Specifications
  *
  * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
  */
class GtfsParser {
private:
    std::string path;///< Chemin vers les fichiers
    boost::gregorian::date start;///< Premier jour où les données sont valables
    Data data;
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

    inline Data getData(){return data;}

 /*   template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & stop_points & stop_areas & validity_patterns & lines & routes & vehicle_journeys & stop_times
                & stop_points_map & stop_areas_map & validity_patterns & lines_map & routes_map & vehicle_journeys_map
                & path & start;
    }*/

};

/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
  *
  * Retourne -1 s'il y a eu un problème
  */
int time_to_int(const std::string & time);
