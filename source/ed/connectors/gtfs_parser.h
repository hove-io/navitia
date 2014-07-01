/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "ed/data.h"
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <queue>
#include "utils/csv.h"
#include "utils/logger.h"
#include "utils/functions.h"
#include <boost/date_time/time_zone_base.hpp>
#include <boost/date_time/local_time/local_time.hpp>

/**
  * Read General Transit Feed Specifications Files
  *
  * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
  */
namespace ed { namespace connectors {

/**
 * handle all tz specific stuff
 *
 * just used to separate those
 */
struct TzHandler {
    TzHandler() {
        tz_db.load_from_file(std::string(FIXTURES_DIR) + "date_time_zonespec.csv"); //TODO real file handling
    }

    boost::local_time::tz_database tz_db;
    //the GTFS spec defines one tz by agency but put a constraint that all those tz must be the same
    //we thus only put a default tz used if the stop area does not define one
    std::pair<std::string, boost::local_time::time_zone_ptr> default_timezone; //associate the tz with it's name (like 'Europe/paris')
    //we need to store the tz of the sp because it will be usefull is no parent stop area is found for the stop_point
    //(and hence the sp is promoted to sa)
    std::unordered_map<ed::types::StopPoint*, std::string> stop_point_tz;
    std::pair<std::string, boost::local_time::time_zone_ptr> get_tz(const std::string&);

    //since a calendar might need to be split over several period because of dst, we need to track the splited calendar (to split also all vj on this calendar)
    std::multimap<std::string, ed::types::ValidityPattern*> vp_by_name;
    std::multimap<std::string, ed::types::VehicleJourney*> vj_by_name;
    std::map<ed::types::ValidityPattern*, int> offset_by_vp; //each validity pattern are on only one dst, thus we can store the utc_offset
};

/**
 * Temporary structure used in the GTFS parser, mainly to keep a relation between ids and the pointers
 */
struct GtfsData {
    GtfsData() : production_date(boost::gregorian::date(), boost::gregorian::date()) {}
    std::unordered_map<std::string, ed::types::CommercialMode*> commercial_mode_map;
    std::unordered_map<std::string, ed::types::StopPoint*> stop_map;
    std::unordered_map<std::string, ed::types::StopArea*> stop_area_map;
    std::unordered_map<std::string, ed::types::Line*> line_map;
    std::unordered_map<std::string, ed::types::Line*> line_map_by_external_code;
    std::unordered_map<std::string, ed::types::Route*> route_map;
//    std::unordered_map<std::string, ed::types::ValidityPattern*> vp_map;
//    std::unordered_map<std::string, ed::types::VehicleJourney*> vj_map;
    std::unordered_map<std::string, ed::types::PhysicalMode*> physical_mode_map;
    std::unordered_map<std::string, ed::types::Network*> agency_map;
    std::unordered_map<std::string, ed::types::Company*> company_map;
    std::unordered_map<std::string, ed::types::Contributor*> contributor_map;
    typedef std::vector<ed::types::StopPoint*> vector_sp;
    std::unordered_map<std::string, vector_sp> sa_spmap;

    // timezone management
    TzHandler tz;

    // used only by fusio2ed
    std::unordered_map<std::string, std::string> comment_map;
    std::unordered_map<std::string, std::string> odt_conditions_map;
    std::unordered_map<std::string, navitia::type::hasProperties> hasProperties_map;
    std::unordered_map<std::string, navitia::type::hasVehicleProperties> hasVehicleProperties_map;
    std::unordered_map<std::string, ed::types::Calendar*> calendars_map;

    boost::gregorian::date_period production_date;// Data validity period
};

//a bit of abstraction around tz time shift to be able to change from boost::date_time::timezone if we need to
struct period_with_utc_shift {
    period_with_utc_shift(boost::gregorian::date_period p, boost::posix_time::time_duration dur) :
        period(p), utc_shift(dur.total_seconds() / 60)
    {}
    period_with_utc_shift(boost::gregorian::date_period p, int dur) :
        period(p), utc_shift(dur)
    {}
    boost::gregorian::date_period period;
    int utc_shift; //shift in minutes

