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
#define BOOST_TEST_MODULE data_manager_test
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "kraken/data_manager.h"
#include "type/data.h"

static const std::string fake_data_file = "fake_data.nav.lz4";
static const std::string fake_disruption_path = "fake_disruption_path";

// We create a empty data with lz4 format in current directory.
#define CREATE_FAKE_DATA(fake_file_name) \
    navitia::type::Data data(0);         \
    data.save(fake_data_file);           \

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

// test sequence :
// 1. load good data
// 2. reload good data
// 3. reload wrong data
BOOST_AUTO_TEST_CASE(load_success) {

    CREATE_FAKE_DATA(fake_data_file)

    DataManager<navitia::type::Data> data_manager;
    BOOST_CHECK(data_manager.get_data());
    auto first_data = data_manager.get_data();

    // Same pointer
    BOOST_CHECK_EQUAL(first_data, data_manager.get_data());

    // real data : Loading successed
    std::string fake_data_path = complete_path(fake_data_file);
    BOOST_CHECK(data_manager.load(fake_data_path));

    // Load success, so the internal shared pointer changes.
    auto second_data = data_manager.get_data();
    BOOST_CHECK_NE(first_data, second_data);

    // Same pointer
    BOOST_CHECK_EQUAL(second_data, data_manager.get_data());

    // When we reload a new data.
    // real data : Loading successed
    BOOST_CHECK(data_manager.load(fake_data_path));

    // Load success, so the internal shared pointer changes.
    auto third_data = data_manager.get_data();
    BOOST_CHECK_NE(second_data, third_data);

    // Same pointer
    BOOST_CHECK_EQUAL(third_data, data_manager.get_data());

    // When we reload a new data.
    // Fake data : Loading failed
    BOOST_CHECK(!data_manager.load("wrong path"));

    // Load failed, so the internal shared pointer no changes.
    // Data has not changed.
    auto fourth_data = data_manager.get_data();
    BOOST_CHECK_EQUAL(third_data, fourth_data);

    // Clean fake file
    boost::filesystem::remove(fake_data_path);
}

BOOST_AUTO_TEST_CASE(load_failed) {

    CREATE_FAKE_DATA(fake_data_file)

    DataManager<navitia::type::Data> data_manager;
    BOOST_CHECK(data_manager.get_data());
    auto first_data = data_manager.get_data();

    // Same pointer
    BOOST_CHECK_EQUAL(first_data, data_manager.get_data());

    // Fake data : Loading failed
    BOOST_CHECK(!data_manager.load("wrong path"));

    // Load failed, so the internal shared pointer no changes.
    // Data has not changed.
    BOOST_CHECK_EQUAL(first_data, data_manager.get_data());
}
