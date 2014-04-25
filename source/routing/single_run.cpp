/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "time_dependent.h"
#include "raptor.h"
//#include "static_raptor.h"
#include "type/data.h"
#include "utils/timer.h"
#include <boost/program_options.hpp>

using namespace navitia;
using namespace routing;
namespace po = boost::program_options ;

int main(int argc, char** argv){
    po::options_description desc("Options du calculateur simple");
    std::vector<std::string> algos;
    std::string start, target;
    unsigned int date, hour;
    bool verif;
    std::string file;
    desc.add_options()
            ("help", "Affiche l'aide")
            ("algo,a", po::value<decltype(algos)>(&algos), "Algorithme(s) à utiliser : raptor, time_dep")
            ("start,s", po::value<std::string>(&start), "ExternalCode du stopArea de départ")
            ("target,t", po::value<std::string>(&target), "ExternalCode du stopArea d'arrivée")
            ("date,d", po::value<unsigned int>(&date)->default_value(0), "Indexe de date")
            ("hour,h", po::value<unsigned int>(&hour)->default_value(8*3600), "Heure en secondes depuis minuit")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"))
            ("verif,v", po::value<bool>(&verif)->default_value(false), "Vérifier la cohérence des itinéraires");

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
            data.load(file);
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

        std::cout << "Calcul de " << data.pt_data.stop_areas[start_idx].name << "(" << start_idx << ")"<< " à " 
                  << data.pt_data.stop_areas[target_idx].name  << "(" << target_idx << ")"<<  std::endl;
        Verification verification(data.pt_data);
        for(auto algo : algos){
            std::cout << std::endl;
            AbstractRouter * router;
            if(algo == "raptor"){
                data.build_raptor();
                router = new raptor::RAPTOR(data);
            } else if(algo == "time_dep"){
                router = new timedependent::TimeDependent(data);
       //     } else if(algo == "static"){
       //         router = new StaticRaptor(data);
            } else {
                std::cerr << "Algorithme inconnu : " << algo << std::endl;
                return 1;
            }

            Timer t("Calcul avec l'algorithme " + algo);
            auto tmp = router->compute(start_idx, target_idx, hour, date);

            if(tmp.size() > 0) {
                for(auto res : tmp) {
                    for(auto item : res.items) {
                        std::cout << item.print(data.pt_data) << std::endl;
                    }

                    if(verif) {
                        verification.verif(res);
                    }
                    std::cout << " --------------------------------" << std::endl;
                }
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
