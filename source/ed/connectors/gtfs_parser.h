/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "ed/data.h"
#include <boost/unordered_map.hpp>
#include <queue>
#include "utils/csv.h"
#include "utils/logger.h"
#include "utils/functions.h"
#include <boost/container/flat_set.hpp>
#include <boost/date_time/time_zone_base.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <utility>

#include "tz_db_wrapper.h"

/**
 * Read General Transit Feed Specifications Files
 *
 * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
 */
namespace ed {
namespace connectors {

static const int UNKNOWN_COLUMN = -1;
static const int DEFAULT_PATHWAYS_VALUE = -1;

/** Return the type enum corresponding to the string*/
nt::Type_e get_type_enum(const std::string&);
nt::RTLevel get_rtlevel_enum(const std::string& str);

/**
 * handle all tz specific stuff
 *
 * just used to separate those
 */
struct TzHandler {
    TzHandler() : tz_db(ed::utils::fill_tz_db()) {}

    boost::local_time::tz_database tz_db;
    // we need to store the tz of the sp because it will be usefull is no parent stop area is found for the stop_point
    //(and hence the sp is promoted to sa)
    std::unordered_map<ed::types::StopPoint*, std::string> stop_point_tz;
    std::pair<std::string, boost::local_time::time_zone_ptr> get_tz(const std::string&);

    // since a calendar might need to be split over several period because of dst, we need to track the splited calendar
    // (to split also all vj on this calendar)
    std::multimap<std::string, ed::types::ValidityPattern*> vp_by_name;
    std::multimap<std::string, ed::types::VehicleJourney*> vj_by_name;

    // since 2 files are mandatory to build validity pattern
    // we need to build first the list of validity pattern,
    // then split them all by dst
    std::map<std::string, ed::types::ValidityPattern> non_split_vp;
};

/**
 * Temporary structure used in the GTFS parser, mainly to keep a relation between ids and the pointers
 */
struct GtfsData {
    std::unordered_map<std::string, ed::types::CommercialMode*> commercial_mode_map;
    std::unordered_map<std::string, ed::types::StopPoint*> stop_point_map;
    std::unordered_map<std::string, ed::types::StopArea*> stop_area_map;
    std::unordered_map<std::string, ed::types::AccessPoint*> access_point_map;
    std::unordered_map<std::string, ed::types::PathWay*> pathway_map;
    std::unordered_map<std::string, ed::types::Line*> line_map;
    std::unordered_map<std::string, ed::types::Line*> line_map_by_external_code;
    std::unordered_map<std::string, ed::types::LineGroup*> line_group_map;
    std::unordered_map<std::string, ed::types::Route*> route_map;
    std::unordered_map<std::string, ed::types::MetaVehicleJourney*> metavj_by_external_code;
    std::unordered_map<std::string, ed::types::PhysicalMode*> physical_mode_map;
    std::unordered_map<std::string, ed::types::Network*> network_map;
    ed::types::Network* default_network = nullptr;
    std::unordered_map<std::string, ed::types::Company*> company_map;
    ed::types::Company* get_or_create_default_company(Data& data);
    ed::types::Company* default_company = nullptr;
    std::unordered_map<std::string, ed::types::Contributor*> contributor_map;
    std::unordered_map<std::string, ed::types::Dataset*> dataset_map;

    std::unordered_map<std::string, std::vector<ed::types::StopTime*>>
        stop_time_map;  // there may be several stoptimes for one id because of dst

    typedef std::vector<ed::types::StopPoint*> vector_sp;
    std::unordered_map<std::string, vector_sp> sa_spmap;

    std::set<std::string> vj_uri;  // we store all vj_uri not to give twice the same uri (since we split some)

    // for gtfs we group the comments together, so the key is the comment and the value is the id of the comment
    std::unordered_map<std::string, std::string> comments_id_map;

    // Store lines linked for each group to avoid duplicates
    std::unordered_map<std::string, boost::container::flat_set<std::string>> linked_lines_by_line_group_uri;

    // timezone management
    TzHandler tz;

