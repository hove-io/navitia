/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ptref_legacy_test

#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential_legacy.h"
#include "tests/utils_test.h"

namespace nt = navitia::type;
using namespace navitia::ptref;

BOOST_AUTO_TEST_CASE(parser){
    std::vector<Filter> filters = parse("stop_areas.uri=42");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[0].value, "42");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}

BOOST_AUTO_TEST_CASE(parse_code){
    std::vector<Filter> filters = parse("line.code=42");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "line");
    BOOST_CHECK_EQUAL(filters[0].attribute, "code");
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

BOOST_AUTO_TEST_CASE(parser_escaped_string) {
    std::vector<Filter> filters = parse("stop_areas.uri=\"bob the coolest\"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[0].value, "bob the coolest");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}

BOOST_AUTO_TEST_CASE(parser_escaped_string_with_particular_char_quote) {
    std::vector<Filter> filters = parse("stop_areas.uri=\"bob the coolést\"");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[0].value, "bob the coolést");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}


BOOST_AUTO_TEST_CASE(parser_escaped_string_with_nested_quote) {
    std::vector<Filter> filters = parse(R"(stop_areas.uri="bob the \"coolést\"")");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[0].value, "bob the \"coolést\"");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}

BOOST_AUTO_TEST_CASE(parser_escaped_string_with_slash) {
    std::vector<Filter> filters = parse(R"(stop_areas.uri="bob the \\ er")");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "stop_areas");
    BOOST_CHECK_EQUAL(filters[0].attribute, "uri");
    BOOST_CHECK_EQUAL(filters[0].value, R"(bob the \ er)");
    BOOST_CHECK_EQUAL(filters[0].op, EQ);
}

BOOST_AUTO_TEST_CASE(parser_method) {
    std::vector<Filter> filters = parse(R"(vehicle_journey.has_headsign("john"))");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "vehicle_journey");
    BOOST_CHECK_EQUAL(filters[0].method, "has_headsign");
    BOOST_CHECK_EQUAL(filters[0].args, std::vector<std::string>{"john"});
}

BOOST_AUTO_TEST_CASE(parser_method_has_disruption) {
    std::vector<Filter> filters = parse(R"(vehicle_journey.has_disruption())");
    BOOST_REQUIRE_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filters[0].object, "vehicle_journey");
    BOOST_CHECK_EQUAL(filters[0].method, "has_disruption");
    BOOST_CHECK_EQUAL(filters[0].args.size(), 0);
}
