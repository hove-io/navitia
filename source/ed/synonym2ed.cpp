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
#include "ed/connectors/synonym_parser.h"
#include "ed_persistor.h"
#include "utils/exception.h"
#include "utils/init.h"
#include "utils/timer.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char* argv[]) {
    std::string input, connection_string;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
        ("help,h", "Show this message")
        ("input,i", po::value<std::string>(&input), "Synonyms file name")
        ("version,v", "Show version")
        ("config-file", po::value<std::string>(), "Path to config file")
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
    navitia::init_app("synonym2ed", "DEBUG", vm.count("local_syslog"), log_comment);
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
        std::cout << "Reads and inserts a synonym file into an ed database" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }
    po::notify(vm);

    pt::ptime start;
    start = pt::microsec_clock::local_time();
    ed::connectors::SynonymParser synonym_parser(input);

    try {
        synonym_parser.fill();
    } catch (const ed::connectors::SynonymParserException& e) {
        LOG4CPLUS_FATAL(logger, "Erreur :" + std::string(e.what()) + "  backtrace :" + e.backtrace());
        return -1;
    }

    ed::EdPersistor p(connection_string);
    p.persist_synonym(synonym_parser.synonym_map);
    LOG4CPLUS_FATAL(logger, "temps :" << to_simple_string(pt::microsec_clock::local_time() - start));

    return 0;
}
