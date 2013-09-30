#include "ed/connectors/osm2nav.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>

using namespace navitia::georef;
using namespace ed::connectors;
using namespace CanalTP;

BOOST_AUTO_TEST_CASE(cas_simple){
    GeoRef geo_ref;
    Visitor v(geo_ref);
    OSMWay w1; w1.refs = {1,2};
    OSMWay w2; w2.refs = {2,1};
    v.ways[1] = w1;
    v.ways[2] = w2;

    Reference r1;
    r1.member_id = 1;
    r1.member_type = OSMPBF::Relation_MemberType_WAY;

    Reference r2;
    r2.member_id = 2;
    r2.member_type = OSMPBF::Relation_MemberType_WAY;

    std::vector<Reference> refs{r1, r2};

    std::vector<uint64_t> res = v.nodes_of_relation(refs);

    BOOST_REQUIRE_EQUAL(res.size(), 3);
    BOOST_REQUIRE_EQUAL(res[0], 1);
    BOOST_REQUIRE_EQUAL(res[1], 2);
    BOOST_REQUIRE_EQUAL(res[2], 1);
}

BOOST_AUTO_TEST_CASE(inversion_simple){
    GeoRef geo_ref;
    Visitor v(geo_ref);
    OSMWay w1; w1.refs = {1,2};
    OSMWay w2; w2.refs = {1,2};
    v.ways[1] = w1;
    v.ways[2] = w2;

    Reference r1;
    r1.member_id = 1;
    r1.member_type = OSMPBF::Relation_MemberType_WAY;

    Reference r2;
    r2.member_id = 2;
    r2.member_type = OSMPBF::Relation_MemberType_WAY;

    std::vector<Reference> refs{r1, r2};

    std::vector<uint64_t> res = v.nodes_of_relation(refs);

    BOOST_REQUIRE_EQUAL(res.size(), 3);
    BOOST_REQUIRE_EQUAL(res[0], 1);
    BOOST_REQUIRE_EQUAL(res[1], 2);
    BOOST_REQUIRE_EQUAL(res[2], 1);
}

BOOST_AUTO_TEST_CASE(cas_polygon_ouverte){
    GeoRef geo_ref;
    Visitor v(geo_ref);
    OSMWay w1; w1.refs = {1,2,3};
    OSMWay w2; w2.refs = {10,3};
    v.ways[1] = w1;
    v.ways[2] = w2;

    Reference r1;
    r1.member_id = 1;
    r1.member_type = OSMPBF::Relation_MemberType_WAY;

    Reference r2;
    r2.member_id = 2;
    r2.member_type = OSMPBF::Relation_MemberType_WAY;

    std::vector<Reference> refs{r1, r2};

    std::vector<uint64_t> res = v.nodes_of_relation(refs);

    BOOST_REQUIRE_EQUAL(res.size(), 4);
    BOOST_REQUIRE_EQUAL(res[0], 1);
    BOOST_REQUIRE_EQUAL(res[1], 2);
    BOOST_REQUIRE_EQUAL(res[2], 3);
    BOOST_REQUIRE_EQUAL(res[3], 10);
}

BOOST_AUTO_TEST_CASE(ways_pas_enorder){
    GeoRef geo_ref;
    Visitor v(geo_ref);
    OSMWay w1; w1.refs = {1,2};
    OSMWay w2; w2.refs = {1,5};
    OSMWay w3; w3.refs = {5,4};
    OSMWay w4; w4.refs = {3,4};
    OSMWay w5; w5.refs = {2,3};
    v.ways[1] = w1;
    v.ways[2] = w2;
    v.ways[3] = w3;
    v.ways[4] = w4;
    v.ways[5] = w5;

    Reference r0;
    r0.member_id = 0;
    r0.member_type = OSMPBF::Relation_MemberType_NODE;

    Reference r1;
    r1.member_id = 1;
    r1.member_type = OSMPBF::Relation_MemberType_WAY;

    Reference r2;
    r2.member_id = 2;
    r2.member_type = OSMPBF::Relation_MemberType_WAY;

    Reference r3;
    r3.member_id = 3;
    r3.member_type = OSMPBF::Relation_MemberType_WAY;

    Reference r4;
    r4.member_id = 4;
    r4.member_type = OSMPBF::Relation_MemberType_WAY;

    Reference r5;
    r5.member_id = 5;
    r5.member_type = OSMPBF::Relation_MemberType_WAY;

    std::vector<Reference> refs{r0, r1, r2, r3, r4, r5};

    std::vector<uint64_t> res = v.nodes_of_relation(refs);

    BOOST_REQUIRE_EQUAL(res.size(), 6);
    BOOST_REQUIRE_EQUAL(res[0], 1);
    BOOST_REQUIRE_EQUAL(res[1], 2);
    BOOST_REQUIRE_EQUAL(res[2], 3);
    BOOST_REQUIRE_EQUAL(res[3], 4);
    BOOST_REQUIRE_EQUAL(res[4], 5);
    BOOST_REQUIRE_EQUAL(res[5], 1);
}

