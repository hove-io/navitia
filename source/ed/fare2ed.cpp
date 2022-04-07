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

#include "fare2ed.h"

#include "conf.h"
#include "ed/connectors/fare_parser.h"
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
namespace ed {

int fare2ed(int argc, const char* argv[]) {
    std::string connection_string, fare_dir;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
        ("help,h", "Show this message")
        ("version,v", "Show version")
        ("config-file", po::value<std::string>(), "Path to configuration file")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
         "Connection parameters to database: host=localhost user=navitia dbname=navitia password=navitia")
        ("fare,f", po::value<std::string>(&fare_dir)->required(), "Directory of fare files")
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
    navitia::init_app("fare2ed", "DEBUG", vm.count("local_syslog"), log_comment);
    auto logger = log4cplus::Logger::getInstance("log");

    if (vm.count("config-file")) {
        std::ifstream stream;
        stream.open(vm["config-file"].as<std::string>());
        if (!stream.is_open()) {
            throw navitia::exception("loading config file failed");
        }
        po::store(po::parse_config_file(stream, desc), vm);
    }

    if (vm.count("help") || !vm.count("connection-string")) {
        std::cout << "Reads and inserts in ed database fare files" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }
    po::notify(vm);

    pt::ptime start = pt::microsec_clock::local_time();

    ed::Data data;

    LOG4CPLUS_INFO(logger, "Fare parsers");
    ed::connectors::fare_parser fareParser(data, fare_dir + "/fares.csv", fare_dir + "/prices.csv",
                                           fare_dir + "/od_fares.csv");
    fareParser.load();

    auto end_load = pt::microsec_clock::local_time();
    LOG4CPLUS_INFO(logger, "tickets: " << data.fare_map.size());
    LOG4CPLUS_INFO(logger, "transitions: " << data.transitions.size());
    LOG4CPLUS_INFO(logger, "od_tickets: " << data.od_tickets.size());

    start = pt::microsec_clock::local_time();
    ed::EdPersistor p(connection_string);
    p.persist_fare(data);

    LOG4CPLUS_INFO(logger, "running time: ");
    LOG4CPLUS_INFO(logger, "fares loaded in : " << (end_load - start).total_milliseconds() << "ms");
    LOG4CPLUS_INFO(logger,
                   "\t data writen in " << (pt::microsec_clock::local_time() - start).total_milliseconds() << "ms");

    return 0;
}

}  // namespace ed
