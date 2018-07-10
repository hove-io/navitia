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
#include "ed/build_helper.h"

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

BOOST_AUTO_TEST_CASE(after_filter) {
    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250)("stop3", 8500)("stop4", 9000);
    b.vj("B")("stop5", 9000,9050)("stop2", 9200,9250);
    b.vj("C")("stop6", 9000,9050)("stop2", 9200,9250)("stop7", 10000);
    b.vj("D")("stop5", 9000,9050)("stop2", 9200,9250)("stop3", 10000);
    b.finish();
    b.data->pt_data->build_uri();

    auto indexes = make_query_legacy(nt::Type_e::StopArea, "AFTER(stop_area.uri=stop2)", {}, {}, {}, {}, *(b.data));
    BOOST_REQUIRE_EQUAL(indexes.size(), 3);
    auto expected_uris = {"stop3", "stop4", "stop7"};
    for(auto stop_area_idx : indexes) {
        auto stop_area = b.data->pt_data->stop_areas[stop_area_idx];
        BOOST_REQUIRE(std::find(expected_uris.begin(), expected_uris.end(),
                            stop_area->uri) != expected_uris.end());
    }
}
