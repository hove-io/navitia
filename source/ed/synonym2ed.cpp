
#include "config.h"
#include <iostream>
#include "ed/connectors/synonym_parser.h"

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
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("input,i", po::value<std::string>(&input), "Repertoire d'entrée")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("connection-string", po::value<std::string>(&connection_string)->required(), "parametres de connexion à la base de données: host=localhost user=navitia dbname=navitia password=navitia");

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
    start = pt::microsec_clock::local_time();
    ed::connectors::SynonymParser synonym_parser(input);

    try{
        synonym_parser.fill();
    }catch(const ed::connectors::SynonymParserException& e){
        LOG4CPLUS_FATAL(logger, "Erreur :"+ std::string(e.what()) + "  backtrace :" + e.backtrace());
        return -1;
    }

    ed::EdPersistor p(connection_string);
    p.persist_synonym(synonym_parser.synonym_map);
    LOG4CPLUS_FATAL(logger, "temps :"<<to_simple_string(pt::microsec_clock::local_time() - start));

    return 0;
}
