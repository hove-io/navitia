#include "time_dependent.h"
#include "raptor.h"
//#include "static_raptor.h"
#include "type/data.h"
#include "utils/timer.h"
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <random>
#include <fstream>
#include <valgrind/callgrind.h>

using namespace navitia;
using namespace routing;
namespace po = boost::program_options;

std::string iso_string(const nt::Data & d, int date, int hour){
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(date));
    date_time += boost::posix_time::seconds(hour);
    return boost::posix_time::to_iso_string(date_time);
}

struct PathDemand {
    type::idx_t start;
    type::idx_t target;
    unsigned int date;
    unsigned int hour;
};

struct Result {
    int duration;
    int visited;
    int time;
    int arrival;
    int nb_changes;

    Result(Path path) : duration(path.duration), visited(path.percent_visited), nb_changes(path.nb_changes) {
        if(path.items.size() > 0)
            arrival = path.items.back().arrival.hour();
        else
            arrival = -1;
    }
};

int main(int argc, char** argv){
    po::options_description desc("Options de l'outil de benchmark");
    std::vector<std::string> algos;
    std::string file, output;
    int iterations;
    desc.add_options()
            ("help", "Affiche l'aide")
            ("algo,a", po::value<decltype(algos)>(&algos), "Algorithme(s) à utiliser : raptor, time_dep")
            ("interations,i", po::value<int>(&iterations)->default_value(100), "Nombre d'itérations (10 calculs par itération)")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"), "Données en entrée")
            ("output,o", po::value<std::string>(&output)->default_value("benchmark.csv"), "Fichier de sortie");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    type::Data data;
    {
        Timer t("Chargement des données : " + file);
        data.load(file);
    }

    // Génération des instances
    std::random_device rd;
    std::mt19937 rng(31442);
    std::uniform_int_distribution<> gen(0,data.pt_data.stop_points.size()-1);

    std::vector<PathDemand> demands;
    for(int i = 0; i < iterations; ++i) {
        std::vector<unsigned int> hours{0, 28800, 36000, 72000, 86000};
        std::vector<unsigned int> days{7};
        if(data.pt_data.validity_patterns.front().beginning_date.day_of_week().as_number() == 6)
            days.push_back(8);
        else
            days.push_back(13);

        PathDemand demand;
        demand.start = gen(rng);
        demand.target = gen(rng);
        while(demand.start == demand.target) {
            demand.target = gen(rng);
        }
        for(auto day : days) {
            for(auto hour : hours) {
                demand.date = day;
                demand.hour = hour;
                demands.push_back(demand);
            }
        }
    }
    // Calculs des itinéraires
    std::map<std::string, std::vector<Result> > results;
    for(auto algo : algos){
        AbstractRouter * router;
        if(algo == "raptor"){        
            data.build_raptor();
            router = new raptor::RAPTOR(data);
        } else if(algo == "time_dep"){
            router = new timedependent::TimeDependent(data);
    //    } else if(algo == "static"){
    //        router = new StaticRaptor(data);
        } else {
            std::cerr << "Algorithme inconnu : " << algo << std::endl;
            return 1;
        }

        std::cout << "On lance le benchmark de l'algo " << algo << std::endl;
        boost::progress_display show_progress(demands.size());
        Timer t("Calcul avec l'algorithme " + algo);
        //ProfilerStart("bench.prof");
        for(auto demand : demands){
            ++show_progress;
            Timer t2;
            CALLGRIND_START_INSTRUMENTATION;

            auto res = router->compute(demand.start, demand.target, demand.hour, demand.date);
            CALLGRIND_STOP_INSTRUMENTATION;

            Path path;
            if(res.size() > 0) {
                path = res[0];
            }

            Result result(path);
            result.time = t2.ms();
            results[algo].push_back(result);
        }
        //ProfilerStop();
        delete router;
    }

    Timer ecriture("Écriture du fichier de résultats");
    std::fstream out_file(output, std::ios::out);
    out_file << "Start, Target, Day, Hour";
    for(auto algo : algos){
        out_file << ", "
                 << algo << "_arrival, "
                 << algo << "_duration, "
                 << algo << "_nb_change, "
                 << algo << "_visited, "
                 << algo << "_time";
    }
    out_file << "\n";

    for(size_t i = 0; i < demands.size(); ++i){
        PathDemand demand = demands[i];
        out_file << data.pt_data.stop_points[demand.start].uri
                 << ", " << data.pt_data.stop_points[demand.target].uri
                 << ", " << demand.date
                 << ", " << demand.hour;
        for(auto algo : algos){
            out_file << ", "
                     << results[algo][i].arrival << ", "
                     << results[algo][i].duration << ", "
                     << results[algo][i].nb_changes << ", "
                     << results[algo][i].visited << ", "
                     << results[algo][i].time;
        }
        out_file << "\n";
    }
    out_file.close();
}
