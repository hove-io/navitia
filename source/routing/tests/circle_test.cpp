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
#define BOOST_TEST_MODULE circle_test

#include "type/geographical_coord.h"
#include "routing/circle.h"
#include <boost/test/unit_test.hpp>
#include <iomanip>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <stdlib.h>
#include <time.h>


using namespace navitia::routing;

BOOST_AUTO_TEST_CASE(project_in_direction_test) {
    navitia::type::GeographicalCoord coord_Paris = navitia::type::GeographicalCoord(2.3522219000000177,48.856614);
    BOOST_CHECK_CLOSE (project_in_direction(coord_Paris, 80,10).distance_to(coord_Paris), 10,0.1);
    BOOST_CHECK_CLOSE (project_in_direction(coord_Paris, 90,42).distance_to(coord_Paris), 42,0.1);
    BOOST_CHECK_CLOSE (project_in_direction(coord_Paris, 115,42).distance_to(coord_Paris), 42,0.1);
    BOOST_CHECK_CLOSE (project_in_direction(coord_Paris, 181,12).distance_to(coord_Paris), 12,0.5);
    BOOST_CHECK_CLOSE (project_in_direction(coord_Paris, 195,115).distance_to(coord_Paris), 115,0.1);
    BOOST_CHECK_CLOSE (project_in_direction(coord_Paris, 270,89).distance_to(coord_Paris), 89,0.1);
    BOOST_CHECK_CLOSE (project_in_direction(coord_Paris, 314,29).distance_to(coord_Paris), 29,0.5);
}

BOOST_AUTO_TEST_CASE(circle_test) {
    navitia::type::GeographicalCoord coord_Paris = navitia::type::GeographicalCoord(2.3522219000000177,48.856614);
    navitia::type::GeographicalCoord coord_Pekin = navitia::type::GeographicalCoord(-89.61,40.5545);
    navitia::type::Polygon c_Paris_42=circle(coord_Paris,42);
    navitia::type::Polygon c_Paris_30=circle(coord_Paris,30);
    navitia::type::Polygon c_Pekin_459=circle(coord_Pekin,459);

    //Check circle is in a bigger when they have the same center
    bool coord_Paris_in_c_Paris_42=boost::geometry::within(coord_Paris, c_Paris_42);
    bool coord_Paris_in_c_Paris_30=boost::geometry::within(coord_Paris, c_Paris_30);
    bool c_Paris_30_in_c_Paris_42=boost::geometry::within(c_Paris_30, c_Paris_42);
    bool c_Paris_42_in_c_Paris_30=boost::geometry::within(c_Paris_42, c_Paris_30);
    bool c_Paris_42_in_c_Pekin_459=boost::geometry::within(c_Paris_30, c_Pekin_459);
    BOOST_CHECK(coord_Paris_in_c_Paris_42);
    BOOST_CHECK(coord_Paris_in_c_Paris_30);
    BOOST_CHECK(c_Paris_30_in_c_Paris_42);
    BOOST_CHECK(!c_Paris_42_in_c_Paris_30);
    BOOST_CHECK(!c_Paris_42_in_c_Pekin_459);

    //Check for a random point de distance to te center
    srand (time(NULL));
    int i = rand() % 360;
    int j = rand() % 360;
    int k = rand() % 360;
    double r_42 = c_Paris_42.outer()[i].distance_to(coord_Paris);
    double r_30 = c_Paris_30.outer()[j].distance_to(coord_Paris);
    double r_459 = c_Pekin_459.outer()[k].distance_to(coord_Pekin);
    BOOST_CHECK_CLOSE (r_42, 42,0.5);
    BOOST_CHECK_CLOSE (r_30, 30,0.5);
    BOOST_CHECK_CLOSE (r_459, 459,0.5);
}
