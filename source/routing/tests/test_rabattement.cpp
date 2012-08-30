
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/raptor.h"
#include "naviMake/build_helper.h"

using namespace navitia;
using namespace routing::raptor;


BOOST_AUTO_TEST_CASE(direct){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 7000, 7050)("stop2", 7100, 7950);
    b.vj("B")("stop1", 8000, 8050)("stop2", 8100, 8150);
    b.vj("C")("stop1", 9000, 9050)("stop2", 9100, 8950);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute_rabattement(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0);


    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[1].said, 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8100);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);

}

BOOST_AUTO_TEST_CASE(change){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 10, 15)("stop2", 20,25);
    b.vj("B")("stop1", 20, 25)("stop2", 30,35);
    b.vj("C")("stop1", 30, 35)("stop2", 40,45);

    b.vj("D")("stop2", 260, 265)("stop3", 270,275);
    b.vj("D")("stop2", 270, 275)("stop3", 280,285);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    auto res = raptor.compute_rabattement(d.stop_areas[1].idx, d.stop_areas[0].idx, 8200, 0);

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[1].said, 1);
    BOOST_CHECK_EQUAL(res.items[2].said, 1);
    BOOST_CHECK_EQUAL(res.items[3].said, 4);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 35);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 40);
    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 260);
    BOOST_CHECK_EQUAL(res.items[3].arrival.hour(), 275);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[3].arrival.date(), 0);
}
