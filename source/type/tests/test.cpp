#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include "type/type.h"

using namespace navitia::type;

BOOST_AUTO_TEST_CASE(uri_sa) {
    std::string uri("stop_area:moo:ext_code");
    EntryPoint ep(uri);

    BOOST_CHECK(ep.type == Type_e::eStopArea);

    // On garde le prefixe de type
    BOOST_CHECK_EQUAL(ep.external_code, "stop_area:moo:ext_code");
}

BOOST_AUTO_TEST_CASE(uri_coord) {
    std::string uri("coord:2.2:4.42");
    EntryPoint ep(uri);

    BOOST_CHECK(ep.type == Type_e::eCoord);
    BOOST_CHECK_CLOSE(ep.coordinates.x, 2.2, 1e-6);
    BOOST_CHECK_CLOSE(ep.coordinates.y, 4.42, 1e-6);

    EntryPoint ep2("coord:2.1");
    BOOST_CHECK_CLOSE(ep2.coordinates.x, 0, 1e-6);
    BOOST_CHECK_CLOSE(ep2.coordinates.y, 0, 1e-6);

    EntryPoint ep3("coord:2.1:bli");
    BOOST_CHECK_CLOSE(ep3.coordinates.x, 0, 1e-6);
    BOOST_CHECK_CLOSE(ep3.coordinates.y, 0, 1e-6);
}
