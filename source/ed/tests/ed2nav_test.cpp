/* Copyright © 2001-2018, Canal TP and/or its affiliates. All rights reserved.

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
#define BOOST_TEST_MODULE ed2nav

#include "utils/logger.h"
#include "ed/data.h"
#include "ed/ed2nav.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};

BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(ed2nav_with_no_param_should_start_without_throwing) {
    const char* argv[] = {"ed2nav_test"};
    BOOST_CHECK_NO_THROW(ed::ed2nav(1, argv));
}

BOOST_AUTO_TEST_CASE(should_throw_on_bad_connection_string) {
    const char* argv[] = {"ed2nav_test", "--connection-string=\"blahblah\""};
    BOOST_CHECK_THROW(ed::ed2nav(2, argv), std::exception);
}

BOOST_AUTO_TEST_CASE(writing_test_ed2nav_should_not_throw) {
    // Cleans previous test binaries
    if (boost::filesystem::exists("ed2nav_test.nav.lz4")) {
        remove("ed2nav_test.nav.lz4");
    }
    if (boost::filesystem::exists("ed2nav_test.nav.lz4.bak")) {
        remove("ed2nav_test.nav.lz4.bak");
    }

    // Checks if the first binary is created
    navitia::type::Data data(0);
    std::string test = "ed2nav_test.nav.lz4";
    bool b = ed::write_data_to_file(test, data);
    BOOST_CHECK(b);

    // Checks if the already existing binary is renamed and the new one is created
    if (boost::filesystem::exists("ed2nav_test.nav.lz4")) {
        b = ed::write_data_to_file(test, data);
        BOOST_CHECK(b);
        BOOST_CHECK(boost::filesystem::exists("ed2nav_test.nav.lz4.bak"));
    }
}
