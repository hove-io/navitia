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

#include "raptor_api.h"
#include "georef/street_network.h"
#include "raptor.h"
#include "type/data.h"
#include "type/pb_converter.h"
#include "utils/csv.h"
#include "utils/init.h"
#include "utils/timer.h"

#include <boost/algorithm/string/predicate.hpp>
#include "type/pb_converter.h"
#include "type/meta_data.h"
#ifdef __BENCH_WITH_CALGRIND__
#include "valgrind/callgrind.h"
#endif
#include <boost/program_options.hpp>
#include <boost/progress.hpp>

#include <fstream>
#include <random>

using namespace navitia;
using namespace routing;
namespace po = boost::program_options;
namespace ba = boost::algorithm;

struct Request {
    std::string start;
    std::string target;

    pt::ptime departure_posix_time;

    type::Mode_e start_mode = type::Mode_e::Walking;
    type::Mode_e target_mode = type::Mode_e::Walking;
};

struct Result {
    int nb_of_journeys_found;
    int computing_time_in_ms;

    Result(int nb_of_journeys_found_, int computing_time_in_ms_)
        : nb_of_journeys_found(nb_of_journeys_found_), computing_time_in_ms(computing_time_in_ms_) {}
};

static type::GeographicalCoord coord_of_entry_point(const type::EntryPoint& entry_point,
                                                    const navitia::type::Data& data) {
    type::GeographicalCoord result;
    switch (entry_point.type) {
        case type::Type_e::Address: {
            auto way = data.geo_ref->way_map.find(entry_point.uri);
            if (way != data.geo_ref->way_map.end()) {
                const auto geo_way = data.geo_ref->ways[way->second];
                return geo_way->nearest_coord(entry_point.house_number, data.geo_ref->graph);
            }
        } break;
        case type::Type_e::StopPoint: {
            auto sp_it = data.pt_data->stop_points_map.find(entry_point.uri);
            if (sp_it != data.pt_data->stop_points_map.end()) {
                return sp_it->second->coord;
            }
        } break;
        case type::Type_e::StopArea: {
            auto sa_it = data.pt_data->stop_areas_map.find(entry_point.uri);
            if (sa_it != data.pt_data->stop_areas_map.end()) {
                return sa_it->second->coord;
            }
        } break;
        case type::Type_e::Coord:
            return entry_point.coordinates;
        case type::Type_e::Admin: {
            auto it_admin = data.geo_ref->admin_map.find(entry_point.uri);
            if (it_admin != data.geo_ref->admin_map.end()) {
                const auto admin = data.geo_ref->admins[it_admin->second];
                return admin->coord;
            }
        } break;
        case type::Type_e::POI: {
            auto poi = data.geo_ref->poi_map.find(entry_point.uri);
            if (poi != data.geo_ref->poi_map.end()) {
                return poi->second->coord;
            }
        } break;
        default:
            break;
    }
    std::cout << "coord not found for " << entry_point.uri << std::endl;
    return {};
}

static type::EntryPoint make_entry_point(const std::string& entry_id, const type::Data& data) {
    type::EntryPoint entry;
    try {
        auto idx = boost::lexical_cast<type::idx_t>(entry_id);

        // if it is a cached idx, we consider it to be a stop area idx
        entry = type::EntryPoint(type::Type_e::StopArea, data.pt_data->stop_areas.at(idx)->uri, 0);
    } catch (const boost::bad_lexical_cast&) {
        // else we use the same way to identify an entry point as the api
        entry = type::EntryPoint(data.get_type_of_id(entry_id), entry_id);
    }

    entry.coordinates = coord_of_entry_point(entry, data);
    return entry;
}

