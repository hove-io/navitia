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
#include <boost/test/unit_test.hpp>
#include "ed/build_helper.h"
#include "type/pb_converter.h"
#include "utils/logger.h"
#include "tests/utils_test.h"
#include "ptreferential/ptreferential_api.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(frequency_vehicle_journeys_test) {
    ed::builder b("20190101");

    b.vj("L1", "10000001").name("vj:0")("stop1", "8:00"_t, "8:00"_t)("stop2", "8:05"_t, "8:05"_t);

    b.frequency_vj("L1", "10:00:00"_t, "11:00:00"_t, "00:30:00"_t, "", "10000001")
        .name("vj:1")("stop1", "09:00:00"_t, "10:00:00"_t)("stop2", "10:05:00"_t, "10:05:00"_t);
    b.make();

    auto* data = b.data.get();
    uint depth = 3;
    uint start_page = 0;
    uint count = 10;

    navitia::PbCreator pb_creator(data, bt::second_clock::universal_time(), null_time_period);
    navitia::ptref::query_pb(pb_creator, nt::Type_e::VehicleJourney, {}, {}, nt::OdtLevel_e::all, depth, start_page,
                             count, boost::make_optional(true, "20190101T000000"_dt),
                             boost::make_optional(true, "20190102T000000"_dt), navitia::type::RTLevel::Base, *data);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys().size(), 2);
    auto vehicle_journey = resp.vehicle_journeys(0);
    BOOST_CHECK_EQUAL(vehicle_journey.uri(), "vehicle_journey:vj:0");
    BOOST_REQUIRE_EQUAL(vehicle_journey.has_start_time(), false);
    BOOST_REQUIRE_EQUAL(vehicle_journey.has_end_time(), false);
    BOOST_REQUIRE_EQUAL(vehicle_journey.has_headway_secs(), false);
    vehicle_journey = resp.vehicle_journeys(1);
    BOOST_CHECK_EQUAL(vehicle_journey.uri(), "vehicle_journey:vj:1");
    // frequency vehicle journey case
    // start_time, end_time, headway_secs field are added
    BOOST_REQUIRE_EQUAL(vehicle_journey.has_start_time(), true);
    BOOST_CHECK_EQUAL(vehicle_journey.start_time(), "10:00:00"_t);
    BOOST_REQUIRE_EQUAL(vehicle_journey.has_end_time(), true);
    BOOST_CHECK_EQUAL(vehicle_journey.end_time(), "11:00:00"_t);
    BOOST_REQUIRE_EQUAL(vehicle_journey.has_headway_secs(), true);
    BOOST_CHECK_EQUAL(vehicle_journey.headway_secs(), "00:30:00"_t);
}

/*
 * Test to check if a stop_point served by only one of 2 vehicle_journeys of the same route is linked only
 * to the good one, not all of them when using PTRef
 *
 * StopPoints (3) :  sp0          Journeys (2) :  vj0 (sp0) ──── (sp1)          Map (1) :                 ┌── sp1 (8:05)
 *                   sp1                          vj1 (sp0) ──── (sp2)                     sp0 (8:00) ────┤
 *                   sp2                                                                                  └── sp2 (8:05)
 */
BOOST_AUTO_TEST_CASE(stop_points_in_vehicle_journeys_test) {
    ed::builder b("20190101");
    b.vj("L1").name("vj0")("sp0", "8:00"_t)("sp1", "8:05"_t);
    b.vj("L1").name("vj1")("sp0", "8:00"_t)("sp2", "8:05"_t);
    b.make();

    // Check that stop_points are correctly linked to the route
    auto* data = b.data.get();
    uint depth = 3;
    uint start_page = 0;
    uint count = 10;

    navitia::PbCreator pb_creator_route(data, bt::second_clock::universal_time(), null_time_period);
    navitia::ptref::query_pb(pb_creator_route, nt::Type_e::StopPoint, "route.uri=L1:0", {}, nt::OdtLevel_e::all, depth,
                             start_page, count, boost::make_optional("20190101T000000"_dt),
                             boost::make_optional("20190102T000000"_dt), navitia::type::RTLevel::Base, *data);
    const auto resp_route = pb_creator_route.get_response();
    BOOST_REQUIRE_EQUAL(resp_route.stop_points().size(), 3);
    BOOST_CHECK_EQUAL(resp_route.stop_points(0).uri(), "sp0");
    BOOST_CHECK_EQUAL(resp_route.stop_points(1).uri(), "sp1");
    BOOST_CHECK_EQUAL(resp_route.stop_points(2).uri(), "sp2");

    // Check that vj0 is only linked to its 2 stop_points, no more no less
    navitia::PbCreator pb_creator_vj(data, bt::second_clock::universal_time(), null_time_period);
    navitia::ptref::query_pb(pb_creator_vj, nt::Type_e::StopPoint, "vehicle_journey.uri=vehicle_journey:vj0", {},
                             nt::OdtLevel_e::all, depth, start_page, count, boost::make_optional("20190101T000000"_dt),
                             boost::make_optional("20190102T000000"_dt), navitia::type::RTLevel::Base, *data);
    const auto resp_vj = pb_creator_vj.get_response();
    BOOST_REQUIRE_EQUAL(resp_vj.stop_points().size(), 2);
    BOOST_CHECK_EQUAL(resp_vj.stop_points(0).uri(), "sp0");
    BOOST_CHECK_EQUAL(resp_vj.stop_points(1).uri(), "sp1");

    // Check that only vj1 is linked to sp2
    navitia::PbCreator pb_creator_sp2(data, bt::second_clock::universal_time(), null_time_period);
    navitia::ptref::query_pb(pb_creator_sp2, nt::Type_e::VehicleJourney, "stop_point.uri=sp2", {}, nt::OdtLevel_e::all,
                             depth, start_page, count, boost::make_optional("20190101T000000"_dt),
                             boost::make_optional("20190102T000000"_dt), navitia::type::RTLevel::Base, *data);
    const auto resp_sp2 = pb_creator_sp2.get_response();
    BOOST_REQUIRE_EQUAL(resp_sp2.vehicle_journeys().size(), 1);
    BOOST_CHECK_EQUAL(resp_sp2.vehicle_journeys(0).uri(), "vehicle_journey:vj1");
    BOOST_CHECK_EQUAL(resp_sp2.vehicle_journeys(0).name(), "vj1");
    BOOST_CHECK_EQUAL(resp_sp2.vehicle_journeys(0).headsign(), "vj1");

    // Check that both vj0 and vj1 are linked to sp0
    navitia::PbCreator pb_creator_sp0(data, bt::second_clock::universal_time(), null_time_period);
    navitia::ptref::query_pb(pb_creator_sp0, nt::Type_e::VehicleJourney, "stop_point.uri=sp0", {}, nt::OdtLevel_e::all,
                             depth, start_page, count, boost::make_optional("20190101T000000"_dt),
                             boost::make_optional("20190102T000000"_dt), navitia::type::RTLevel::Base, *data);
    const auto resp_sp0 = pb_creator_sp0.get_response();
    BOOST_REQUIRE_EQUAL(resp_sp0.vehicle_journeys().size(), 2);
    BOOST_CHECK_EQUAL(resp_sp0.vehicle_journeys(0).uri(), "vehicle_journey:vj0");
    BOOST_CHECK_EQUAL(resp_sp0.vehicle_journeys(0).name(), "vj0");
    BOOST_CHECK_EQUAL(resp_sp0.vehicle_journeys(0).headsign(), "vj0");
    BOOST_CHECK_EQUAL(resp_sp0.vehicle_journeys(1).uri(), "vehicle_journey:vj1");
}
