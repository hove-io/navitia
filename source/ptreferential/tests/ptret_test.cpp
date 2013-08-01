#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ptref_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential.h"
#include "ptreferential/reflexion.h"
#include "ptreferential/ptref_graph.h"
#include "ed/build_helper.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>

namespace navitia{namespace ptref {
template<typename T> std::vector<idx_t> get_indexes(Filter filter,  Type_e requested_type, const Data & d);
}}

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


BOOST_AUTO_TEST_CASE(escaped_value){
    std::vector<Filter> filters = parse("stop_areas.uri=\"42\"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].value, "42");

    filters = parse("stop_areas.uri=\"4-2\"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].value, "4-2");

    filters = parse("stop_areas.uri=\"4|2\"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].value, "4|2");

    filters = parse("stop_areas.uri=\"42.\"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].value, "42.");

    filters = parse("stop_areas.uri=\"42 12\"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].value, "42 12");

    filters = parse("stop_areas.uri=\"  42  \"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].value, "  42  ");

    filters = parse("stop_areas.uri=\"4&2\"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].value, "4&2");
}

BOOST_AUTO_TEST_CASE(exception){
    BOOST_CHECK_THROW(parse(""), parsing_error);
    BOOST_CHECK_THROW(parse("mouuuhh bliiii"), parsing_error);
    BOOST_CHECK_THROW(parse("stop_areas.uri==42"), parsing_error);
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

    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);

    auto indexes = make_query(navitia::type::Type_e::Line, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::JourneyPattern, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::StopPoint, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 4);

    indexes = make_query(navitia::type::Type_e::StopArea, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 4);

    indexes = make_query(navitia::type::Type_e::Network, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(navitia::type::Type_e::PhysicalMode, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::CommercialMode, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::Connection, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::JourneyPatternPoint, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 4);

    indexes = make_query(navitia::type::Type_e::VehicleJourney, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::Route, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);
}


BOOST_AUTO_TEST_CASE(get_indexes_test){
    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.data.pt_data.index();

    // On cherche à retrouver la ligne 1, en passant le stoparea en filtre
    Filter filter;
    filter.navitia_type = Type_e::StopArea;
    filter.attribute = "uri";
    filter.op = EQ;
    filter.value = "stop1";
    auto indexes = get_indexes<StopArea>(filter, Type_e::Line, b.data);
    BOOST_REQUIRE_EQUAL(indexes.size(), 1);
    BOOST_CHECK_EQUAL(indexes[0], 0);

    // On cherche les stopareas de la ligneA
    filter.navitia_type = Type_e::Line;
    filter.value = "A";
    indexes = get_indexes<Line>(filter, Type_e::StopArea,b. data);
    BOOST_REQUIRE_EQUAL(indexes.size(), 2);
    BOOST_CHECK_EQUAL(indexes[0], 0);
    BOOST_CHECK_EQUAL(indexes[1], 1);
}


BOOST_AUTO_TEST_CASE(make_query_filtre_direct) {

    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);

    auto indexes = make_query(navitia::type::Type_e::Line, "line.uri=A", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(navitia::type::Type_e::JourneyPattern, "journey_pattern.uri=A:0:0", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(navitia::type::Type_e::StopPoint, "stop_point.uri=stop1", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(navitia::type::Type_e::StopArea, "stop_area.uri=stop1", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(navitia::type::Type_e::Network, "network.uri=base_network", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(navitia::type::Type_e::PhysicalMode, "physical_mode.uri=0x1", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(navitia::type::Type_e::CommercialMode, "commercial_mode.uri=0x1", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(navitia::type::Type_e::Connection, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::VehicleJourney, "", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::Route, "route.uri=A:0", b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);
}

BOOST_AUTO_TEST_CASE(after_filter) {
    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250)("stop3", 8500)("stop4", 9000);
    b.vj("B")("stop5", 9000,9050)("stop2", 9200,9250);
    b.vj("C")("stop6", 9000,9050)("stop2", 9200,9250)("stop7", 10000);
    b.vj("D")("stop5", 9000,9050)("stop2", 9200,9250)("stop3", 10000);

    auto indexes = make_query(navitia::type::Type_e::StopArea,
                              "AFTER(stop_area.uri=stop2)", b.data);
    BOOST_REQUIRE_EQUAL(indexes.size(), 3);
    auto expected_uris = {"stop3", "stop4", "stop7"};
    for(auto stop_area_idx : indexes) {
        auto stop_area = b.data.pt_data.stop_areas[stop_area_idx];
        BOOST_REQUIRE(std::find(expected_uris.begin(), expected_uris.end(),
                            stop_area->uri) != expected_uris.end());
    }
}


BOOST_AUTO_TEST_CASE(find_path_test){
    auto res = find_path(navitia::type::Type_e::Route);
    // Cas où on veut partir et arriver au même point
    BOOST_CHECK(res[navitia::type::Type_e::Route] == navitia::type::Type_e::Route);
    // On regarde qu'il y a un semblant d'itinéraire pour chaque type : il faut un prédécesseur
    for(auto node_pred : res){
        // Route c'est lui même puisque c'est la source, et validity pattern est puni, donc il est seul dans son coin, de même pour POI et POIType
        if(node_pred.first != Type_e::Route && node_pred.first != Type_e::ValidityPattern && node_pred.first != Type_e::POI && node_pred.first != Type_e::POIType)
            BOOST_CHECK(node_pred.first != node_pred.second);
    }

    // On vérifie qu'il n'y ait pas de nœud qui ne soit pas atteignable depuis un autre nœud
    // En connexité non forte, il y a un seul ensemble : validity pattern est relié (en un seul sens) au gros paté
    // Avec les composantes fortement connexes : there must be only two : validity pattern est isolé car on ne peut y aller
    Jointures j;
    std::vector<vertex_t> component(boost::num_vertices(j.g));
    int num = boost::connected_components(j.g, &component[0]);
    BOOST_CHECK_EQUAL(num, 2);
    num =  boost::strong_components(j.g, &component[0]);
    BOOST_CHECK_EQUAL(num, 3);

    // Type qui n'existe pas dans le graph : il n'y a pas de chemin
    BOOST_CHECK_THROW(find_path(navitia::type::Type_e::Unknown), ptref_error);
}
