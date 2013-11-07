#include "config.h"
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

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    init_logger();
    std::string output, input, connection_string, media_lang,
        media_media;
    uint32_t shift_days;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("output,o", po::value<std::string>(&output)->default_value("rtdata.nav.lz4"), "Fichier de sortie")
        ("input,i", po::value<std::string>(&input)->default_value("data.nav.lz4"), "Fichier d'entrée")
        ("shift-days,d", po::value<uint32_t>(&shift_days)->default_value(2), "Elargissement de la période de publication")
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
    int read, load, load_pert, apply_adapted, save;

    std::cout << "chargement du nav" << std::endl;

    now = start = pt::microsec_clock::local_time();
    navitia::type::Data data;
    data.load(input);
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();

#define SIZE_EXT_CODE(type_name, collection_name)size_t collection_name##_size = data.pt_data.collection_name.size();\
SIZE_EXT_CODE(CLEAR_EXT_CODE)
    std::cout << "chargement des messages" << std::endl;

    ed::connectors::RealtimeLoaderConfig config(connection_string, shift_days);
//    config.connection_string = connection_string;
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

    std::cout << "chargement des perturbation AT" << std::endl;
    std::vector<navitia::type::AtPerturbation> perturbations;
    try{
        start = pt::microsec_clock::local_time();
        perturbations = ed::connectors::load_at_perturbations(config, now);
        load_pert = (pt::microsec_clock::local_time() - start).total_milliseconds();
    }catch(const navitia::exception& ex){
        std::cout << ex.what() << std::endl;
        return 1;
    }
    std::cout << perturbations.size() << " perturbations chargées" << std::endl;

    start = pt::microsec_clock::local_time();
    ed::AtAdaptedLoader adapter;
    adapter.apply(perturbations, data.pt_data);
    //aprés avoir modifié les graphs; on retrie
    data.pt_data.sort();
#define COMP_SIZE1(type_name, collection_name)BOOST_ASSERT(collection_name##_size == data.pt_data.collection_name.size());\
ITERATE_NAVITIA_PT_TYPES(COMP_SIZE1)
    apply_adapted = (pt::microsec_clock::local_time() - start).total_milliseconds();
    data.build_proximity_list();
#define COMP_SIZE2(type_name, collection_name)BOOST_ASSERT(collection_name##_size == data.pt_data.collection_name.size());\
ITERATE_NAVITIA_PT_TYPES(COMP_SIZE2)

    std::cout << "Construction de first letter" << std::endl;
    data.build_autocomplete();

#define COMP_SIZE(type_name, collection_name)BOOST_ASSERT(collection_name##_size == data.pt_data.collection_name.size());\
ITERATE_NAVITIA_PT_TYPES(COMP_SIZE)

    std::cout << "Debut sauvegarde ..." << std::endl;

    start = pt::microsec_clock::local_time();
    try {
        data.save(output);
    } catch(const navitia::exception &e) {
        std::cout << "Impossible de sauvegarder" << std::endl;
        std::cout << e.what() << std::endl;
    }
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    std::cout << "temps de traitement" << std::endl;
    std::cout << "\t lecture du nav " << read << "ms" << std::endl;
    std::cout << "\t chargement des messages " << load << "ms" << std::endl;
    std::cout << "\t chargement des perturbations " << load_pert << "ms" << std::endl;
    std::cout << "\t application des perturbations " << apply_adapted << "ms" << std::endl;
    std::cout << "\t enregistrement des données " << save << "ms" << std::endl;

    return 0;
}
