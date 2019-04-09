/* Copyright © 2001-2019, Canal TP and/or its affiliates. All rights reserved.

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
#define BOOST_TEST_MODULE equipment_test

#include <string>
#include <vector>
#include <map>

#include <boost/range/algorithm/sort.hpp>
#include <boost/test/unit_test.hpp>

#include "ed/build_helper.h"
#include "equipment/equipment_api.h"
#include "ptreferential/ptreferential.h"
#include "utils/logger.h"
#include "type/type.h"
#include "type/pt_data.h"
#include "tests/utils_test.h"


using namespace navitia;
using std::set;
using std::string;
using std::map;
using boost::posix_time::time_period;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );

namespace {

class Test_fixture {
public:
    ed::builder b;
    navitia::PbCreator pb_creator;
    const type::Data& data;

    Test_fixture(): b("20190101"), data(*b.data) {

        b.sa("sa1")("stop1", {{"CodeType1", {"code1", "code2"}}} );
        b.sa("sa2")("stop2", {{"CodeType3", {"code5"}}} );
        b.sa("sa3")("stop3", {{"CodeType1", {"code6"}}} );
        b.sa("sa4")("stop4", {});
        b.sa("sa5")("stop5", {{"CodeType3", {"code7"}}} );
        b.sa("sa6")("stop6", {{"CodeType1", {"code8"}}} );

        b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250)("stop3", 9000,9050);
        b.vj("B")("stop4", 8000,8050)("stop5", 8200,8250)("stop6", 9000,9050);
        b.make();

        const ptime since = "20190101T000000"_dt;
        const ptime until = "21190101T000000"_dt;
        pb_creator.init(&data, since, time_period(since, until));
    }
};

BOOST_FIXTURE_TEST_CASE(test_stop_point_codes_creation, Test_fixture) {

    type::Indexes idx = ptref::make_query(nt::Type_e::StopPoint,
                        "(stop_point.has_code_type(CodeType1))",
                        data);

    set<string> stop_points_uris = get_uris<type::StopPoint>(idx, data);

    BOOST_CHECK_EQUAL(stop_points_uris.size(), 3);
    BOOST_CHECK_EQUAL_RANGE(stop_points_uris, (set<string>{"stop1", "stop3", "stop6"}));
}

BOOST_FIXTURE_TEST_CASE(test_stop_area_query, Test_fixture) {

    type::Indexes idx = ptref::make_query(nt::Type_e::StopArea,
                        "(stop_point.has_code_type(CodeType1))",
                        data);

    set<string> stop_area_uris = get_uris<type::StopArea>(idx, data);

    BOOST_CHECK_EQUAL(stop_area_uris.size(), 3);
    BOOST_CHECK_EQUAL_RANGE(stop_area_uris, (set<string>{"sa1", "sa3", "sa6"}));
}

BOOST_FIXTURE_TEST_CASE(test_stop_area_per_line_query, Test_fixture) {

    type::Indexes idx = ptref::make_query(nt::Type_e::StopArea,
                        "(line.uri=A and stop_point.has_code_type(CodeType1))",
                        data);

    set<string> stop_area_uris = get_uris<type::StopArea>(idx, data);

    BOOST_CHECK_EQUAL(stop_area_uris.size(), 2);
    BOOST_CHECK_EQUAL_RANGE(stop_area_uris, (set<string>{"sa1", "sa3"}));
}

BOOST_FIXTURE_TEST_CASE(equipment_report_get_lines, Test_fixture) {
    const auto filter = "stop_point.has_code_type(CodeType1)";
    equipment::StopAreasPerLine sas_per_line;
    size_t total_lines = 0;
    std::tie(sas_per_line, total_lines) = equipment::get_paginated_stop_areas_per_line(data, filter, 10);

    BOOST_CHECK_EQUAL(total_lines, 2);

    // Convert result from object pointers to uris
    map<string, set<string>> sa_per_line_uris;
    for(const auto & line : sas_per_line) {
        set<string> sa_uris;
        for(const auto & sa : line.second) { sa_uris.insert(sa->uri); }
        sa_per_line_uris[line.first->uri] = sa_uris;
    }

    map<string, set<string>> expected_uris {
        {"A", {"sa1", "sa3"}},
        {"B", {"sa6"}},
    };
    BOOST_CHECK_EQUAL_RANGE(sa_per_line_uris, expected_uris);
}

BOOST_FIXTURE_TEST_CASE(equipment_report_should_forbid_uris, Test_fixture) {
    auto filter = "stop_point.has_code_type(CodeType1)";
    equipment::StopAreasPerLine sas_per_line;
    std::tie(sas_per_line, std::ignore) = equipment::get_paginated_stop_areas_per_line(data, filter, 10, 0, {"A"});

    // Stop Areas "A" and "B" have cpde "CodeType1", but "A" has been forbidden,
    // Hense the fact that only "B" is returned
    BOOST_REQUIRE_EQUAL(sas_per_line.size(), 1);
    BOOST_REQUIRE_EQUAL(sas_per_line[0].first->uri, "B");
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_test_api, Test_fixture) {
    const auto filter = "stop_point.has_code_type(CodeType1)";
    equipment::equipment_reports(pb_creator, filter, 10);

    BOOST_CHECK_EQUAL(pb_creator.equipment_reports_size(), 2);
    const auto resp = pb_creator.get_response();

    map<string, set<string>> sa_per_line_uris;
    for(const auto equip_rep : resp.equipment_reports()) {
        BOOST_REQUIRE(equip_rep.has_line());
        set<string> stop_area_uris;
        for(const auto sae : equip_rep.stop_area_equipments()){
            BOOST_REQUIRE(sae.has_stop_area());
            stop_area_uris.emplace(sae.stop_area().uri());
        }
        sa_per_line_uris[equip_rep.line().uri()] = stop_area_uris;
    }

     map<string, set<string>> expected_uris {
        {"A", {"sa1", "sa3"}},
        {"B", {"sa6"}},
    };
    BOOST_CHECK_EQUAL_RANGE(sa_per_line_uris, expected_uris);
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_should_fail_on_bad_filter, Test_fixture) {
    const auto filter = "this filter is just nonsense";
    equipment::equipment_reports(pb_creator, filter, 10);

    BOOST_CHECK_NO_THROW();
    BOOST_CHECK(pb_creator.has_error());
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_should_paginate_page_0, Test_fixture) {
    const auto filter = "stop_point.has_code_type(CodeType1)";
    // PAGE 0 - COUNT = 1
    equipment::equipment_reports(pb_creator, filter, 1, 0, 0);
    BOOST_REQUIRE_EQUAL(pb_creator.equipment_reports_size(), 1);
    auto line_A_uri = pb_creator.get_response().equipment_reports(0).line().uri();
    BOOST_CHECK_EQUAL(line_A_uri, "A");
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_should_paginate_page_1, Test_fixture) {
    const auto filter = "stop_point.has_code_type(CodeType1)";
    // PAGE 1 - COUNT = 1
    equipment::equipment_reports(pb_creator, filter, 1, 0, 1);
    BOOST_REQUIRE_EQUAL(pb_creator.equipment_reports_size(), 1);
    auto line_A_uri = pb_creator.get_response().equipment_reports(0).line().uri();
    BOOST_CHECK_EQUAL(line_A_uri, "B");
}

} // namespace