    //add info to handle the cornercase of the day of the DST (the time of the shift)
};

std::vector<period_with_utc_shift> get_dst_periods(const boost::gregorian::date_period&, const boost::local_time::time_zone_ptr&);
std::vector<period_with_utc_shift> split_over_dst(const boost::gregorian::date_period&, const boost::local_time::time_zone_ptr&);

inline bool has_col(int col_idx, const std::vector<std::string>& row) {
    return col_idx >= 0 && static_cast<size_t>(col_idx) < row.size();
}

inline bool is_active(int col_idx, const std::vector<std::string>& row) {
    return (has_col(col_idx, row) && row[col_idx] == "1");
}

inline bool is_valid(int col_idx, const std::vector<std::string>& row) {
    return (has_col(col_idx, row) && (!row[col_idx].empty()));
}


/**
 * Parser used to parse one kind of file
 * The actual parsing is handed to the Handler who need to have 4 methods
 * - required_headers: for the required file headers
 * - init(Data&) called before reading the file to init what needs to be inited
 * - finish(Data&) called after reading the file to clean and log if needed
 * - handle_line(Data& data, const csv_row& line, bool is_first_line): called at each line
 */
template <typename Handler>
class FileParser {
protected:
    CsvReader csv;
    bool fail_if_no_file;
    Handler handler;
public:
    FileParser(GtfsData& gdata, std::string file_name, bool fail = false) :
        csv(file_name, ',' , true), fail_if_no_file(fail), handler(gdata, csv) {}
    FileParser(GtfsData& gdata, std::stringstream& ss, bool fail = false) :
        csv(ss, ',' , true), fail_if_no_file(fail), handler(gdata, csv)  {}

    void fill(Data& data);
};

/**
 * Base handler
 * for handiness handler can inherit from it (but it's not mandatory)
 * No virtual call will be done, so no need for virtual method
 *
 * provide default method for init, finish and required_headers
 */
struct GenericHandler {
    GenericHandler(GtfsData& gdata, CsvReader& reader) : gtfs_data(gdata), csv(reader) {}

    GtfsData& gtfs_data;
    using csv_row = std::vector<std::string>;
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    CsvReader& csv;

    //default definition (not virtual since no dynamic polymorphism will occur)
    const std::vector<std::string> required_headers() const { return {}; }
    void init(Data&) {}
    void finish(Data&) {}
};

struct AgencyGtfsHandler : public GenericHandler {
    AgencyGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c, time_zone_c;
    void init(Data&);
    types::Network* handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"agency_name", "agency_url", "agency_timezone"}; }
};

//read no file, add a default contributor
struct DefaultContributorHandler : public GenericHandler {
    DefaultContributorHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    void init(Data&);
    void handle_line(Data&, const csv_row&, bool) {} //nothing to do
};

struct StopsGtfsHandler : public GenericHandler {
    StopsGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    using stop_point_and_area = std::pair<ed::types::StopPoint*, ed::types::StopArea*>;

    int id_c,
    code_c,
    name_c,
    desc_c,
    lat_c,
    lon_c,
    type_c,
    parent_c,
    wheelchair_c,
    platform_c,
    timezone_c;

    int ignored = 0;
    std::vector<types::StopPoint*> wheelchair_heritance;
    void init(Data& data);
    void finish(Data& data);
    stop_point_and_area handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"stop_id", "stop_name", "stop_lat", "stop_lon"};
    }
    template <typename T>
    bool parse_common_data(const csv_row& row, T* stop);

    void handle_stop_point_without_area(Data& data); //might be different between stopss parser
};

struct RouteGtfsHandler : public GenericHandler {
    RouteGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, short_name_c,
    long_name_c, type_c,
    desc_c,
    color_c, agency_c;
    int ignored = 0;
    void init(Data&);
    void finish(Data& data);
    ed::types::Line* handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"route_id", "route_short_name", "route_long_name", "route_type"};
    }
};

struct TransfersGtfsHandler : public GenericHandler {
    TransfersGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int from_c,
    to_c,
    time_c;

    size_t nblines = 0;
    void init(Data&);
    void finish(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"from_stop_id", "to_stop_id"};
    }
    virtual void fill_stop_point_connection(ed::types::StopPointConnection* connection, const csv_row& row) const;
};

struct CalendarGtfsHandler : public GenericHandler {
    CalendarGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
        int id_c, monday_c,
            tuesday_c, wednesday_c,
            thursday_c, friday_c,
            saturday_c, sunday_c,
            start_date_c, end_date_c;
    size_t nb_lines = 0;
    void init(Data& data);
    void finish(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"service_id" , "monday", "tuesday", "wednesday", "thursday",
            "friday", "saturday", "sunday", "start_date", "end_date"};
    }
};

