#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "network/time_dependent.h"
#include "naviMake/build_helper.h"

using namespace navitia;
using namespace routing;

BOOST_AUTO_TEST_CASE(direct){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8100);
    type::PT_Data d = b.build();
    TimeDependent tp(d);
    tp.build_graph();
    auto res = tp.compute(d.stop_areas[0], d.stop_areas[1], 7900, 0);

    BOOST_REQUIRE_EQUAL(res.size(), 2);
    BOOST_CHECK_EQUAL(res[0].stop_point_name, "stop1");
    BOOST_CHECK_EQUAL(res[1].stop_point_name, "stop2");

}

BOOST_AUTO_TEST_CASE(change){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8100)("stop3", 8200);
    b.vj("B")("stop4", 8000)("stop2", 8200)("stop5", 8300);
    type::PT_Data d = b.build();
    TimeDependent tp(d);
    tp.build_graph();
    auto res = tp.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 4);
    BOOST_CHECK_EQUAL(res[0].stop_point_name, "stop1");
    BOOST_CHECK_EQUAL(res[1].stop_point_name, "stop2");
    BOOST_CHECK_EQUAL(res[2].stop_point_name, "stop2");
    BOOST_CHECK_EQUAL(res[3].stop_point_name, "stop5");
}

BOOST_AUTO_TEST_CASE(passe_minuit){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 10*60);
    b.vj("B")("stop2", 5*60)("stop3", 20*60);
    type::PT_Data d = b.build();
    TimeDependent tp(d);
    tp.build_graph();
    auto res = tp.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 4);
    BOOST_CHECK_EQUAL(res[0].stop_point_name, "stop1");
    BOOST_CHECK_EQUAL(res[1].stop_point_name, "stop2");
    BOOST_CHECK_EQUAL(res[2].stop_point_name, "stop2");
    BOOST_CHECK_EQUAL(res[3].stop_point_name, "stop3");
    BOOST_CHECK_EQUAL(res[0].day, 0);
    BOOST_CHECK_EQUAL(res[3].day, 1);
}

BOOST_AUTO_TEST_CASE(validity_pattern){
    navimake::builder b("20120614");
    b.vj("A", "0")("stop1", 8000)("stop2", 8200);
    b.vj("B", "1")("stop1", 9000)("stop2", 9200);
    type::PT_Data d = b.build();
    TimeDependent tp(d);
    tp.build_graph();
    auto res = tp.compute(d.stop_areas[0], d.stop_areas[1], 7900, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 2);
    BOOST_CHECK_EQUAL(res[1].time, 9200);

    res = tp.compute(d.stop_areas[0], d.stop_areas[1], 7900, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 0);
}
