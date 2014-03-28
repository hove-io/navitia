#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE data_manager_test
#include <boost/test/unit_test.hpp>

#include "kraken/data_manager.h"

//mock of navitia::type::Data class
class Data{
    public:
        bool load(const std::string&){return load_status;}
        static bool load_status;
        static bool destructor_called;

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
        first_data = nullptr;
    }
    BOOST_CHECK_EQUAL(Data::destructor_called, true);
    BOOST_CHECK(data_manager.get_data());
}

BOOST_AUTO_TEST_SUITE_END()
