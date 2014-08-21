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
#include "ed/connectors/fusio_parser.h"
#include "ed/connectors/fare_parser.h"

#include "utils/timer.h"
#include "utils/init.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "utils/exception.h"
#include "ed_persistor.h"
#include "fare/fare.h"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");

    std::string input, date, connection_string,
                fare_dir;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("date,d", po::value<std::string>(&date), "Date de début")
        ("input,i", po::value<std::string>(&input), "Repertoire d'entrée")
        ("version,v", "Affiche la version")
        ("fare,f", po::value<std::string>(&fare_dir), "Repertoire des fichiers fare")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
             "parametres de connexion à la base de données: host=localhost "
             "user=navitia dbname=navitia password=navitia");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        LOG4CPLUS_INFO(logger, argv[0] << " V" << KRAKEN_VERSION << " " << NAVITIA_BUILD_TYPE);
        return 0;
    }

    if(vm.count("config-file")){
        std::ifstream stream;
        stream.open(vm["config-file"].as<std::string>());
        if(!stream.is_open()){
            throw navitia::exception("loading config file failed");
        }else{
            po::store(po::parse_config_file(stream, desc), vm);
        }
    }

    if(vm.count("help") || !vm.count("input")) {
        std::cout << desc <<  "\n";
        return 1;
    }
    po::notify(vm);

    pt::ptime start;
    int read, complete, clean, sort, save, fare(0);

    ed::Data data;

    start = pt::microsec_clock::local_time();

    ed::connectors::FusioParser fusio_parser(input);
    fusio_parser.fill(data, date);
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();

    navitia::type::MetaData meta;
    meta.production_date = fusio_parser.gtfs_data.production_date;
    meta.timezone = fusio_parser.gtfs_data.tz.default_timezone.first;

    start = pt::microsec_clock::local_time();
    data.complete();
    complete = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.clean();
    clean = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.sort();
    sort = (pt::microsec_clock::local_time() - start).total_milliseconds();

    data.normalize_uri();

    if(vm.count("fare")){
        start = pt::microsec_clock::local_time();
        LOG4CPLUS_INFO(logger, "Alimentation de fare");
        ed::connectors::fare_parser fareParser(data, fare_dir + "/fares.csv",
                                           fare_dir + "/prices.csv",
                                           fare_dir + "/od_fares.csv");
        fareParser.load();
        fare = (pt::microsec_clock::local_time() - start).total_milliseconds();
    }


    LOG4CPLUS_INFO(logger, "line: " << data.lines.size());
    LOG4CPLUS_INFO(logger, "route: " << data.routes.size());
    LOG4CPLUS_INFO(logger, "journey_pattern: " << data.journey_patterns.size());
    LOG4CPLUS_INFO(logger, "stoparea: " << data.stop_areas.size());
    LOG4CPLUS_INFO(logger, "stoppoint: " << data.stop_points.size());
    LOG4CPLUS_INFO(logger, "vehiclejourney: " << data.vehicle_journeys.size());
    LOG4CPLUS_INFO(logger, "stop: " << data.stops.size());
    LOG4CPLUS_INFO(logger, "connection: " << data.stop_point_connections.size());
    LOG4CPLUS_INFO(logger, "journey_pattern points: " << data.journey_pattern_points.size());
    LOG4CPLUS_INFO(logger, "modes: " << data.physical_modes.size());
    LOG4CPLUS_INFO(logger, "validity pattern : " << data.validity_patterns.size());

    start = pt::microsec_clock::local_time();
    ed::EdPersistor p(connection_string);
    p.persist(data, meta);
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "temps de traitement");
    LOG4CPLUS_INFO(logger, "\t lecture des fichiers " << read << "ms");
    LOG4CPLUS_INFO(logger, "\t completion des données " << complete << "ms");
    LOG4CPLUS_INFO(logger, "\t netoyage des données " << clean << "ms");
    LOG4CPLUS_INFO(logger, "\t trie des données " << sort << "ms");
    if (vm.count("fare")) {
        LOG4CPLUS_INFO(logger, "\t fares loaded in : " << fare << "ms");
    }
    LOG4CPLUS_INFO(logger, "\t enregistrement des données " << save << "ms");

    return 0;
}
