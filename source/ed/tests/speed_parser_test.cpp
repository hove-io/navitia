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

#include "boost/optional/optional.hpp"
#include "ed/data.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_speed_parser
#include <boost/test/unit_test.hpp>
#include "ed/connectors/speed_parser.h"
#include "tests/utils_test.h"
#include <boost/optional/optional_io.hpp>
struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(empty_speed_parser_test) {
    ed::connectors::SpeedsParser parser;
    BOOST_CHECK_EQUAL(parser.get_speed("primary", boost::none), boost::none);
    BOOST_CHECK_EQUAL(parser.get_speed("primary", boost::make_optional(std::string("50"))), boost::none);
}

BOOST_AUTO_TEST_CASE(france_speed_parser_test) {
    auto parser = ed::connectors::SpeedsParser::defaults();
    BOOST_CHECK_CLOSE(parser.get_speed("primary", boost::none).get(), 17.77, 0.1);
    BOOST_CHECK_CLOSE(parser.get_speed("primary", boost::make_optional(std::string("50"))).get(), 11.11, 0.1);
    BOOST_CHECK_CLOSE(parser.get_speed("primary", boost::make_optional(std::string("fr:urban"))).get(), 11.11, 0.1);

    BOOST_CHECK_CLOSE(parser.get_speed("motorway", boost::none).get(), 32.5, 0.1);
    BOOST_CHECK_CLOSE(parser.get_speed("motorway", boost::make_optional(std::string("70"))).get(), 17.5, 0.1);

    // not existing highway
    BOOST_CHECK_EQUAL(parser.get_speed("noway", boost::none), boost::none);
    // no factor applied but we take the maxspeed into account and applied default speed factor
    BOOST_CHECK_CLOSE(parser.get_speed("noway", boost::make_optional(std::string("70"))).get(), 9.72, 0.1);
}
