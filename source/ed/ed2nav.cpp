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

#include "ed2nav.h"

#include "conf.h"
#include "ed_reader.h"
#include "type/meta_data.h"
#include "utils/exception.h"
#include "utils/functions.h"
#include "utils/init.h"
#include "utils/timer.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/geometry.hpp>
#include <pqxx/pqxx>

#include <fstream>
#include <iostream>

namespace po = boost::program_options;
namespace pt = boost::posix_time;
namespace georef = navitia::georef;
namespace ed {

#if PQXX_COMPATIBILITY
typedef pqxx::tuple pqxx_row;
#else
using pqxx_row = pqxx::row;
#endif

// A functor that first asks to GeoRef the admins of coord, and, if
// GeoRef found nothing, asks to the cities database.
struct FindAdminWithCities {
    using AdminMap = std::unordered_map<std::string, georef::Admin*>;
    using result_type = std::vector<georef::Admin*>;

    boost::shared_ptr<pqxx::connection> conn;
    std::shared_ptr<pqxx::work> work;
    georef::GeoRef& georef;
    AdminMap added_admins;
    AdminMap insee_admins_map;
    size_t nb_call = 0;
    size_t nb_uninitialized = 0;
    size_t nb_georef = 0;
    std::map<size_t, size_t> cities_stats;  // number of response for size of the result

    FindAdminWithCities(const std::string& connection_string, georef::GeoRef& gr)
        : conn(boost::make_shared<pqxx::connection>(connection_string)), georef(gr) {
        work = std::make_shared<pqxx::work>(*conn);
    }

    FindAdminWithCities(const FindAdminWithCities&) = default;
    FindAdminWithCities& operator=(const FindAdminWithCities&) = delete;
    FindAdminWithCities(FindAdminWithCities&&) noexcept = default;
    FindAdminWithCities& operator=(FindAdminWithCities&&) = delete;
    ~FindAdminWithCities() {
        if (nb_call == 0) {
            return;
        }

        auto log = log4cplus::Logger::getInstance("ed2nav::FindAdminWithCities");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_call << " calls");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_uninitialized << " calls with uninitialized or zeroed coord");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_georef << " GeoRef responses");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << added_admins.size() << " admins added using cities");
        for (const auto& elt : cities_stats) {
            LOG4CPLUS_INFO(
                log, "FindAdminWithCities: " << elt.second << " cities responses with " << elt.first << " admins.");
        }
        for (const auto& admin : added_admins) {
            LOG4CPLUS_INFO(log, "FindAdminWithCities: "
                                    << "We have added the following admin: " << admin.second->label
                                    << " insee: " << admin.second->insee << " uri: " << admin.second->uri);
        }
    }

    void init() {
        for (auto* admin : georef.admins) {
            if (!admin->insee.empty()) {
                insee_admins_map[admin->insee] = admin;
            }
        }
    }

    template <class Row>
    georef::Admin* make_admin(const Row& r) {
        auto admin = new georef::Admin();
        admin->comment = "from cities";
        r["uri"] >> admin->uri;
        r["name"] >> admin->name;
        r["insee"] >> admin->insee;
        r["level"] >> admin->level;
        double lon, lat;
        r["lon"] >> lon;
        r["lat"] >> lat;
        admin->coord = navitia::type::GeographicalCoord(lon, lat);
        admin->idx = georef.admins.size();
        admin->from_original_dataset = false;
        std::string post_codes = r["post_code"].c_str();
        boost::split(admin->postal_codes, post_codes, boost::is_any_of("-"));
        std::string boundary = r["boundary"].c_str();
        boost::geometry::read_wkt(boundary, admin->boundary);

        // Add admins to added list
        added_admins[admin->uri] = admin;
        // Add admins to georef's admins list
        georef.admins.push_back(admin);

        return admin;
    }

    result_type get_admins_from_cities(const navitia::type::GeographicalCoord& c, georef::AdminRtree& admin_tree) {
        /*
            We only fetch admins containings the coordinate in their boundary shape
            because a left join between above result and another query using boundary shape
            is very very slow.

            +--------------------+
            |A                   |
            |                    |
            |                    |
            |                  +-|-------------+
            |                  |X|             |
            +------------------|+       B      |
                               |               |
                               |               |
                               +---------------+

            For instance, when looking at 'X', we want:
                - admins {A, B} to be returned and will be added in the cache

        */
        const auto sql_req = boost::format(R"sql(
            SELECT
                DISTINCT uri,
                name,
                coalesce(insee, '') as insee,
                level,
                coalesce(post_code, '') as post_code,
                ST_X(coord::geometry) as lon,
                ST_Y(coord::geometry) as lat,
                ST_ASTEXT(boundary) as boundary
            FROM
                administrative_regions
                    WHERE ST_DWithin(
                    ST_GeographyFromText('POINT(%.16f %.16f)'),
                    boundary, 0.001
                    ))sql") % c.lon()
                             % c.lat();
        pqxx::result db_result = work->exec(sql_req.str());

        auto not_in_insee_admins_map = [&](const pqxx_row& row) {
            return insee_admins_map.count(row["insee"].c_str()) == 0;
        };
        auto not_already_added = [&](const pqxx_row& row) { return added_admins.count(row["uri"].c_str()) == 0; };
        auto make_admin_from_row = [&](const pqxx_row& row) { return make_admin(row); };

        std::vector<georef::Admin*> new_admins;
        // clang-format off
        auto filtered_res = db_result | boost::adaptors::filtered(not_in_insee_admins_map)
                                      | boost::adaptors::filtered(not_already_added);
        //clang-format on
        boost::range::transform(filtered_res, std::back_inserter(new_admins), make_admin_from_row);

        auto add_admin_to_cache = [&](georef::Admin* admin) {
            if (admin->boundary.empty()) {
                return;
            }
            const auto box = boost::geometry::return_envelope<georef::Box>(admin->boundary);
            const double min[2] = {box.min_corner().lon(), box.min_corner().lat()};
            const double max[2] = {box.max_corner().lon(), box.max_corner().lat()};
            admin_tree.Insert(min, max, admin);
        };

        // Add admins to RTree cache
        boost::range::for_each(new_admins, add_admin_to_cache);

        return georef.find_admins(c, admin_tree);
    }

    result_type operator()(const navitia::type::GeographicalCoord& c, georef::AdminRtree& admin_tree) {
        auto log = log4cplus::Logger::getInstance("ed2nav::FindAdminWithCities");

        if (nb_call == 0) {
            init();
        }
        ++nb_call;

        if (!c.is_initialized()) {
            ++nb_uninitialized;
            return {};
        }

        const auto& georef_res = georef.find_admins(c, admin_tree);

        if (!georef_res.empty()) {
            ++nb_georef;
            return georef_res;
        }

        TimerGuard tg([&](const StopWatch& stopwatch) {
            LOG4CPLUS_TRACE(log, "Find admin in cities db | " << stopwatch.elapsed() << " us");
        });
        auto res = get_admins_from_cities(c, admin_tree);
        ++cities_stats[res.size()];
        return res;
    }
};