    // used only by fusio2ed
    std::unordered_map<std::string, std::string> odt_conditions_map;
    std::unordered_map<std::string, navitia::type::hasProperties> hasProperties_map;
    std::unordered_map<std::string, navitia::type::hasVehicleProperties> hasVehicleProperties_map;
    std::unordered_map<std::string, ed::types::Calendar*> calendars_map;
    ed::types::CommercialMode* get_or_create_default_commercial_mode(Data& data);
    ed::types::CommercialMode* default_commercial_mode = nullptr;
    ed::types::PhysicalMode* get_or_create_default_physical_mode(Data& data);
    ed::types::PhysicalMode* default_physical_mode = nullptr;

    ed::types::Network* get_or_create_default_network(ed::Data&);
};

void split_validity_pattern_over_dst(Data& data, GtfsData& gtfs_data);

// Africa/Abidjan is equivalent to utc since there is no dst and 0 offset from utc
const std::string UTC_TIMEZONE = "Africa/Abidjan";

inline bool has_col(int col_idx, const std::vector<std::string>& row) {
    return col_idx >= 0 && static_cast<size_t>(col_idx) < row.size();
}

inline bool is_active(int col_idx, const std::vector<std::string>& row) {
    return (has_col(col_idx, row) && row[col_idx] == "1");
}

inline bool is_valid(int col_idx, const std::vector<std::string>& row) {
    return (has_col(col_idx, row) && (!row[col_idx].empty()));
}

std::string generate_unique_vj_uri(const GtfsData& gtfs_data, const std::string original_uri, int cpt_vj);

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
    FileParser(GtfsData& gdata, std::string file_name, bool fail = false)
        : csv(file_name, ',', true), fail_if_no_file(fail), handler(gdata, csv) {}
    FileParser(GtfsData& gdata, std::stringstream& ss, bool fail = false)
        : csv(ss, ',', true), fail_if_no_file(fail), handler(gdata, csv) {}

    bool fill(Data& data);
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

    // default definition (not virtual since no dynamic polymorphism will occur)
    const std::vector<std::string> required_headers() const { return {}; }
    void init(Data&) {}
    void finish(Data&) {}
};

struct FeedInfoGtfsHandler : public GenericHandler {
    FeedInfoGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int feed_publisher_name_c, feed_publisher_url_c, feed_start_date_c, feed_end_date_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
};

struct AgencyGtfsHandler : public GenericHandler {
    AgencyGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, name_c, time_zone_c;
    void init(Data&);
    types::Network* handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"agency_name", "agency_url", "agency_timezone"}; }
};

// read no file, add a default contributor
struct DefaultContributorHandler : public GenericHandler {
    DefaultContributorHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    void init(Data&);
    void handle_line(Data&, const csv_row&, bool) {}  // nothing to do
};

struct StopsGtfsHandler : public GenericHandler {
    StopsGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader), stop_code_is_present(false) {}
    using stop_point_and_area = std::pair<ed::types::StopPoint*, ed::types::StopArea*>;

    int id_c, code_c, name_c, desc_c, lat_c, lon_c, zone_c, type_c, parent_c, wheelchair_c, platform_c, timezone_c,
        ext_code_c;

    int ignored = 0;
    bool stop_code_is_present;
    std::vector<types::StopPoint*> wheelchair_heritance;
    void init(Data& data);
    void finish(Data& data);
    stop_point_and_area handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"stop_id", "stop_name", "stop_lat", "stop_lon"}; }
    template <typename T>
    bool parse_common_data(const csv_row& row, T* stop);
    template <typename T>
    bool add_gtfs_stop_code(Data& data, const T& obj, const std::string& value);

    void handle_stop_point_without_area(Data& data);  // might be different between stops parser

    ed::types::StopPoint* build_stop_point(Data& data, const csv_row& line);
    ed::types::StopArea* build_stop_area(Data& data, const csv_row& line);
    ed::types::AccessPoint* build_access_point(Data& data, const csv_row& row);
    bool is_duplicate(const csv_row& row);
};

