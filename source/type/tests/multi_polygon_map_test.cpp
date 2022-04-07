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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE multi_polygon_map_test
#include <boost/test/unit_test.hpp>

#include "type/multi_polygon_map.h"
#include "tests/utils_test.h"
#include <boost/geometry.hpp>
#include <boost/range/algorithm/sort.hpp>

using namespace navitia::type;

BOOST_AUTO_TEST_CASE(inserting_diamond_and_find) {
    MultiPolygonMap<int> map;
    MultiPolygon elt;
    boost::geometry::read_wkt("MULTIPOLYGON(((0 3, 3 6, 6 3, 3 0, 0 3)))", elt);
    map.insert(elt, 42);
    auto search = map.find(GeographicalCoord(7, 7));
    BOOST_CHECK(search.empty());
    search = map.find(GeographicalCoord(6, 6));
    BOOST_CHECK(search.empty());
    search = map.find(GeographicalCoord(5, 5));
    BOOST_CHECK(search.empty());
    search = map.find(GeographicalCoord(4, 4));
    BOOST_REQUIRE_EQUAL(search.size(), 1);
    BOOST_CHECK_EQUAL(search.at(0), 42);
}

//       ^ ^
// Lucy / X \  the
// in  < < > > sky
//      \ X /
//       V V
BOOST_AUTO_TEST_CASE(lucy_in_the_sky_with_diamonds) {
    MultiPolygonMap<std::string> map;
    MultiPolygon elt;
    boost::geometry::read_wkt("MULTIPOLYGON(((0 3, 3 6, 6 3, 3 0, 0 3)))", elt);
    map.insert(elt, "Lucy");
    boost::geometry::read_wkt("MULTIPOLYGON(((0 3, 3 6, 6 3, 3 0, 0 3)))", elt);
    map.insert(elt, "in");
    boost::geometry::read_wkt("MULTIPOLYGON(((4 3, 7 6, 10 3, 7 0, 4 3)))", elt);
    map.insert(elt, "the");
    boost::geometry::read_wkt("MULTIPOLYGON(((4 3, 7 6, 10 3, 7 0, 4 3)))", elt);
    map.insert(elt, "sky");

    auto search = map.find(GeographicalCoord(7, 7));
    BOOST_CHECK(search.empty());
    search = map.find(GeographicalCoord(1, 5));
    BOOST_CHECK(search.empty());
    search = map.find(GeographicalCoord(5, 5));
    BOOST_CHECK(search.empty());
    search = map.find(GeographicalCoord(5, 9));
    BOOST_CHECK(search.empty());
    search = map.find(GeographicalCoord(1, 3));
    boost::sort(search);
    BOOST_CHECK_EQUAL(search, (std::vector<std::string>{"Lucy", "in"}));
    search = map.find(GeographicalCoord(5, 3));
    boost::sort(search);
    BOOST_CHECK_EQUAL(search, (std::vector<std::string>{"Lucy", "in", "sky", "the"}));
    search = map.find(GeographicalCoord(6, 2));
    boost::sort(search);
    BOOST_CHECK_EQUAL(search, (std::vector<std::string>{"sky", "the"}));
}