static bool rename_file(const std::string& source_name, const std::string& dest_name) {
    auto logger = log4cplus::Logger::getInstance("ed2nav::rename_file");
    LOG4CPLUS_INFO(logger, "Trying to rename " << source_name << " to " << dest_name);
    if (boost::filesystem::exists(source_name)) {
        if (rename(source_name.c_str(), dest_name.c_str()) != 0) {
            LOG4CPLUS_ERROR(logger, "Unable to rename data file: " << source_name << std::strerror(errno));
            return false;
        }
    }
    LOG4CPLUS_INFO(logger, "Renaming file success");
    return true;
}

static bool remove_file(const std::string& filename) {
    if(!boost::filesystem::exists(filename))
        return true;

    if(!boost::filesystem::remove(filename)) {
        auto logger = log4cplus::Logger::getInstance("ed2nav::rename_file");
        LOG4CPLUS_ERROR(logger, "Unable to remove file " << filename);
        return false;
    }

    return true;
}
template <class T>
bool write_data_to_file(const std::string& output_filename, const T& data) {
    std::string temp_output_filename = output_filename + ".temp";
    std::string backup_output_filename = output_filename + ".bak";
    if (!try_save_file(temp_output_filename, data)) {
        return false;
    }
    if (!rename_file(output_filename, backup_output_filename)) {
        return false;
    }
    if (!rename_file(temp_output_filename, output_filename)) {
        return false;
    }

    return remove_file(backup_output_filename);
}

