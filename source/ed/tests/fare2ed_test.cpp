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
#define BOOST_TEST_MODULE fare2ed

#include <boost/test/unit_test.hpp>
#include "ed/fare2ed.h"
#include "utils/logger.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};

BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(fare2ed_with_no_param_should_start_without_throwing) {
    const char* argv[] = {"fare2ed_test"};
    BOOST_CHECK_NO_THROW(ed::fare2ed(1, argv));
}

BOOST_AUTO_TEST_CASE(should_throw_on_bad_connection_string) {
    const char* argv[] = {"fare2ed_test", "--connection-string=\"blahblah\""};
    BOOST_CHECK_THROW(ed::fare2ed(2, argv), std::exception);
}
