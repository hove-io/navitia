#pragma once
#include "ed/data.h"
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <queue>
#include "utils/csv.h"
#include <utils/logger.h>
#include <utils/functions.h>

/** Lit les fichiers au format General Transit Feed Specifications
  *
  * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
  */
namespace ed{ namespace connectors{
class GtfsParser {
private:
    std::string path;///< Chemin vers les fichiers

    // Plusieurs maps pour savoir à quel position est quel objet identifié par son ID GTFS
    std::unordered_map<std::string, ed::types::CommercialMode*> commercial_mode_map;
    std::unordered_map<std::string, ed::types::StopPoint*> stop_map;
    std::unordered_map<std::string, ed::types::StopArea*> stop_area_map;
    std::unordered_map<std::string, ed::types::Line*> line_map;
    std::unordered_map<std::string, ed::types::ValidityPattern*> vp_map;
    std::unordered_map<std::string, ed::types::VehicleJourney*> vj_map;
    std::unordered_map<std::string, ed::types::PhysicalMode*> physical_mode_map;
    std::unordered_map<std::string, ed::types::Network*> agency_map;
    std::unordered_map<std::string, ed::types::Company*> company_map;
    std::unordered_map<std::string, ed::types::Contributor*> contributor_map;
    typedef std::vector<ed::types::StopPoint*> vector_sp;
    std::unordered_map<std::string, vector_sp> sa_spmap;
    log4cplus::Logger logger;

public:
    boost::gregorian::date_period production_date;///<Période de validité des données

    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GtfsParser(const std::string & path);

    /// Constructeur d'une instance vide
    GtfsParser() : production_date(boost::gregorian::date(), boost::gregorian::date()) {}

    /// Remplit la structure passée en paramètre
    void fill(ed::Data& data, const std::string beginning_date = "");

    /// Ajout des objets par défaut
    void fill_default_objects(Data & data);

    /// Remplit les commercial_mode et les physical_mode
    void parse_physical_modes(Data & data, CsvReader &csv);
    void parse_commercial_modes(Data & data, CsvReader &csv);

    /// Parse le fichier des agency, on s'en sert pour remplir les network
    void parse_agency(Data & data, CsvReader &csv);
    /// Parse le fichier des contributor, on s'en sert pour remplir les contributor
    void parse_contributor(Data & data, CsvReader &csv);
    /// Parse le fichier des company, on s'en sert pour remplir les company
    void parse_company(Data & data, CsvReader &csv);

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
    boost::gregorian::date_period find_production_date(const std::string &beginning_date);

    boost::gregorian::date_period basic_production_date(const std::string &beginning_date);
};

/// Normalise les external code des stop_point et stop_areas
void normalize_extcodes(Data & data);



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
