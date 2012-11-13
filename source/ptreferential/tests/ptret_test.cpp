#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE georef_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential.h"
/*
navitia::type::Type_e navitia_type; //< Le type parsé
std::string object; //< L'objet sous forme de chaîne de caractère ("stop_area")
std::string attribute; //< L'attribu ("external code")
Operator_e op; //< la comparaison ("=")
std::string value; //< la valeur comparée ("kikoolol")
*/
using namespace navitia::ptref;
BOOST_AUTO_TEST_CASE(parser){
    std::vector<Filter> filters = parse("stop_areas.external_code=42");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "external_code");
    BOOST_CHECK_EQUAL(filters[0].value, "42");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}

BOOST_AUTO_TEST_CASE(whitespaces){
    std::vector<Filter> filters = parse("  stop_areas.external_code    =  42    ");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "external_code");
    BOOST_CHECK_EQUAL(filters[0].value, "42");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}


BOOST_AUTO_TEST_CASE(many_filter){
    std::vector<Filter> filters = parse("stop_areas.external_code = 42 and  line.name=bus");
    BOOST_REQUIRE_EQUAL(filters.size(), 2);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "external_code");
    BOOST_CHECK_EQUAL(filters[0].value, "42");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);

    BOOST_CHECK_EQUAL(filters[1].object, "line");
    BOOST_CHECK_EQUAL(filters[1].attribute, "name");
    BOOST_CHECK_EQUAL(filters[1].value, "bus");
    BOOST_CHECK_EQUAL(filters[1].op, EQ);
}

BOOST_AUTO_TEST_CASE(exception){
    BOOST_CHECK_THROW(parse("mouuuhh bliiii"), ptref_parsing_error);
}
