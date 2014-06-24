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

#include "config.h"
#include <iostream>

#include "utils/timer.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "utils/exception.h"
#include "ed_reader.h"
#include "type/data.h"
#include "utils/init.h"


namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");
    std::string output, connection_string, region_name;
    double min_non_connected_graph_ratio;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "Path to config file")
        ("output,o", po::value<std::string>(&output)->default_value("data.nav.lz4"),
            "Output file")
        ("name,n", po::value<std::string>(&region_name)->default_value("default"),
            "Name of the region you are extracting")
        ("min_non_connected_ratio,m",
         po::value<double>(&min_non_connected_graph_ratio)->default_value(0.1),
         "min ratio for the size of non connected graph")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
         "database connection parameters: host=localhost user=navitia dbname=navitia password=navitia");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        std::cout << argv[0] << " V" << KRAKEN_VERSION << " " << NAVITIA_BUILD_TYPE << std::endl;
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
        std::cout << desc <<  "\n";
        return 1;
    }

    po::notify(vm);

    pt::ptime start, now;
    int read, save;

    navitia::type::Data data;

    //on init now pour le moment à now, à rendre paramétrable pour le debug
    now = start = pt::microsec_clock::local_time();

    ed::EdReader reader(connection_string);
    reader.fill(data, min_non_connected_graph_ratio);
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();
    data.complete();

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
    }
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();
    LOG4CPLUS_INFO(logger, "Data saved");

    LOG4CPLUS_INFO(logger, "Computing times");
    LOG4CPLUS_INFO(logger, "\t File reading: " << read << "ms");
    LOG4CPLUS_INFO(logger, "\t Data writing: " << save << "ms");

    return 0;
}
