#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ptref_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential.h"
#include "ptreferential/reflexion.h"
#include "ptreferential/ptref_graph.h"
#include "naviMake/build_helper.h"

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



BOOST_AUTO_TEST_CASE(get_indexes_test){
    navimake::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);

    navitia::type::Data data;
    b.build(data.pt_data);

    // On cherche à retrouver la ligne 1, en passant le stoparea en filtre
    Filter filter;
    filter.navitia_type = Type_e::eStopArea;
    filter.attribute = "uri";
    filter.op = EQ;
    filter.value = "stop_area:stop1";
    auto indexes = get_indexes<StopArea>(filter, Type_e::eLine, data);
    BOOST_REQUIRE_EQUAL(indexes.size(), 1);
    BOOST_CHECK_EQUAL(indexes[0], 0);

    // On cherche les stopareas de la ligneA
    filter.navitia_type = Type_e::eLine;
    filter.value = "line:A";
    indexes = get_indexes<Line>(filter, Type_e::eStopArea, data);
    BOOST_REQUIRE_EQUAL(indexes.size(), 2);
    BOOST_CHECK_EQUAL(indexes[0], 0);
    BOOST_CHECK_EQUAL(indexes[1], 1);
}

BOOST_AUTO_TEST_CASE(make_query_filtre_direct) {

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

BOOST_AUTO_TEST_CASE(find_path_test){
    auto res = find_path(navitia::type::Type_e::eRoute);
    // Cas où on veut partir et arriver au même point
    BOOST_CHECK(res[navitia::type::Type_e::eRoute] == navitia::type::Type_e::eRoute);
    // On regarde qu'il y a un semblant d'itinéraire pour chaque type : il faut un prédécesseur
    for(auto node_pred : res){
        // Route c'est lui même puisque c'est la source, et validity pattern est puni, donc il est seul dans son coin
        if(node_pred.first != Type_e::eRoute && node_pred.first != Type_e::eValidityPattern)
            BOOST_CHECK(node_pred.first != node_pred.second);
    }

    // On vérifie qu'il n'y ait pas de nœud qui ne soit pas atteignable depuis un autre nœud
    // En connexité non forte, il y a un seul ensemble : validity pattern est relié (en un seul sens) au gros paté
    // Avec les composantes fortement connexes : there must be only two : validity pattern est isolé car on ne peut y aller
    Jointures j;
    std::vector<vertex_t> component(boost::num_vertices(j.g));
    int num = boost::connected_components(j.g, &component[0]);
    BOOST_CHECK_EQUAL(num, 1);
    num =  boost::strong_components(j.g, &component[0]);
    BOOST_CHECK_EQUAL(num, 2);

    // Type qui n'existe pas dans le graph : il n'y a pas de chemin
    BOOST_CHECK_THROW(find_path(navitia::type::Type_e::eUnknown), ptref_error);

}
