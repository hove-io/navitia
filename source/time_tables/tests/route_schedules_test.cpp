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
#include "ed/build_helper.h"
#include "type/type.h"
#include "tests/utils_test.h"
#include "time_tables/route_schedules.h"
//for more concice test
pt::ptime d(std::string str) {
    return boost::posix_time::from_iso_string(str);
}
/*
    Wanted schedules:
            VJ1    VJ2    VJ3    VJ4
        A          8h00   8h05
        B          8h10   8h20   8h25
        C   8h05
        D   9h30                 9h35

        But the thermometer algorithm is buggy, is doesn't give the shorest common superstring..
        So, actually we have this schedule
            VJ1
        C
        D
        A   8h
        B   8h10
        D

With VJ1, and VJ2, we ensure that we are no more comparing the two first jpp of
each VJ, but we have a more human order.
VJ2 and VJ3, is a basic test.
The difficulty with VJ4 comes when we have to compare it to VJ1.
When comparing VJ1 and VJ4, we compare VJ1(stopC) with VJ4(stopD)
and when we compare VJ4 and VJ1, we compare VJ4(stopB) with VJ1(stopC).
*/
BOOST_AUTO_TEST_CASE(test1) {
    ed::builder b("20120614");
    const std::string a_name = "stopA",
                      b_name = "stopB",
                      c_name = "stopC",
                      d_name = "stopD";
    b.vj("A", "1111111", "", true, "1", "1", "JP1")(c_name, 8*3600 + 5*60)(d_name, 9*3600 + 30*60);
    b.vj("A", "1111111", "", true, "2", "2", "JP2")(a_name, 8*3600)(b_name, 8*3600 + 10*60);
    b.vj("A", "1111111", "", true, "3", "3", "JP2")(a_name, 8*3600 + 5*60)(b_name, 8*3600 + 20*60);
    b.vj("A", "1111111", "", true, "4", "4", "JP3")(b_name, 8*3600+25*60)(d_name, 9*3600 + 35*60);
    b.data->pt_data->index();
    b.data->build_raptor();

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20120613");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20120630");

    b.data->meta->production_date = boost::gregorian::date_period(begin, end);

    pbnavitia::Response resp = navitia::timetables::route_schedule("line.uri=A", {}, d("20120615T070000"), 86400, 1, 3,
    10, 0, *(b.data), false, false);
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    auto get_vj = [](pbnavitia::RouteSchedule r, int i) {
        return r.table().headers(i).pt_display_informations().uris().vehicle_journey();
    };
    BOOST_REQUIRE_EQUAL(get_vj(route_schedule, 0), "1");
    BOOST_REQUIRE_EQUAL(get_vj(route_schedule, 1), "2");
    BOOST_REQUIRE_EQUAL(get_vj(route_schedule, 2), "3");
    BOOST_REQUIRE_EQUAL(get_vj(route_schedule, 3), "4");
}


