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
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();

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
 * Test to check if a stop_point presents in one over two vehicule_journey is not always returned.
 *
 * StopPoints (3) :  sp0          Journeys (2) :  vj0 (sp0) ──── (sp1)          Map (1) :                 ┌── sp1 (8:05)
 *                   sp1                          vj1 (sp0) ──── (sp2)                     sp0 (8:00) ────┤
 *                   sp2                                                                                  └── sp2 (8:05)
 *
 * This test should not return journeys with sp1 in it if we ask to get journey with sp2 and vice versa but both if
 * none of them is specified.
 */
BOOST_AUTO_TEST_CASE(stop_points_in_vehicle_journeys_test) {
    ed::builder b("20190101");
    b.vj("L1").name("vj0")("sp0", "8:00"_t)("sp1", "8:05"_t);
    b.vj("L1").name("vj1")("sp0", "8:00"_t)("sp2", "8:05"_t);
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();

    // Check if 2 vehicule_journey are present
    auto* data = b.data.get();
    uint depth = 3;
    uint start_page = 0;
    uint count = 10;

    navitia::PbCreator pb_creator(data, bt::second_clock::universal_time(), null_time_period);
    navitia::ptref::query_pb(pb_creator, nt::Type_e::VehicleJourney, {}, {}, nt::OdtLevel_e::all, depth, start_page,
                             count, boost::make_optional("20190101T000000"_dt),
                             boost::make_optional("20190102T000000"_dt), navitia::type::RTLevel::Base, *data);
    pbnavitia::Response const & resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys().size(), 2);
    BOOST_CHECK_EQUAL(resp.vehicle_journeys(0).uri(), "vehicle_journey:vj:0");
    BOOST_CHECK_EQUAL(resp.vehicle_journeys(1).uri(), "vehicle_journey:vj:1");

    // Check if the vehicule_journey with sp2 is not present
    data = b.data.get();
    depth = 3;
    start_page = 0;
    count = 10;

    navitia::PbCreator pb_creator2(data, bt::second_clock::universal_time(), null_time_period);
    navitia::ptref::query_pb(pb_creator2, nt::Type_e::VehicleJourney, "stop_point.uri=sp1", {}, nt::OdtLevel_e::all, depth, start_page,
                             count, boost::make_optional("20190101T000000"_dt),
                             boost::make_optional("20190102T000000"_dt), navitia::type::RTLevel::Base, *data);
    resp = pb_creator2.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys().size(), 1);
    BOOST_CHECK_EQUAL(resp.vehicle_journeys(0).uri(), "vehicle_journey:vj:0");

    // Check if the vehicule_journey with sp1 is not present
    data = b.data.get();
    depth = 3;
    start_page = 0;
    count = 10;

    navitia::PbCreator pb_creator3(data, bt::second_clock::universal_time(), null_time_period);
    navitia::ptref::query_pb(pb_creator3, nt::Type_e::VehicleJourney, "stop_point.uri=sp2", {}, nt::OdtLevel_e::all, depth, start_page,
                             count, boost::make_optional("20190101T000000"_dt),
                             boost::make_optional("20190102T000000"_dt), navitia::type::RTLevel::Base, *data);
    resp = pb_creator3.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys().size(), 1);
    BOOST_CHECK_EQUAL(resp.vehicle_journeys(0).uri(), "vehicle_journey:vj:1");
}