int main(int argc, char** argv) {
    navitia::init_app();
    po::options_description desc("Benchmark tool options");
    std::string data_file, benchmark_output_file, requests_input_file, requests_output_file;
    int iterations, nb_second_pass;

    // clang-format off
    desc.add_options()
            ("help", "Show this message.")
            ("iterations,i", po::value<int>(&iterations)->default_value(100),
                     "Number of (start, target) pairs to be generated. Default = 100.\n"
                     "10 requests will be created for each (start, target) pair, with different departure date_time.")
            ("file,f", po::value<std::string>(&data_file)->default_value("data.nav.lz4"),
                     "Path to data.nav.lz4")
            ("verbose,v", "Verbose debugging output.")
            ("nb_second_pass", po::value<int>(&nb_second_pass)->default_value(0), "nb second pass")
            ("requests", po::value<std::string>(&requests_input_file),
                        "List of requests to benchmark on.\n"
                        "Must be a comma-separated csv file where the first 3 columns are :  start point uri, target point uri, departure posix time.\n"
                        "For example, a line could be :\n"
                        "stop_area:SAR:SA:1443, stop_area:GT5:SA:4253, 20190717T080000\n"
                        "The first line of the csv file is assumed to be column headers and is skipped.\n"
                        "Can contains more than the 3 aforementionned columns.")
            ("dump_requests", po::value<std::string>(&requests_output_file),
                                "Write a csv file with the list of requests to be used, before starting the benchmark.\n"
                                "Each line contains : start uri, target uri, departure posix time."
                                )
            ("output,o", po::value<std::string>(&benchmark_output_file),
                     "Write a csv file with the list of requests used, the computing time (in ms) needed to answer each request, "
                     "and the number of journey found.\n"
                     "Each line contains : start uri, target uri, departure posix time, computing time, number of journeys found."
                     );
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
        Timer t("Loading data from : " + data_file);
        data.load_nav(data_file);
        data.build_raptor();
    }
    std::vector<Request> requests;

    if (!requests_input_file.empty()) {
        // csv file should be the same as the output one
        CsvReader csv(requests_input_file, ',');
        csv.next();
        size_t cpt_not_found = 0;
        for (auto it = csv.next(); !csv.eof(); it = csv.next()) {
            Request request;
            request.start = it[0];
            request.target = it[1];
            request.departure_posix_time = boost::posix_time::from_iso_string(it[2]);
            requests.push_back(request);
        }
        std::cout << "nb start not found " << cpt_not_found << std::endl;
    } else {
        // Generating random requests
        std::random_device rd;
        std::mt19937 rng(31442);
        std::uniform_int_distribution<> gen(0, data.pt_data->stop_areas.size() - 1);
        std::vector<unsigned int> hours{0, 28800, 36000, 72000, 86000};
        std::vector<unsigned int> days({7});
        if (data.meta->production_date.begin().day_of_week().as_number() == 6) {
            days.push_back(days.front() + 1);
        } else {
            days.push_back(days.front() + 6);
        }

        for (int i = 0; i < iterations; ++i) {
            Request request;
            const type::StopArea* sa_start;
            const type::StopArea* sa_dest;
            do {
                sa_start = data.pt_data->stop_areas[gen(rng)];
                sa_dest = data.pt_data->stop_areas[gen(rng)];
                request.start = sa_start->uri;
                request.target = sa_dest->uri;
            } while (sa_start == sa_dest || ba::starts_with(sa_dest->uri, "stop_area:SNC:")
                     || ba::starts_with(sa_start->uri, "stop_area:SNC:"));

            for (auto day : days) {
                for (auto hour : hours) {
                    const DateTime departure_datetime = DateTimeUtils::set(day + 1, hour);
                    request.departure_posix_time = to_posix_time(departure_datetime, data);
                    requests.push_back(request);
                }
            }
        }
    }

    std::cout << data.meta->production_date << std::endl;

    // writing requests to a file
    if (vm.count("dump_requests")) {
        std::fstream out_file(requests_output_file, std::ios::out);
        out_file << "Start point uri, Target uri, Departure posix date time"
                 << "\n";

        for (const Request& request : requests) {
            out_file << request.start << ", " << request.target << ", " << to_iso_string(request.departure_posix_time)
                     << "\n";
        }
        out_file.close();
    }

    // Journeys computation
    std::vector<Result> results;
    data.build_raptor();
    RAPTOR raptor(data);
    auto georef_worker = georef::StreetNetwork(*data.geo_ref);

    // disabling logging, to not pollute std::cout
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    logger.setLogLevel(log4cplus::WARN_LOG_LEVEL);
    auto logger_raptor = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("raptor"));
    logger_raptor.setLogLevel(log4cplus::WARN_LOG_LEVEL);

    std::cout << "Launching benchmark " << std::endl;
    boost::progress_display show_progress(requests.size());
    int nb_reponses = 0, nb_journeys = 0;
    {
        Timer total_compute_timer("Total computing time");
        // ProfilerStart("bench.prof");

#ifdef __BENCH_WITH_CALGRIND__
        CALLGRIND_START_INSTRUMENTATION;
#endif
        for (auto request : requests) {
            ++show_progress;
            Timer t2;

            if (verbose) {
                std::cout << request.start << ", " << request.target << ", " << request.departure_posix_time << "\n";
            }

            type::EntryPoint origin = make_entry_point(request.start, data);
            type::EntryPoint destination = make_entry_point(request.target, data);

            origin.streetnetwork_params.mode = request.start_mode;
            origin.streetnetwork_params.offset = data.geo_ref->offsets[request.start_mode];
            origin.streetnetwork_params.max_duration = navitia::seconds(30 * 60);
            origin.streetnetwork_params.speed_factor = 1;
            destination.streetnetwork_params.mode = request.target_mode;
            destination.streetnetwork_params.offset = data.geo_ref->offsets[request.target_mode];
            destination.streetnetwork_params.max_duration = navitia::seconds(30 * 60);
            destination.streetnetwork_params.speed_factor = 1;
            type::AccessibiliteParams accessibilite_params;
            navitia::PbCreator pb_creator(&data, boost::gregorian::not_a_date_time, null_time_period);

            int days_since_epoch = (request.departure_posix_time.date() - boost::gregorian::date(1970, 1, 1)).days();
            int total_second_in_day = request.departure_posix_time.time_of_day().total_seconds();

            const DateTime departure_datetime = DateTimeUtils::set(days_since_epoch, total_second_in_day);
            make_response(pb_creator, raptor, origin, destination, {departure_datetime},
                          true,                      // clockwise ?
                          accessibilite_params, {},  // forbidden
                          {},                        // allowed
                          georef_worker,
                          type::RTLevel::Base,             // real time level
                          2_min,                           // arrival_transfer penalty
                          2_min,                           // walking_transfer penalty
                          DateTimeUtils::SECONDS_PER_DAY,  // max_duration
                          10,                              // max_transfers
                          nb_second_pass);
            auto resp = pb_creator.get_response();

            Result result(resp.journeys().size(), t2.ms());
            results.push_back(result);

            if (resp.journeys_size() > 0) {
                ++nb_reponses;
                nb_journeys += resp.journeys_size();
            }
        }
        // ProfilerStop();
#ifdef __BENCH_WITH_CALGRIND__
        CALLGRIND_STOP_INSTRUMENTATION;
#endif
    }
    if (vm.count("output")) {
        std::fstream out_file(benchmark_output_file, std::ios::out);
        out_file << "Start point uri, Target uri, Departure posix date time, Computing time (ms), Nb of journeys found"
                 << "\n";

        for (size_t i = 0; i < requests.size(); ++i) {
            const Request& request = requests[i];
            const Result& result = results[i];
            out_file << request.start << ", " << request.target << ", " << to_iso_string(request.departure_posix_time)
                     << ", " << result.computing_time_in_ms << ", " << result.nb_of_journeys_found << "\n";
        }
        out_file.close();
    }

    std::cout << "Number of requests: " << requests.size() << std::endl;
    std::cout << "Number of results with solution: " << nb_reponses << std::endl;
    std::cout << "Number of journey found: " << nb_journeys << std::endl;
}
