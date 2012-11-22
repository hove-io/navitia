#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>
#include "time_tables/thermometer.h"

#include "routing/raptor.h"
#include "naviMake/build_helper.h"
using namespace navitia::timetables;

//BOOST_AUTO_TEST_CASE(t1) {
//    navimake::builder b("20120614");
//    b.vj("A")("A", 8000, 8050)("B", 9000, 9050)("C", 10000, 10050);
//    navitia::type::Data data;
//    b.build(data.pt_data);
//    data.build_raptor();
//    navitia::routing::raptor::RAPTOR raptor(data);

//    navitia::type::PT_Data & d = data.pt_data;

//    Thermometer t(data);
//    auto result = t.get_thermometer(0);

//    BOOST_REQUIRE_EQUAL(result.size(), 3);

//    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:A"]));
//    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:B"]));
//    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:C"]));

//    BOOST_REQUIRE_LT(posA, posB);
//    BOOST_REQUIRE_LT(posB, posC);
//}

//BOOST_AUTO_TEST_CASE(t2) {
//    navimake::builder b("20120614");
//    b.vj("A")("A", 8000, 8050)("B", 9000, 9050)("C", 10000, 10050)("B", 19000, 19050)("E", 19100, 19150);
//    navitia::type::Data data;
//    b.build(data.pt_data);
//    data.build_raptor();
//    navitia::routing::raptor::RAPTOR raptor(data);

//    navitia::type::PT_Data & d = data.pt_data;

//    Thermometer t(data);
//    auto result = t.get_thermometer(0);

//    BOOST_REQUIRE_EQUAL(result.size(), 5);


//    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:A"]));
//    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:B"]));
//    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:C"]));
//    auto posB1 = distance(result.begin(), std::find(result.begin()+posB+1, result.end(), d.stop_point_map["stop_point:B"]));
//    auto posE = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:E"]));

//    BOOST_REQUIRE_LT(posA, posB);
//    BOOST_REQUIRE_LT(posB, posC);
//    BOOST_REQUIRE_LT(posC, posB1);
//    BOOST_REQUIRE_LT(posB1, posE);
//}

//BOOST_AUTO_TEST_CASE(t3) {
//    navimake::builder b("20120614");
//    b.vj("A")("A", 8000, 8050)("B", 9000, 9050)("C", 10000, 10050)("D", 19000, 19050)("A", 19100, 19150);
//    navitia::type::Data data;
//    b.build(data.pt_data);
//    data.build_raptor();
//    navitia::routing::raptor::RAPTOR raptor(data);

//    navitia::type::PT_Data & d = data.pt_data;

//    Thermometer t(data);
//    auto result = t.get_thermometer(0);

//    BOOST_REQUIRE_EQUAL(result.size(), 5);

//    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:A"]));
//    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:B"]));
//    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:C"]));
//    auto posD = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:D"]));
//    auto posA1 = distance(result.begin(), std::find(result.begin()+posA+1, result.end(), d.stop_point_map["stop_point:A"]));


//    BOOST_REQUIRE_LT(posA, posB);
//    BOOST_REQUIRE_LT(posB, posC);
//    BOOST_REQUIRE_LT(posC, posD);
//    BOOST_REQUIRE_LT(posD, posA1);
//}

//BOOST_AUTO_TEST_CASE(t4) {
//    navimake::builder b("20120614");
//    b.vj("A")("A", 8000, 8050)("B", 9000, 9050)("C", 10000, 10050)("D", 19000, 19050)("A", 19100, 19150)("E", 29100, 29150);
//    navitia::type::Data data;
//    b.build(data.pt_data);
//    data.build_raptor();
//    navitia::routing::raptor::RAPTOR raptor(data);

//    navitia::type::PT_Data & d = data.pt_data;

//    Thermometer t(data);
//    auto result = t.get_thermometer(0);

//    BOOST_REQUIRE_EQUAL(result.size(), 6);

//    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:A"]));
//    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:B"]));
//    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:C"]));
//    auto posD = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:D"]));
//    auto posA1 = distance(result.begin(), std::find(result.begin()+posA+1, result.end(), d.stop_point_map["stop_point:A"]));
//    auto posE = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:E"]));


//    BOOST_REQUIRE_LT(posA, posB);
//    BOOST_REQUIRE_LT(posB, posC);
//    BOOST_REQUIRE_LT(posC, posD);
//    BOOST_REQUIRE_LT(posD, posA1);
//    BOOST_REQUIRE_LT(posA1, posE);
//}


//BOOST_AUTO_TEST_CASE(t5) {
//    navimake::builder b("20120614");
//    b.vj("A")("A", 8000, 8050)("B", 9000, 9050)("C", 10000, 10050)("B", 11000, 11050)("D", 19000, 19050)("A", 19100, 19150)("E", 29100, 29150);
//    navitia::type::Data data;
//    b.build(data.pt_data);
//    data.build_raptor();
//    navitia::routing::raptor::RAPTOR raptor(data);

//    navitia::type::PT_Data & d = data.pt_data;

//    Thermometer t(data);
//    auto result = t.get_thermometer(0);

//    BOOST_REQUIRE_EQUAL(result.size(), 7);

//    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:A"]));
//    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:B"]));
//    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:C"]));
//    auto posB1 = distance(result.begin(), std::find(result.begin()+posB+1, result.end(), d.stop_point_map["stop_point:B"]));
//    auto posD = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:D"]));
//    auto posA1 = distance(result.begin(), std::find(result.begin()+posA+1, result.end(), d.stop_point_map["stop_point:A"]));
//    auto posE = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:E"]));

//    BOOST_REQUIRE_LT(posA, posB);
//    BOOST_REQUIRE_LT(posB, posC);
//    BOOST_REQUIRE_LT(posC, posB1);
//    BOOST_REQUIRE_LT(posB1, posD);
//    BOOST_REQUIRE_LT(posD, posA1);
//    BOOST_REQUIRE_LT(posA1, posE);
//}

BOOST_AUTO_TEST_CASE(t6) {
    navimake::builder b("20120614");
    b.vj("A")("A", 8000, 8050)("B", 9000, 9050)("C", 10000, 10050);
    b.vj("A")("A", 8000, 8050)("C", 9000, 9050)("B", 10000, 10050);
    navitia::type::Data data;
    b.build(data.pt_data);
    data.build_raptor();
    navitia::routing::raptor::RAPTOR raptor(data);

    navitia::type::PT_Data & d = data.pt_data;

    Thermometer t(data);
    auto result = t.get_thermometer(0);

    BOOST_REQUIRE_EQUAL(result.size(), 4);

    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:A"]));
    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:B"]));
    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), d.stop_point_map["stop_point:C"]));

    BOOST_REQUIRE_LT(posA, posB);
    BOOST_REQUIRE_LT(posA, posC);
    BOOST_REQUIRE_NE(posB, posC);

    if(posB < posC) {
        auto posB1 = distance(result.begin(), std::find(result.begin() + posB + 1, result.end(), d.stop_point_map["stop_point:B"]));
        BOOST_REQUIRE_LT(posC, posB1);
    } else {
        auto posC1 = distance(result.begin(), std::find(result.begin() + posC + 1, result.end(), d.stop_point_map["stop_point:C"]));
        BOOST_REQUIRE_LT(posB, posC1);
    }
}



