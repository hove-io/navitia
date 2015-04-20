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

#include "raptor_api.h"
#include "raptor.h"
#include "georef/street_network.h"
#include "type/data.h"
#include "utils/timer.h"
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <random>
#include <fstream>
#include "utils/init.h"
#include "utils/csv.h"
#include <boost/algorithm/string/predicate.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#ifdef __BENCH_WITH_CALGRIND__
#include "valgrind/callgrind.h"
#endif

using namespace navitia;
using namespace routing;
namespace po = boost::program_options;
namespace ba = boost::algorithm;

struct PathDemand {
    std::string start;
    std::string target;
    boost::posix_time::ptime datetime;

    type::Mode_e start_mode = type::Mode_e::Walking;
    type::Mode_e target_mode = type::Mode_e::Walking;
};

struct Result {
    size_t nb_sol;
    int time;
    boost::posix_time::ptime datetime;

    std::string start;
    std::string target;
    type::Mode_e start_mode;
    type::Mode_e target_mode;

};

type::GeographicalCoord coord_of_entry_point(const type::EntryPoint& entry_point, const navitia::type::Data& data) {
    type::GeographicalCoord result;
    switch (entry_point.type) {
    case type::Type_e::Address: {
            auto way = data.geo_ref->way_map.find(entry_point.uri);
            if (way != data.geo_ref->way_map.end()){
                const auto geo_way = data.geo_ref->ways[way->second];
                return geo_way->nearest_coord(entry_point.house_number, data.geo_ref->graph);
            }
        }
        break;
    case type::Type_e::StopPoint: {
            auto sp_it = data.pt_data->stop_points_map.find(entry_point.uri);
            if(sp_it != data.pt_data->stop_points_map.end()) {
                return  sp_it->second->coord;
            }
        }
        break;
    case type::Type_e::StopArea: {
            auto sa_it = data.pt_data->stop_areas_map.find(entry_point.uri);
            if(sa_it != data.pt_data->stop_areas_map.end()) {
                return  sa_it->second->coord;
            }
        }
        break;
    case type::Type_e::Coord:
        return entry_point.coordinates;
        break;
    case type::Type_e::Admin: {
            auto it_admin = data.geo_ref->admin_map.find(entry_point.uri);
            if (it_admin != data.geo_ref->admin_map.end()) {
                const auto admin = data.geo_ref->admins[it_admin->second];
                return  admin->coord;
            }
        }
        break;
    case type::Type_e::POI: {
            auto poi = data.geo_ref->poi_map.find(entry_point.uri);
            if (poi != data.geo_ref->poi_map.end()){
                const auto geo_poi = data.geo_ref->pois[poi->second];
                return geo_poi->coord;
            }
        }
        break;
    default:
        break;
    }
    std::cout << "coord not found for " << entry_point.uri << std::endl;
    return {};
}

static type::EntryPoint make_entry_point(const std::string& entry_id, const type::Data& data) {
    type::EntryPoint entry;
    try {
        type::idx_t idx = boost::lexical_cast<type::idx_t>(entry_id);

        //if it is a idx, we consider it to be a stop area idx
        entry = type::EntryPoint(type::Type_e::StopArea, data.pt_data->stop_areas.at(idx)->uri, 0);
    } catch (boost::bad_lexical_cast) {
        // else we use the same way to identify an entry point as the api
        entry = type::EntryPoint(data.get_type_of_id(entry_id), entry_id);
    }

    entry.coordinates = coord_of_entry_point(entry, data);
    return entry;
}