int ed2nav(int argc, const char* argv[]) {
    std::string output, connection_string, region_name, cities_connection_string;
    double min_non_connected_graph_ratio;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
        ("help,h", "Show this message")
        ("version,v", "Show version")
        ("config-file", po::value<std::string>(), "Path to config file")
        ("output,o", po::value<std::string>(&output)->default_value("data.nav.lz4"),
            "Output file")
        ("name,n", po::value<std::string>(&region_name)->default_value("default"),
            "Name of the region you are extracting")
        ("min_non_connected_ratio,m",
         po::value<double>(&min_non_connected_graph_ratio)->default_value(0.01),
         "min ratio for the size of non connected graph")
        ("full_street_network_geometries", "If true export street network geometries allowing kraken to return accurate"
         "geojson for street network sections. Also improve projections accuracy. "
         "WARNING : memory intensive. The lz4 can more than double in size and kraken will consume significantly more memory.")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
         "database connection parameters: host=localhost user=navitia dbname=navitia password=navitia")
        ("cities-connection-string", po::value<std::string>(&cities_connection_string)->default_value(""),
         "cities database connection parameters: host=localhost user=navitia dbname=cities password=navitia")
        ("local_syslog", "activate log redirection within local syslog")
        ("log_comment", po::value<std::string>(), "optional field to add extra information like coverage name");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    bool export_georef_edges_geometries(vm.count("full_street_network_geometries"));

    if (vm.count("version")) {
        std::cout << argv[0] << " " << navitia::config::project_version << " " << navitia::config::navitia_build_type
                  << std::endl;
        return 0;
    }

    // Construct logger and signal handling
    std::string log_comment;
    if (vm.count("log_comment")) {
        log_comment = vm["log_comment"].as<std::string>();
    }
    navitia::init_app("ed2nav", "DEBUG", vm.count("local_syslog"), log_comment);
    auto logger = log4cplus::Logger::getInstance("log");

    if (vm.count("config-file")) {
        std::ifstream stream;
        stream.open(vm["config-file"].as<std::string>());
        if (!stream.is_open()) {
            throw navitia::exception("Unable to load config file");
        }
        po::store(po::parse_config_file(stream, desc), vm);
    }

    if (vm.count("help") || !vm.count("connection-string")) {
        std::cout << "Extracts data from a database to a file readable by kraken" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    po::notify(vm);

    pt::ptime start, now;
    int read, save;

    navitia::type::Data data;

    // on init now pour le moment à now, à rendre paramétrable pour le debug
    now = start = pt::microsec_clock::local_time();

    ed::EdReader reader(connection_string);

    if (!cities_connection_string.empty()) {
        data.find_admins = FindAdminWithCities(cities_connection_string, *data.geo_ref);
    }

    try {
        reader.fill(data, min_non_connected_graph_ratio, export_georef_edges_geometries);
    } catch (const navitia::exception& e) {
        LOG4CPLUS_ERROR(logger, "error while reading the database " << e.what());
        LOG4CPLUS_ERROR(logger, "stack: " << e.backtrace());
        throw;
    }

    read = (pt::microsec_clock::local_time() - start).total_milliseconds();
    data.complete();

    // fill addresses for stop points
    start = pt::microsec_clock::local_time();
    data.fill_stop_point_address(reader.address_by_address_id);
    int address = (pt::microsec_clock::local_time() - start).total_milliseconds();
    LOG4CPLUS_INFO(logger, "\t Building address " << address << "ms");

    data.meta->publication_date = pt::microsec_clock::universal_time();

    LOG4CPLUS_INFO(logger, "line: " << data.pt_data->lines.size());
    LOG4CPLUS_INFO(logger, "line_groups: " << data.pt_data->line_groups.size());
    LOG4CPLUS_INFO(logger, "route: " << data.pt_data->routes.size());
    LOG4CPLUS_INFO(logger, "stoparea: " << data.pt_data->stop_areas.size());
    LOG4CPLUS_INFO(logger, "stoppoint: " << data.pt_data->stop_points.size());
    LOG4CPLUS_INFO(logger, "vehiclejourney: " << data.pt_data->vehicle_journeys.size());
    LOG4CPLUS_INFO(logger, "stop: " << data.pt_data->nb_stop_times());
    LOG4CPLUS_INFO(logger, "connection: " << data.pt_data->stop_point_connections.size());
    LOG4CPLUS_INFO(logger, "modes: " << data.pt_data->physical_modes.size());
    LOG4CPLUS_INFO(logger, "validity pattern : " << data.pt_data->validity_patterns.size());
    LOG4CPLUS_INFO(logger, "calendars: " << data.pt_data->calendars.size());
    LOG4CPLUS_INFO(logger, "synonyms : " << data.geo_ref->synonyms.size());
    LOG4CPLUS_INFO(logger, "fare tickets: " << data.fare->fare_map.size());
    LOG4CPLUS_INFO(logger, "fare transitions: " << data.fare->nb_transitions());
    LOG4CPLUS_INFO(logger, "fare od: " << data.fare->od_tickets.size());
    LOG4CPLUS_INFO(logger, "Begin to save ...");

    start = pt::microsec_clock::local_time();

    if (!write_data_to_file(output, data)) {
        LOG4CPLUS_ERROR(logger, "Exiting ed2nav with errors");
        return 1;
    }
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "Computing times");
    LOG4CPLUS_INFO(logger, "\t File reading: " << read << "ms");
    LOG4CPLUS_INFO(logger, "\t Data writing: " << save << "ms");

    return 0;
}

}  // namespace ed
