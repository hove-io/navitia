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

#include "conf.h"

#include "utils/timer.h"
#include "utils/exception.h"
#include "ed_reader.h"
#include "type/data.h"
#include "utils/init.h"
#include "utils/functions.h"
#include "type/meta_data.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <pqxx/pqxx>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;
namespace pt = boost::posix_time;
namespace georef = navitia::georef;

// A functor that first asks to GeoRef the admins of coord, and, if
// GeoRef found nothing, asks to the cities database.
struct FindAdminWithCities {
    typedef std::unordered_map<std::string, navitia::georef::Admin*> AdminMap;
    typedef std::vector<georef::Admin*> result_type;

    boost::shared_ptr<pqxx::connection> conn;
    georef::GeoRef& georef;
    AdminMap& admin_by_insee_code;
    size_t nb_call = 0;
    size_t nb_uninitialized = 0;
    size_t nb_georef = 0;
    size_t nb_admin_added = 0;
    std::map<size_t, size_t> cities_stats;// number of response for size of the result

    FindAdminWithCities(const std::string& connection_string, georef::GeoRef& gr, AdminMap& m):
        conn(boost::make_shared<pqxx::connection>(connection_string)),
        georef(gr),
        admin_by_insee_code(m)
        {}

    FindAdminWithCities(const FindAdminWithCities&) = default;
    FindAdminWithCities& operator=(const FindAdminWithCities&) = default;
    ~FindAdminWithCities() {
        if (nb_call == 0) return;

        auto log = log4cplus::Logger::getInstance("ed2nav::FindAdminWithCities");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_call << " calls");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_uninitialized
                       << " calls with uninitialized or zeroed coord");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_georef << " GeoRef responses");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_admin_added << " admins added using cities");
        for (const auto& elt: cities_stats) {
            LOG4CPLUS_INFO(log, "FindAdminWithCities: "
                           << elt.second << " cities responses with "
                           << elt.first << " admins.");
        }
    }

    result_type operator()(const navitia::type::GeographicalCoord& c) {
        ++nb_call;

        if (!c.is_initialized()) {++nb_uninitialized; return {};}

        const auto &georef_res = georef.find_admins(c);
        if (!georef_res.empty()) {++nb_georef; return georef_res;}

        std::stringstream request;
        request << "SELECT uri, name, insee, level, post_code, "
                << "ST_X(coord::geometry) as lon, ST_Y(coord::geometry) as lat "
                << "FROM administrative_regions "
                << "WHERE ST_DWithin(ST_GeographyFromText('POINT("
                << std::setprecision(16) << c.lon() << " " << c.lat() << ")'), boundary, 0.001)";
        pqxx::work work(*conn);
        pqxx::result result = work.exec(request);
        result_type res;
        for (auto it = result.begin(); it != result.end(); ++it) {
            navitia::georef::Admin*& admin =
                admin_by_insee_code[it["insee"].as<std::string>()];
            if (!admin) {
                georef.admins.push_back(new navitia::georef::Admin());
                admin = georef.admins.back();
                admin->comment = "from cities";
                it["uri"].to(admin->uri);
                it["name"].to(admin->name);
                it["insee"].to(admin->insee);
                it["level"].to(admin->level);
                it["post_code"].to(admin->post_code);
                admin->coord.set_lon(it["lon"].as<double>());
                admin->coord.set_lat(it["lat"].as<double>());
                admin->idx = georef.admins.size() - 1;
                admin->from_original_dataset = false;
                ++nb_admin_added;
            }
            res.push_back(admin);
        }
        ++cities_stats[res.size()];
        return res;
    }
};