int main(int argc, char** argv){
    navitia::init_app();
    po::options_description desc("Options de l'outil de benchmark");
    std::string file, output, stop_input_file, start, target;
    int iterations;
    std::string datetime;

    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    logger.setLogLevel(log4cplus::WARN_LOG_LEVEL);

    desc.add_options()
            ("help", "Show this message")
            ("interations,i", po::value<int>(&iterations)->default_value(100),
                     "Number of iterations (10 calcuations par iteration)")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"),
                     "Path to data.nav.lz4")
            ("start,s", po::value<std::string>(&start),
                    "Start of a particular journey")
            ("target,t", po::value<std::string>(&target),
                    "Target of a particular journey")
            ("datetime,d", po::value<std::string>(&datetime),
                    "Begginning date of a particular journey")
            ("verbose,v", "Verbose debugging output")
            ("stop_files", po::value<std::string>(&stop_input_file), "File with list of start and target")
            ("output,o", po::value<std::string>(&output)->default_value("benchmark.csv"),
                     "Output file");
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
        data.load(file);
    }
    std::vector<PathDemand> demands;

    if (! stop_input_file.empty()) {
        //csv file should be the same as the output one
        CsvReader csv(stop_input_file, ',');
        csv.next();
        size_t cpt_not_found = 0;
        for (auto it = csv.next(); ! csv.eof(); it = csv.next()) {
            PathDemand demand;
            demand.start = it[0];
            demand.target = it[1];
            demand.datetime = boost::posix_time::from_iso_string(it[2]);
            demands.push_back(demand);
        }
        std::cout << "nb start not found " << cpt_not_found << std::endl;
    }
    else if(! start.empty() && ! target.empty() && ! datetime.empty()) {
        PathDemand demand;
        demand.start = start;
        demand.target = target;
        demand.datetime = boost::posix_time::from_iso_string(datetime);
        std::cout << "we use the entry param " << start << " -> " << target << std::endl;
        demands.push_back(demand);
    } else {
        // Génération des instances
        std::random_device rd;
        std::mt19937 rng(31442);
        std::uniform_int_distribution<> gen(0,data.pt_data->stop_areas.size()-1);
        std::uniform_int_distribution<> days_gen(30, 45);
        std::uniform_int_distribution<> hour_gen(0, 86399);


        for(int i = 0; i < iterations; ++i) {
            PathDemand demand;
            const type::StopArea* sa_start;
            const type::StopArea* sa_dest;
            do {
                sa_start = data.pt_data->stop_areas[gen(rng)];
                sa_dest = data.pt_data->stop_areas[gen(rng)];
                demand.start = sa_start->uri;
                demand.target = sa_dest->uri;
            }
            while(sa_start == sa_dest
                    || ba::starts_with(sa_dest->uri, "stop_area:SNC:")
                    || ba::starts_with(sa_start->uri, "stop_area:SNC:"));

            if (! datetime.empty()) {
                demand.datetime = boost::posix_time::from_iso_string(datetime);
            } else {
                unsigned d = days_gen(rng);
                demand.datetime = boost::posix_time::ptime(data.pt_data->validity_patterns.front()->beginning_date + boost::gregorian::days(d),
                                                           boost::posix_time::seconds(hour_gen(rng)));
            }
            demands.push_back(demand);
        }
    }

    // Calculs des itinéraires
    std::vector<Result> results;
    data.build_raptor();
    RAPTOR router(data);
    auto georef_worker = georef::StreetNetwork(*data.geo_ref);

    std::cout << "On lance le benchmark de l'algo " << std::endl;
    boost::progress_display show_progress(demands.size());
    Timer t("Calcul avec l'algorithme ");
    //ProfilerStart("bench.prof");
    int nb_reponses = 0;
#ifdef __BENCH_WITH_CALGRIND__
    CALLGRIND_START_INSTRUMENTATION;
#endif
    for(auto demand : demands){
        ++show_progress;
        Timer t2;
        if (verbose){
            std::cout << demand.start
                      << ", " << demand.start
                      << ", " << demand.target
                      << ", " << demand.datetime
                      << "\n";
        }

        Result result;
        type::EntryPoint origin = make_entry_point(demand.start, data);
        type::EntryPoint destination = make_entry_point(demand.target, data);

        origin.streetnetwork_params.mode = demand.start_mode;
        origin.streetnetwork_params.offset = data.geo_ref->offsets[demand.start_mode];
        origin.streetnetwork_params.max_duration = navitia::seconds(15*60);
        origin.streetnetwork_params.speed_factor = 1;
        destination.streetnetwork_params.mode = demand.target_mode;
        destination.streetnetwork_params.offset = data.geo_ref->offsets[demand.target_mode];
        destination.streetnetwork_params.max_duration = navitia::seconds(15*60);
        destination.streetnetwork_params.speed_factor = 1;
        type::AccessibiliteParams accessibilite_params;
        auto resp = make_response(router, origin, destination,
              {navitia::to_posix_timestamp(demand.datetime)}, true,
              accessibilite_params,
              {},
              georef_worker,
              false,
              true,
              std::numeric_limits<int>::max(), 10, false);

        if(resp.journeys_size() > 0) {
            ++ nb_reponses;
        }

        result.start = origin.uri;
        result.target = destination.uri;
        result.datetime = demand.datetime;
        result.start_mode = demand.start_mode;
        result.target_mode = demand.target_mode;
        result.nb_sol = resp.journeys_size();
        result.time = t2.ms();

        results.push_back(result);
    }
    //ProfilerStop();
#ifdef __BENCH_WITH_CALGRIND__
    CALLGRIND_STOP_INSTRUMENTATION;
#endif

    Timer file_write("Writing results\n");
    std::fstream out_file(output, std::ios::out);
    out_file << "Start, Target, datetime"
             << ", nb solutions, time \n";

    for (const auto& res: results) {

        out_file << res.start
                 << "," << res.target
                 << "," << boost::posix_time::to_iso_string(res.datetime)
                 << "," << res.nb_sol
                 << "," << res.time
                 << "\n";
    }
    out_file.close();

    std::cout << "Number of requests: " << demands.size() << std::endl;
    std::cout << "Number of results with solution: " << nb_reponses << std::endl;
}
