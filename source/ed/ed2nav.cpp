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


namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    std::string output, connection_string, at_connection_string, media_lang,
        media_media;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("output,o", po::value<std::string>(&output)->default_value("data.nav.lz4"), "Fichier de sortie")
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

    if(vm.count("help")) {
        std::cout << desc <<  "\n";
        return 1;
    }

    po::notify(vm);

    pt::ptime start, end, now;
    int read, sort, autocomplete, save;

    navitia::type::Data data;

    //on init now pour le moment à now, à rendre paramétrable pour le debug
    now = start = pt::microsec_clock::local_time();

    ed::EdReader reader(connection_string);
    reader.fill(data);
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();


    std::cout << "line: " << data.pt_data.lines.size() << std::endl;
    std::cout << "route: " << data.pt_data.routes.size() << std::endl;
    std::cout << "journey_pattern: " << data.pt_data.journey_patterns.size() << std::endl;
    std::cout << "stoparea: " << data.pt_data.stop_areas.size() << std::endl;
    std::cout << "stoppoint: " << data.pt_data.stop_points.size() << std::endl;
    std::cout << "vehiclejourney: " << data.pt_data.vehicle_journeys.size() << std::endl;
    std::cout << "stop: " << data.pt_data.stop_times.size() << std::endl;
    std::cout << "connection: " << data.pt_data.stop_point_connections.size() << std::endl;
    std::cout << "journey_pattern points: " << data.pt_data.journey_pattern_points.size() << std::endl;
    std::cout << "modes: " << data.pt_data.physical_modes.size() << std::endl;
    std::cout << "validity pattern : " << data.pt_data.validity_patterns.size() << std::endl;
    std::cout << "journey_pattern point connections : " << data.pt_data.journey_pattern_point_connections.size() << std::endl;

    start = pt::microsec_clock::local_time();
    data.pt_data.sort();
    sort = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    std::cout << "Construction de proximity list" << std::endl;
    data.build_proximity_list();
    std::cout << "Construction de external code" << std::endl;
    //construction des map uri => idx
    data.build_uri();

    std::cout << "Construction de first letter" << std::endl;
    data.build_autocomplete();

    /* ça devrait etre fait avant, à vérifier
    std::cout << "On va construire les correspondances" << std::endl;
    {Timer t("Construction des correspondances");  data.pt_data.build_connections();}
    */
    autocomplete = (pt::microsec_clock::local_time() - start).total_milliseconds();


    std::cout << "Debut sauvegarde ..." << std::endl;

    start = pt::microsec_clock::local_time();
    data.save(output);
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    std::cout << "temps de traitement" << std::endl;
    std::cout << "\t lecture des fichiers " << read << "ms" << std::endl;
    std::cout << "\t trie des données " << sort << "ms" << std::endl;
    std::cout << "\t autocomplete " << autocomplete << "ms" << std::endl;
    std::cout << "\t enregistrement des données " << save << "ms" << std::endl;

    return 0;
}
