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
#define BOOST_TEST_MODULE data_test

// Boost
#include <boost/test/unit_test.hpp>

// Std
#include <string>
#include <stdio.h>
#include <unistd.h>  // getcwd() definition

// Data to test
#include "type/data.h"

using namespace navitia;

static const std::string fake_data_file = "fake_data.nav.lz4";
static const std::string fake_disruption_path = "fake_disruption_path";

// Absolute path
static std::string complete_path(const std::string file_name){
    char buf[256];
    if (getcwd(buf, sizeof(buf))) {
        return std::string(buf) + "/" + file_name;
    } else {
        std::perror("getcwd");
        return "";
    }
}

BOOST_AUTO_TEST_CASE(load_data) {

    navitia::type::Data data(0);
    data.save(fake_data_file);

    // load .nav
    bool failed = false;
    BOOST_CHECK_EQUAL(data.last_load_succeeded, false);
    try {
        data.load_nav(complete_path(fake_data_file));
    } catch(const navitia::data::data_loading_error&) {
        failed = true;
    }
    BOOST_CHECK_EQUAL(failed, false);
    BOOST_CHECK_EQUAL(data.last_load_succeeded, true);
    try {
        data.load_nav("wrong_path");
    } catch(const navitia::data::data_loading_error&) {
        failed = true;
    }
    BOOST_CHECK_EQUAL(failed, true);
    BOOST_CHECK_EQUAL(data.last_load_succeeded, true);
}

BOOST_AUTO_TEST_CASE(load_disruptions_fail) {
    navitia::type::Data data(0);

    // load disruptions
    bool failed = false;
    try {
        data.load_disruptions(fake_disruption_path);
    } catch(const navitia::data::disruptions_broken_connection&) {
        failed = true;
    }
    BOOST_CHECK_EQUAL(failed, true);
}
