#include "config.h"
#include <iostream>
#include "ed/connectors/fusio_parser.h"
#include "external_parser.h"

#include "utils/timer.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "utils/exception.h"
#include "ed_persistor.h"
#include "connectors/extcode2uri.h"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    init_logger();
    auto logger = log4cplus::Logger::getInstance("log");

    std::string input, date, connection_string, aliases_file,
                synonyms_file, redis_string;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("date,d", po::value<std::string>(&date), "Date de début")
        ("input,i", po::value<std::string>(&input), "Repertoire d'entrée")
        ("aliases,a", po::value<std::string>(&aliases_file), "Fichier aliases")
        ("synonyms,s", po::value<std::string>(&synonyms_file), "Fichier synonymes")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("connection-string", po::value<std::string>(&connection_string)->required(), "parametres de connexion à la base de données: host=localhost user=navitia dbname=navitia password=navitia")
        ("redis-string,r", po::value<std::string>(&redis_string), "parametres de connexion à redis: host=localhost db=0 password=navitia port=6379 timeout=2");

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
    int read, complete, clean, sort, save;

    ed::Data data;

    start = pt::microsec_clock::local_time();

    ed::connectors::FusioParser fusio_parser(input);
    fusio_parser.fill(data, date);
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();

    navitia::type::MetaData meta;
    meta.production_date = fusio_parser.gtfs_data.production_date;

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

    ed::connectors::ExternalParser extConnecteur;
    if(vm.count("synonyms")){
        extConnecteur.fill_synonyms(synonyms_file, data);
    }

    if(vm.count("aliases")){
        extConnecteur.fill_aliases(aliases_file, data);
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
    LOG4CPLUS_INFO(logger, "journey_pattern point connections : " << data.journey_pattern_point_connections.size());
    LOG4CPLUS_INFO(logger, "alias : " <<data.alias.size());
    LOG4CPLUS_INFO(logger, "synonyms : " <<data.synonymes.size());

    // Ajout dans la table des correspondances
    if (vm.count("redis-string")) {
        LOG4CPLUS_INFO(logger, "Alimentation de redis");
        ed::connectors::ExtCode2Uri ext_code_2_uri(redis_string);
        ext_code_2_uri.to_redis(data);
    }

    start = pt::microsec_clock::local_time();
    ed::EdPersistor p(connection_string);
    p.persist(data, meta);
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "temps de traitement");
    LOG4CPLUS_INFO(logger, "\t lecture des fichiers " << read << "ms");
    LOG4CPLUS_INFO(logger, "\t completion des données " << complete << "ms");
    LOG4CPLUS_INFO(logger, "\t netoyage des données " << clean << "ms");
    LOG4CPLUS_INFO(logger, "\t trie des données " << sort << "ms");
    LOG4CPLUS_INFO(logger, "\t enregistrement des données " << save << "ms");

    return 0;
}
