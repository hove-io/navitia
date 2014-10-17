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
#include <iostream>

#include "utils/timer.h"
#include "utils/logger.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "utils/exception.h"
#include "type/data.h"
#include "ed/connectors/messages_connector.h"
#include "ed/adapted.h"
#include "utils/init.h"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    navitia::init_app();
    std::string output, input, connection_string;
    uint32_t shift_days;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show this message")
        ("version,v", "Show version")
        ("config-file", po::value<std::string>(), "Path to config file")
        ("output,o", po::value<std::string>(&output)->default_value("rtdata.nav.lz4"), "Output file")
        ("input,i", po::value<std::string>(&input)->default_value("data.nav.lz4"), "Input file")
        ("shift-days,d", po::value<uint32_t>(&shift_days)->default_value(2),
            "Enlarge publication period by a number of days")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
             "Database connection parameters: host=localhost user=navitia"
             " dbname=navitia password=navitia");


    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        std::cout << argv[0] << " V" << navitia::config::kraken_version << " "
                  << navitia::config::navitia_build_type << std::endl;
        return 0;
    }

    init_logger();
    auto logger = log4cplus::Logger::getInstance("nav2rt");
    if(vm.count("config-file")){
        std::ifstream stream;
        stream.open(vm["config-file"].as<std::string>());
        if(!stream.is_open()){
            throw navitia::exception("loading config file failed");
        }else{
            po::store(po::parse_config_file(stream, desc), vm);
        }
    }

    if(vm.count("help")) {
        std::cout << "Read a navitia binary file and a realtime database and write "
                     "a new file with realtime informations" << std::endl;
        std::cout << desc <<  "\n";
        return 1;
    }

    po::notify(vm);

    pt::ptime start, end, now;
    int read, load, load_pert, apply_adapted, save;

    LOG4CPLUS_INFO(logger, "loading of data.nav.lz4");

    now = start = pt::microsec_clock::local_time();
    navitia::type::Data data;
    if(! data.load(input)){
        LOG4CPLUS_ERROR(logger, "error while reading input file!");
        return 1;
    }
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "loading messages");

    ed::connectors::RealtimeLoaderConfig config(connection_string, shift_days);
    try{
        start = pt::microsec_clock::local_time();
        ed::connectors::load_disruptions(*data.pt_data, config, now);
        load = (pt::microsec_clock::local_time() - start).total_milliseconds();
    }catch(const navitia::exception& ex){
        std::cout << ex.what() << std::endl;
        return 1;
    }
    LOG4CPLUS_INFO(logger, data.pt_data->disruption_holder.disruptions.size() << " loaded messages");

    LOG4CPLUS_INFO(logger, "loading of disruptions");
    std::vector<ed::AtPerturbation> perturbations;
    try{
        start = pt::microsec_clock::local_time();
        perturbations = ed::connectors::load_at_perturbations(config, now);
        load_pert = (pt::microsec_clock::local_time() - start).total_milliseconds();
    }catch(const navitia::exception& ex){
        std::cout << ex.what() << std::endl;
        return 1;
    }
    LOG4CPLUS_INFO(logger, perturbations.size() << " disruptions loaded");

    start = pt::microsec_clock::local_time();
    ed::AtAdaptedLoader adapter;
    adapter.apply(perturbations, *data.pt_data);
    //aprés avoir modifié les graphs; on retrie
    data.pt_data->sort();
    apply_adapted = (pt::microsec_clock::local_time() - start).total_milliseconds();
    data.build_proximity_list();

    LOG4CPLUS_INFO(logger, "building of autocomplete");
    data.build_autocomplete();

    LOG4CPLUS_INFO(logger, "saving...");

    start = pt::microsec_clock::local_time();
    try {
        data.save(output);
    } catch(const navitia::exception &e) {
        LOG4CPLUS_ERROR(logger, "Impossible to save: " << e.what());
        return 1;
    }
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "timing: ");
    LOG4CPLUS_INFO(logger, "\t loading of data.nav.lz4: " << read << "ms");
    LOG4CPLUS_INFO(logger, "\t loading of messages: " << load << "ms");
    LOG4CPLUS_INFO(logger, "\t loading of disruptions: " << load_pert << "ms");
    LOG4CPLUS_INFO(logger, "\t application of disruptions: " << apply_adapted << "ms");
    LOG4CPLUS_INFO(logger, "\t saving: " << save << "ms");

    return 0;
}
