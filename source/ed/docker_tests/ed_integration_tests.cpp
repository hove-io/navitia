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
#define BOOST_TEST_MODULE associated_calendar_test
#include <boost/test/unit_test.hpp>
#include "utils/logger.h"
#include "type/data.h"
#include "type/pt_data.h"
#include <boost/algorithm/string.hpp>

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )


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
static void check_headsigns(const navitia::type::Data& data, const std::string& headsign,
                     size_t first_it_vj, size_t last_it_vj,
                     size_t first_it_st = 0, size_t last_it_st = std::numeric_limits<size_t>::max()) {
    const navitia::type::HeadsignHandler& headsigns = data.pt_data->headsign_handler;
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
    navitia::type::Data data;

    const auto res = data.load(input_file);

    BOOST_REQUIRE(res);

    BOOST_REQUIRE_EQUAL(data.pt_data->networks.size(), 1);
    BOOST_CHECK_EQUAL(data.pt_data->networks[0]->name, "ligne flixible");
    BOOST_CHECK_EQUAL(data.pt_data->networks[0]->uri, "network:ligneflexible");

    // check stop_time headsigns
    const navitia::type::HeadsignHandler& headsigns = data.pt_data->headsign_handler;
    auto& vj_vec = data.pt_data->vehicle_journeys;
    // check that vj 0 & 1 have headsign N1 from first 3 stop_time, then N2
    check_headsigns(data, "N1", 0, 1, 0, 2);
    check_headsigns(data, "N2", 0, 1, 3, 4);
    // vj 2 & 3 are named vehiclejourney2 with no headsign overload, 4 & 5 are named vehiclejourney3
    check_headsigns(data, "vehiclejourney2", 2, 3);
    check_headsigns(data, "vehiclejourney3", 4, 5);
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
    BOOST_CHECK_EQUAL(headsigns.get_vj_from_headsign("vehiclejourney3").size(), 2);
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney3"), vj_vec[4]));
    BOOST_CHECK(navitia::contains(headsigns.get_vj_from_headsign("vehiclejourney3"), vj_vec[5]));

    BOOST_CHECK_EQUAL(data.pt_data->meta_vjs.size(), 3);
}
