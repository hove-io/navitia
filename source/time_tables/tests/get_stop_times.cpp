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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "time_tables/get_stop_times.h"
#include "ed/build_helper.h"
using namespace navitia::timetables;

BOOST_AUTO_TEST_CASE(test1){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    b.data->pt_data->index();
    b.data->build_raptor();

    std::vector<navitia::type::idx_t> rps;
    for(auto jpp : b.data->pt_data->journey_pattern_points)
        rps.push_back(jpp->idx);
    auto result = get_stop_times(rps, navitia::DateTimeUtils::min, navitia::DateTimeUtils::inf, 1, *b.data, false);
    BOOST_REQUIRE_EQUAL(result.size(), 1);

}
