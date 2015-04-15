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
#define BOOST_TEST_MODULE multi_polygon_map_test
#include <boost/test/unit_test.hpp>

#include "type/multi_polygon_map.h"

using namespace navitia::type;

BOOST_AUTO_TEST_CASE(inserting_diamon_and_find) {
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
    BOOST_REQUIRE(!search.empty());
    BOOST_CHECK_EQUAL(std::get<2>(*search.begin()), 42);

    for (const auto& e: map) {}
}
