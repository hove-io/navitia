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
#define BOOST_TEST_MODULE poi2ed

#include "ed/connectors/poi_parser.h"

#include <boost/test/unit_test.hpp>
#include "utils/logger.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};

BOOST_GLOBAL_FIXTURE(logger_initialized);

using namespace ed::connectors;

namespace {
class ArgsFixture {
public:
    ArgsFixture() {
        auto& master = boost::unit_test::framework::master_test_suite();

        BOOST_REQUIRE_MESSAGE(master.argc > 1,
                              "Missing parameter - The test needs a path to a directory containing your POI's files");

        poi_dir_path = master.argv[1];
    }

    std::string poi_dir_path;
};
}  // namespace

BOOST_FIXTURE_TEST_CASE(poi_parser_should_create_well_formed_poi_uri, ArgsFixture) {
    PoiParser parser(poi_dir_path);

    BOOST_REQUIRE_NO_THROW(parser.fill());

    BOOST_CHECK_EQUAL(parser.data.pois["ADM44_100"].uri, "poi:ADM44_100");
    BOOST_CHECK_EQUAL(parser.data.pois["CULT44_30021"].uri, "poi:CULT44_30021");
    BOOST_CHECK_EQUAL(parser.data.pois["PAIHABIT0000000028850973"].uri, "poi:PAIHABIT0000000028850973");
}
