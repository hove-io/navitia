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
#define BOOST_TEST_MODULE test_ed

#include "ed/types.h"
#include "ed/data.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"

#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm.hpp>
#include <string>
#include <algorithm>

using namespace ed;
using namespace navitia::test;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

struct TestEdData {
    Data d;
    std::vector<types::VehicleJourney*> vjs;

    TestEdData() {
        // clang-format off
        std::vector<types::ValidityPattern*> vps{
            new types::ValidityPattern(),
        };
        vjs = {
            new types::VehicleJourney(0, "A",   vps[0]),
            new types::VehicleJourney(1, "B",   vps[0]),
            new types::VehicleJourney(2, "C",   vps[0]),
            new types::VehicleJourney(3, "D",   vps[0]),
            new types::VehicleJourney(4, "Orphan", vps[0]),
        };
        std::vector<types::StopArea*> sas {
            new types::StopArea(0, "sa0"),
        };
        std::vector<types::StopPoint*> sps {
            new types::StopPoint(0, "A_sp0", sas[0]),
            new types::StopPoint(1, "AB_sp1", sas[0]),
            new types::StopPoint(2, "B_sp2", sas[0]),

            new types::StopPoint(3, "C_sp3", sas[0]),
            new types::StopPoint(4, "C_sp4", sas[0]),
            new types::StopPoint(5, "D_sp5", sas[0]),
            new types::StopPoint(6, "D_sp6", sas[0]),

            new types::StopPoint(7, "Orphan_sp", sas[0]),

        };
        std::vector<types::StopTime*> sts {
            //                  idx,arr,dep,boa,ali,vj,     sp,     shape,   order, ODT,   pick, drop, freq,  wheel, estim, headsign
            // Stay-in on same stop point (sp1)
            new types::StopTime( 0, 00, 01, 00, 01, vjs[0], sps[0], nullptr, 0,     false, true, true, false, false, false, false, "" ),
            new types::StopTime( 1, 10, 21, 10, 11, vjs[0], sps[1], nullptr, 1,     false, true, true, false, false, false, false, "" ),
            new types::StopTime( 2, 10, 21, 10, 21, vjs[1], sps[1], nullptr, 2,     false, true, true, false, false, false, false, "" ),
            new types::StopTime( 3, 30, 31, 30, 31, vjs[1], sps[2], nullptr, 3,     false, true, true, false, false, false, false, "" ),

            // Stay-in on different stop points (sp4->sp5)
            new types::StopTime( 4, 00, 01, 00, 01, vjs[2], sps[3], nullptr, 0,     false, true, true, false, false, false, false, "" ),
            new types::StopTime( 5, 10, 11, 10, 11, vjs[2], sps[4], nullptr, 1,     false, true, true, false, false, false, false, "" ),
            new types::StopTime( 6, 20, 21, 20, 21, vjs[3], sps[5], nullptr, 2,     false, true, true, false, false, false, false, "" ),
            new types::StopTime( 7, 30, 31, 30, 31, vjs[3], sps[6], nullptr, 3,     false, true, true, false, false, false, false, "" ),

            // Classical VJ with no stay-in
            new types::StopTime( 8, 30, 31, 30, 31, vjs[4], sps[7], nullptr, 0,     false, false,false,false, false, false, false, "" ),
        };
        // clang-format on

        vjs[0]->block_id = vjs[1]->block_id = "123";
        vjs[0]->stop_time_list = {sts[0], sts[1]};
        vjs[1]->stop_time_list = {sts[2], sts[3]};

        vjs[2]->block_id = vjs[3]->block_id = "678";
        vjs[2]->stop_time_list = {sts[4], sts[5]};
        vjs[3]->stop_time_list = {sts[6], sts[7]};

        vjs[4]->stop_time_list = {sts[8]};

        d.vehicle_journeys = vjs;
        d.validity_patterns = vps;
        d.stop_points = sps;
        d.stops = sts;
        d.stop_areas = sas;
        d.complete();
    }
};

BOOST_FIXTURE_TEST_CASE(vjs_with_same_block_id_should_be_linked, TestEdData) {
    BOOST_CHECK(vjs[0]->prev_vj == nullptr);
    BOOST_CHECK_EQUAL(vjs[0]->next_vj, vjs[1]);
    BOOST_CHECK_EQUAL(vjs[1]->prev_vj, vjs[0]);
    BOOST_CHECK(vjs[1]->next_vj == nullptr);

    BOOST_CHECK(vjs[2]->prev_vj == nullptr);
    BOOST_CHECK_EQUAL(vjs[2]->next_vj, vjs[3]);
    BOOST_CHECK_EQUAL(vjs[3]->prev_vj, vjs[2]);
    BOOST_CHECK(vjs[3]->next_vj == nullptr);
}

BOOST_FIXTURE_TEST_CASE(vehicle_journeys_with_stayin_at_start, TestEdData) {
    BOOST_CHECK_EQUAL(vjs[0]->starts_with_stayin_on_same_stop_point(), false);
    BOOST_CHECK_EQUAL(vjs[1]->starts_with_stayin_on_same_stop_point(), true);

    BOOST_CHECK_EQUAL(vjs[2]->starts_with_stayin_on_same_stop_point(), false);
    BOOST_CHECK_EQUAL(vjs[3]->starts_with_stayin_on_same_stop_point(), false);

    BOOST_CHECK_EQUAL(vjs[4]->starts_with_stayin_on_same_stop_point(), false);
}

BOOST_FIXTURE_TEST_CASE(vehicle_journeys_with_stayin_at_the_end, TestEdData) {
    BOOST_CHECK_EQUAL(vjs[0]->ends_with_stayin_on_same_stop_point(), true);
    BOOST_CHECK_EQUAL(vjs[1]->ends_with_stayin_on_same_stop_point(), false);

    BOOST_CHECK_EQUAL(vjs[2]->ends_with_stayin_on_same_stop_point(), false);
    BOOST_CHECK_EQUAL(vjs[3]->ends_with_stayin_on_same_stop_point(), false);

    BOOST_CHECK_EQUAL(vjs[4]->ends_with_stayin_on_same_stop_point(), false);
}
