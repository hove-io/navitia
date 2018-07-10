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
#define BOOST_TEST_MODULE ptref_ng_test

#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential_ng.h"

using namespace navitia::ptref;

static void assert_expr(const std::string& to_parse, const std::string& expected) {
    const ast::Expr expr = parse(to_parse);
    std::stringstream ss;
    ss << expr;
    BOOST_CHECK_EQUAL(ss.str(), expected);
}

BOOST_AUTO_TEST_CASE(parse_pred) {
    assert_expr("  all  ", "all");
    assert_expr("  empty  ", "empty");
    assert_expr("line . a ( ) ", "line.a()");
    assert_expr(
        R"#(vehicle_journey . has_code ( external_code , "OIF:42" ) )#",
        R"#(vehicle_journey.has_code("external_code", "OIF:42"))#"
    );
    assert_expr(R"#(stop_area . uri ( "OIF:42" ) )#", R"#(stop_area.uri("OIF:42"))#");
    assert_expr(R"#(stop_area . uri = "OIF:42" )#", R"#(stop_area.uri("OIF:42"))#");
    assert_expr(R"#(stop_area . uri = OIF:42 )#", R"#(stop_area.uri("OIF:42"))#");
    assert_expr(R"#(stop_area . uri = foo )#", R"#(stop_area.uri("foo"))#");
    assert_expr(R"#(stop_area . uri = 42 )#", R"#(stop_area.uri("42"))#");
    assert_expr(R"#(stop_area . uri = 1ee7_: )#", R"#(stop_area.uri("1ee7_:"))#");
    assert_expr(
        R"#(stop_point.within(42, -2.2;4.9e-2))#",
        R"#(stop_point.within("42", "-2.2;4.9e-2"))#"
    );
    assert_expr(
        R"#(stop_point.coord DWITHIN (-2.2, 4.9e-2, 42))#",
        R"#(stop_point.within("42", "-2.2;4.9e-2"))#"
    );
}

BOOST_AUTO_TEST_CASE(parse_expr) {
    assert_expr(" all - empty - all ", "(all - (empty - all))");
    assert_expr(" all and empty and all ", "(all AND (empty AND all))");
    assert_expr(" all or empty or all ", "(all OR (empty OR all))");
    assert_expr("all and all or empty and all", "((all AND all) OR (empty AND all))");
    assert_expr(
        "all and all or empty and all or empty and all",
        "((all AND all) OR ((empty AND all) OR (empty AND all)))"
    );
    assert_expr("all - all and empty - all", "((all - all) AND (empty - all))");
    assert_expr("all - all or empty - all", "((all - all) OR (empty - all))");
    assert_expr(
        "all - all and empty - empty or empty - all",
        "(((all - all) AND (empty - empty)) OR (empty - all))"
    );
    assert_expr(" ( all ) ", "all");
    assert_expr("( all and all ) - ( empty and all )", "((all AND all) - (empty AND all))");
    assert_expr("( all and all ) - ( empty or all ) ", "((all AND all) - (empty OR all))");
    assert_expr("get line <- all", "(GET line <- all)");
    assert_expr(
        "get line <- empty and get stop_area <- all",
        "(GET line <- (empty AND (GET stop_area <- all)))"
    );
}

BOOST_AUTO_TEST_CASE(parse_str) {
    assert_expr(R"#(a.a(""))#", R"#(a.a(""))#");
    assert_expr(R"#(a.a("foo"))#", R"#(a.a("foo"))#");
    assert_expr(R"#(a.a("\""))#", R"#(a.a("\""))#");
    assert_expr(R"#(a.a("\\"))#", R"#(a.a("\\"))#");
    assert_expr(R"#(a.a("  "))#", R"#(a.a("  "))#");    
}
