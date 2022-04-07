/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "raptor.h"
#include "type/data.h"
#include "utils/csv.h"
#include "utils/init.h"
#include "utils/timer.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#ifdef __BENCH_WITH_CALGRIND__
#include "valgrind/callgrind.h"
#endif

#include <fstream>
#include <random>

using namespace navitia;
using namespace routing;
namespace ba = boost::algorithm;
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

    explicit Result(Path path)
        : duration(path.duration.total_seconds()), time(-1), arrival(-1), nb_changes(path.nb_changes) {
        if (!path.items.empty()) {
            arrival = path.items.back().arrival.time_of_day().total_seconds();
        }
    }
};

int main(int argc, char** argv) {
    navitia::init_app();
    po::options_description desc("Options de l'outil de benchmark");
    std::string file, output, stop_input_file;
    int iterations, start, target, date, hour;

    // clang-format off
    desc.add_options()
            ("help", "Show this message")
            ("interations,i", po::value<int>(&iterations)->default_value(100),
                     "Number of iterations (10 calcuations par iteration)")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"),
                     "Path to data.nav.lz4")
            ("start,s", po::value<int>(&start)->default_value(-1),
                    "Start of a particular journey")
            ("target,t", po::value<int>(&target)->default_value(-1),
                    "Target of a particular journey")
            ("date,d", po::value<int>(&date)->default_value(-1),
                    "Begginning date of a particular journey")
            ("hour,h", po::value<int>(&hour)->default_value(-1),
                    "Begginning hour of a particular journey")
            ("verbose,v", "Verbose debugging output")
            ("stop_files", po::value<std::string>(&stop_input_file), "File with list of start and target")
            ("output,o", po::value<std::string>(&output)->default_value("benchmark.csv"),
                     "Output file");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    bool verbose = vm.count("verbose");

    if (vm.count("help")) {
        std::cout << "This is used to benchmark journey computation" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    type::Data data;
    {
        Timer t("Chargement des données : " + file);
        data.load_nav(file);
        data.build_raptor();
    }
    std::vector<PathDemand> demands;

    if (!stop_input_file.empty()) {
        // csv file should be the same as the output one
        CsvReader csv(stop_input_file, ',');
        csv.next();
        size_t cpt_not_found = 0;
        for (auto it = csv.next(); !csv.eof(); it = csv.next()) {
            const auto it_start = data.pt_data->stop_areas_map.find(it[0]);
            if (it_start == data.pt_data->stop_areas_map.end()) {
                std::cout << "impossible to find " << it[0] << std::endl;
                cpt_not_found++;
                continue;
            }
            const auto start = it_start->second;

            const auto it_end = data.pt_data->stop_areas_map.find(it[2]);
            if (it_end == data.pt_data->stop_areas_map.end()) {
                std::cout << "impossible to find " << it[2] << std::endl;
                cpt_not_found++;
                continue;
            }
            const auto end = it_end->second;

            PathDemand demand{};
            demand.start = start->idx;
            demand.target = end->idx;
            demand.hour = boost::lexical_cast<unsigned int>(it[5]);
            demand.date = boost::lexical_cast<unsigned int>(it[4]);
            demands.push_back(demand);
        }
        std::cout << "nb start not found " << cpt_not_found << std::endl;
    } else if (start != -1 && target != -1 && date != -1 && hour != -1) {
        PathDemand demand{};
        demand.start = start;
        demand.target = target;
        demand.hour = hour;
        demand.date = date;
        demands.push_back(demand);
    } else {
        // Génération des instances
        std::random_device rd;
        std::mt19937 rng(31442);
        std::uniform_int_distribution<> gen(0, data.pt_data->stop_areas.size() - 1);
        std::vector<unsigned int> hours{0, 28800, 36000, 72000, 86000};
        std::vector<unsigned int> days({7});
        if (data.pt_data->validity_patterns.front()->beginning_date.day_of_week().as_number() == 6) {
            days.push_back(8);
        } else {
            days.push_back(13);
        }

        for (int i = 0; i < iterations; ++i) {
            PathDemand demand{};
            demand.start = gen(rng);
            demand.target = gen(rng);
            while (demand.start == demand.target
                   || ba::starts_with(data.pt_data->stop_areas[demand.start]->uri, "stop_area:SNC:")
                   || ba::starts_with(data.pt_data->stop_areas[demand.target]->uri, "stop_area:SNC:")) {
                demand.target = gen(rng);
                demand.start = gen(rng);
            }
            for (auto day : days) {
                for (auto hour : hours) {
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
    // ProfilerStart("bench.prof");
    int nb_reponses = 0;
#ifdef __BENCH_WITH_CALGRIND__
    CALLGRIND_START_INSTRUMENTATION;
#endif
    for (auto demand : demands) {
        ++show_progress;
        Timer t2;
        if (verbose) {
            std::cout << data.pt_data->stop_areas[demand.start]->uri << ", " << demand.start << ", "
                      << data.pt_data->stop_areas[demand.target]->uri << ", " << demand.target << ", " << demand.date
                      << ", " << demand.hour << "\n";
        }
        auto res = router.compute(data.pt_data->stop_areas[demand.start], data.pt_data->stop_areas[demand.target],
                                  demand.hour, demand.date, DateTimeUtils::set(demand.date + 1, demand.hour),
                                  type::RTLevel::Base, 2_min, 2_min, true, {}, 10);

        Path path;
        if (!res.empty()) {
            path = res[0];
            ++nb_reponses;
        }

        Result result(path);
        result.time = t2.ms();
        results.push_back(result);
    }
    // ProfilerStop();
#ifdef __BENCH_WITH_CALGRIND__
    CALLGRIND_STOP_INSTRUMENTATION;
#endif

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

    for (size_t i = 0; i < demands.size(); ++i) {
        PathDemand demand = demands[i];
        out_file << data.pt_data->stop_areas[demand.start]->uri << ", " << demand.start << ", "
                 << data.pt_data->stop_areas[demand.target]->uri << ", " << demand.target << ", " << demand.date << ", "
                 << demand.hour;

        out_file << ", " << results[i].arrival << ", " << results[i].duration << ", " << results[i].nb_changes << ", "
                 << results[i].time;

        out_file << "\n";
    }
    out_file.close();

    std::cout << "Number of requests: " << demands.size() << std::endl;
    std::cout << "Number of results with solution: " << nb_reponses << std::endl;
}
