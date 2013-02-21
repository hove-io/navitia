#include <fstream>
#include "type/type.h"
#include "ratimake/adjustit_connector.h"

#include <boost/program_options.hpp>

#include "type/message.h"
#include <log4cplus/logger.h>

namespace nt = navitia::type;
namespace po = boost::program_options;

po::variables_map load_config(const std::string& filename){
    po::options_description desc("Allowed options");
    desc.add_options()
        //("log4cplus-config-file", po::value<std::string>()->default_value("log4cplus.ini"), "chemin vers le fichier de configuration de log4cplus")
        ("connect-string", po::value<std::string>(), "parametres de connection à la base de données : DRIVER=FreeTDS;SERVER=;UID=;PWD=;DATABASE=;TDS_Version=8.0;Port=1433;ClientCharset=UTF-8")
        ("media-lang", po::value<std::string>()->default_value("FR"), "filename")
        ("media-media", po::value<std::string>()->default_value("INTERNET"), "filename")
        ("destination", po::value<std::string>(), "filename");

    std::ifstream stream;
    stream.open(filename);
    po::variables_map vm;
    if(!stream.is_open()){
        //throw spam::exception("loading config file failed");
        std::cerr << "impossible d'ouvrir le fichier de conf" << std::endl;
        throw std::exception();
    } else {
        po::store(po::parse_config_file(stream, desc), vm);
        po::notify(vm);
    }
    return vm;
}



int main(int argc, char const* argv[]){
    po::variables_map params = load_config("ratimake.ini");//@TODO: config

    navitia::ratimake::AtLoader loader;
    std::map<std::string, std::vector<navitia::type::Message>> messages;
    messages = loader.load(params);

    nt::MessageHolder holder;
    holder.messages = messages;
    holder.generation_date = pt::second_clock::unoversal_time();

    std::cout << "nb objects impactés: " << messages.size() << std::endl;
    holder.save(params["destination"].as<std::string>());



    return 0;
}
