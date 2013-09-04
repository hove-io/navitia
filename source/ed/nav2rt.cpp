#include "config.h"
#include <iostream>

#include "utils/timer.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "utils/exception.h"
#include "type/data.h"
#include "ed/connectors/messages_connector.h"


namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    std::string output, input, connection_string, media_lang,
        media_media;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("output,o", po::value<std::string>(&output)->default_value("rtdata.nav.lz4"), "Fichier de sortie")
        ("input,i", po::value<std::string>(&input)->default_value("data.nav.lz4"), "Fichier d'entrée")
        ("connection-string", po::value<std::string>(&connection_string)->required(), "parametres de connexion à la base de données: host=localhost user=navitia dbname=navitia password=navitia");


    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        std::cout << argv[0] << " V" << NAVITIA_VERSION << " " << NAVITIA_BUILD_TYPE << std::endl;
        return 0;
    }

    init_logger();
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
    int read, load, save;

    std::cout << "chargement du nav" << std::endl;

    now = start = pt::microsec_clock::local_time();
    navitia::type::Data data;
    data.load(input);
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();

    std::cout << "chargement des messages" << std::endl;

    ed::connectors::MessageLoaderConfig config;
    config.connection_string = connection_string;
    try{
        start = pt::microsec_clock::local_time();
        data.pt_data.message_holder.messages = ed::connectors::load_messages(
                config, now);
        load = (pt::microsec_clock::local_time() - start).total_milliseconds();
    }catch(const navitia::exception& ex){
        std::cout << ex.what() << std::endl;
        return 1;
    }
    std::cout << data.pt_data.message_holder.messages.size() << " messages chargés" << std::endl;

    std::cout << "application des messages sur data" << std::endl;
    ed::connectors::apply_messages(data);

    std::cout << "Debut sauvegarde ..." << std::endl;

    start = pt::microsec_clock::local_time();
    data.save(output);
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    std::cout << "temps de traitement" << std::endl;
    std::cout << "\t lecture du nav " << read << "ms" << std::endl;
    std::cout << "\t chargement des messages " << load << "ms" << std::endl;
    std::cout << "\t enregistrement des données " << save << "ms" << std::endl;

    return 0;
}
