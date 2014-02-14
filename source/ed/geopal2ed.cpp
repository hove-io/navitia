#include "config.h"
#include <iostream>
#include "ed/connectors/geopal_parser.h"

#include "utils/timer.h"
#include "utils/init.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include "utils/exception.h"
#include "ed_persistor.h"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");

    std::string input, connection_string;
    uint32_t coord_system;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("input,i", po::value<std::string>(&input), "Repertoire d'entrée")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("coord-system,c", po::value<uint32_t>(&coord_system)->default_value(27572), "Système de coordonnée: 27572 pour lambert 2 Etendu")
        ("connection-string", po::value<std::string>(&connection_string)->required(), "parametres de connexion à la base de données: host=localhost user=navitia dbname=navitia password=navitia");

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

    if(vm.count("help") || !vm.count("input")) {
        std::cout << desc <<  "\n";
        return 1;
    }
    po::notify(vm);

    pt::ptime start;

    start = pt::microsec_clock::local_time();
    ed::Georef data;
    ed::connectors::Projection origin("Lambert 2 étendu", std::to_string(coord_system), false);
    ed::connectors::GeopalParser geopal_parser(input, ed::connectors::ConvCoord(origin));

    try{
            geopal_parser.fill(data);
        }catch(const ed::connectors::GeopalParserException& e){
            LOG4CPLUS_FATAL(logger, "Erreur :"+ std::string(e.what()) + "  backtrace :" + e.backtrace());
            return -1;
        }

    ed::EdPersistor p(connection_string);
    p.persist(data);
    std::cout<<std::endl<<"temps :"<<to_simple_string(pt::microsec_clock::local_time() - start)<<std::endl;

    return 0;
}
