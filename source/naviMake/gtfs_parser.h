#pragma once
#include "data.h"
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <queue>
#include "utils/csv.h"
#include <utils/logger.h>

/** Lit les fichiers au format General Transit Feed Specifications
  *
  * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
  */
namespace navimake{ namespace connectors{
class GtfsParser {
private:
    std::string path;///< Chemin vers les fichiers

    // Plusieurs maps pour savoir à quel position est quel objet identifié par son ID GTFS
    boost::unordered_map<std::string, navimake::types::CommercialMode*> commercial_mode_map;
    boost::unordered_map<std::string, navimake::types::StopPoint*> stop_map;
    boost::unordered_map<std::string, navimake::types::StopArea*> stop_area_map;
    boost::unordered_map<std::string, navimake::types::Line*> line_map;
    boost::unordered_map<std::string, navimake::types::ValidityPattern*> vp_map;
    boost::unordered_map<std::string, navimake::types::VehicleJourney*> vj_map;
    boost::unordered_map<std::string, navimake::types::PhysicalMode*> mode_map;
    boost::unordered_map<std::string, navimake::types::Network*> agency_map;
    typedef std::vector<navimake::types::StopPoint*> vector_sp;
    std::unordered_map<std::string, vector_sp> sa_spmap;
    log4cplus::Logger logger;

public:
    boost::gregorian::date_period production_date;///<Période de validité des données

    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GtfsParser(const std::string & path);

    /// Constructeur d'une instance vide
    GtfsParser() : production_date(boost::gregorian::date(), boost::gregorian::date()) {}

    /// Remplit la structure passée en paramètre
    void fill(navimake::Data& data, const std::string beginning_date = "");

    /// Remplit les commercial_mode et les physical_mode
    void fill_modes(Data & data);

    /// Parse le fichier des agency, on s'en sert pour remplir les network
    void parse_agency(Data & data, CsvReader &csv);

    /// Parse le fichier calendar.txt
    /// Remplit les validity_patterns par période
    void parse_calendar(Data & data, CsvReader &csv);

    /// Parse le fichier calendar_dates.txt
    /// Contient les dates définies par jour (et non par période)
    void parse_calendar_dates(Data & data, CsvReader &csv);

    /// Parse le fichier journey_patterns.txt
    /// Contient les lignes (au sens navitia)
    void parse_lines(Data & data, CsvReader &csv);

    /// Parse le fichier stops.txt
    /// Contient les points d'arrêt et les zones d'arrêt
    void parse_stops(Data & data, CsvReader &csv);

    /// Parse le fichier stop_times.txt
    /// Contient les horaires
    void parse_stop_times(Data & data, CsvReader &csv);

    /// Parse le fichier transfers.txt
    /// Contient les correspondances entre arrêts
    void parse_transfers(Data & data, CsvReader &csv);

    /// Parse le fichier trips.txt
    /// Contient les VehicleJourney
    void parse_trips(Data & data, CsvReader &csv);

    /// Parse le fichier frequencies.txt
    /// Contient les fréquencs
    void parse_frequencies(Data & data, CsvReader &csv);
    //
    ///parse le fichier calendar.txt afin de trouver la période de validité des données
    boost::gregorian::date_period find_production_date(const std::string beginning_date);

    boost::gregorian::date_period basic_production_date(const std::string beginning_date);
};

/// Normalise les external code des stop_point et stop_areas
void normalize_extcodes(Data & data);

/// Construit les journey_patterns en retrouvant les paterns à partir des VJ
void build_journey_patterns(Data & data);

/// Construit les journey_patternpoint
void build_journey_pattern_points(Data & data);

/// Ajoute une connection entre deux journey_pattern_point
void add_journey_pattern_point_connection(navimake::types::JourneyPatternPoint *rp1, navimake::types::JourneyPatternPoint *rp2, int length,
                                std::multimap<std::string, navimake::types::JourneyPatternPointConnection> &journey_pattern_point_connections);

/// Construit les connections pour les correspondances garanties
void build_journey_pattern_point_connections(Data & data);

/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
  *
  * Retourne -1 s'il y a eu un problème
  */
int time_to_int(const std::string & time);

struct FileNotFoundException{
    std::string filename;
    FileNotFoundException(std::string filename) : filename(filename) {}
};

struct UnableToFindProductionDateException {};

struct InvalidHeaders{
    std::string filename;
    InvalidHeaders(std::string filename) : filename(filename) {}
};
}}