struct CalendarDatesGtfsHandler : public GenericHandler {
    CalendarDatesGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, date_c, e_type_c;
    void init(Data&);
    void finish(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"service_id", "date", "exception_type"};
    }
};

struct TripsGtfsHandler : public GenericHandler {
    TripsGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, service_c,
            trip_c, headsign_c,
            block_id_c, wheelchair_c,
            bikes_c;

    int ignored = 0;
    int ignored_vj = 0;

    void init(Data& data);
    void finish(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"route_id", "service_id", "trip_id"};
    }
};
struct StopTimeGtfsHandler : public GenericHandler {
    StopTimeGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, arrival_c,
    departure_c, stop_c,
    stop_seq_c, pickup_c,
    drop_off_c;

    size_t count = 0;
    void init(Data& data);
    void finish(Data& data);
    std::vector<ed::types::StopTime*> handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"trip_id" , "arrival_time", "departure_time", "stop_id", "stop_sequence"};
    }
};

struct FrequenciesGtfsHandler : public GenericHandler {
    FrequenciesGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int trip_id_c, start_time_c,
    end_time_c, headway_secs_c;
    void init(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"trip_id", "start_time", "end_time", "headway_secs"};
    }
};

/**
 * Generic class for GTFS file read
 * the list of elemental parser to be called has to be defined
 */
class GenericGtfsParser {
protected:
    std::string path;///< Chemin vers les fichiers
    log4cplus::Logger logger;
    template <typename Handler>
    void parse(Data&, std::string file_name, bool fail_if_no_file = false);
    template <typename Handler>
    void parse(Data&); //some parser do not need a file since they just add default data

    virtual void parse_files(Data&) = 0;
public:
    GtfsData gtfs_data;

    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GenericGtfsParser(const std::string & path);

    /// Remplit la structure passée en paramètre
    void fill(ed::Data& data, const std::string beginning_date = "");

    /// Ajout des objets par défaut
    void fill_default_modes(Data & data);

    /// Ajout des companies
    void fill_default_company(Data & data);

    ///parse le fichier calendar.txt afin de trouver la période de validité des données
    boost::gregorian::date_period find_production_date(const std::string &beginning_date);

    boost::gregorian::date_period basic_production_date(const std::string &beginning_date);
};

template <typename Handler>
inline void GenericGtfsParser::parse(Data& data, std::string file_name, bool fail_if_no_file) {
    FileParser<Handler> parser (this->gtfs_data, path + "/" + file_name, fail_if_no_file);
    parser.fill(data);
}
template <typename Handler>
inline void GenericGtfsParser::parse(Data& data) {
    FileParser<Handler> parser (this->gtfs_data, "");
    parser.fill(data);
}


/**
 * GTFS parser
 * simply define the list of elemental parsers to use
 */
struct GtfsParser : public GenericGtfsParser {
    virtual void parse_files(Data&);
    GtfsParser(const std::string & path) : GenericGtfsParser(path) {}
};

/// Normalise les external code des stop_point et stop_areas
void normalize_extcodes(Data & data);



/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
  *
  * Retourne -1 s'il y a eu un problème
  */
int time_to_int(const std::string & time);

struct FileNotFoundException {
    std::string filename;
    FileNotFoundException(std::string filename) : filename(filename) {}
};

struct UnableToFindProductionDateException {};

struct InvalidHeaders {
    std::string filename;
    InvalidHeaders(std::string filename) : filename(filename) {}
};

template <typename Handler>
inline void FileParser<Handler>::fill(Data& data) {
    auto logger = log4cplus::Logger::getInstance("log");
    if (! csv.is_open() && ! csv.filename.empty()) {
        LOG4CPLUS_ERROR(logger, "Impossible to read " + csv.filename);
        if ( fail_if_no_file ) {
            throw FileNotFoundException(csv.filename);
        }
        return;
    }

    typename Handler::csv_row headers = handler.required_headers();
    if(!csv.validate(headers)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        " missing headers : " + csv.missing_headers(headers));
        throw InvalidHeaders(csv.filename);
    }
    handler.init(data);

    bool line_read = true;
    while(!csv.eof()) {
        auto row = csv.next();
        if(!row.empty()) {
            handler.handle_line(data, row, line_read);
            line_read = false;
        }
    }
    handler.finish(data);
}

}}
