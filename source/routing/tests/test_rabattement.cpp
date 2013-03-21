
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
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8100);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);

}

BOOST_AUTO_TEST_CASE(change){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8*3600 + 10*60, 8*3600 + 15*60)("stop2", 8*3600 + 20*60, 8*3600 + 25*60);
    b.vj("B")("stop1", 8*3600 + 20*60, 8*3600 + 25*60)("stop2", 8*3600 + 30*60, 8*3600 + 35*60);
    b.vj("C")("stop1", 8*3600 + 30*60, 8*3600 + 35*60)("stop2", 8*3600 + 40*60, 8*3600 + 45*60);

    b.vj("D")("stop2", 9*3600 + 10*60, 9*3600 + 15*60)("stop3", 9*3600 + 20*60, 9*3600 + 25*60);
    b.vj("E")("stop2", 9*3600 + 20*60, 9*3600 + 25*60)("stop3", 9*3600 + 30*60, 9*3600 + 35*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 0, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();



    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1], 2);

    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8*3600 + 35*60);
    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8*3600 + 40*60);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8*3600 + 42*60);
    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 8*3600 + 42*60);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 9*3600 + 15*60);
    BOOST_CHECK_EQUAL(res.items[3].departure.hour(), 9*3600 + 15*60);
    BOOST_CHECK_EQUAL(res.items[3].arrival.hour(), 9*3600 + 20*60);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 0);
}

BOOST_AUTO_TEST_CASE(marche_a_pied_debut) {
    navimake::builder b("20120614");

    b.vj("A")("stop1", 8000)("stop20", 8200);
    b.vj("B")("stop2", 30000)("stop3",40000);
    b.vj("C")("stop2", 7900)("stop3",8000);
    b.connection("stop1", "stop2", 10*60);

    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7000, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8000);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7300, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8000);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 29400, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 40000);
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

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 1);
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

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);


    for(auto & r : res1) {
        r.print(d);
    }
    auto res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);
}


BOOST_AUTO_TEST_CASE(passe_minuit_3){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 5*60);
    b.vj("B")("stop1", 0)("stop2", 7*60);

    b.vj("C")("stop2", 10*60)("stop3", 20*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 7*60);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);
}

BOOST_AUTO_TEST_CASE(passe_minuit_interne){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 23*3600 + 30*60, 24*3600 + 30*60)("stop3", 24*40+3600);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);


    auto res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
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

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9200);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 1);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_milieu){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 7900, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.hour(), 9200);
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

//    type::PT_Data & d = data.pt_data;

//    TODO : ça ne passe pas…
//    auto res = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 7900, 0, routing::partirapresrab);
//    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
//    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
//    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8200+10*60);
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

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();


    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[3].arrival.hour(), 2*3600+20);
    BOOST_CHECK_EQUAL(res.items[3].arrival.date(), 1);
}


BOOST_AUTO_TEST_CASE(test_rattrapage) {
    navimake::builder b("20120614");
    b.vj("A")("stop1", 1000)("stop2", 1500)("stop3", 3500)("stop4", 4500);
    b.vj("B")("stop1", 2000)("stop2", 2500)("stop3", 4000)("stop4", 5000);
    b.vj("C")("stop2", 3000)("stop5", 3100)("stop3", 3200);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;
    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 1600, 0);

    BOOST_REQUIRE_EQUAL(res1.size(), 3);



    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 7);
    BOOST_CHECK_EQUAL(res.items[6].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[6].arrival.hour(), 4500);

}
