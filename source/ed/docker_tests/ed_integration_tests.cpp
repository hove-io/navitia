/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.
  
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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ed_integration_tests
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>
#include "utils/logger.h"
#include "type/data.h"
#include "type/pt_data.h"
#include "type/meta_data.h"
#include "tests/utils_test.h"
#include "routing/raptor_utils.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace pt = boost::posix_time;
namespace nt = navitia::type;
namespace nr = navitia::routing;

struct ArgsFixture {
   ArgsFixture() {
       auto argc = boost::unit_test::framework::master_test_suite().argc;
       auto argv = boost::unit_test::framework::master_test_suite().argv;

       //we should have at least more than one arg, the first is the exe name, the rest is the input file paths
       BOOST_REQUIRE(argc > 1);

       for (int i(1); i < argc; ++i) {
           std::vector<std::string> arg;

           std::string raw_arg = argv[i];
           // dumb method, we erase all '-' not to bother with the '--bob=toto' format
           boost::erase_all(raw_arg, "-");
           boost::split(arg, raw_arg, boost::is_any_of("="));

           BOOST_REQUIRE_EQUAL(arg.size(), 2);
           input_file_paths[arg.at(0)] = arg.at(1);
       }
   }

   std::map<std::string, std::string> input_file_paths;
};

// check headsign value for stop times on given vjs
static void check_headsigns(const nt::Data& data, const std::string& headsign,
                     size_t first_it_vj, size_t last_it_vj,
                     size_t first_it_st = 0, size_t last_it_st = std::numeric_limits<size_t>::max()) {
    const nt::HeadsignHandler& headsigns = data.pt_data->headsign_handler;
    auto& vj_vec = data.pt_data->vehicle_journeys;
    for (size_t it_vj = first_it_vj; it_vj <= last_it_vj; ++it_vj) {
        size_t real_last_it_st = vj_vec[it_vj]->stop_time_list.size() - 1;
        if (real_last_it_st > last_it_st) {
            real_last_it_st = last_it_st;
        }
        for (size_t it_st = first_it_st; it_st <= real_last_it_st; ++it_st) {
            BOOST_CHECK_EQUAL(headsigns.get_headsign(vj_vec[it_vj]->stop_time_list[it_st]), headsign);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(fusio_test, ArgsFixture) {
    const auto input_file = input_file_paths.at("ntfs_file");
    nt::Data data;

    const auto res = data.load(input_file);

    BOOST_REQUIRE(res);

    BOOST_REQUIRE_EQUAL(data.pt_data->networks.size(), 1);
    BOOST_CHECK_EQUAL(data.pt_data->networks[0]->name, "ligne flixible");
    BOOST_CHECK_EQUAL(data.pt_data->networks[0]->uri, "network:ligneflexible");

    // check stop_time headsigns
    const nt::HeadsignHandler& headsigns = data.pt_data->headsign_handler;
    auto& vj_vec = data.pt_data->vehicle_journeys;
    BOOST_REQUIRE_EQUAL(vj_vec.size(), 8);
    // check that vj 0 & 1 have headsign N1 from first 3 stop_time, then N2
    check_headsigns(data, "N1", 0, 1, 0, 2);
    check_headsigns(data, "N2", 0, 1, 3, 4);
    // vj 2 & 3 are named vehiclejourney2 with no headsign overload, 4-7 are named vehiclejourney3
    check_headsigns(data, "vehiclejourney2", 2, 3);
    check_headsigns(data, "vehiclejourney3", 4, 7);
    // check vj from headsign
    BOOST_CHECK_EQUAL(headsigns.get_vj_from_headsign("vehiclejourney1").size(), 2);
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney1"), vj_vec[0]));
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney1"), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsigns.get_vj_from_headsign("N1").size(), 2);
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("N1"), vj_vec[0]));
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("N1"), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsigns.get_vj_from_headsign("N2").size(), 2);
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("N2"), vj_vec[0]));
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("N2"), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsigns.get_vj_from_headsign("vehiclejourney2").size(), 2);
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney2"), vj_vec[2]));
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney2"), vj_vec[3]));
    BOOST_CHECK_EQUAL(headsigns.get_vj_from_headsign("vehiclejourney3").size(), 4);
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney3"), vj_vec[4]));
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney3"), vj_vec[5]));
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney3"), vj_vec[6]));
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney3"), vj_vec[7]));

    BOOST_CHECK_EQUAL(data.pt_data->meta_vjs.size(), 4);
    for (auto& mvj : data.pt_data->meta_vjs) {
        BOOST_CHECK_EQUAL(data.pt_data->meta_vjs[mvj->uri], mvj.get());
        BOOST_CHECK_EQUAL(data.pt_data->meta_vjs[nr::MvjIdx(*mvj)], mvj.get());
    }
}

