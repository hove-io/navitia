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

    // Plusieurs maps pour savoir à quel position est quel objet identifié par son ID GTFS
    boost::unordered_map<std::string, navimake::types::ModeType*> mode_type_map;
    boost::unordered_map<std::string, navimake::types::StopPoint*> stop_map;
    boost::unordered_map<std::string, navimake::types::StopArea*> stop_area_map;
    boost::unordered_map<std::string, navimake::types::Line*> line_map;
    boost::unordered_map<std::string, navimake::types::ValidityPattern*> vp_map;
    boost::unordered_map<std::string, navimake::types::VehicleJourney*> vj_map;
    boost::unordered_map<std::string, navimake::types::Mode*> mode_map;
    boost::unordered_map<std::string, navimake::types::Network*> agency_map;

public:
    boost::gregorian::date_period production_date;///<Période de validité des données

    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GtfsParser(const std::string & path);

    /// Constructeur d'une instance vide
    GtfsParser() : production_date(boost::gregorian::date(), boost::gregorian::date()) {}

    /// Remplit la structure passée en paramètre
    void fill(navimake::Data& data);

    /// Remplit les modes types
    void fill_mode_types(Data & data);

    /// Parse le fichier des agency, on s'en sert pour remplir les network
    void parse_agency(Data & data);

    /// Parse le fichier calendar.txt
    /// Remplit les validity_patterns par période
    void parse_calendar(Data & data);

    /// Parse le fichier calendar_dates.txt
    /// Contient les dates définies par jour (et non par période)
    void parse_calendar_dates(Data & data);

    /// Parse le fichier routes.txt
    /// Contient les lignes (au sens navitia)
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
    //
    ///parse le fichier calendar.txt afin de trouver la période de validité des données
    boost::gregorian::date_period find_production_date();
};

/// Normalise les external code des stop_point et stop_areas
void normalize_extcodes(Data & data);

/// Construit les routes en retrouvant les paterns à partir des VJ
void build_routes(Data & data);

/// Construit les routepoint
void build_route_points(Data & data);

/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
  *
  * Retourne -1 s'il y a eu un problème
  */
int time_to_int(const std::string & time);
}}
