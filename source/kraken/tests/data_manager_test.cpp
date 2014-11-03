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

#include "kraken/data_manager.h"
#include <atomic>

//mock of navitia::type::Data class
class Data{
    public:
        bool load(const std::string&){return load_status;}
        mutable std::atomic<bool> is_connected_to_rabbitmq;
        static bool load_status;
        static bool destructor_called;

        Data(){is_connected_to_rabbitmq = false;}

        ~Data(){Data::destructor_called = true;}
};
bool Data::load_status = true;
bool Data::destructor_called = false;

struct fixture{
    fixture(){
        Data::load_status = true;
        Data::destructor_called = false;
    }
};

BOOST_FIXTURE_TEST_SUITE(s, fixture)

BOOST_AUTO_TEST_CASE(get_data){
    DataManager<Data> data_manager;
    auto data = data_manager.get_data();
    BOOST_REQUIRE(data);
    BOOST_CHECK_EQUAL(Data::destructor_called, false);
}

BOOST_AUTO_TEST_CASE(load_success){
    DataManager<Data> data_manager;
    auto first_data = data_manager.get_data();
    BOOST_CHECK_EQUAL(first_data, data_manager.get_data());
    BOOST_CHECK(data_manager.load(""));
    auto second_data = data_manager.get_data();
    BOOST_CHECK_NE(first_data, second_data);
    BOOST_CHECK_EQUAL(Data::destructor_called, false);
}

BOOST_AUTO_TEST_CASE(load_fail){
    DataManager<Data> data_manager;
    auto first_data = data_manager.get_data();
    BOOST_CHECK_EQUAL(first_data, data_manager.get_data());
    Data::load_status = false;
    BOOST_CHECK(! data_manager.load(""));
    Data::load_status = true;
    auto second_data = data_manager.get_data();
    BOOST_CHECK_EQUAL(first_data, second_data);
}

BOOST_AUTO_TEST_CASE(destructor_called){
    DataManager<Data> data_manager;
    {
        auto first_data = data_manager.get_data();
        BOOST_CHECK_EQUAL(first_data, data_manager.get_data());
        BOOST_CHECK(data_manager.load(""));
        auto second_data = data_manager.get_data();
        BOOST_CHECK_NE(first_data, second_data);
        BOOST_CHECK_EQUAL(Data::destructor_called, false);
        first_data = boost::shared_ptr<Data>();
    }
    BOOST_CHECK_EQUAL(Data::destructor_called, true);
    BOOST_CHECK(data_manager.get_data());
}

BOOST_AUTO_TEST_SUITE_END()