BOOST_FIXTURE_TEST_CASE(gtfs_test, ArgsFixture) {
    const auto input_file = input_file_paths.at("gtfs_google_example_file");
    navitia::type::Data data;
    const auto res = data.load(input_file);

    BOOST_REQUIRE(res);

    const auto& pt_data = *data.pt_data;

    // TODO add generic check on collections
    // check map/vec same size + uri + idx + ...

    BOOST_REQUIRE_EQUAL(pt_data.networks.size(), 1);
    BOOST_CHECK_EQUAL(pt_data.networks[0]->name, "Demo Transit Authority");
    BOOST_CHECK_EQUAL(pt_data.networks[0]->uri, "network:DTA");

    BOOST_REQUIRE_EQUAL(pt_data.stop_areas.size(), 9);
    const auto* fur_creek = pt_data.stop_areas_map.at("stop_area:FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(fur_creek->uri, "stop_area:FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(fur_creek->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(fur_creek->coord.lat(), 36.425288, 0.001);
    BOOST_CHECK_CLOSE(fur_creek->coord.lon(), -117.133162, 0.001);

    for (auto sa: pt_data.stop_areas) {
        BOOST_CHECK_EQUAL(sa->timezone, "America/Los_Angeles");
    }

    //stop point, and lines should be equals too
    BOOST_REQUIRE_EQUAL(pt_data.stop_points.size(), 9);
    const auto* fur_creek_sp = pt_data.stop_points_map.at("stop_point:FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(fur_creek_sp->uri, "stop_point:FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(fur_creek_sp->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(fur_creek_sp->coord.lat(), 36.425288, 0.001);
    BOOST_CHECK_CLOSE(fur_creek_sp->coord.lon(), -117.133162, 0.001);
    BOOST_CHECK_EQUAL(fur_creek_sp->stop_area, fur_creek);

    // no connection in the gtfs provided, so we only create only cnx
    // for each stop_point
    // Note: the dataset is kind of useless without cnx but for
    // the moment we need them to be explicitly provided
    BOOST_REQUIRE_EQUAL(pt_data.stop_point_connections.size(), 9);

    BOOST_REQUIRE_EQUAL(pt_data.lines.size(), 5);
    const auto* ab_line = pt_data.lines_map.at("line:AB");
    BOOST_CHECK_EQUAL(ab_line->uri, "line:AB");
    BOOST_CHECK_EQUAL(ab_line->name, "Airport - Bullfrog");
    BOOST_REQUIRE(ab_line->network);
    BOOST_CHECK_EQUAL(ab_line->network->uri, "network:DTA");
    BOOST_REQUIRE(ab_line->commercial_mode != nullptr);
    BOOST_CHECK_EQUAL(ab_line->commercial_mode->uri, "commercial_mode:3");
    BOOST_REQUIRE_EQUAL(ab_line->route_list.size(), 2);
    BOOST_CHECK(boost::find_if(ab_line->route_list, [](nt::Route* r) { return r->uri == "route:AB:0"; })
                != std::end(ab_line->route_list));
    BOOST_CHECK(boost::find_if(ab_line->route_list, [](nt::Route* r) { return r->uri == "route:AB:1"; })
                != std::end(ab_line->route_list));

    // we need to also check the number of routes created
    BOOST_REQUIRE_EQUAL(pt_data.routes.size(), 9);
    for (const auto& r: pt_data.routes) {
        BOOST_REQUIRE(r->line);
        BOOST_CHECK_EQUAL(r->name, r->line->name);//route's name is it's line's name
    }

    BOOST_CHECK_EQUAL(data.meta->production_date,
                      boost::gregorian::date_period("20070101"_d, "20080102"_d));

    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 11 * 2);
    const auto* vj_ab1_dst1 = pt_data.vehicle_journeys_map.at("vehicle_journey:AB1_dst_1");
    BOOST_CHECK_EQUAL(vj_ab1_dst1->uri, "vehicle_journey:AB1_dst_1");
    //TODO check all vp
    BOOST_CHECK_EQUAL(vj_ab1_dst1->name, "to Bullfrog");
    BOOST_REQUIRE_EQUAL(vj_ab1_dst1->stop_time_list.size(), 2);
    // UTC offset is 8h in winter for LA
    BOOST_CHECK_EQUAL(vj_ab1_dst1->stop_time_list[0].arrival_time, "08:00"_t + "08:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst1->stop_time_list[0].departure_time, "08:00"_t + "08:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst1->stop_time_list[1].arrival_time, "08:10"_t + "08:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst1->stop_time_list[1].departure_time, "08:15"_t + "08:00"_t);
    BOOST_REQUIRE(vj_ab1_dst1->next_vj);
    BOOST_CHECK_EQUAL(vj_ab1_dst1->next_vj->uri, "vehicle_journey:BFC1_dst_1");
    // and ab1 should be the previous vj of it
    BOOST_CHECK_EQUAL(vj_ab1_dst1->next_vj->prev_vj, vj_ab1_dst1);

    // we also check the other AB1 split for the summer DST
    const auto* vj_ab1_dst2 = pt_data.vehicle_journeys_map.at("vehicle_journey:AB1_dst_2");
    // they should have the same meta vj
    BOOST_CHECK_EQUAL(vj_ab1_dst2->meta_vj, vj_ab1_dst1->meta_vj);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->uri, "vehicle_journey:AB1_dst_2");
    BOOST_CHECK_EQUAL(vj_ab1_dst2->name, "to Bullfrog");
    BOOST_REQUIRE_EQUAL(vj_ab1_dst2->stop_time_list.size(), 2);
    // UTC offset is 7h in summer for LA
    BOOST_CHECK_EQUAL(vj_ab1_dst2->stop_time_list[0].arrival_time, "08:00"_t + "07:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->stop_time_list[0].departure_time, "08:00"_t + "07:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->stop_time_list[1].arrival_time, "08:10"_t + "07:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->stop_time_list[1].departure_time, "08:15"_t + "07:00"_t);
    BOOST_REQUIRE(vj_ab1_dst2->next_vj);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->next_vj->uri, "vehicle_journey:BFC1_dst_2");
    // and ab1 should be the previous vj of it
    BOOST_CHECK_EQUAL(vj_ab1_dst2->next_vj->prev_vj, vj_ab1_dst2);

    // check that STBA is a frequency vj
    const auto* vj_stba = pt_data.vehicle_journeys_map.at("vehicle_journey:STBA_dst_1");
    const auto* vj_stba_freq = dynamic_cast<const nt::FrequencyVehicleJourney*>(vj_stba);
    BOOST_REQUIRE(vj_stba_freq);
    BOOST_CHECK_EQUAL(vj_stba_freq->start_time, "14:00"_t);
    BOOST_CHECK_EQUAL(vj_stba_freq->end_time, "30:00"_t);
    BOOST_CHECK_EQUAL(vj_stba_freq->headway_secs, 1800);
    for (const auto& st: vj_stba->stop_time_list) {
        BOOST_CHECK(st.is_frequency());
    }
    vj_stba = pt_data.vehicle_journeys_map.at("vehicle_journey:STBA_dst_2");
    vj_stba_freq = dynamic_cast<const nt::FrequencyVehicleJourney*>(vj_stba);
    BOOST_REQUIRE(vj_stba_freq);
    BOOST_CHECK_EQUAL(vj_stba_freq->start_time, "13:00"_t);
    BOOST_CHECK_EQUAL(vj_stba_freq->end_time, "29:00"_t);
    BOOST_CHECK_EQUAL(vj_stba_freq->headway_secs, 1800);
    for (const auto& st: vj_stba->stop_time_list) {
        BOOST_CHECK(st.is_frequency());
    }
}
