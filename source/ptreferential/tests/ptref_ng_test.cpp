/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ptref_ng_test

#include <boost/test/unit_test.hpp>
#include "tests/utils_test.h"
#include "ptreferential/ptreferential_ng.h"
#include "ptreferential/ptreferential.h"
#include "ed/build_helper.h"
#include "type/pt_data.h"
#include "kraken/apply_disruption.h"

#include <boost/range/algorithm/transform.hpp>
#include <memory>

using namespace navitia::ptref;
using boost::posix_time::time_period;
using navitia::type::make_indexes;
using navitia::type::OdtLevel_e;
using navitia::type::RTLevel;
using navitia::type::Type_e;
using navitia::type::disruption::Tag;

namespace {
static void assert_expr(const std::string& to_parse, const std::string& expected) {
    const ast::Expr expr = parse(to_parse);
    std::stringstream ss;
    ss << expr;
    BOOST_CHECK_EQUAL(ss.str(), expected);
}

std::string get_disruption_name(navitia::idx_t id, const navitia::type::Data& data) {
    auto impact = data.pt_data->disruption_holder.get_impact(id);
    BOOST_REQUIRE(impact && impact->disruption);
    return impact->disruption->uri;
}
}  // namespace

BOOST_AUTO_TEST_CASE(parse_pred) {
    assert_expr("  all  ", "all");
    assert_expr("  empty  ", "empty");
    assert_expr("line . a ( ) ", "line.a()");
    assert_expr(
        R"#(vehicle_journey . has_code ( external_code , "OIF:42" ) )#",
        R"#(vehicle_journey.has_code("external_code", "OIF:42"))#");
    assert_expr(
        R"#(vehicle_journey . has_code_type ( external_code ) )#",
        R"#(vehicle_journey.has_code_type("external_code"))#");
    assert_expr(
        R"#(route . has_direction_type ( forward ) )#",
        R"#(route.has_direction_type("forward"))#");
    assert_expr(R"#(stop_area . uri ( "OIF:42" ) )#", R"#(stop_area.uri("OIF:42"))#");
    assert_expr(R"#(stop_area . uri = "OIF:42" )#", R"#(stop_area.uri("OIF:42"))#");
    assert_expr(R"#(stop_area . uri = OIF:42 )#", R"#(stop_area.uri("OIF:42"))#");
    assert_expr(R"#(stop_area . uri = foo )#", R"#(stop_area.uri("foo"))#");
    assert_expr(R"#(stop_area . uri = 42 )#", R"#(stop_area.uri("42"))#");
    assert_expr(R"#(stop_area . uri = 1ee7_: )#", R"#(stop_area.uri("1ee7_:"))#");
    assert_expr(
        R"#(stop_point.within(42, -2.2;4.9e-2))#",
        R"#(stop_point.within("42", "-2.2;4.9e-2"))#");
    assert_expr(
        R"#(stop_point.coord DWITHIN (-2.2, 4.9e-2, 42))#",
        R"#(stop_point.within("42", "-2.2;4.9e-2"))#");
}

BOOST_AUTO_TEST_CASE(parse_expr) {
    assert_expr(" all - empty - all ", "((all - empty) - all)");
    assert_expr(" all and empty and all ", "((all AND empty) AND all)");
    assert_expr(" all or empty or all ", "((all OR empty) OR all)");
    assert_expr("all and all or empty and all", "((all AND all) OR (empty AND all))");
    assert_expr("all and all or empty and all or empty and all",
                "(((all AND all) OR (empty AND all)) OR (empty AND all))");
    assert_expr("all - all and empty - all", "((all - all) AND (empty - all))");
    assert_expr("all - all or empty - all", "((all - all) OR (empty - all))");
    assert_expr("all - all and empty - empty or empty - all", "(((all - all) AND (empty - empty)) OR (empty - all))");
    assert_expr(" ( all ) ", "all");
    assert_expr("( all and all ) - ( empty and all )", "((all AND all) - (empty AND all))");
    assert_expr("( all and all ) - ( empty or all ) ", "((all AND all) - (empty OR all))");
    assert_expr("get line <- all", "(GET line <- all)");
    assert_expr("get line <- empty and get stop_area <- all", "(GET line <- (empty AND (GET stop_area <- all)))");
}

BOOST_AUTO_TEST_CASE(parse_str) {
    assert_expr(R"#(a.a(""))#", R"#(a.a(""))#");
    assert_expr(R"#(a.a("foo"))#", R"#(a.a("foo"))#");
    assert_expr(R"#(a.a("\""))#", R"#(a.a("\""))#");
    assert_expr(R"#(a.a("\\"))#", R"#(a.a("\\"))#");
    assert_expr(R"#(a.a("  "))#", R"#(a.a("  "))#");
    BOOST_CHECK_THROW(parse(R"#(a.a=\"42)#"), parsing_error);  // unclosed "
}

