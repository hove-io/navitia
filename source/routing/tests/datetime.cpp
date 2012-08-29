#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/routing.h"

using namespace navitia::routing;

BOOST_AUTO_TEST_CASE(ctor){
    DateTime d(1,2);
    BOOST_CHECK_EQUAL(d.hour(), 2);
    BOOST_CHECK_EQUAL(d.date(), 1);
}

BOOST_AUTO_TEST_CASE(date_incr_decr){
    DateTime d(1,2);
    d.date_increment();
    BOOST_CHECK_EQUAL(d.date(), 2);
    d.date_decrement();
    BOOST_CHECK_EQUAL(d.date(), 1);
}

BOOST_AUTO_TEST_CASE(date_update){
    DateTime d(10, 10);
    d.update(100);
    BOOST_CHECK_EQUAL(d.date(), 10);
    BOOST_CHECK_EQUAL(d.hour(), 100);

    d.update(50);
    BOOST_CHECK_EQUAL(d.date(), 11);
    BOOST_CHECK_EQUAL(d.hour(), 50);
}

BOOST_AUTO_TEST_CASE(increment){
    DateTime d(0, 0);
    d.increment(10);
    BOOST_CHECK_EQUAL(d.date(), 0);
    BOOST_CHECK_EQUAL(d.hour(), 10);
}

BOOST_AUTO_TEST_CASE(passe_minuit){
    DateTime d(10, 24*3600 - 1);

    d.increment(10);
    BOOST_CHECK_EQUAL(d.date(), 11);
    BOOST_CHECK_EQUAL(d.hour(), 9);
}

BOOST_AUTO_TEST_CASE(decrement){
    DateTime d(2, 100);
    d.decrement(50);
    BOOST_CHECK_EQUAL(d.date(), 2);
    BOOST_CHECK_EQUAL(d.hour(), 50);
    d.decrement(100);
    BOOST_CHECK_EQUAL(d.date(), 1);
    BOOST_CHECK_EQUAL(d.hour(), 24*3600 - 50);
}
