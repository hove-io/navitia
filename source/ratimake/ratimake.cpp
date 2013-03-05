#include <fstream>
#include "type/type.h"
#include "connectors/adjustit_connector.h"

#include <boost/program_options.hpp>

#include "type/message.h"
#include <utils/logger.h>
#include "utils/exception.h"
#include "config.h"
#include <boost/format.hpp>

namespace nt = navitia::type;
namespace po = boost::program_options;
namespace pt = boost::posix_time;

po::variables_map load_config(int argc, const char** argv){
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("date,d", po::value<std::string>(), "Date considéré comme maintenant")
        ("version,v", "Affiche la version")
        ("config-file", po::value<std::string>()->default_value(argv[0] + std::string(".ini")), "chemin vers le fichier de configuration")
        ("log4cplus-config-file", po::value<std::string>()->default_value("log4cplus.ini"), "chemin vers le fichier de configuration de log4cplus")
        ("connect-string", po::value<std::string>(), "parametres de connection à la base de données : DRIVER=FreeTDS;SERVER=;UID=;PWD=;DATABASE=;TDS_Version=8.0;Port=1433;ClientCharset=UTF-8")
        ("media-lang", po::value<std::string>()->default_value("FR"), "langue du media à charger")
        ("media-media", po::value<std::string>()->default_value("INTERNET"), "media à cahrger")
        ("destination", po::value<std::string>()->default_value("at.rt.lz4"), "nom du fichier généré par ratimake");

    po::variables_map vm;

    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("help")){
        std::cout << desc << "\n";
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
    po::notify(vm);

    return vm;
}

void generate(const po::variables_map& params, const pt::ptime& now){
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    navitia::AtLoader loader;
    std::map<std::string, std::vector<navitia::type::Message>> messages;

    messages = loader.load(params, now);

    nt::MessageHolder holder;
    holder.messages = messages;
    holder.generation_date = pt::second_clock::universal_time();

    LOG4CPLUS_DEBUG(logger, boost::format("nb objects impactés: %s") % messages.size());
    holder.save(params["destination"].as<std::string>());
}

int main(int argc, char const* argv[]){
    init_logger();

    po::variables_map params;

    try{
        params = load_config(argc, argv);//@TODO: config
    }catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }

    if(params.count("version")){
        std::cout << argv[0] << " V" << NAVITIA_VERSION << " " << NAVITIA_BUILD_TYPE << std::endl;
        return 0;
    }

    pt::ptime now;
    if(params.count("date")){
        now = pt::time_from_string(params["date"].as<std::string>());
    }else{
        now = pt::second_clock::local_time();
    }


    if(params.count("log4cplus-config-file")){
        init_logger(params["log4cplus-config-file"].as<std::string>());
    }
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if(!params.count("connect-string") || params["connect-string"].as<std::string>().empty()){
        LOG4CPLUS_FATAL(logger, "chaine de connection à la base non défini");
        return 2;
    }

    try{
        generate(params, now);
    }catch(const std::exception& e){
        LOG4CPLUS_FATAL(logger, e.what());
        return 3;
    }

    return 0;
}
