
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
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8100);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0, routing::DateTime(0, 8200));

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8100);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);


    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0, routing::DateTime(0, 8100));

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8100);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);


    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7900, 0, routing::DateTime(0, 8099));

    BOOST_REQUIRE_EQUAL(res1.size(), 0);


}

BOOST_AUTO_TEST_CASE(change){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("B")("stop4", 8000, 8050)("stop2", 8300, 8350)("stop5", 8400, 8450);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 7900, 0);
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
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8100);

    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8180);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8300);

    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 8350);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 8400);

    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 7900, 0, routing::DateTime(0,8500));
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
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8100);

    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8180);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8300);

    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 8350);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 8400);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 7900, 0, routing::DateTime(0, 8400));
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
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 8100);

    BOOST_CHECK_EQUAL(res.items[1].departure.hour(), 8180);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8300);

    BOOST_CHECK_EQUAL(res.items[2].departure.hour(), 8350);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 8400);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 0);
    BOOST_CHECK_EQUAL(res.items[1].arrival.date(), 0);
    
    
    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[4].idx, 7900, 0,routing::DateTime(0, 8399));
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

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0);
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

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing ::DateTime(1, 8500));
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

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing::DateTime(1, 20*60));
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

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing::DateTime(1, (20*60)-1));
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

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0);

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

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing::DateTime(1, 5000));

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

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing::DateTime(1, 20*60));

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

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing::DateTime(1, (20*60)-1));

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit_interne){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 23*3600 + 30*60, 24*3600 + 30*60)("stop3", 24*3600 + 40 * 60);
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
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing::DateTime(1, 3600));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing::DateTime(1, 40*60));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[0].departure.date(), 0);
    BOOST_CHECK_EQUAL(res.items[0].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[2].idx, 22*3600, 0, routing::DateTime(1, (40*60)-1));
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

    auto res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7800, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);



    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9200);

    
    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7800, 0, routing::DateTime(0, 10000));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9200);


    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7800, 0, routing::DateTime(0, 9200));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.hour(), 9200);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[1].idx, 7800, 0, routing::DateTime(0, 9199));
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
    
    
    
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

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 9200);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 7900, 0, routing::DateTime(0,10000));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 9200);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 7900, 0, routing::DateTime(0,9200));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 9200);

    res1 = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 7900, 0, routing::DateTime(0,9199));
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

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 7900, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8200 + 60*10);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 7900, 0, routing::DateTime(0, 9200));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8200 + 60*10);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 7900, 0, routing::DateTime(0, 8200+10*60));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 8200 + 60*10);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 7900, 0, routing::DateTime(0, (8200+10*60)-1));
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

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 2*3600+20);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0, routing::DateTime(1 , (3*3600+20)));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 2*3600+20);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0, routing::DateTime(1 , (2*3600+20)));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.hour(), 2*3600+20);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date(), 1);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0, routing::DateTime(1 , (2*3600+20)-1));
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_debut) {
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop20", 8200);
    b.vj("B")("stop2", 30000)("stop3",40000);
    b.vj("C")("stop2", 7900 + 10*60-1)("stop3", 9500);
    b.connection("stop1", "stop2", 10*60);

    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);
    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 40000);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0, routing::DateTime(0, 50000));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 40000);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0, routing::DateTime(0, 40000));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 2);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[1].arrival.hour(), 40000);

    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 7900, 0, routing::DateTime(0, 40000 - 1));
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(test_rattrapage) {
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8*3600 + 10*60)("stop2", 8*3600 + 15*60)("stop3", 8*3600 + 35*60)("stop4", 8*3600 + 45*60);
    b.vj("B")("stop1", 8*3600 + 20*60)("stop2", 8*3600 + 25*60)("stop3", 8*3600 + 40*60)("stop4", 8*3600 + 50*60);
    b.vj("C")("stop2", 8*3600 + 30*60)("stop5", 8*3600 + 31*60)("stop3", 8*3600 + 32*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;
    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 8*3600 + 15*60, 0);

    for(auto r : res1) {
        std::cout << std::endl << std::endl << "Itineraire " << std::endl;
        r.print(d);
    }

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 5);
    BOOST_REQUIRE_EQUAL(res.items[0].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[1].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[2].stop_points.size(), 3);
    BOOST_REQUIRE_EQUAL(res.items[3].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[4].stop_points.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[0], 2);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[4].stop_points[0], 2);
    BOOST_CHECK_EQUAL(res.items[4].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8*3600+20*60);
    BOOST_CHECK_EQUAL(res.items[4].arrival.hour(), 8*3600+45*60);
    
    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 8*3600 + 15*60, 0, routing::DateTime(0, (9*3600+45*60)));

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 5);
    BOOST_REQUIRE_EQUAL(res.items[0].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[1].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[2].stop_points.size(), 3);
    BOOST_REQUIRE_EQUAL(res.items[3].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[4].stop_points.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[0], 2);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[4].stop_points[0], 2);
    BOOST_CHECK_EQUAL(res.items[4].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8*3600+20*60);
    BOOST_CHECK_EQUAL(res.items[4].arrival.hour(), 8*3600+45*60);
    
    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 8*3600 + 15*60, 0, routing::DateTime(0, (8*3600+45*60)));

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 5);
    BOOST_REQUIRE_EQUAL(res.items[0].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[1].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[2].stop_points.size(), 3);
    BOOST_REQUIRE_EQUAL(res.items[3].stop_points.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[4].stop_points.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0], 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[1], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0], 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[2], 2);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[0], 2);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1], 2);
    BOOST_CHECK_EQUAL(res.items[4].stop_points[0], 2);
    BOOST_CHECK_EQUAL(res.items[4].stop_points[1], 3);
    BOOST_CHECK_EQUAL(res.items[0].departure.hour(), 8*3600+20*60);
    BOOST_CHECK_EQUAL(res.items[4].arrival.hour(), 8*3600+45*60);
    
    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 8*3600 + 15*60, 0, routing::DateTime(0, (8*3600+45*60)-1));

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(pam_veille) {
    navimake::builder b("20120614");
    b.vj("A")("stop1", 10*60)("stop2", 20*60)("stop3", 50*60);
    b.vj("B", "01")("stop0", 23*3600)("stop2", 24*3600 + 30*60)("stop3", 24*3600 + 40*60);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(1).idx, d.stop_areas.at(3).idx, 5*60, 1);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
}

