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

#include "georef/georef.h"
#include "type/data.h"
#include "type/pb_converter.h"
#include "utils/init.h"  // init_app()

#include <boost/program_options.hpp>

#include <fstream>

using namespace navitia;

namespace po = boost::program_options;

int main(int argc, char** argv) {
    navitia::init_app();
    po::options_description desc("options of dump streetnetwork");
    std::string file, output;

    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    logger.setLogLevel(log4cplus::WARN_LOG_LEVEL);

    // clang-format off
    desc.add_options()
            ("help", "Show this message")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"),
                     "Path to data.nav.lz4")
            ("output,o", po::value<std::string>(&output)->default_value("graph.csv"),
                     "Output file");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "this dump the streetnetwork of a datanav as a csv to import it in qgis" << std::endl;
        std::cout << desc << std::endl;
        return 0;
    }

    LOG4CPLUS_INFO(logger, "loading data");
    type::Data data;
    data.load_nav(file);

    LOG4CPLUS_INFO(logger, "opening file");
    std::fstream out;
    auto& g = data.geo_ref->graph;
    out.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    out.open(output, std::ios::out | std::ios::trunc);

    LOG4CPLUS_INFO(logger, "writing csv");
    out << std::setprecision(16);
    for (const auto* way : data.geo_ref->ways) {
        if (way->geoms.empty()) {
            for (const auto& edge : way->edges) {
                out << "LINESTRING(" << g[edge.first].coord.lon() << " " << g[edge.first].coord.lat() << ", "
                    << g[edge.second].coord.lon() << " " << g[edge.second].coord.lat() << ")" << '\n';
            }
        } else {
            for (const auto& geom : way->geoms) {
                out << "LINESTRING(";
                auto first = true;
                for (const auto& coord : geom) {
                    if (not first) {
                        out << ", ";
                    } else {
                        first = false;
                    }
                    out << coord.lon() << " " << coord.lat();
                }
                out << ")" << '\n';
            }
        }
    }
    out.close();

    return 0;
}
