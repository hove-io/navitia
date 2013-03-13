#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "type/type.h"
using namespace navitia::type;
BOOST_AUTO_TEST_CASE(required_false_properties_false) {
    Properties required_properties= 0;
    StopPoint sp;
    sp.properties = 0;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(required_false_properties_true) {
    Properties required_properties = 0;
    StopPoint sp;
    sp.properties = 1;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(required_true_properties_false) {
    Properties required_properties = 1;
    StopPoint sp;
    sp.properties = 0;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), false);
}
BOOST_AUTO_TEST_CASE(required_true_properties_true) {
    Properties required_properties = 1;
    StopPoint sp;
    sp.properties = 1;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(required_false_properties_false_const) {
    Properties required_properties= 0;
    StopPoint tmp_sp;
    tmp_sp.properties = 0;
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(required_false_properties_true_const) {
    Properties required_properties = 0;
    StopPoint tmp_sp;
    tmp_sp.properties = 1;
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(required_true_properties_false_const) {
    Properties required_properties = 1;
    StopPoint tmp_sp;
    tmp_sp.properties = 0;
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), false);
}
BOOST_AUTO_TEST_CASE(required_true_properties_true_const) {
    Properties required_properties = 1;
    StopPoint tmp_sp;
    tmp_sp.properties = 1;
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
