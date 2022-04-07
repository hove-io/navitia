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
#define BOOST_TEST_MODULE equipment_test

#include <boost/test/unit_test.hpp>

#include "ed/build_helper.h"
#include "equipment/equipment_api.h"
#include "ptreferential/ptreferential.h"
#include "utils/logger.h"
#include "type/pt_data.h"
#include "tests/utils_test.h"

#include <string>
#include <vector>
#include <map>
#include <set>

using namespace navitia;
using boost::posix_time::time_period;
using std::map;
using std::multiset;
using std::pair;
using std::set;
using std::string;
using std::vector;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace {

class EquipmentTestFixture {
public:
    ed::builder b;
    navitia::PbCreator pb_creator;
    const type::Data& data;

    EquipmentTestFixture() : b("20190101"), data(*b.data) {
        b.sa("sa1")("stop1", {{"CodeType1", {"code1", "code2"}}});
        b.sa("sa2")("stop2", {{"CodeType3", {"code5"}}});
        b.sa("sa3")("stop3", {{"CodeType1", {"code6"}}});
        b.sa("sa4")("stop4", {});
        b.sa("sa5")("stop5", {{"CodeType3", {"code7"}}});
        b.sa("sa6")("stop6", {{"CodeType1", {"code8"}}});

        b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250)("stop3", 9000, 9050);
        b.vj("B")("stop4", 8000, 8050)("stop5", 8200, 8250)("stop6", 9000, 9050);
        b.make();

        const ptime since = "20190101T000000"_dt;
        const ptime until = "21190101T000000"_dt;
        pb_creator.init(&data, since, time_period(since, until));
    }
};

BOOST_FIXTURE_TEST_CASE(test_stop_point_codes_creation, EquipmentTestFixture) {
    type::Indexes idx = ptref::make_query(nt::Type_e::StopPoint, "(stop_point.has_code_type(CodeType1))", data);

    set<string> stop_points_uris = get_uris<type::StopPoint>(idx, data);

    BOOST_CHECK_EQUAL(stop_points_uris.size(), 3);
    BOOST_CHECK_EQUAL_RANGE(stop_points_uris, (set<string>{"stop1", "stop3", "stop6"}));
}

BOOST_FIXTURE_TEST_CASE(test_stop_area_query, EquipmentTestFixture) {
    type::Indexes idx = ptref::make_query(nt::Type_e::StopArea, "(stop_point.has_code_type(CodeType1))", data);

    set<string> stop_area_uris = get_uris<type::StopArea>(idx, data);

    BOOST_CHECK_EQUAL(stop_area_uris.size(), 3);
    BOOST_CHECK_EQUAL_RANGE(stop_area_uris, (set<string>{"sa1", "sa3", "sa6"}));
}

BOOST_FIXTURE_TEST_CASE(test_stop_area_per_line_query, EquipmentTestFixture) {
    type::Indexes idx =
        ptref::make_query(nt::Type_e::StopArea, "(line.uri=A and stop_point.has_code_type(CodeType1))", data);

    set<string> stop_area_uris = get_uris<type::StopArea>(idx, data);

    BOOST_CHECK_EQUAL(stop_area_uris.size(), 2);
    BOOST_CHECK_EQUAL_RANGE(stop_area_uris, (set<string>{"sa1", "sa3"}));
}

BOOST_FIXTURE_TEST_CASE(equipment_report_get_total_lines, EquipmentTestFixture) {
    equipment::EquipmentReports er(data, "stop_point.has_code_type(CodeType1)");

    BOOST_CHECK_EQUAL(er.get_total_lines(), 0);
    er.get_paginated_equipment_report_list();
    BOOST_CHECK_EQUAL(er.get_total_lines(), 2);
}

BOOST_FIXTURE_TEST_CASE(equipment_report_get_report_list_should_return_the_stop_areas_per_line, EquipmentTestFixture) {
    equipment::EquipmentReports er(data, "stop_point.has_code_type(CodeType1)");
    auto reports = er.get_paginated_equipment_report_list();

    // Convert result from object pointers to uris
    map<string, set<string>> sa_per_line_uris;
    for (const auto& report : reports) {
        type::Line* line = std::get<0>(report);
        auto stop_area_equipments = std::get<1>(report);
        set<string> sa_uris;
        for (const auto& sa_equip : stop_area_equipments) {
            type::StopArea* sa = std::get<0>(sa_equip);
            sa_uris.insert(sa->uri);
        }
        sa_per_line_uris[line->uri] = sa_uris;
    }

    map<string, set<string>> expected_uris{
        {"A", {"sa1", "sa3"}},
        {"B", {"sa6"}},
    };
    BOOST_CHECK_EQUAL_RANGE(sa_per_line_uris, expected_uris);
}

