
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/raptor.h"
#include "naviMake/build_helper.h"

using namespace navitia;
using namespace routing::raptor;


BOOST_AUTO_TEST_CASE(direct){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 9100, 9150);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 9200, 0, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9100);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 9200, 0, routing::DateTime(0,8050-1000), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9100);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 9200, 0, routing::DateTime(0,(8050)), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9100);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 9200, 0, routing::DateTime(0,(8050)+1), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(change){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250)("stop3", 8400, 8450);
    b.vj("B")("stop4", 9000, 9050)("stop2", 9500, 9550)("stop5", 10000,10050);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 13000, 0, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 4);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8320);
    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 10000);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 13000, 0, routing::DateTime(0, 8050-1000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 4);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8320);
    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 10000);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 13000, 0, routing::DateTime(0, 8050), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 4);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8320);
    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 10000);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 13000, 0, routing::DateTime(0, 8050 +1), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 5*60);
    b.vj("B")("stop2", 10*60)("stop3", 20*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600-1000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600+1), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit_2){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 23*3600 + 59*60);
    b.vj("B")("stop4", 23*3600 + 10*60)("stop2", 10*60)("stop3", 20*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600 - 1000), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600 + 1), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit_interne){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 23*3600 + 30*60, 24*3600 + 30*60)("stop3", 24*40+3600);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    res.print(d);
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600 - 1000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 1, routing::DateTime(0, 23*3600 + 1), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(validity_pattern){
    navimake::builder b("20120614");
    b.vj("D", "0")("stop1", 8000)("stop2", 8200);
    b.vj("C", "1")("stop1", 9000)("stop2", 9200);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

//    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 17900, 0, false);

//    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
//    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9200);

    // TODO : le test ne passe pas , mais ne devrait pas !
    auto &res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7000, 1, false)[0];
    //BOOST_REQUIRE_EQUAL(res.items.size(), 0);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_milieu){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 10000, 19050)("stop4", 19200, 19250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 27900, 0, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 19200);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 27900, 0, routing::DateTime(0, 8050 - 1000), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 19200);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 27900, 0, routing::DateTime(0, 8050), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 19200);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 27900, 0, routing::DateTime(0, 8050 + 1), false);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_fin){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8200);
    b.vj("B")("stop3", 30000)("stop4",40000);
    b.connection("stop2", "stop3", 10*60);

    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 8200+10*60, 0, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8800);
    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8200);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 8200+10*60, 0, routing::DateTime(0, 8000 - 1000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8800);
    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8200);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 8200+10*60, 0, routing::DateTime(0, 8000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8800);
    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8200);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 8200+10*60, 0, routing::DateTime(0, 8000 + 1), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(marche_a_pied_pam){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 23*3600+59*60);
    b.vj("B")("stop3", 2*3600)("stop4",2*3600+20);
    b.connection("stop2", "stop3", 10*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 17900, 1, routing::DateTime(0, 8000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 2*3600+20);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 17900, 1, routing::DateTime(0, 8000 - 1000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 2*3600+20);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 17900, 1, routing::DateTime(0, 8000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 2*3600+20);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 17900, 1, routing::DateTime(0, 8000 + 1), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_debut) {
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop20", 8200);
    b.vj("B")("stop2", 30000)("stop3",40000);
    b.connection("stop1", "stop2", 10*60);

    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 50900, 0, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 40000);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 50900, 0, routing::DateTime(0, 8000 - 1000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 40000);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 50900, 0, routing::DateTime(0, 8000), false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 40000);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 50900, 0, routing::DateTime(0, (30000 - 10*60) + 1), false);
    for(auto r : res1)
        r.print(d);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(sn_fin) {
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8*3600)("stop2", 8*3600 + 20*60);
    b.vj("B")("stop1", 9*3600)("stop2", 9*3600 + 20*60);

    std::vector<std::pair<navitia::type::idx_t, double>> departs, destinations;
    departs.push_back(std::make_pair(0, 0));
    destinations.push_back(std::make_pair(1, 10 * 60));

    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);


    auto res1 = raptor.compute_all(departs, destinations, routing::DateTime(0, 9*3600 + 20 * 60));

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].departure.hour(), 8*3600);
}

BOOST_AUTO_TEST_CASE(prolongement_service) {
    navimake::builder b("20120614");
    b.vj("A", "1111111", "block1")("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1")("stop4", 8*3600+10*60)("stop3", 8*3600 + 20*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 8*3600 + 25*60, 0, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    res1.back().print(d);
}

BOOST_AUTO_TEST_CASE(itl) {
    navimake::builder b("20120614");
    b.vj("A")("stop1",8*3600+10*60, 8*3600 + 10*60,1)("stop2",8*3600+15*60,8*3600+15*60,1)("stop3", 8*3600+20*60);
    b.vj("B")("stop1",9*3600)("stop2",10*3600);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(1).idx, 9*3600+15*60, 0, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);


    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 8*3600+20*60, 0, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].departure.hour(), 8*3600+10*60);

}