BOOST_AUTO_TEST_CASE(parse_legacy_tests) {
    assert_expr("stop_areas.uri=42", "stop_areas.uri(\"42\")");
    assert_expr("line.code=42", "line.code(\"42\")");
    assert_expr("  stop_areas.uri    =  42    ", "stop_areas.uri(\"42\")");
    assert_expr("stop_areas.uri = 42 and  line.name=bus", "(stop_areas.uri(\"42\") AND line.name(\"bus\"))");

    // escaped values
    assert_expr("stop_areas.uri=\"42\"", "stop_areas.uri(\"42\")");
    assert_expr("stop_areas.uri=\"4-2\"", "stop_areas.uri(\"4-2\")");
    assert_expr("stop_areas.uri=\"4|2\"", "stop_areas.uri(\"4|2\")");
    assert_expr("stop_areas.uri=\"42.\"", "stop_areas.uri(\"42.\")");
    assert_expr("stop_areas.uri=\"42 12\"", "stop_areas.uri(\"42 12\")");
    assert_expr("stop_areas.uri=\"  42  \"", "stop_areas.uri(\"  42  \")");
    assert_expr("stop_areas.uri=\"4&2\"", "stop_areas.uri(\"4&2\")");

    // exception
    BOOST_CHECK_THROW(parse(""), parsing_error);
    BOOST_CHECK_THROW(parse("mouuuhh bliiii"), parsing_error);
    BOOST_CHECK_THROW(parse("stop_areas.uri==42"), parsing_error);

    // escaped string
    assert_expr("stop_areas.uri=\"bob the coolest\"", "stop_areas.uri(\"bob the coolest\")");
    assert_expr("stop_areas.uri=\"bob the coolést\"", "stop_areas.uri(\"bob the coolést\")");
    assert_expr(R"(stop_areas.uri="bob the \"coolést\"")", R"(stop_areas.uri("bob the \"coolést\""))");
    assert_expr(R"(stop_areas.uri="bob the \\ er")", R"(stop_areas.uri("bob the \\ er"))");
    assert_expr(R"(vehicle_journey.has_headsign("john"))", R"(vehicle_journey.has_headsign("john"))");
    assert_expr(R"(vehicle_journey.has_disruption())", R"(vehicle_journey.has_disruption())");
}

BOOST_AUTO_TEST_CASE(parse_deprecated_features) {
    BOOST_CHECK_THROW(parse("stop_areas HAVING (line.uri=1)"), parsing_error);
    BOOST_CHECK_THROW(parse("line.uri>=2"), parsing_error);
    BOOST_CHECK_THROW(parse("line.uri<=2"), parsing_error);
    BOOST_CHECK_THROW(parse("line.uri<>2"), parsing_error);
    BOOST_CHECK_THROW(parse("AFTER(stop_area.uri=stop2)"), parsing_error);
}

static void assert_odt_level(const Type_e requested_type,
                             const std::string& request,
                             const OdtLevel_e odt_level,
                             const std::string& expected) {
    const auto req = make_request(requested_type, request, {}, odt_level, {}, {}, {}, {});
    assert_expr(req, expected);
}

BOOST_AUTO_TEST_CASE(make_request_odt_level) {
    assert_odt_level(Type_e::VehicleJourney, "", OdtLevel_e::all, "all");
    assert_odt_level(Type_e::VehicleJourney, "", OdtLevel_e::zonal, "all");
    assert_odt_level(Type_e::Line, "", OdtLevel_e::all, "all");
    assert_odt_level(Type_e::Line, "", OdtLevel_e::zonal, "(all AND line.odt_level(\"zonal\"))");
    assert_odt_level(Type_e::Line, "all or all", OdtLevel_e::zonal, "((all OR all) AND line.odt_level(\"zonal\"))");
}

static void assert_is_active_vj(const Type_e requested_type,
                                const std::string& request,
                                const RTLevel rt_level,
                                const std::string& now,
                                const std::string& expected) {
    boost::optional<boost::posix_time::ptime> current_datetime = boost::posix_time::from_iso_string(now);
    const auto req = make_request(requested_type, request, {}, {}, {}, {}, rt_level, {}, current_datetime);
    assert_expr(req, expected);
}

