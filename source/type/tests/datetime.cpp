#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/routing.h"

using namespace navitia;

BOOST_AUTO_TEST_CASE(ctor){
    DateTime d = DateTimeUtils::set(1,2);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 1);
}

BOOST_AUTO_TEST_CASE(date_update){
    DateTime d = DateTimeUtils::set(10, 10);
    DateTimeUtils::update(d, 100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 10);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 100);

    DateTimeUtils::update(d,50);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 11);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 50);
}

BOOST_AUTO_TEST_CASE(increment){
    DateTime d = DateTimeUtils::set(0, 0);
    d += 10;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 10);
}

BOOST_AUTO_TEST_CASE(passe_minuit){
    DateTime d = DateTimeUtils::set(10, 24*3600 - 1);
    d += 10;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 11);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 9);
}


BOOST_AUTO_TEST_CASE(passe_minuit_update){
    DateTime d = DateTimeUtils::set(10, 23*3600);
    d += 3600 + 10;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 11);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 10);
}

BOOST_AUTO_TEST_CASE(passe_minuit_update_reverse){
    DateTime d = DateTimeUtils::set(1, 300);
    DateTimeUtils::update(d, 23*3600, false);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 23*3600);
}

BOOST_AUTO_TEST_CASE(decrement){
    DateTime d = DateTimeUtils::set(2, 100);
    d -= 50;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 50);
    d -= 100;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 24*3600 - 50);
}

BOOST_AUTO_TEST_CASE(init2) {
    DateTime d = DateTimeUtils::set(1, 24*3600 + 10);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 10);
}

BOOST_AUTO_TEST_CASE(moins) {
    DateTime d1 = DateTimeUtils::set(10, 8*3600);
    DateTime d2 = DateTimeUtils::set(10, 9*3600);
    DateTime d3 = DateTimeUtils::set(11, 5*3600);
    uint32_t t1 = d2 - d1;
    uint32_t t2 = d3 - d1;
    BOOST_CHECK_EQUAL(t1, 3600);
    BOOST_CHECK_EQUAL(t2, 86400 - 3 * 3600);
}

BOOST_AUTO_TEST_CASE(freq_stop_time_validation){
    type::StopTime st;
    st.start_time = 8000;
    st.end_time = 9000;
    st.properties.set(type::StopTime::IS_FREQUENCY);
    BOOST_CHECK(st.valid_hour(7999, true));
    BOOST_CHECK(st.valid_hour(8000, true));
    BOOST_CHECK(st.valid_hour(8500, true));
    BOOST_CHECK(st.valid_hour(9000, true));
    BOOST_CHECK(!st.valid_hour(9001, true));
    BOOST_CHECK(!st.valid_hour(7999, false));
    BOOST_CHECK(st.valid_hour(8000, false));
    BOOST_CHECK(st.valid_hour(8500, false));
    BOOST_CHECK(st.valid_hour(9000, false));
    BOOST_CHECK(st.valid_hour(9001, false));

    st.start_time = 23 * 3600;
    st.end_time = 26 * 3600;
    BOOST_CHECK(st.valid_hour(3600, true));
    BOOST_CHECK(st.valid_hour(22*3600, true));
    BOOST_CHECK(!st.valid_hour(22*3600 + 24*3600, true));
    BOOST_CHECK(st.valid_hour(23*3600, true));
    BOOST_CHECK(st.valid_hour(25*3600, true));
    BOOST_CHECK(!st.valid_hour(27*3600, true));

    BOOST_CHECK(!st.valid_hour(3600, false));
    BOOST_CHECK(!st.valid_hour(22*3600, false));
    BOOST_CHECK(st.valid_hour(22*3600 + 24*3600, false));
    BOOST_CHECK(st.valid_hour(23*3600, false));
    BOOST_CHECK(st.valid_hour(25*3600, false));
    BOOST_CHECK(st.valid_hour(27*3600, false));
}


BOOST_AUTO_TEST_CASE(weekday_conversion) {
    boost::gregorian::date today(2014, 03, 5);
    BOOST_CHECK_EQUAL(navitia::get_weekday(today), navitia::Wednesday);
    BOOST_CHECK_EQUAL(today.day_of_week(), boost::date_time::Wednesday);

    {
        boost::gregorian::date after(2014, 03, 6);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Thursday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Thursday);
    }
    {
        boost::gregorian::date after(2014, 03, 7);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Friday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Friday);
    }
    {
        boost::gregorian::date after(2014, 3, 8);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Saturday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Saturday);
    }
    {
        boost::gregorian::date after(2014, 3, 9);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Sunday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Sunday);
    }
    {
        boost::gregorian::date after(2014, 3, 10);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Monday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Monday);
    }
    {
        boost::gregorian::date after(2014, 3, 11);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Tuesday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Tuesday);
    }

}
