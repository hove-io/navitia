#include "config.h"
#include <iostream>
#include "ed/connectors/fusio_parser.h"
#include "ed/connectors/fare_parser.h"
#include "external_parser.h"

#include "utils/timer.h"
#include "utils/init.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "utils/exception.h"
#include "ed_persistor.h"
#include "connectors/extcode2uri.h"
#include "fare/fare.h"



namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");

    std::string connection_string, fare_dir;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("connection-string", po::value<std::string>(&connection_string)->required(), "parametres de connexion à la base de données: host=localhost user=navitia dbname=navitia password=navitia")
        ("fare,f", po::value<std::string>(&fare_dir)->required(), "Repertoire des fichiers fare");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        LOG4CPLUS_INFO(logger, argv[0] << " V" << NAVITIA_VERSION << " " << NAVITIA_BUILD_TYPE);
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

    if(vm.count("help")) {
        std::cout << desc <<  "\n";
        return 1;
    }
    po::notify(vm);

    pt::ptime start = pt::microsec_clock::local_time();

    ed::Data data;

    LOG4CPLUS_INFO(logger, "Fare parsers");
    ed::connectors::fare_parser fareParser(data, fare_dir + "/fares.csv",
                                       fare_dir + "/prices.csv",
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
    LOG4CPLUS_INFO(logger, "fares loaded in : " << (end_load- start).total_milliseconds() << "ms");
    LOG4CPLUS_INFO(logger, "\t data writen in " << (pt::microsec_clock::local_time() - start).total_milliseconds() << "ms");

    return 0;
}
