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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "ed/connectors/conv_coord.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_convcoord
#include <boost/test/unit_test.hpp>

class CoordParams {
public:
    ed::connectors::Projection lambert2;
    ed::connectors::Projection wgs84;
    CoordParams() : lambert2("Lambert 2 étendu", "27572", false), wgs84("wgs84", "4326", true) {}
};

BOOST_FIXTURE_TEST_CASE(lambertII2Wgs84, CoordParams) {
    navitia::type::GeographicalCoord coord_lambert2(537482.27, 2381791.96);
    ed::connectors::ConvCoord conv_coord(lambert2, wgs84);
    navitia::type::GeographicalCoord coord_wgs84 = conv_coord.convert_to(coord_lambert2);

    BOOST_REQUIRE_EQUAL(coord_wgs84 == navitia::type::GeographicalCoord(1.491911486572199, 48.431961616400599), true);
}

BOOST_FIXTURE_TEST_CASE(Wgs842lambertII, CoordParams) {
    navitia::type::GeographicalCoord coord_wgs84(1.491911486572199, 48.431961616400599);
    ed::connectors::ConvCoord conv_coord(wgs84, lambert2);
    navitia::type::GeographicalCoord coord_lambert2 = conv_coord.convert_to(coord_wgs84);

    BOOST_REQUIRE_EQUAL((coord_lambert2.lon() - 537482.27) < 0.1, true);
    BOOST_REQUIRE_EQUAL((coord_lambert2.lon() - 2381791.96) < 0.1, true);
}

BOOST_FIXTURE_TEST_CASE(copy_test, CoordParams) {
    // test if there is no double delete, or wrong allocation with copy (since proj4 is in raw C)
    navitia::type::GeographicalCoord coord_lambert2(537482.27, 2381791.96);
    ed::connectors::Projection lambert_bis(lambert2);
    ed::connectors::ConvCoord conv_coord(lambert_bis, wgs84);
    navitia::type::GeographicalCoord coord_wgs84 = conv_coord.convert_to(coord_lambert2);

    BOOST_REQUIRE_EQUAL(coord_wgs84 == navitia::type::GeographicalCoord(1.491911486572199, 48.431961616400599), true);
}

BOOST_FIXTURE_TEST_CASE(default_constructor_test, CoordParams) {
    navitia::type::GeographicalCoord coord_lambert2(537482.27, 2381791.96);
    ed::connectors::ConvCoord conv_coord(lambert2);
    navitia::type::GeographicalCoord coord_wgs84 = conv_coord.convert_to(coord_lambert2);

    BOOST_REQUIRE_EQUAL(coord_wgs84 == navitia::type::GeographicalCoord(1.491911486572199, 48.431961616400599), true);
}

BOOST_FIXTURE_TEST_CASE(construct_with_copy, CoordParams) {
    ed::connectors::ConvCoord conv_coord(lambert2);
    ed::connectors::ConvCoord new_conv_coord = ed::connectors::ConvCoord(conv_coord);

    BOOST_REQUIRE_EQUAL(new_conv_coord.origin.definition == conv_coord.origin.definition, true);
    BOOST_REQUIRE_EQUAL(new_conv_coord.destination.definition == conv_coord.destination.definition, true);
#ifdef PROJ_API_VERSION_MAJOR_6
    BOOST_REQUIRE_EQUAL(new_conv_coord.origin.definition == "EPSG:27572", true);
    BOOST_REQUIRE_EQUAL(new_conv_coord.destination.definition == "EPSG:4326", true);
    BOOST_REQUIRE_EQUAL(new_conv_coord.p_for_gis != conv_coord.p_for_gis, true);
#else
    BOOST_REQUIRE_EQUAL(new_conv_coord.origin.definition == "+init=epsg:27572", true);
    BOOST_REQUIRE_EQUAL(new_conv_coord.destination.definition == "+init=epsg:4326", true);
#endif
}

BOOST_FIXTURE_TEST_CASE(operator_equal, CoordParams) {
    ed::connectors::ConvCoord conv_coord(lambert2);
    ed::connectors::ConvCoord new_conv_coord = conv_coord;

    BOOST_REQUIRE_EQUAL(new_conv_coord.origin.definition == conv_coord.origin.definition, true);
    BOOST_REQUIRE_EQUAL(new_conv_coord.destination.definition == conv_coord.destination.definition, true);
#ifdef PROJ_API_VERSION_MAJOR_6
    BOOST_REQUIRE_EQUAL(new_conv_coord.origin.definition == "EPSG:27572", true);
    BOOST_REQUIRE_EQUAL(new_conv_coord.destination.definition == "EPSG:4326", true);
    BOOST_REQUIRE_EQUAL(new_conv_coord.p_for_gis != conv_coord.p_for_gis, true);
#else
    BOOST_REQUIRE_EQUAL(new_conv_coord.origin.definition == "+init=epsg:27572", true);
    BOOST_REQUIRE_EQUAL(new_conv_coord.destination.definition == "+init=epsg:4326", true);
#endif
}
