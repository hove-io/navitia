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
namespace ed { namespace connectors {

struct GtfsData {
    GtfsData() : production_date(boost::gregorian::date(), boost::gregorian::date()) {}
        //    ("calendar_dates.txt", &GtfsParser::p
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

    boost::gregorian::date_period production_date;///<Période de validité des données
};

class FileParser {
protected:
    GtfsData& gtfs_data;
    Data& data;
    using csv_row = std::vector<std::string>;
    log4cplus::Logger logger;

    virtual const std::vector<std::string> required_headers() const { return {}; }
    virtual void init() {}
    virtual void handle_line(const csv_row& line, bool is_first_line) = 0;
    virtual void finish() {}
public:
    FileParser(GtfsData& gdata, Data& d, std::string file_name) : gtfs_data(gdata), data(d), logger(log4cplus::Logger::getInstance("log")), csv(file_name, ',' , true){}
    CsvReader csv;
    void read();
};

class AgencyGtfsParser : public FileParser {
    int id_c, name_c;
    virtual void init() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override { return {"agency_name", "agency_url", "agency_timezone"}; }
public:
    AgencyGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class ContributorGtfsParser : public FileParser {
    int id_c, name_c;
    virtual void init() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override { return {"contributor_name", "contributor_id"}; }
public:
    ContributorGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class CompanyGtfsParser : public FileParser {
    int id_c,
    name_c,
    company_address_name_c,
    company_address_number_c,
    company_address_type_c,
    company_url_c,
    company_mail_c,
    company_phone_c,
    company_fax_c;
    virtual void init() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override { return {"company_name", "company_id"}; }
public:
    CompanyGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class PhysicalModeGtfsParser : public FileParser {
    int id_c, name_c;
    virtual void init() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override {
        return {"physical_mode_id", "physical_mode_name"};
    }
public:
    PhysicalModeGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class CommercialModeGtfsParser : public FileParser {
    int id_c, name_c;
    virtual void init() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override {
        return {"commercial_mode_id", "commercial_mode_name"};
    }
public:
    CommercialModeGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class StopsGtfsParser : public FileParser {
    int id_c, code_c, lat_c, lon_c,
    type_c, parent_c,
    name_c, desc_c, wheelchair_c,
    sheltered_c, elevator_c,
    escalator_c, bike_accepted_c,
    bike_depot_c, visual_announcement_c,
    audible_announcement_c, appropriate_escort_c,
    appropriate_signage_c;

    int ignored = 0;
    std::vector<types::StopPoint*> wheelchair_heritance;
    virtual void init() override;
    virtual void finish() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override {
        return {"stop_id", "stop_name", "stop_lat", "stop_lon"};
    }
    template <typename T>
    bool parse_common_data(const csv_row& row, T* stop);
public:
    StopsGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class RouteGtfsParser : public FileParser {
    int id_c, short_name_c,
    long_name_c, type_c,
    desc_c,
    color_c, agency_c,
    commercial_mode_c;
    int ignored = 0;
    virtual void init() override;
    virtual void finish() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override {
        return {"route_id", "route_short_name", "route_long_name", "route_type"};
    }
public:
    RouteGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class TransfersGtfsParser : public FileParser {
    int from_c,
    to_c,
    time_c,
    real_time_c,
    wheelchair_c,
    sheltered_c, elevator_c,
    escalator_c, bike_accepted_c,
    bike_depot_c, visual_announcement_c,
    audible_announcement_c, appropriate_escort_c,
    appropriate_signage_c;

    size_t nblines = 0;
    virtual void init() override;
    virtual void finish() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override {
        return {"route_id", "route_short_name", "route_long_name", "route_type"};
    }
public:
    TransfersGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class CalendarGtfsParser : public FileParser {
        int id_c, monday_c,
            tuesday_c, wednesday_c,
            thursday_c, friday_c,
            saturday_c, sunday_c,
            start_date_c, end_date_c;
    size_t nblignes = 0;
    virtual void init() override;
    virtual void finish() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override {
        return {"service_id" , "monday", "tuesday", "wednesday", "thursday",
            "friday", "saturday", "sunday", "start_date", "end_date"};
    }
public:
    CalendarGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class CalendarDatesGtfsParser : public FileParser {
    int id_c, date_c, e_type_c;
    virtual void init() override;
    virtual void finish() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override {
        return {"service_id", "date", "exception_type"};
    }
public:
    CalendarDatesGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

class TripsGtfsParser : public FileParser {
    int id_c, service_c,
            trip_c, headsign_c,
            block_id_c, wheelchair_c,
            trip_desc_c,
            bike_accepted_c,
            air_conditioned_c,
            visual_announcement_c,
            audible_announcement_c,
            appropriate_escort_c,
            appropriate_signage_c,
            school_vehicle_c,
            odt_type_c,
            company_id_c,
            condition_c,
            physical_mode_c;

    int ignored = 0;
    int ignored_vj = 0;

    virtual void init() override;
    virtual void finish() override;
    virtual void handle_line(const csv_row& line, bool is_first_line) override;
    virtual const std::vector<std::string> required_headers() const override {
        return {"route_id", "service_id", "trip_id"};
    }
public:
    TripsGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
};

//    ("trips.txt", &GtfsParser::parse_trips)},
//    ("stop_times.txt", &GtfsParser::parse_stop_times)},
//    ("frequencies.txt", &GtfsParser::parse_frequencies)}

//class TransfertGtfsParser : public FileParser {
//    virtual void init() override;
//    virtual void finish() override;
//    virtual void handle_line(const csv_row& line, bool is_first_line) override;
//    virtual const std::vector<std::string> required_headers() const override {
//        return {"route_id", "route_short_name", "route_long_name", "route_type"};
//    }
//public:
//    RouteGtfsParser(GtfsData& gdata, Data& d, std::string file_name) : FileParser(gdata, d, file_name) {}
//};

class GtfsParser {
private:
    std::string path;///< Chemin vers les fichiers
    log4cplus::Logger logger;

public:
    GtfsData gtfs_data;

    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GtfsParser(const std::string & path);

    /// Constructeur d'une instance vide
    GtfsParser() {}

    /// Remplit la structure passée en paramètre
    void fill(ed::Data& data, const std::string beginning_date = "");

    /// Ajout des objets par défaut
    void fill_default_objects(Data & data);

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
