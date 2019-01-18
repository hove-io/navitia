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
#include "utils/csv.h"
#include <boost/algorithm/string/predicate.hpp>
#include <thread>
#ifdef __BENCH_WITH_CALGRIND__
#include "valgrind/callgrind.h"
#endif
#include <gperftools/profiler.h>

using namespace navitia;
using namespace routing;
namespace ba = boost::algorithm;
namespace po = boost::program_options;

struct Demand {
    unsigned int date;
    unsigned int hour;
    nt::RTLevel level;
    nt::AccessibiliteParams accessibilite_params;
};

static void compute(std::vector<Demand> demands, boost::progress_display& show_progress, const type::Data& data){

    std::random_shuffle(demands.begin(), demands.end());
    std::vector<const CachedNextStopTime*> results;
    for(auto demand : demands){
        ++show_progress;
        auto next_st = data.dataRaptor->cached_next_st_manager->load(DateTimeUtils::set(demand.date + 1, demand.hour),
                            demand.level,
                            demand.accessibilite_params
        );
        std::this_thread::yield();
        //store the pointer to prevent any kind of optimisation from the compiler
        // DO NOT USE THIS POINTER!!! it will have been freed
        results.push_back(next_st.get());
    }
    std::cout << "Number of results: " << results.size() << std::endl;
}

int main(int argc, char** argv){
    navitia::init_app();
    po::options_description desc("Options de l'outil de benchmark");
    std::string file, profile;
    int nb_days, size, nb_threads;

    desc.add_options()
            ("help", "Show this message")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"), "Path to data.nav.lz4")
            ("days,d", po::value<int>(&nb_days)->default_value(30), "number of day to build")
            ("size,s", po::value<int>(&size)->default_value(10), "raptor cache size")
            ("threads,t", po::value<int>(&nb_threads)->default_value(1), "number of threads to run")
            ("profile,p", po::value<std::string>(&profile)->default_value(""), "profile file");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "This is used to benchmark building of raptor cache" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    type::Data data;
    {
        Timer t("Data loading: " + file);
        data.load_nav(file);
        data.build_raptor(size);
    }
    std::vector<Demand> demands;
    std::vector<nt::RTLevel> levels{nt::RTLevel::Base, nt::RTLevel::Adapted, nt::RTLevel::RealTime};
    for(int day=1; day<nb_days; ++day){
        Demand demand;
        demand.date = day;
        demand.hour = 0;
        for(auto l: levels){
            demand.level = l;
            demands.push_back(demand);
        }
    }

    RAPTOR router(data);

    boost::progress_display show_progress(demands.size()*nb_threads);
    {
        Timer t("Build raptor cache ");
        std::vector<std::thread> threads;

        if(!profile.empty()){
            ProfilerStart(profile.c_str());
        }
#ifdef __BENCH_WITH_CALGRIND__
        CALLGRIND_START_INSTRUMENTATION;
#endif
        for(int i=0; i<nb_threads; ++i){
            auto t = std::thread(compute, std::ref(demands), std::ref(show_progress), std::ref(data));
            threads.push_back(std::move(t));
        }
        for(auto& th: threads){
            th.join();
        }
        if(!profile.empty()){
            ProfilerStop();
        }
#ifdef __BENCH_WITH_CALGRIND__
        CALLGRIND_STOP_INSTRUMENTATION;
#endif
    }


    std::cout << "Number of requests: " << demands.size() << std::endl;
}
