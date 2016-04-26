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
#include "ed/build_helper.h"
#include "routing/raptor.h"
#include "routing/routing.h"
#include "tests/utils_test.h"

#include <boost/test/unit_test.hpp>
#include <iomanip>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <iostream>


using namespace navitia::routing;

BOOST_AUTO_TEST_CASE(project_in_direction_test) {
    std::vector<navitia::type::GeographicalCoord> center;
    auto coord_Paris = navitia::type::GeographicalCoord(2.3522219000000177,48.856614);
    auto coord_North = navitia::type::GeographicalCoord(90,0);
    auto coord_Equator = navitia::type::GeographicalCoord(0,48.856614);
    center.push_back(coord_Paris);
    center.push_back(coord_North);
    center.push_back(coord_Equator);
    for (int i = 0; i < 3; i++) {
        for (double angle = - 100; angle < 400; angle++) {
            for (double radius = 10; radius < 100; radius++) {
                BOOST_CHECK_CLOSE (project_in_direction(center[i], angle, radius).distance_to(center[i]), radius, 1);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(circle_test) {
    auto coord_Paris = navitia::type::GeographicalCoord(2.3522219000000177, 48.856614);
    auto coord_Pekin = navitia::type::GeographicalCoord(-89.61, 40.5545);
    auto coord_Almost_North = navitia::type::GeographicalCoord(180, 0.5);
    auto coord_North = navitia::type::GeographicalCoord(268, 0);
    auto c_Paris_42 = circle(coord_Paris, 42);
    auto c_Paris_30 = circle(coord_Paris, 30);
    auto c_Pekin_459 = circle(coord_Pekin, 459);
    double d_almost_North_to_North = coord_North.distance_to(coord_Almost_North);
    auto c_Almost_North = circle(coord_Almost_North, d_almost_North_to_North + 20);
    BOOST_CHECK(boost::geometry::within(coord_Paris, c_Paris_42));
    BOOST_CHECK(boost::geometry::within(coord_Paris, c_Paris_30));
    BOOST_CHECK(boost::geometry::within(c_Paris_30, c_Paris_42));
    BOOST_CHECK(boost::geometry::within(coord_North, c_Almost_North));
    BOOST_CHECK(!boost::geometry::within(c_Paris_42, c_Paris_30));
    BOOST_CHECK(!boost::geometry::within(c_Paris_30, c_Pekin_459));
    double r_42 = c_Paris_42.outer()[53].distance_to(coord_Paris);
    double r_30 = c_Paris_30.outer()[12].distance_to(coord_Paris);
    double r_459 = c_Pekin_459.outer()[324].distance_to(coord_Pekin);
    BOOST_CHECK_CLOSE(r_42, 42, 0.5);
    BOOST_CHECK_CLOSE(r_30, 30, 0.5);
    BOOST_CHECK_CLOSE(r_459, 459, 0.5);
}