BOOST_AUTO_TEST_CASE(sn_debut) {
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8*3600)("stop2", 8*3600 + 20*60);
    b.vj("B")("stop1", 9*3600)("stop2", 9*3600 + 20*60);

    std::vector<std::pair<navitia::type::idx_t, double>> departs, destinations;
    departs.push_back(std::make_pair(0, 10 * 60));
    destinations.push_back(std::make_pair(1,0));

    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute_all(departs, destinations, routing::DateTime(0, 8*3600));
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.hour(), 9*3600 + 20 * 60);
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

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 5*60, 0);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.hour(), 8*3600 + 20*60);
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

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(1).idx, 5*60, 0);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.hour(), 10*3600);


    res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 5*60, 0);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.hour(), 8*3600+20*60);

}


BOOST_AUTO_TEST_CASE(mdi) {
    navimake::builder b("20120614");

    b.vj("B")("stop1",17*3600, 17*3600,std::numeric_limits<uint32_t>::max(), true, false)("stop2", 17*3600+15*60)("stop3",17*3600+30*60, 17*3600+30*60,std::numeric_limits<uint32_t>::max(), true, true);
    b.vj("C")("stop4",16*3600, 16*3600,std::numeric_limits<uint32_t>::max(), true, true)("stop5", 16*3600+15*60)("stop6",16*3600+30*60, 16*3600+30*60,std::numeric_limits<uint32_t>::max(), false, true);
    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(2).idx, 5*60, 0);
    BOOST_CHECK_EQUAL(res1.size(), 0);
    res1 = raptor.compute(d.stop_areas.at(1).idx, d.stop_areas.at(2).idx, 5*60, 0);
    BOOST_CHECK_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas.at(3).idx, d.stop_areas.at(5).idx, 5*60, 0);
    BOOST_CHECK_EQUAL(res1.size(), 0);
    res1 = raptor.compute(d.stop_areas.at(3).idx, d.stop_areas.at(4).idx, 5*60, 0);
    BOOST_CHECK_EQUAL(res1.size(), 1);
}

BOOST_AUTO_TEST_CASE(multiples_vj) {
    navimake::builder b("20120614");
    b.vj("A1")("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("A2")("stop1", 8*3600 + 15*60 )("stop2", 8*3600+20*60);
    b.vj("B1")("stop2", 8*3600 + 25*60)("stop3", 8*3600+30*60);
    b.vj("B2")("stop2", 8*3600 + 35*60)("stop3", 8*3600+40*60);
    b.vj("C")("stop3", 8*3600 + 45*60)("stop4", 8*3600+50*60);
    b.vj("E1")("stop1", 8*3600)("stop5", 8*3600 + 5*60);
    b.vj("E2")("stop5", 8*3600 + 10 * 60)("stop1", 8*3600 + 12*60);

    type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    RAPTOR raptor(data);

    type::PT_Data & d = data.pt_data;

    auto res1 = raptor.compute(d.stop_areas.at(0).idx, d.stop_areas.at(3).idx, 5*60, 0);
    for(auto r : res1)
        r.print(d);

    BOOST_CHECK_EQUAL(res1.size(), 1);
}




