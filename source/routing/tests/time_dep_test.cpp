#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/time_dependent.h"
#include "naviMake/build_helper.h"

using namespace navitia;
using namespace routing::timedependent;

BOOST_AUTO_TEST_CASE(direct){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    type::Data data;
    b.build(data.pt_data);
    TimeDependent tp(data);
    auto res = tp.compute(data.pt_data.stop_areas[0].idx, data.pt_data.stop_areas[1].idx, 6900, 0)[0];

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8100);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
}

BOOST_AUTO_TEST_CASE(change){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8100)("stop3", 8200);
    b.vj("B")("stop4", 8000)("stop2", 8200)("stop5", 8300);
    type::Data data;
    b.build(data.pt_data);
    TimeDependent tp(data);

    auto res = tp.compute(data.pt_data.stop_areas[0].idx, data.pt_data.stop_areas[4].idx, 7900, 0)[0];
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 4);
}

BOOST_AUTO_TEST_CASE(passe_minuit){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 5*60);
    b.vj("B")("stop2", 10*60)("stop3", 20*60);
    type::Data data;
    b.build(data.pt_data);
    TimeDependent tp(data);

    auto res = tp.compute(data.pt_data.stop_areas[0].idx, data.pt_data.stop_areas[2].idx, 22*3600, 0)[0];
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 1);
}

BOOST_AUTO_TEST_CASE(passe_minuit_fail){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 10*60);
    b.vj("B")("stop2", 5*60)("stop3", 20*60);
    type::Data data;
    b.build(data.pt_data);
    TimeDependent tp(data);

    auto res = tp.compute(data.pt_data.stop_areas[0].idx, data.pt_data.stop_areas[2].idx, 22*3600, 0)[0];
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 2);
}

BOOST_AUTO_TEST_CASE(validity_pattern){
    navimake::builder b("20120614");
    b.vj("A", "0")("stop1", 8000)("stop2", 8200);
    b.vj("B", "1")("stop1", 9000)("stop2", 9200);
    type::Data data;
    b.build(data.pt_data);
    TimeDependent tp(data);

    auto res = tp.compute(data.pt_data.stop_areas[0].idx, data.pt_data.stop_areas[1].idx, 7900, 0)[0];
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9200);

    res = tp.compute(data.pt_data.stop_areas[0].idx, data.pt_data.stop_areas[1].idx, 7900, 1)[0];
    BOOST_REQUIRE_EQUAL(res.items.size(), 0);
}