template <typename T>
bool StopsGtfsHandler::add_gtfs_stop_code(Data& data, const T& obj, const std::string& value) {
    if (value != "") {
        data.add_object_code(obj, value, "gtfs_stop_code");
        return true;
    }
    return false;
}

struct RouteGtfsHandler : public GenericHandler {
    RouteGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, short_name_c, long_name_c, type_c, desc_c, color_c, text_color_c, agency_c;
    int ignored = 0;
    void init(Data&);
    void finish(Data& data);
    ed::types::Line* handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"route_id", "route_short_name", "route_long_name", "route_type"};
    }
};

struct PathWayGtfsHandler : public GenericHandler {
    PathWayGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int pathway_id_c, from_stop_id_c, to_stop_id_c, pathway_mode_c, is_bidirectional_c, length_c, traversal_time_c,
        stair_count_c, max_slope_c, min_width_c, signposted_as_c, reversed_signposted_as_c;
    void init(Data&);
    void finish(Data& data);
    ed::types::PathWay* handle_line(Data& data, const csv_row& line, bool is_first_line);
    int fill_pathway_field(const csv_row& row, const int key, const std::string type);
    const std::vector<std::string> required_headers() const {
        return {"pathway_id", "from_stop_id", "to_stop_id", "pathway_mode", "is_bidirectional"};
    }
};

struct TransfersGtfsHandler : public GenericHandler {
    TransfersGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int from_c, to_c, time_c;

    size_t nblines = 0;
    virtual ~TransfersGtfsHandler() = default;
    void init(Data&);
    void finish(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"from_stop_id", "to_stop_id"}; }
    virtual void fill_stop_point_connection(ed::types::StopPointConnection* connection, const csv_row& row) const;
};

struct CalendarGtfsHandler : public GenericHandler {
    CalendarGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, monday_c, tuesday_c, wednesday_c, thursday_c, friday_c, saturday_c, sunday_c, start_date_c, end_date_c;
    size_t nb_lines = 0;
    void init(Data& data);
    void finish(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"service_id", "monday",   "tuesday", "wednesday",  "thursday",
                "friday",     "saturday", "sunday",  "start_date", "end_date"};
    }
};

struct CalendarDatesGtfsHandler : public GenericHandler {
    CalendarDatesGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, date_c, e_type_c;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"service_id", "date", "exception_type"}; }
};

struct ShapesGtfsHandler : public GenericHandler {
    ShapesGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int shape_id_c = -1, shape_pt_lat_c = -1, shape_pt_lon_c = -1, shape_pt_sequence_c = -1;
    std::map<std::string, std::map<int, navitia::type::GeographicalCoord>> shapes;
    void init(Data&);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    void finish(Data& data);
    const std::vector<std::string> required_headers() const {
        return {"shape_id", "shape_pt_lat", "shape_pt_lon", "shape_pt_sequence"};
    }
};

struct TripsGtfsHandler : public GenericHandler {
    TripsGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int id_c, service_c, trip_c, trip_headsign_c, block_id_c, wheelchair_c, bikes_c, shape_id_c, direction_id_c;

    int ignored = 0;
    int ignored_vj = 0;

    void init(Data& data);
    void finish(Data& data);
    void handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const { return {"route_id", "service_id", "trip_id"}; }

    using RouteId = std::pair<types::Line*, const std::string>;
    std::map<RouteId, types::Route*> routes;

    types::Route* get_or_create_route(Data& data, const RouteId&);
};
struct StopTimeGtfsHandler : public GenericHandler {
    StopTimeGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int trip_c, arrival_c, departure_c, stop_c, stop_seq_c, pickup_c, drop_off_c;

    size_t count = 0;
    void init(Data& data);
    void finish(Data& data);
    std::vector<ed::types::StopTime*> handle_line(Data& data, const csv_row& line, bool is_first_line);
    const std::vector<std::string> required_headers() const {
        return {"trip_id", "arrival_time", "departure_time", "stop_id", "stop_sequence"};
    }
};

