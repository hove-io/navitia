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
#include "routing/isochron.h"
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


struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

using namespace navitia::routing;

BOOST_AUTO_TEST_CASE(project_in_direction_test) {
    std::vector<navitia::type::GeographicalCoord> center;
    auto coord_Paris = navitia::type::GeographicalCoord(2.3522219000000177,48.856614);
    auto coord_North = navitia::type::GeographicalCoord(0,90);
    auto coord_Equator = navitia::type::GeographicalCoord(180,0);
    center.push_back(coord_Paris);
    center.push_back(coord_North);
    center.push_back(coord_Equator);
    for (int i = 0; i < 3; i++) {
        for (double angle = - 100; angle < 400; angle++) {
            for (double radius = 10; radius < 100; radius++) {
                BOOST_CHECK_CLOSE (project_in_direction(center[i], angle, radius).distance_to(center[i]), radius, 2);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(circle_test) {
    using coord = navitia::type::GeographicalCoord;
    using poly = boost::geometry::model::polygon<coord>;
    coord coord_Paris {2.3522219000000177, 48.856614};
    coord coord_Pekin = {-89.61, 40.5545};
    coord coord_almost_North = {0, 89};
    coord coord_North = {0, 90};
    coord coord_Equator = {180, 0};
    coord coord_change_day = {180, 18};
    auto c_Paris_42 = circle(coord_Paris, 42);
    auto c_Paris_30 = circle(coord_Paris, 30);
    auto c_Pekin_459 = circle(coord_Pekin, 459);
    double d_North_to_almost_North = coord_North.distance_to(coord_almost_North);
    auto c_almost_North_to_North = circle(coord_almost_North, d_North_to_almost_North + 2000000);
    double d_Equator_to_change_day = coord_Equator.distance_to(coord_change_day);
    auto c_change_day_to_Equator = circle(coord_change_day, d_Equator_to_change_day + 2000000);
    auto within_coord_poly = [](const coord& c, const poly& p) {
        return boost::geometry::within<coord, poly>(c, p);
    };
    BOOST_CHECK(within_coord_poly(coord_Paris, c_Paris_42));
    BOOST_CHECK(within_coord_poly(coord_Paris, c_Paris_30));
    BOOST_CHECK(within_coord_poly(coord_North, c_almost_North_to_North));
    BOOST_CHECK(within_coord_poly(coord_Equator, c_change_day_to_Equator));
    // within is not defined for two polygons in boost 1.49
#if BOOST_VERSION >= 105600
    BOOST_CHECK(boost::geometry::within(c_Paris_30, c_Paris_42));
    BOOST_CHECK(!boost::geometry::within(c_Paris_42, c_Paris_30));
    BOOST_CHECK(!boost::geometry::within(c_Paris_30, c_Pekin_459));
#endif
    double r_42 = c_Paris_42.outer()[53].distance_to(coord_Paris);
    double r_30 = c_Paris_30.outer()[12].distance_to(coord_Paris);
    double r_459 = c_Pekin_459.outer()[324].distance_to(coord_Pekin);
    BOOST_CHECK_CLOSE(r_42, 42, 0.5);
    BOOST_CHECK_CLOSE(r_30, 30, 0.5);
    BOOST_CHECK_CLOSE(r_459, 459, 0.5);
}


BOOST_AUTO_TEST_CASE(build_ischron_test) {
    using coord = navitia::type::GeographicalCoord;
    coord coord_Paris = {2.3522219000000177, 48.856614};
    coord coord_Notre_Dame = {2.35, 48.853};
    coord coord_Rennes = {-1.68333,48.083328};
    coord coord_Pantheon = {2.3461,48.8463};
    coord coord_Concorde = {2.32,48.87};
    coord coord_Luxembourg = {2.339981,48.845404};
    ed::builder b("20120614");
    b.vj("A")("stop1", "08:00"_t)("stop2", "08:10"_t)("stop3", "08:20"_t);
    b.vj("B")("stop4", "08:00"_t)("stop2", "08:30"_t)("stop5", "09:00"_t)("stop6", "23:59:40"_t);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.connection("stop5", "stop5", 120);
    b.connection("stop6", "stop6", 120);
    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.sps["stop1"]->coord = coord_Paris;
    b.sps["stop2"]->coord = coord_Notre_Dame;
    b.sps["stop3"]->coord = coord_Pantheon;
    b.sps["stop4"]->coord = coord_Concorde;
    b.sps["stop5"]->coord = coord_Luxembourg;
    b.sps["stop6"]->coord = coord_Rennes;
    RAPTOR raptor(*b.data);
    navitia::routing::map_stop_point_duration d;
    d.emplace(navitia::routing::SpIdx(*b.sps["stop1"]), navitia::seconds(0));
    raptor.isochrone(d, navitia::DateTimeUtils::set(0, "08:00"_t), navitia::DateTimeUtils::set(0, "09:12"_t));
    double speed = 0.8;
    navitia::type::MultiPolygon isochron = build_isochron(raptor, b.data->pt_data->stop_points, true,
                                                         navitia::DateTimeUtils::set(0, "09:12"_t),
                                                         d, speed, 3600 + 60*12);
#if BOOST_VERSION >= 105600
    BOOST_CHECK(boost::geometry::within(coord_Paris, isochron));
    BOOST_CHECK(boost::geometry::within(coord_Notre_Dame, isochron));
    BOOST_CHECK(boost::geometry::within(coord_Concorde, isochron));
    BOOST_CHECK(boost::geometry::within(coord_Pantheon, isochron));
    BOOST_CHECK(boost::geometry::within(coord_Luxembourg, isochron));
    BOOST_CHECK(!boost::geometry::within(coord_Rennes,isochron));
    BOOST_CHECK(boost::geometry::within(circle(coord_Luxembourg, 8 * 60 * speed - 1), isochron));
#endif
}
