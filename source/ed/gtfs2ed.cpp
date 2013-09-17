#include "config.h"
#include <iostream>

#include "gtfs_parser.h"
#include "external_parser.h"

#include "utils/timer.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "utils/exception.h"
#include "ed_persistor.h"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    std::string type, input, output, date, connection_string, aliases_file,
                synonyms_file;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("date,d", po::value<std::string>(&date), "Date de début")
        ("input,i", po::value<std::string>(&input), "Repertoire d'entrée")
        ("aliases,a", po::value<std::string>(&aliases_file), "Fichier aliases")
        ("synonyms,s", po::value<std::string>(&synonyms_file), "Fichier synonymes")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("connection-string", po::value<std::string>(&connection_string)->required(), "parametres de connexion à la base de données: host=localhost user=navitia dbname=navitia password=navitia");


    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        std::cout << argv[0] << " V" << NAVITIA_VERSION << " " << NAVITIA_BUILD_TYPE << std::endl;
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

    pt::ptime start, end;
    int read, complete, clean, sort, save;

    ed::Data data;


    start = pt::microsec_clock::local_time();


    ed::connectors::GtfsParser connector(input);
    connector.fill(data, date);
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();

    navitia::type::MetaData meta;
    meta.production_date = connector.production_date;

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

    std::cout << "line: " << data.lines.size() << std::endl;
    std::cout << "route: " << data.routes.size() << std::endl;
    std::cout << "journey_pattern: " << data.journey_patterns.size() << std::endl;
    std::cout << "stoparea: " << data.stop_areas.size() << std::endl;
    std::cout << "stoppoint: " << data.stop_points.size() << std::endl;
    std::cout << "vehiclejourney: " << data.vehicle_journeys.size() << std::endl;
    std::cout << "stop: " << data.stops.size() << std::endl;
    std::cout << "connection: " << data.stop_point_connections.size() << std::endl;
    std::cout << "journey_pattern points: " << data.journey_pattern_points.size() << std::endl;
    std::cout << "modes: " << data.physical_modes.size() << std::endl;
    std::cout << "validity pattern : " << data.validity_patterns.size() << std::endl;
    std::cout << "journey_pattern point connections : " << data.journey_pattern_point_connections.size() << std::endl;
    std::cout << "alias : " <<data.alias.size() << std::endl;
    std::cout << "synonyms : " <<data.synonymes.size() << std::endl;

    start = pt::microsec_clock::local_time();
    ed::EdPersistor p(connection_string);
    p.persist(data, meta);
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    std::cout << "temps de traitement" << std::endl;
    std::cout << "\t lecture des fichiers " << read << "ms" << std::endl;
    std::cout << "\t completion des données " << complete << "ms" << std::endl;
    std::cout << "\t netoyage des données " << clean << "ms" << std::endl;
    std::cout << "\t trie des données " << sort << "ms" << std::endl;
    std::cout << "\t enregistrement des données " << save << "ms" << std::endl;

    return 0;
}
