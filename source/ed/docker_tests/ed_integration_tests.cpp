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

BOOST_FIXTURE_TEST_CASE(fusio_test, ArgsFixture) {
    const auto input_file = input_file_paths.at("ntfs_file");
    navitia::type::Data data;

    const auto res = data.load(input_file);

    BOOST_REQUIRE(res);

    BOOST_REQUIRE_EQUAL(data.pt_data->networks.size(), 1);
    BOOST_CHECK_EQUAL(data.pt_data->networks[0]->name, "ligne flixible");
    BOOST_CHECK_EQUAL(data.pt_data->networks[0]->uri, "network:ligneflexible");

    //TODO, have fun ! :)
}
