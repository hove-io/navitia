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

#include "raptor.h"
#include "type/data.h"
#include "utils/timer.h"
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <random>
#include <fstream>
#include "utils/init.h"

using namespace navitia;
using namespace routing;
namespace po = boost::program_options;

struct PathDemand {
    type::idx_t start;
    type::idx_t target;
    unsigned int date;
    unsigned int hour;
};

struct Result {
    int duration;
    int time;
    int arrival;
    int nb_changes;

    Result(Path path) : duration(path.duration.total_seconds()), time(-1), arrival(-1), nb_changes(path.nb_changes) {
        if(!path.items.empty())
            arrival = path.items.back().arrival.time_of_day().total_seconds();
    }
};

int main(int argc, char** argv){
    navitia::init_app();
    po::options_description desc("Options de l'outil de benchmark");
    std::string file, output;
    int iterations, start, target, date, hour;

    desc.add_options()
            ("help", "Affiche l'aide")
            ("interations,i", po::value<int>(&iterations)->default_value(100), "Number of iterations (10 calcuations par iteration)")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"), "Path to data.nav.lz4")
            ("start,s", po::value<int>(&start)->default_value(-1), "Start pour du debug")
            ("target,t", po::value<int>(&target)->default_value(-1), "Target pour du debug")
            ("date,d", po::value<int>(&date)->default_value(-1), "Date for the debug")
            ("hour,h", po::value<int>(&hour)->default_value(-1), "Hour for the debug")
            ("verbose,v", "Verbose debugging output")
            ("output,o", po::value<std::string>(&output)->default_value("benchmark.csv"), "Output file");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    bool verbose = vm.count("verbose");

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    type::Data data;
    {
        Timer t("Chargement des données : " + file);
        data.load(file);
    }
    std::vector<PathDemand> demands;

    if(start != -1 && target != -1 && date != -1 && hour != -1) {
        PathDemand demand;
        demand.start = start;
        demand.target = target;
        demand.hour = hour;
        demand.date = date;
        demands.push_back(demand);
    } else {
        // Génération des instances
        std::random_device rd;
        std::mt19937 rng(31442);
        std::uniform_int_distribution<> gen(0,data.pt_data->stop_areas.size()-1);
        std::vector<unsigned int> hours{0, 28800, 36000, 72000, 86000};
        std::vector<unsigned int> days({7});
        if(data.pt_data->validity_patterns.front()->beginning_date.day_of_week().as_number() == 6)
            days.push_back(8);
        else
            days.push_back(13);

        for(int i = 0; i < iterations; ++i) {
            PathDemand demand;
            demand.start = gen(rng);
            demand.target = gen(rng);
            while(demand.start == demand.target) {
                demand.target = gen(rng);
                demand.start = gen(rng);
            }
            for(auto day : days) {
                for(auto hour : hours) {
                    demand.date = day;
                    demand.hour = hour;
                    demands.push_back(demand);
                }
            }
        }
    }

    // Calculs des itinéraires
    std::vector<Result> results;
    data.build_raptor();
    RAPTOR router(data);

    std::cout << "On lance le benchmark de l'algo " << std::endl;
    boost::progress_display show_progress(demands.size());
    Timer t("Calcul avec l'algorithme ");
    //ProfilerStart("bench.prof");
    int nb_reponses = 0;
    for(auto demand : demands){
        ++show_progress;
        Timer t2;
        if (verbose){
            std::cout << data.pt_data->stop_areas[demand.start]->uri
                      << ", " << demand.start
                      << ", " << data.pt_data->stop_areas[demand.target]->uri
                      << ", " << demand.target
                      << ", " << demand.date
                      << ", " << demand.hour
                      << "\n";
        }
        auto res = router.compute(data.pt_data->stop_areas[demand.start], data.pt_data->stop_areas[demand.target], demand.hour, demand.date, navitia::DateTimeUtils::inf,false, true);

        Path path;
        if(res.size() > 0) {
            path = res[0];
            ++ nb_reponses;
        }

        Result result(path);
        result.time = t2.ms();
        results.push_back(result);
    }
    //ProfilerStop();


    Timer ecriture("Writing results");
    std::fstream out_file(output, std::ios::out);
    out_file << "Start, Start id, Target, Target id, Day, Hour";
        out_file << ", "
                 << "arrival, "
                 << "duration, "
                 << "nb_change, "
                 << "visited, "
                 << "time";
    out_file << "\n";

    for(size_t i = 0; i < demands.size(); ++i){
        PathDemand demand = demands[i];
        out_file << data.pt_data->stop_areas[demand.start]->uri
                 << ", " << demand.start
                 << ", " << data.pt_data->stop_areas[demand.target]->uri
                 << ", " << demand.target
                 << ", " << demand.date
                 << ", " << demand.hour;

        out_file << ", "
                 << results[i].arrival << ", "
                 << results[i].duration << ", "
                 << results[i].nb_changes << ", "
                 << results[i].time;

        out_file << "\n";
    }
    out_file.close();

    std::cout << "Number of requests :" << demands.size() << std::endl;
    std::cout << "Number of results with solution" << nb_reponses << std::endl;
}
