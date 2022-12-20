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

#include "conf.h"
#include "ed/connectors/fare_parser.h"
#include "ed/connectors/fusio_parser.h"
#include "ed_persistor.h"
#include "fare/fare.h"
#include "utils/exception.h"
#include "utils/init.h"
#include "utils/timer.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char* argv[]) {
    std::string input, date, connection_string, fare_dir;
    double simplify_tolerance;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
        ("help,h", "Show this message")
        ("date,d", po::value<std::string>(&date), "Beginning date")
        ("input,i", po::value<std::string>(&input), "Input directory")
        ("simplify_tolerance,s", po::value<double>(&simplify_tolerance)->default_value(0.00003),
         "Distance in unit of coordinate used to simplify geometries and reduce memory usage. Default is "
         "0.00003 (~ 3m). Pass 0 to disable any simplification.")
        ("version,v", "Show version")
        ("fare,f", po::value<std::string>(&fare_dir), "Directory of fare files")
        ("config-file", po::value<std::string>(), "Path to configuration file")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
             "Database connection parameters: host=localhost "
             "user=navitia dbname=navitia password=navitia")
        ("local_syslog", "activate log redirection within local syslog")
        ("log_comment", po::value<std::string>(), "optional field to add extra information like coverage name");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

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
    navitia::init_app("fusio2ed", "DEBUG", vm.count("local_syslog"), log_comment);
    auto logger = log4cplus::Logger::getInstance("log");

    if (vm.count("config-file")) {
        std::ifstream stream;
        stream.open(vm["config-file"].as<std::string>());
        if (!stream.is_open()) {
            throw navitia::exception("loading config file failed");
        }
        po::store(po::parse_config_file(stream, desc), vm);
    }

    if (vm.count("help") || !vm.count("input")) {
        std::cout << "Reads and inserts in database fusio files" << std::endl;
        std::cout << desc << "\n";
        return 1;
    }
    po::notify(vm);

    if (fare_dir.empty()) {
        fare_dir = input;
    }

    pt::ptime start;
    int read, complete, clean, sort, save, fare(0), main_destination(0);

    ed::Data data;
    data.simplify_tolerance = simplify_tolerance;

    start = pt::microsec_clock::local_time();

    ed::connectors::FusioParser fusio_parser(input);
    fusio_parser.fill(data, date);
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "We excluded " << data.count_too_long_connections
                                          << " connections "
                                             " because they were too long");
    LOG4CPLUS_INFO(logger, "We excluded " << data.count_empty_connections
                                          << " connections "
                                             " because they had no duration time");

    start = pt::microsec_clock::local_time();
    data.complete();
    complete = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "Starting da ugly ODT hack...");
    size_t nb_hacked = 0;
    for (auto* vj : data.vehicle_journeys) {
        if (vj->stop_time_list.size() != 2) {
            continue;
        }
        if (vj->stop_time_list[0]->stop_point != vj->stop_time_list[1]->stop_point) {
            continue;
        }
        if (vj->stop_time_list[0]->departure_time != vj->stop_time_list[1]->arrival_time) {
            continue;
        }

        // No, teleportation can't exist, even on a null distance!
        // You'll take 10 min, I said!
        ++nb_hacked;
        vj->stop_time_list[1]->arrival_time += 10 * 60;
        vj->stop_time_list[1]->departure_time += 10 * 60;
        vj->stop_time_list[1]->alighting_time += 10 * 60;
        vj->stop_time_list[1]->boarding_time += 10 * 60;
    }
    LOG4CPLUS_INFO(logger, "Da ugly ODT hack: " << nb_hacked << " patched");

    start = pt::microsec_clock::local_time();
    data.clean();
    clean = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.sort();
    sort = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.build_route_destination();
    main_destination = (pt::microsec_clock::local_time() - start).total_milliseconds();

    data.normalize_uri();

    if (vm.count("fare") || boost::filesystem::exists(fare_dir + "/fares.csv")) {
        start = pt::microsec_clock::local_time();
        LOG4CPLUS_INFO(logger, "loading fare");

        ed::connectors::fare_parser fareParser(data, fare_dir + "/fares.csv", fare_dir + "/prices.csv",
                                               fare_dir + "/od_fares.csv");
        fareParser.load();
        fare = (pt::microsec_clock::local_time() - start).total_milliseconds();
    }

    LOG4CPLUS_INFO(logger, "line: " << data.lines.size());
    LOG4CPLUS_INFO(logger, "line_group: " << data.line_groups.size());
    LOG4CPLUS_INFO(logger, "route: " << data.routes.size());
    LOG4CPLUS_INFO(logger, "stoparea: " << data.stop_areas.size());
    LOG4CPLUS_INFO(logger, "stoppoint: " << data.stop_points.size());
    LOG4CPLUS_INFO(logger, "vehiclejourney: " << data.vehicle_journeys.size());
    LOG4CPLUS_INFO(logger, "stop: " << data.stops.size());
    LOG4CPLUS_INFO(logger, "connection: " << data.stop_point_connections.size());
    LOG4CPLUS_INFO(logger, "modes: " << data.physical_modes.size());
    LOG4CPLUS_INFO(logger, "validity pattern : " << data.validity_patterns.size());

    start = pt::microsec_clock::local_time();
    ed::EdPersistor p(connection_string);
    p.persist(data);
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "processing times");
    LOG4CPLUS_INFO(logger, "\t reading files " << read << "ms");
    LOG4CPLUS_INFO(logger, "\t data completed " << complete << "ms");
    LOG4CPLUS_INFO(logger, "\t data cleanup " << clean << "ms");
    LOG4CPLUS_INFO(logger, "\t data ordering " << sort << "ms");
    if (vm.count("fare")) {
        LOG4CPLUS_INFO(logger, "\t fares loaded in : " << fare << "ms");
    }
    LOG4CPLUS_INFO(logger, "\t route destination " << main_destination << "ms");
    LOG4CPLUS_INFO(logger, "\t data saving " << save << "ms");

    return 0;
}
