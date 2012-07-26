
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/raptor.h"
#include "naviMake/build_helper.h"

using namespace navitia;
using namespace routing::raptor;

BOOST_AUTO_TEST_CASE(direct){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0);


    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[1].said, 1);
    BOOST_CHECK_EQUAL(res.items[0].time, 8050);
    BOOST_CHECK_EQUAL(res.items[1].time, 8100);
    BOOST_CHECK_EQUAL(res.items[0].day, 0);
    BOOST_CHECK_EQUAL(res.items[1].day, 0);

}

BOOST_AUTO_TEST_CASE(change){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("B")("stop4", 8000, 8050)("stop2", 8200,8250)("stop5", 8300,8350);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 7900, 0);
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[1].said, 1);
    BOOST_CHECK_EQUAL(res.items[2].said, 1);
    BOOST_CHECK_EQUAL(res.items[3].said, 4);
    BOOST_CHECK_EQUAL(res.items[0].time, 8050);
    BOOST_CHECK_EQUAL(res.items[1].time, 8100);
    BOOST_CHECK_EQUAL(res.items[2].time, 8250);
    BOOST_CHECK_EQUAL(res.items[3].time, 8300);
    BOOST_CHECK_EQUAL(res.items[0].day, 0);
    BOOST_CHECK_EQUAL(res.items[1].day, 0);
    BOOST_CHECK_EQUAL(res.items[2].day, 0);
    BOOST_CHECK_EQUAL(res.items[3].day, 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 5*60);
    b.vj("B")("stop2", 10*60)("stop3", 20*60);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0);

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[0].said, 0);
    BOOST_CHECK_EQUAL(res.items[1].said, 1);
    BOOST_CHECK_EQUAL(res.items[2].said, 1);
    BOOST_CHECK_EQUAL(res.items[3].said, 2);
    BOOST_CHECK_EQUAL(res.items[0].day, 0);
    BOOST_CHECK_EQUAL(res.items[3].day, 1);
}

BOOST_AUTO_TEST_CASE(validity_pattern){
    navimake::builder b("20120614");
    b.vj("A", "0")("stop1", 8000)("stop2", 8200);
    b.vj("B", "1")("stop1", 9000)("stop2", 9200);
    type::Data data;
    data.pt_data =  b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0);
    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].time, 9200);

    res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 1);
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



    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 7900, 0);

    BOOST_REQUIRE_EQUAL(res.items.size(), 5);
    BOOST_CHECK_EQUAL(res.items[4].time, 9200);
}


BOOST_AUTO_TEST_CASE(deux_marche_a_pied_milieu){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.vj("B")("stop3", 9500,9550)("stop4", 9600,9650);

    b.vj("C")("stop1", 80000,80050)("stop5", 82000,82500);
    b.connection("stop2", "stop5", 10*60);
    b.connection("stop5", "stop2", 10*60);

    b.connection("stop5", "stop3", 10*60);
    b.connection("stop3", "stop5", 10*60);

    type::Data data;
    data.pt_data = b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;



    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 7900, 0);
    std::cout << res;
    BOOST_REQUIRE_EQUAL(res.items.size(), 6);
    BOOST_CHECK_EQUAL(res.items[5].time, 9600);
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



    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 7900, 0);

    std::cout << res << std::endl;
    BOOST_REQUIRE_EQUAL(res.items.size(), 6);
    BOOST_CHECK_EQUAL(res.items[5].time, 9200);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_fin){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8200);
    b.vj("B")("stop3", 30000)("stop4",40000);
    b.connection("stop2", "stop3", 10*60);

    type::Data data;
    data.pt_data = b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;


    auto res = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 7900, 0);

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].said, 2);
    BOOST_CHECK_EQUAL(res.items[2].time, 8200+10*60);
}

BOOST_AUTO_TEST_CASE(marche_a_pied_pam){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8200);
    b.vj("B")("stop3", 10)("stop4",20);
    b.connection("stop2", "stop3", 10*60);

    type::Data data;
    data.pt_data = b.build();
    RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;


    auto res = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0);
    std::cout << res << std::endl;
    BOOST_REQUIRE_EQUAL(res.items.size(), 5);
    BOOST_CHECK_EQUAL(res.items[4].said, 3);
    BOOST_CHECK_EQUAL(res.items[4].time, 20);
    BOOST_CHECK_EQUAL(res.items[4].day, 1);
}

