#pragma once
#include "data.h"
#include <boost/unordered_map.hpp>

/** Lit les fichiers au format General Transit Feed Specifications
  *
  * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
  */
class GtfsParser {
private:
    std::string path;///< Chemin vers les fichiers
    boost::gregorian::date start;///< Premier jour où les données sont valables
    Data data;

    // Plusieurs maps pour savoir à quel position est quel objet identifié par son ID GTFS
    boost::unordered_map<std::string, int> stop_map;
    boost::unordered_map<std::string, int> route_map;
    boost::unordered_map<std::string, int> line_map;
    boost::unordered_map<std::string, int> vp_map;
    boost::unordered_map<std::string, int> vj_map;
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


    inline Data getData(){return data;}

};

/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
  *
  * Retourne -1 s'il y a eu un problème
  */
int time_to_int(const std::string & time);
