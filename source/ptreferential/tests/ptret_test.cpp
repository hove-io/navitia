#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ptref_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential.h"
#include "ptreferential/reflexion.h"
#include "naviMake/build_helper.h"

using namespace navitia::ptref;
BOOST_AUTO_TEST_CASE(parser){
    std::vector<Filter> filters = parse("stop_areas.uri=42");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[0].value, "42");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}

BOOST_AUTO_TEST_CASE(whitespaces){
    std::vector<Filter> filters = parse("  stop_areas.uri    =  42    ");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[0].value, "42");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}


BOOST_AUTO_TEST_CASE(many_filter){
    std::vector<Filter> filters = parse("stop_areas.uri = 42 and  line.name=bus");
    BOOST_REQUIRE_EQUAL(filters.size(), 2);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[0].value, "42");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);

    BOOST_CHECK_EQUAL(filters[1].object, "line");
    BOOST_CHECK_EQUAL(filters[1].attribute, "name");
    BOOST_CHECK_EQUAL(filters[1].value, "bus");
    BOOST_CHECK_EQUAL(filters[1].op, EQ);
}

BOOST_AUTO_TEST_CASE(having) {
    std::vector<Filter> filters = parse("stop_areas HAVING (line.uri=1 and stop_area.uri=3) and line.uri>=2");
    BOOST_REQUIRE_EQUAL(filters.size(), 2);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "");
    BOOST_CHECK_EQUAL(filters[0].value, "line.uri=1 and stop_area.uri=3");
    BOOST_CHECK_EQUAL(filters[0].op, HAVING);

    BOOST_CHECK_EQUAL(filters[1].object, "line");
    BOOST_CHECK_EQUAL(filters[1].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[1].value, "2");
    BOOST_CHECK_EQUAL(filters[1].op,  GEQ);
}

BOOST_AUTO_TEST_CASE(exception){
    BOOST_CHECK_THROW(parse("mouuuhh bliiii"), ptref_parsing_error);
}

struct Moo {
    int bli;
};
DECL_HAS_MEMBER(bli)
DECL_HAS_MEMBER(foo)

BOOST_AUTO_TEST_CASE(reflexion){
    BOOST_CHECK(Reflect_bli<Moo>::value);
    BOOST_CHECK(!Reflect_foo<Moo>::value);

    Moo m;
    m.bli = 42;
    BOOST_CHECK_EQUAL(get_bli(m), 42);
    BOOST_CHECK_THROW(get_foo(m), unknown_member);
}


BOOST_AUTO_TEST_CASE(sans_filtre) {

    navimake::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);


    navitia::type::Data data;
    b.build(data.pt_data);

    auto indexes = make_query(navitia::type::Type_e::eLine, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

     indexes = make_query(navitia::type::Type_e::eJourneyPattern, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

     indexes = make_query(navitia::type::Type_e::eStopPoint, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 4);


     indexes = make_query(navitia::type::Type_e::eStopArea, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 4);


     indexes = make_query(navitia::type::Type_e::eNetwork, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);


     indexes = make_query(navitia::type::Type_e::ePhysicalMode, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 8);


     indexes = make_query(navitia::type::Type_e::eCommercialMode, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 8);


     indexes = make_query(navitia::type::Type_e::eCity, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

     indexes = make_query(navitia::type::Type_e::eConnection, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);
     indexes = make_query(navitia::type::Type_e::eJourneyPatternPoint, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 4);

     indexes = make_query(navitia::type::Type_e::eVehicleJourney, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);
     
    indexes = make_query(navitia::type::Type_e::eRoute, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);
}


BOOST_AUTO_TEST_CASE(filtre_direct) {

    navimake::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);


    navitia::type::Data data;
    b.build(data.pt_data);

    auto indexes = make_query(navitia::type::Type_e::eLine, "line.uri=line:A", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

     indexes = make_query(navitia::type::Type_e::eJourneyPattern, "journey_pattern.uri=journey_pattern:A-1", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

     indexes = make_query(navitia::type::Type_e::eStopPoint, "stop_point.uri=stop_point:stop1", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);


     indexes = make_query(navitia::type::Type_e::eStopArea, "stop_area.uri=stop_area:stop1", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);


     indexes = make_query(navitia::type::Type_e::eNetwork, "network.uri=network:network:base_network", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);


     indexes = make_query(navitia::type::Type_e::ePhysicalMode, "physical_mode.uri=physical_mode:0x3", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);


     indexes = make_query(navitia::type::Type_e::eCommercialMode, "commercial_mode.uri=commercial_mode:0x3", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);


     indexes = make_query(navitia::type::Type_e::eCity, "city.uri=city:city:base_city", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

     indexes = make_query(navitia::type::Type_e::eConnection, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);
     indexes = make_query(navitia::type::Type_e::eJourneyPatternPoint, "journey_pattern_point.uri=A-1:stop1:0", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

     indexes = make_query(navitia::type::Type_e::eVehicleJourney, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);
     
    indexes = make_query(navitia::type::Type_e::eRoute, "route.uri=route:A-1", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);
}
