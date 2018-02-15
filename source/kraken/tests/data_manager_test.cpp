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
#include <boost/optional.hpp>

#include "kraken/data_manager.h"
#include <atomic>

namespace navitia { namespace type {

// Wrong data version
struct wrong_version : public navitia::exception {
    wrong_version(const std::string& msg): navitia::exception(msg){}
    wrong_version(const wrong_version&) = default;
    wrong_version& operator=(const wrong_version&) = default;
    virtual ~wrong_version() noexcept;
};

// Data loading exceptions handler
struct data_loading_error : public navitia::exception {
    data_loading_error(const std::string& msg): navitia::exception(msg){}
    data_loading_error(const data_loading_error&) = default;
    data_loading_error& operator=(const data_loading_error&) = default;
    virtual ~data_loading_error() noexcept;
};

// Disruptions exceptions handler. Broken connection
struct disruptions_broken_connection : public navitia::exception {
    disruptions_broken_connection(const std::string& msg): navitia::exception(msg){}
    disruptions_broken_connection(const disruptions_broken_connection&) = default;
    disruptions_broken_connection& operator=(const disruptions_broken_connection&) = default;
    virtual ~disruptions_broken_connection() noexcept;
};

// Disruptions exceptions handler. Loading error
struct disruptions_loading_error : public navitia::exception {
    disruptions_loading_error(const std::string& msg): navitia::exception(msg){}
    disruptions_loading_error(const disruptions_loading_error&) = default;
    disruptions_loading_error& operator=(const disruptions_loading_error&) = default;
    virtual ~disruptions_loading_error() noexcept;
};

// Raptor building exceptions handler
struct raptor_building_error : public navitia::exception {
    raptor_building_error(const std::string& msg): navitia::exception(msg){}
    raptor_building_error(const raptor_building_error&) = default;
    raptor_building_error& operator=(const raptor_building_error&) = default;
    virtual ~raptor_building_error() noexcept;
};

//mock of navitia::type::Data class
class Data{
    public:
        void load_nav(const std::string& filename);
        void load_disruptions(const std::string& database,
                          const std::vector<std::string>& contributors = {});
        void build_raptor(size_t cache_size = 10);
        mutable std::atomic<bool> loading;
        mutable std::atomic<bool> is_connected_to_rabbitmq;
        static bool load_status;
        static bool last_load;
        static bool destructor_called;
        size_t data_identifier;

        Data(size_t data_identifier=0):
            data_identifier(data_identifier)
        {is_connected_to_rabbitmq = false;}

        ~Data(){Data::destructor_called = true;}
};
bool Data::load_status = true;
bool Data::last_load = true;
bool Data::destructor_called = false;

}} //namespace navitia::type

struct fixture{
    fixture(){
        navitia::type::Data::load_status = true;
        navitia::type::Data::destructor_called = false;
    }
};

BOOST_FIXTURE_TEST_SUITE(s, fixture)

BOOST_AUTO_TEST_CASE(get_data) {
    DataManager<navitia::type::Data> data_manager;
    auto data = data_manager.get_data();
    BOOST_REQUIRE(data);
    BOOST_CHECK_EQUAL(navitia::type::Data::destructor_called, false);
}

BOOST_AUTO_TEST_CASE(load_failed) {
    DataManager<navitia::type::Data> data_manager;
    BOOST_CHECK(data_manager.get_data());
    auto first_data = data_manager.get_data();

    // Same pointer
    BOOST_CHECK_EQUAL(first_data, data_manager.get_data());

    // Fake data : Loading failed
    BOOST_CHECK(!data_manager.load("fake path"));

    // Load failed, so the internal shared pointer no change.
    // Data has not changed.
    BOOST_CHECK_EQUAL(first_data, data_manager.get_data());
}

BOOST_AUTO_TEST_CASE(destructor_called) {
    DataManager<navitia::type::Data> data_manager;
    {
        auto first_data = data_manager.get_data();
        BOOST_CHECK(!data_manager.load("fake path"));
        BOOST_CHECK_EQUAL(first_data, data_manager.get_data());
    }
    // type::Data destructor is called because when load function is called,
    // new shared pointer is not used.
    BOOST_CHECK_EQUAL(navitia::type::Data::destructor_called, true);
    BOOST_CHECK(data_manager.get_data());
}

BOOST_AUTO_TEST_CASE(destructor_not_called) {
    DataManager<navitia::type::Data> data_manager;
    {
        auto first_data = data_manager.get_data();
        BOOST_CHECK_EQUAL(first_data, data_manager.get_data());
    }
    // type::Data destructor is called because when load function is called,
    // new shared pointer is not used.
    BOOST_CHECK_EQUAL(navitia::type::Data::destructor_called, false);
    BOOST_CHECK(data_manager.get_data());
}

BOOST_AUTO_TEST_SUITE_END()
