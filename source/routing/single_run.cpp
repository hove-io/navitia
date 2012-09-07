#include "time_dependent.h"
#include "raptor.h"
#include "type/data.h"
#include "utils/timer.h"
#include <boost/program_options.hpp>

#include <valgrind/callgrind.h>

using namespace navitia;
using namespace routing;
namespace po = boost::program_options ;

int main(int argc, char** argv){
    po::options_description desc("Options du calculateur simple");
    std::vector<std::string> algos;
    std::string start, target;
    unsigned int date, hour;
    bool verif = true;
    std::string file;
    desc.add_options()
            ("help", "Affiche l'aide")
            ("algo,a", po::value<decltype(algos)>(&algos), "Algorithme(s) à utiliser : raptor, time_dep")
            ("start,s", po::value<std::string>(&start), "ExternalCode du stopArea de départ")
            ("target,t", po::value<std::string>(&target), "ExternalCode du stopArea d'arrivée")
            ("date,d", po::value<unsigned int>(&date)->default_value(0), "Indexe de date")
            ("hour,h", po::value<unsigned int>(&hour)->default_value(8*3600), "Heure en secondes depuis minuit")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"));

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    if(vm.count("algo") && vm.count("start") && vm.count("target")) {
        type::Data data;
        {
            Timer t("Charegement des données : " + file);
            data.load_lz4(file);
        }

        type::idx_t start_idx, target_idx;
        if(data.pt_data.stop_area_map.find(start) != data.pt_data.stop_area_map.end()){
            start_idx = data.pt_data.stop_area_map[start];
        } else {
            std::cerr << "StopArea inconnu : " << start << std::endl;
            return 1;
        }
        if(data.pt_data.stop_area_map.find(target) != data.pt_data.stop_area_map.end()){
            target_idx = data.pt_data.stop_area_map[target];
        } else {
            std::cerr << "StopArea inconnu : " << target << std::endl;
            return 1;
        }

        std::cout << "Calcul de " << data.pt_data.stop_areas[start_idx].name << "(" << start_idx << ")"<< " à " << data.pt_data.stop_areas[target_idx].name  << "(" << target_idx << ")"<<  std::endl;
        Verification verification(data.pt_data);
        for(auto algo : algos){
            std::cout << std::endl;
            AbstractRouter * router;
            if(algo == "raptor"){
                router = new raptor::RAPTOR(data);
            } else if(algo == "time_dep"){
                router = new timedependent::TimeDependent(data);
            } else {
                std::cerr << "Algorithme inconnu : " << algo << std::endl;
                return 1;
            }

            Timer t("Calcul avec l'algorithme " + algo);
            CALLGRIND_START_INSTRUMENTATION;
            Path res = router->compute(start_idx, target_idx, hour, date);
            CALLGRIND_STOP_INSTRUMENTATION;
            //std::cout << res << std::endl;
            for(auto item : res.items) {
                std::cout << item.print(data.pt_data) << std::endl;
            }

            if(verif) {
                verification.verif(res);
            }
            std::cout << std::endl << "______________________________________" << std::endl << std::endl;
            delete router;
        }
        return 0;
    } else {
        std::cout << desc << std::endl;
        return 1;
    }
}