BOOST_AUTO_TEST_CASE(make_request_is_active_vj) {
    assert_is_active_vj(Type_e::VehicleJourney, "", navitia::type::RTLevel::Base, "20210910T151500",
                        R"((all AND vehicle_journey.active_at("20210910T151500Z", "base_schedule")))");
    assert_is_active_vj(Type_e::VehicleJourney, "", navitia::type::RTLevel::RealTime, "20210910T151500",
                        R"((all AND vehicle_journey.active_at("20210910T151500Z", "realtime")))");
}

static void assert_since_until(const Type_e requested_type,
                               const std::string& request,
                               const std::string& since_str,
                               const std::string& until_str,
                               const RTLevel rt_level,
                               const std::string& expected) {
    boost::optional<boost::posix_time::ptime> since, until;
    if (!since_str.empty()) {
        since = boost::posix_time::from_iso_string(since_str);
    }
    if (!until_str.empty()) {
        until = boost::posix_time::from_iso_string(until_str);
    }
    const auto req = make_request(requested_type, request, {}, OdtLevel_e::all, since, until, rt_level, {});
    assert_expr(req, expected);
}

BOOST_AUTO_TEST_CASE(make_request_since_until) {
    assert_since_until(Type_e::Line, "", "", "", RTLevel::Base, "all");
    assert_since_until(Type_e::Line, "", "20180714T1337", "20180714T1338", RTLevel::Base, "all");
    assert_since_until(Type_e::VehicleJourney, "", "20180714T1337", "20180714T1338", RTLevel::Base,
                       R"((all AND vehicle_journey.between("20180714T133700Z", "20180714T133800Z", "base_schedule")))");
    assert_since_until(Type_e::VehicleJourney, "", "20180714T1337", "", RTLevel::RealTime,
                       R"((all AND vehicle_journey.since("20180714T133700Z", "realtime")))");
    assert_since_until(Type_e::VehicleJourney, "", "", "20180714T1338", RTLevel::Base,
                       R"((all AND vehicle_journey.until("20180714T133800Z", "base_schedule")))");
    assert_since_until(Type_e::Impact, "", "20180714T1337", "20180714T1338", RTLevel::RealTime,
                       R"((all AND disruption.between("20180714T133700Z", "20180714T133800Z")))");
    assert_since_until(Type_e::Impact, "", "20180714T1337", "", RTLevel::Base,
                       R"((all AND disruption.since("20180714T133700Z")))");
    assert_since_until(Type_e::Impact, "", "", "20180714T1338", RTLevel::Base,
                       R"((all AND disruption.until("20180714T133800Z")))");
    assert_since_until(Type_e::Impact, "all or all", "", "20180714T1338", RTLevel::Adapted,
                       R"(((all OR all) AND disruption.until("20180714T133800Z")))");
}

static void assert_forbidden(const Type_e requested_type,
                             const std::string& request,
                             const std::vector<std::string>& forbidden,
                             const std::string& expected) {
    ed::builder b("20180710");
    b.generate_dummy_basis();
    b.finish();
    const auto req = make_request(requested_type, request, forbidden, OdtLevel_e::all, {}, {}, {}, *b.data);
    assert_expr(req, expected);
}

BOOST_AUTO_TEST_CASE(make_request_forbidden_uris) {
    assert_forbidden(Type_e::Line, "", {}, "all");
    assert_forbidden(Type_e::Line, "", {"Bus"}, R"((all - commercial_mode.id("Bus")))");
    assert_forbidden(Type_e::Line, "", {"Bus", "0x1"},
                     R"((all - (commercial_mode.id("Bus") OR commercial_mode.id("0x1"))))");
    assert_forbidden(Type_e::Line, "all or all", {"Bus", "0x1"},
                     R"(((all OR all) - (commercial_mode.id("Bus") OR commercial_mode.id("0x1"))))");
}

BOOST_AUTO_TEST_CASE(make_request_odt_level_forbidden_uris) {
    ed::builder b("20180710");
    b.generate_dummy_basis();
    b.finish();
    const auto req = make_request(Type_e::Line, "all or all", {"Bus", "0x1"}, OdtLevel_e::zonal, {}, {}, {}, *b.data);
    assert_expr(
        req,
        R"((((all OR all) AND line.odt_level("zonal")) - (commercial_mode.id("Bus") OR commercial_mode.id("0x1"))))");
}

BOOST_AUTO_TEST_CASE(make_request_since_forbidden_uris) {
    ed::builder b("20180710");
    b.generate_dummy_basis();
    b.finish();
    const auto req = make_request(Type_e::Impact, "all or all", {"Bus", "0x1"}, OdtLevel_e::all,
                                  boost::posix_time::from_iso_string("20180714T1337"), {}, {}, *b.data);
    assert_expr(
        req,
        R"((((all OR all) AND disruption.since("20180714T133700Z")) - (commercial_mode.id("Bus") OR commercial_mode.id("0x1"))))");
}