BOOST_FIXTURE_TEST_CASE(get_report_list_should_return_the_correct_stop_points, EquipmentTestFixture) {
    equipment::EquipmentReports er(data, "stop_point.has_code_type(CodeType1)");
    auto reports = er.get_paginated_equipment_report_list();

    // Convert result from object pointers to uris
    multiset<string> stop_points_with_equipment;
    for (const auto& report : reports) {
        auto stop_area_equipments = std::get<1>(report);
        for (const auto& sa_equip : stop_area_equipments) {
            auto stop_points = std::get<1>(sa_equip);
            for (const type::StopPoint* sp : stop_points) {
                stop_points_with_equipment.insert(sp->uri);
            }
        }
    }

    multiset<string> expected_sp = {"stop1", "stop3", "stop6"};
    BOOST_CHECK_EQUAL_RANGE(stop_points_with_equipment, expected_sp);
}

BOOST_FIXTURE_TEST_CASE(equipment_report_should_forbid_uris, EquipmentTestFixture) {
    equipment::EquipmentReports er(data, "stop_point.has_code_type(CodeType1)", 10, 0, {"A"});
    auto reports = er.get_paginated_equipment_report_list();

    // Stop Areas "A" and "B" have cpde "CodeType1", but "A" has been forbidden,
    // Hense the fact that only "B" is returned
    BOOST_REQUIRE_EQUAL(reports.size(), 1);
    BOOST_CHECK_EQUAL(std::get<0>(reports[0])->uri, "B");
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_test_api, EquipmentTestFixture) {
    const auto filter = "stop_point.has_code_type(CodeType1)";
    equipment::equipment_reports(pb_creator, filter, 10);

    BOOST_CHECK_EQUAL(pb_creator.equipment_reports_size(), 2);
    const auto resp = pb_creator.get_response();

    map<string, set<string>> sa_per_line_uris;
    for (const auto& equip_rep : resp.equipment_reports()) {
        BOOST_REQUIRE(equip_rep.has_line());
        set<string> stop_area_uris;
        for (const auto& sae : equip_rep.stop_area_equipments()) {
            BOOST_REQUIRE(sae.has_stop_area());
            stop_area_uris.emplace(sae.stop_area().uri());
        }
        sa_per_line_uris[equip_rep.line().uri()] = stop_area_uris;
    }

    map<string, set<string>> expected_uris{
        {"A", {"sa1", "sa3"}},
        {"B", {"sa6"}},
    };
    BOOST_CHECK_EQUAL_RANGE(sa_per_line_uris, expected_uris);
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_should_fail_on_bad_filter, EquipmentTestFixture) {
    const auto filter = "this filter is just nonsense";
    equipment::equipment_reports(pb_creator, filter, 10);

    BOOST_CHECK_NO_THROW();
    BOOST_CHECK(pb_creator.has_error());
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_should_paginate_page_0, EquipmentTestFixture) {
    const auto filter = "stop_point.has_code_type(CodeType1)";
    // PAGE 0 - COUNT = 1
    equipment::equipment_reports(pb_creator, filter, 1, 0, 0);
    BOOST_REQUIRE_EQUAL(pb_creator.equipment_reports_size(), 1);
    auto line_A_uri = pb_creator.get_response().equipment_reports(0).line().uri();
    BOOST_CHECK_EQUAL(line_A_uri, "A");
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_should_paginate_page_1, EquipmentTestFixture) {
    const auto filter = "stop_point.has_code_type(CodeType1)";
    // PAGE 1 - COUNT = 1
    equipment::equipment_reports(pb_creator, filter, 1, 0, 1);
    BOOST_REQUIRE_EQUAL(pb_creator.equipment_reports_size(), 1);
    auto line_A_uri = pb_creator.get_response().equipment_reports(0).line().uri();
    BOOST_CHECK_EQUAL(line_A_uri, "B");
}

BOOST_FIXTURE_TEST_CASE(equipment_reports_should_have_stop_points_codes, EquipmentTestFixture) {
    const auto filter = "stop_point.has_code_type(CodeType1)";
    equipment::equipment_reports(pb_creator, filter, 1);

    /*
     * With count = 1, we'll only get stop_areas/stop_points from the 1st line ("A")
     * This means we'll only populate codes from stop1 and stop3,
     * as they both have code type "CodeType1"
     */
    BOOST_REQUIRE_EQUAL(pb_creator.equipment_reports_size(), 1);

    multiset<string> codes;
    for (const auto& sae : pb_creator.get_response().equipment_reports(0).stop_area_equipments()) {
        for (const auto& sp : sae.stop_area().stop_points()) {
            for (const auto& c : sp.codes()) {
                codes.emplace(c.value());
            }
        }
    }

    multiset<string> expected_codes = {"code1", "code2", "code6"};
    BOOST_CHECK_EQUAL_RANGE(codes, expected_codes);
}

}  // namespace
