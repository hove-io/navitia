#pragma once
#include "data.h"
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <queue>

/** Lit les fichiers au format General Transit Feed Specifications
  *
  * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
  */
namespace navimake{ namespace connectors{
class GtfsParser {
private:
    std::string path;///< Chemin vers les fichiers
    boost::gregorian::date start;///< Premier jour où les données sont valables

    // Plusieurs maps pour savoir à quel position est quel objet identifié par son ID GTFS
    boost::unordered_map<std::string, navimake::types::StopPoint*> stop_map;
    boost::unordered_map<std::string, navimake::types::StopArea*> stop_area_map;
    boost::unordered_map<std::string, navimake::types::Route*> route_map;
    boost::unordered_map<std::string, navimake::types::Line*> line_map;
    boost::unordered_map<std::string, navimake::types::ValidityPattern*> vp_map;
    boost::unordered_map<std::string, navimake::types::VehicleJourney*> vj_map;
public:
    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GtfsParser(const std::string & path, const std::string & start_date);

    /// Constructeur d'une instance vide
    GtfsParser() {}

    /// Remplis la structure passée en paramètre
    void fill(navimake::Data& data);

    /// Parse le fichier calendar_dates.txt
    /// Contient les dates définies par jour (et non par période)
    void parse_calendar_dates(Data & data);

    /// Parse le fichier routes.txt
    /// Contient les routes
    void parse_routes(Data & data);

    /// Parse le fichier stops.txt
    /// Contient les points d'arrêt et les zones d'arrêt
    void parse_stops(Data & data);

    /// Parse le fichier stop_times.txt
    /// Contient les horaires
    void parse_stop_times(Data & data);

    /// Parse le fichier transfers.txt
    /// Contient les correspondances entre arrêts
    void parse_transfers(Data & data);

    /// Parse le fichier trips.txt
    /// Contient les VehicleJourney
    void parse_trips(Data & data);
};

/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
  *
  * Retourne -1 s'il y a eu un problème
  */
int time_to_int(const std::string & time);
}}
