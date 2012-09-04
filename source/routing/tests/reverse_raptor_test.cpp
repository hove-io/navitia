
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/raptor.h"
#include "naviMake/build_helper.h"

using namespace navitia;
using namespace routing::raptor;

BOOST_AUTO_TEST_CASE(direct){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 7000, 7050)("stop2", 7100,7150);
    b.vj("B")("stop1", 8000, 8050)("stop2", 8100,8150);
    b.vj("C")("stop1", 9000, 9050)("stop2", 9100,9150);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 8200, 0, routing::arriveravant);

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
    b.vj("A")("stop1", 7000, 7050)("stop2", 7100, 7150)("stop3", 7200, 7250);
    b.vj("B")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("C")("stop1", 9000, 9050)("stop2", 9100, 9150)("stop3", 9200, 9250);
    b.vj("H")("stop1", 24*3600 + 9000, 24*3600 + 9050)("stop2", 24*3600 + 9100, 24*3600 + 9150)("stop3", 24*3600 + 9200, 24*3600 + 9250);
    b.vj("D")("stop4", 7000, 7050)("stop2", 7200, 7250)("stop5", 7300, 7350);
    b.vj("E")("stop4", 8000, 8050)("stop2", 8200, 8250)("stop5", 8300, 8350);
    b.vj("F")("stop4", 9000, 9050)("stop2", 9200, 9250)("stop5", 9300, 9350);
    b.vj("G")("stop4", 24*3600 + 9000, 24*3600 + 9050)("stop2", 24*3600 + 9200, 24*3600 + 9250)("stop5", 24*3600 + 9300, 24*3600 + 9350);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 8500, 0, routing::arriveravant);
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[1].said, 1);
    BOOST_CHECK_EQUAL(res.items[2].said, 1);
    BOOST_CHECK_EQUAL(res.items[3].said, 4);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8100);
    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 8250);
    BOOST_CHECK_EQUAL(res.items[3].arrival.hour(), 8300);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[2].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[3].arrival.date(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 5*60);
    b.vj("B")("stop2", 10*60)("stop3", 20*60);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 30*60, 1, routing::arriveravant);


    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[1].said, 1);
    BOOST_CHECK_EQUAL(res.items[2].said, 1);
    BOOST_CHECK_EQUAL(res.items[3].said, 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[3].arrival.date(), 1);
}

BOOST_AUTO_TEST_CASE(validity_pattern){
    navimake::builder b("20120614");
    b.vj("A", "0")("stop1", 8000)("stop2", 8200);
    b.vj("B", "1001")("stop1", 9000)("stop2", 9200);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 9300, 0, routing::arriveravant);
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 9200);

    res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 9300, 2, routing::arriveravant);
    BOOST_REQUIRE_EQUAL(res.items.size(), 0);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_milieu){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    type::Data data;
    data.pt_data = b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;



    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 9300, 0, routing::arriveravant);

    BOOST_REQUIRE_EQUAL(res.items.size(), 5);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[4].arrival.hour(), 9200);
}



BOOST_AUTO_TEST_CASE(marche_a_pied_trcky){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3",8900,8950)("stop2", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 1*60);
    b.connection("stop3", "stop2", 1*60);
    type::Data data;
    data.pt_data = b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;



    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 9300, 0, routing::arriveravant);
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[3].arrival.hour(), 9200);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_pam){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8200);
    b.vj("B")("stop3", 10)("stop4",20);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    type::Data data;
    data.pt_data = b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;


    auto res = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 30, 1, routing::arriveravant);
    std::cout << res << std::endl;
    BOOST_REQUIRE_EQUAL(res.items.size(), 5);
    BOOST_CHECK_EQUAL(res.items[4].said, 3);
    BOOST_CHECK_EQUAL(res.items[4].arrival.hour(), 20);
    BOOST_CHECK_EQUAL(res.items[4].arrival.date(), 1);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8000);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
}

BOOST_AUTO_TEST_CASE(marche_a_pied_debut) {
    navimake::builder b("20120614");

    b.vj("A")("stop1", 8000)("stop20", 8200);
    b.vj("B")("stop2", 30000)("stop3",40000);
    b.vj("B")("stop2", 7900)("stop3",8000);
    b.connection("stop1", "stop2", 10*60);

    type::Data data;
    data.pt_data = b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;


    auto res = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 45000, 0, routing::arriveravant);

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 29400);
    BOOST_CHECK_EQUAL(res.items[2].said, 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 40000);
}

