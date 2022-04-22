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
#include "tests/utils_test.h"
#include "time_tables/passages.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

using namespace navitia::timetables;

// In this test, according to boarding_times vj:1 is before vj:0 which will cause vj:1 to be
// fetched before vj:0 in get_stop_times. Since passages display departure / arrival times
// we need to check that the departures are correctly ordered regardless.
BOOST_AUTO_TEST_CASE(passages_boarding_order) {
    ed::builder b("20170101", [](ed::builder& b) {
        b.vj("L1").name("vj:0")("stop1", "8:00"_t, "8:01"_t, std::numeric_limits<uint16_t>::max(), false, true, 900, 0)(
            "stop2", "8:05"_t, "8:06"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 0)(
            "stop3", "8:10"_t, "8:11"_t, std::numeric_limits<uint16_t>::max(), true, false, 900, 0);

        b.vj("L1").name("vj:1")("stop1", "8:05"_t, "8:06"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 900)(
            "stop2", "8:10"_t, "8:11"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 900)(
            "stop3", "8:15"_t, "8:16"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 900);
    });

    auto* data_ptr = b.data.get();

    // next departures
    navitia::PbCreator pb_creator_ndep(data_ptr, bt::second_clock::universal_time(), null_time_period);
    passages(pb_creator_ndep, "stop_point.uri=stop2", {}, "20170101T073000"_dt, 86400, 10, 3,
             navitia::type::AccessibiliteParams(), nt::RTLevel::Base, pbnavitia::NEXT_DEPARTURES, 10, 0);
    auto resp = pb_creator_ndep.get_response();
    BOOST_REQUIRE_EQUAL(resp.next_departures().size(), 2);
    BOOST_REQUIRE_EQUAL(resp.next_departures(0).stop_date_time().departure_date_time(), "20170101T080600"_pts);
    BOOST_REQUIRE_EQUAL(resp.next_departures(1).stop_date_time().departure_date_time(), "20170101T081100"_pts);

    // next arrivals
    navitia::PbCreator pb_creator_narr(data_ptr, bt::second_clock::universal_time(), null_time_period);
    passages(pb_creator_narr, "stop_point.uri=stop2", {}, "20170101T073000"_dt, 86400, 10, 3,
             navitia::type::AccessibiliteParams(), nt::RTLevel::Base, pbnavitia::NEXT_ARRIVALS, 10, 0);
    resp = pb_creator_narr.get_response();
    BOOST_REQUIRE_EQUAL(resp.next_arrivals().size(), 2);
    BOOST_REQUIRE_EQUAL(resp.next_arrivals(0).stop_date_time().arrival_date_time(), "20170101T080500"_pts);
    BOOST_REQUIRE_EQUAL(resp.next_arrivals(1).stop_date_time().arrival_date_time(), "20170101T081000"_pts);
}

BOOST_AUTO_TEST_CASE(next_passages_on_last_production_day) {
    ed::builder b("20170101", [](ed::builder& b) {
        boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20170101");
        boost::gregorian::date end = boost::gregorian::date_from_iso_string("20170108");
        b.data->meta->production_date = boost::gregorian::date_period(begin, end);
        b.vj("L1", "1111111")
            .name("vj:0")("stop1", "8:00"_t, "8:01"_t)("stop2", "8:05"_t, "8:06"_t)("stop3", "8:10"_t, "8:11"_t);

        b.vj("L1", "1111111")
            .name("vj:1")("stop1", "9:00"_t, "9:01"_t)("stop2", "9:05"_t, "9:06"_t)("stop3", "9:10"_t, "9:11"_t);
    });
    auto* data_ptr = b.data.get();

    navitia::PbCreator pb_creator_pdep(data_ptr, bt::second_clock::universal_time(), null_time_period);
    passages(pb_creator_pdep, "stop_point.uri=stop2", {}, "20170107T080000"_dt, 86400, 10, 3,
             navitia::type::AccessibiliteParams(), nt::RTLevel::Base, pbnavitia::NEXT_DEPARTURES, 10, 0);
    auto resp = pb_creator_pdep.get_response();
    BOOST_REQUIRE_EQUAL(resp.next_departures().size(), 2);
    BOOST_REQUIRE_EQUAL(resp.next_departures(0).stop_date_time().departure_date_time(), "20170107T080600"_pts);
    BOOST_REQUIRE_EQUAL(resp.next_departures(1).stop_date_time().departure_date_time(), "20170107T090600"_pts);

    // Production_date.begin = 20170101, request_datetime = 20161231T080000 and request_datetime is not included
    // in production_date period. The request_datetime is well within 24 hours before production_date.begin
    // In such cas instead of raising date out of bound, we init request dates as
    // date_time = 0, duration = max_datetime = 86400 + 32400(=09h00) - 86400(=24 hours) = 32400(=09h00)
    navitia::PbCreator pb_creator_pdep_1(data_ptr, bt::second_clock::universal_time(), null_time_period);
    passages(pb_creator_pdep_1, "stop_point.uri=stop2", {}, "20161231T090000"_dt, 86400, 10, 3,
             navitia::type::AccessibiliteParams(), nt::RTLevel::Base, pbnavitia::NEXT_DEPARTURES, 10, 0);
    resp = pb_creator_pdep_1.get_response();
    BOOST_REQUIRE_EQUAL(resp.next_departures().size(), 1);
    BOOST_REQUIRE_EQUAL(resp.next_departures(0).stop_date_time().departure_date_time(), "20170101T080600"_pts);

    // date_time = 0, duration = max_datetime = 86400 + 36000(=10h00) - 86400(=24 hours) = 36000(=10h00)
    navitia::PbCreator pb_creator_pdep_2(data_ptr, bt::second_clock::universal_time(), null_time_period);
    passages(pb_creator_pdep_2, "stop_point.uri=stop2", {}, "20161231T100000"_dt, 86400, 10, 3,
             navitia::type::AccessibiliteParams(), nt::RTLevel::Base, pbnavitia::NEXT_DEPARTURES, 10, 0);
    resp = pb_creator_pdep_2.get_response();
    BOOST_REQUIRE_EQUAL(resp.next_departures().size(), 2);
    BOOST_REQUIRE_EQUAL(resp.next_departures(0).stop_date_time().departure_date_time(), "20170101T080600"_pts);
    BOOST_REQUIRE_EQUAL(resp.next_departures(1).stop_date_time().departure_date_time(), "20170101T090600"_pts);
}
