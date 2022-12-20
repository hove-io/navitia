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
#include "ed/connectors/gtfs_parser.h"
#include "ed_persistor.h"
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
    std::string input, date, connection_string;
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
        ("config-file", po::value<std::string>(), "Path to a config file")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
            "Database connection parameters: host=localhost user=navitia"
            " dbname=navitia password=navitia")
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
    navitia::init_app("gtfs2ed", "DEBUG", vm.count("local_syslog"), log_comment);
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
        std::cout << "Reads and inserts into a ed database gtfs files" << std::endl;
        std::cout << desc << "\n";
        return 1;
    }
    po::notify(vm);

    pt::ptime start;
    int read, complete, clean, sort, save, main_destination(0);

    ed::Data data;
    data.simplify_tolerance = simplify_tolerance;

    start = pt::microsec_clock::local_time();

    ed::connectors::GtfsParser gtfs_parser(input);
    gtfs_parser.fill(data, date);
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

    LOG4CPLUS_INFO(logger, "line: " << data.lines.size());
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
    LOG4CPLUS_INFO(logger, "\t route destination " << main_destination << "ms");
    LOG4CPLUS_INFO(logger, "\t data saving " << save << "ms");

    return 0;
}