struct FrequenciesGtfsHandler : public GenericHandler {
    FrequenciesGtfsHandler(GtfsData& gdata, CsvReader& reader) : GenericHandler(gdata, reader) {}
    int trip_id_c, start_time_c, end_time_c, headway_secs_c;
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
    std::string path;  ///< Chemin vers les fichiers
    log4cplus::Logger logger;
    template <typename Handler>
    bool parse(Data&, std::string file_name, bool fail_if_no_file = false);
    template <typename Handler>
    void parse(Data&);  // some parser do not need a file since they just add default data

    virtual void parse_files(Data&, const std::string& beginning_date = "") = 0;

public:
    GtfsData gtfs_data;

    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GenericGtfsParser(std::string path);
    virtual ~GenericGtfsParser();

    /// Remplit la structure passée en paramètre
    void fill(ed::Data& data, const std::string& beginning_date = "");

    /// Ajout des objets par défaut
    void fill_default_modes(Data& data);

    void manage_production_date(Data& data, const std::string& beginning_date);
    boost::gregorian::date_period complete_production_date(const std::string& beginning_date,
                                                           boost::gregorian::date start_date,
                                                           boost::gregorian::date end_date);
    /// parse le fichier calendar.txt afin de trouver la période de validité des données
    boost::gregorian::date_period find_production_date(const std::string& beginning_date);

    boost::gregorian::date_period basic_production_date(const std::string& beginning_date);
};

template <typename Handler>
inline bool GenericGtfsParser::parse(Data& data, std::string file_name, bool fail_if_no_file) {
    FileParser<Handler> parser(this->gtfs_data, path + "/" + file_name, fail_if_no_file);
    return parser.fill(data);
}
template <typename Handler>
inline void GenericGtfsParser::parse(Data& data) {
    FileParser<Handler> parser(this->gtfs_data, "");
    parser.fill(data);
}

/**
 * GTFS parser
 * simply define the list of elemental parsers to use
 */
struct GtfsParser : public GenericGtfsParser {
    void parse_files(Data&, const std::string& beginning_date) override;
    GtfsParser(const std::string& path) : GenericGtfsParser(path) {}
};

/// Normalise les external code des stop_point et stop_areas
void normalize_extcodes(Data& data);

/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
 *
 * Retourne -1 s'il y a eu un problème
 */
int time_to_int(const std::string& time);

struct FileNotFoundException {
    std::string filename;
    FileNotFoundException(std::string filename) : filename(std::move(filename)) {}
};

struct UnableToFindProductionDateException {};

struct InvalidHeaders {
    std::string filename;
    InvalidHeaders(std::string filename) : filename(std::move(filename)) {}
};

template <typename Handler>
inline bool FileParser<Handler>::fill(Data& data) {
    auto logger = log4cplus::Logger::getInstance("log");
    if (!csv.is_open() && !csv.filename.empty()) {
        if (fail_if_no_file) {
            LOG4CPLUS_FATAL(logger, "Impossible to read " << csv.filename);
            throw FileNotFoundException(csv.filename);
        }
        LOG4CPLUS_WARN(logger, "Impossible to read " << csv.filename);
        return false;
    }

    typename Handler::csv_row headers = handler.required_headers();
    if (!csv.validate(headers)) {
        LOG4CPLUS_FATAL(
            logger, "Error while reading " << csv.filename << " missing headers : " << csv.missing_headers(headers));
        throw InvalidHeaders(csv.filename);
    }
    handler.init(data);

    bool line_read = true;
    while (!csv.eof()) {
        auto row = csv.next();
        if (!row.empty()) {
            handler.handle_line(data, row, line_read);
            line_read = false;
        }
    }
    handler.finish(data);

    return true;
}

template <typename T>
bool empty(const std::pair<T, T>& r) {
    return r.first == r.second;
}

template <typename T>
bool more_than_one_elt(const std::pair<T, T>& r) {
    return !empty(r) && boost::next(r.first) != r.second;
}

}  // namespace connectors
}  // namespace ed