int main(int argc, char * argv[])
{
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");
    std::string output, connection_string, region_name, cities_connection_string;
    double min_non_connected_graph_ratio;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show this message")
        ("version,v", "Show version")
        ("config-file", po::value<std::string>(), "Path to config file")
        ("output,o", po::value<std::string>(&output)->default_value("data.nav.lz4"),
            "Output file")
        ("name,n", po::value<std::string>(&region_name)->default_value("default"),
            "Name of the region you are extracting")
        ("min_non_connected_ratio,m",
         po::value<double>(&min_non_connected_graph_ratio)->default_value(0.1),
         "min ratio for the size of non connected graph")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
         "database connection parameters: host=localhost user=navitia dbname=navitia password=navitia")
        ("cities-connection-string", po::value<std::string>(&cities_connection_string)->default_value(""),
         "cities database connection parameters: host=localhost user=navitia dbname=cities password=navitia");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        std::cout << argv[0] << " V" << navitia::config::kraken_version << " "
                  << navitia::config::navitia_build_type << std::endl;
        return 0;
    }


    if(vm.count("config-file")){
        std::ifstream stream;
        stream.open(vm["config-file"].as<std::string>());
        if(!stream.is_open()){
            throw navitia::exception("Unable to load config file");
        }else{
            po::store(po::parse_config_file(stream, desc), vm);
        }
    }

    if(vm.count("help")) {
        std::cout << "Extracts data from a database to a file readable by kraken" << std::endl;
        std::cout << desc <<  std::endl;
        return 1;
    }

    po::notify(vm);

    pt::ptime start, now;
    int read, save;

    navitia::type::Data data;

    //on init now pour le moment à now, à rendre paramétrable pour le debug
    now = start = pt::microsec_clock::local_time();

    ed::EdReader reader(connection_string);

    if (!cities_connection_string.empty()) {
        data.find_admins = FindAdminWithCities(
            cities_connection_string, *data.geo_ref, reader.admin_by_insee_code);
    }

    try {
        reader.fill(data, min_non_connected_graph_ratio);
    }
    catch (const navitia::exception& e) {
        LOG4CPLUS_ERROR(logger, "error while reading the database "  << e.what());
        LOG4CPLUS_ERROR(logger, "stack: "  << e.backtrace());
        throw;
    }

    read = (pt::microsec_clock::local_time() - start).total_milliseconds();
    data.complete();
    data.meta->publication_date = pt::microsec_clock::local_time();

    LOG4CPLUS_INFO(logger, "line: " << data.pt_data->lines.size());
    LOG4CPLUS_INFO(logger, "route: " << data.pt_data->routes.size());
    LOG4CPLUS_INFO(logger, "journey_pattern: " << data.pt_data->journey_patterns.size());
    LOG4CPLUS_INFO(logger, "stoparea: " << data.pt_data->stop_areas.size());
    LOG4CPLUS_INFO(logger, "stoppoint: " << data.pt_data->stop_points.size());
    LOG4CPLUS_INFO(logger, "vehiclejourney: " << data.pt_data->vehicle_journeys.size());
    LOG4CPLUS_INFO(logger, "stop: " << data.pt_data->stop_times.size());
    LOG4CPLUS_INFO(logger, "connection: " << data.pt_data->stop_point_connections.size());
    LOG4CPLUS_INFO(logger, "journey_pattern points: " << data.pt_data->journey_pattern_points.size());
    LOG4CPLUS_INFO(logger, "modes: " << data.pt_data->physical_modes.size());
    LOG4CPLUS_INFO(logger, "validity pattern : " << data.pt_data->validity_patterns.size());
    LOG4CPLUS_INFO(logger, "calendars: " << data.pt_data->calendars.size());
    LOG4CPLUS_INFO(logger, "synonyms : " << data.geo_ref->synonyms.size());
    LOG4CPLUS_INFO(logger, "fare tickets: " << data.fare->fare_map.size());
    LOG4CPLUS_INFO(logger, "fare transitions: " << data.fare->nb_transitions());
    LOG4CPLUS_INFO(logger, "fare od: " << data.fare->od_tickets.size());

    LOG4CPLUS_INFO(logger, "Begin to save ...");

    start = pt::microsec_clock::local_time();
    try {
        data.save(output);
    } catch(const navitia::exception &e) {
        LOG4CPLUS_ERROR(logger, "Unable to save");
        LOG4CPLUS_ERROR(logger, e.what());
        return 1;
    }
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();
    LOG4CPLUS_INFO(logger, "Data saved");

    LOG4CPLUS_INFO(logger, "Computing times");
    LOG4CPLUS_INFO(logger, "\t File reading: " << read << "ms");
    LOG4CPLUS_INFO(logger, "\t Data writing: " << save << "ms");

    return 0;
}