BOOST_AUTO_TEST_CASE(ng_specific_features) {
    ed::builder b("20180710");
    b.vj("A")("stop0", 700)("stop1", 800)("stop2", 900);
    b.vj("B")("stop2", 700)("stop3", 800)("stop4", 900);
    b.vj("C")("stop1", 700)("stop3", 800)("stop5", 900);
    b.make();
    auto rt_level = navitia::type::RTLevel::Base;
    auto indexes = make_query_ng(Type_e::StopArea, "get vehicle_journey <- stop_area.id=stop2", {}, OdtLevel_e::all, {},
                                 {}, rt_level, *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, make_indexes({0, 1, 2, 3, 4}));

    indexes = make_query_ng(Type_e::StopArea, "(get vehicle_journey <- stop_area.id=stop2) - stop_area.id=stop2", {},
                            OdtLevel_e::all, {}, {}, rt_level, *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, make_indexes({0, 1, 3, 4}));

    indexes = make_query_ng(Type_e::StopArea, "all", {}, OdtLevel_e::all, {}, {}, rt_level, *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, make_indexes({0, 1, 2, 3, 4, 5}));

    indexes = make_query_ng(Type_e::StopArea, "empty", {}, OdtLevel_e::all, {}, {}, rt_level, *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, make_indexes({}));

    indexes = make_query_ng(Type_e::StopArea, "all and empty", {}, OdtLevel_e::all, {}, {}, rt_level, *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, make_indexes({}));

    indexes = make_query_ng(Type_e::StopArea, "stop_area.id=stop2 or stop_area.id=stop5", {}, OdtLevel_e::all, {}, {},
                            rt_level, *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, make_indexes({2, 5}));
}

BOOST_AUTO_TEST_CASE(get_connection) {
    ed::builder b("20180710");
    b.vj("A")("stop0", 700)("stop1", 800);
    b.vj("B")("stop2", 900)("stop3", 1000);
    b.connection("stop1", "stop2", 50);
    b.connection("stop2", "stop1", 50);
    b.make();

    auto indexes = make_query_ng(Type_e::Line, "(get connection <- line.id=A) - line.id=A", {}, OdtLevel_e::all, {}, {},
                                 navitia::type::RTLevel::Base, *b.data);
    BOOST_REQUIRE_EQUAL(indexes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.at(*indexes.begin())->uri, "B");
}

BOOST_AUTO_TEST_CASE(get_disruption_by_tag) {
    ed::builder b("20180710");

    navitia::apply_disruption(b.disrupt(nt::RTLevel::RealTime, "disrupt_0")
                                  .tag("my_tag")
                                  .impact()
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    b.make();

    auto indexes = make_query_ng(Type_e::Impact, "disruption.tag=\"my_tag name\"", {}, OdtLevel_e::all, {}, {},
                                 navitia::type::RTLevel::Base, *b.data);

    BOOST_REQUIRE_EQUAL(indexes.size(), 1);

    auto idx = *indexes.begin();
    BOOST_CHECK_EQUAL(get_disruption_name(idx, *b.data), "disrupt_0");
}

BOOST_AUTO_TEST_CASE(get_disruptions_with_multiple_tags) {
    ed::builder b("20180710");

    navitia::apply_disruption(b.disrupt(nt::RTLevel::RealTime, "disrupt_0")
                                  .tag("tag_0")
                                  .impact()
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    navitia::apply_disruption(b.disrupt(nt::RTLevel::RealTime, "disrupt_1")
                                  .tag("tag_1")
                                  .impact()
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    navitia::apply_disruption(b.disrupt(nt::RTLevel::RealTime, "disrupt_2")
                                  .tag("tag_2")
                                  .impact()
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    b.make();

    auto indexes = make_query_ng(Type_e::Impact, "disruption.tags(\"tag_0 name\", \"tag_2 name\")", {}, OdtLevel_e::all,
                                 {}, {}, navitia::type::RTLevel::Base, *b.data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    std::vector<std::string> disruption_names;
    boost::range::transform(indexes, std::back_inserter(disruption_names),
                            [&b](const navitia::idx_t id) { return get_disruption_name(id, *b.data); });

    auto expected_disruption_names = {"disrupt_0", "disrupt_2"};
    BOOST_CHECK_EQUAL_RANGE(disruption_names, expected_disruption_names);
}
