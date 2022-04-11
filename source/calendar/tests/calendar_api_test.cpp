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
#define BOOST_TEST_MODULE test_api_calendar
#include <boost/test/unit_test.hpp>
#include "ed/build_helper.h"
#include "calendar/calendar_api.h"
#include "type/pt_data.h"
#include "type/pb_converter.h"

/*
                                            Mars 2014
                                    di lu ma me je ve sa
                                                      1
                                    2  3  4  5  6  7  8
                                    9 10 11 12 13 14 15
                                    16 17 18 19 20 21 22
                                    23 24 25 26 27 28 29
                                    30 31
2 calendars used :

calA        1***************5---------------10**************14
calB                            7*******9------------12**************16-------21*******25
*/

static boost::gregorian::date date(std::string str) {
    return boost::gregorian::from_undelimited_string(str);
}

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

struct calendar_fixture {
    ed::builder b;
    calendar_fixture() : b("20140301") {
        b.vj("line:A", "1", "", true, "VJA")("stop1", 10 * 3600, 10 * 3600 + 10 * 60)("stop2", 12 * 3600,
                                                                                      12 * 3600 + 10 * 60);
        b.vj("line:B", "1", "", true, "VJB")("stop11", 11 * 3600, 11 * 3600 + 10 * 60)("stop22", 14 * 3600,
                                                                                       14 * 3600 + 10 * 60);
        b.data->build_uri();

        auto calA = new navitia::type::Calendar(b.data->meta->production_date.begin());
        calA->uri = "calA";
        calA->active_periods.emplace_back(date("20140301"), date("20140305"));
        calA->active_periods.emplace_back(date("20140310"), date("20140314"));
        calA->week_pattern = std::bitset<7>{"1111100"};
        b.data->pt_data->calendars.push_back(calA);

        auto calB = new navitia::type::Calendar(b.data->meta->production_date.begin());
        calB->uri = "calB";
        calB->active_periods.emplace_back(date("20140307"), date("20140309"));
        calB->active_periods.emplace_back(date("20140312"), date("20140316"));
        calB->active_periods.emplace_back(date("20140321"), date("20140325"));
        calB->week_pattern = std::bitset<7>{"0000011"};
        b.data->pt_data->calendars.push_back(calB);

        // both calendars are associated to the line
        b.lines["line:A"]->calendar_list.push_back(calA);
        b.lines["line:B"]->calendar_list.push_back(calB);

        b.data->build_uri();
        b.data->pt_data->sort_and_index();
    }
};
/*
    test_count_calendar : No filter
        => Result 2 calendars
*/
BOOST_FIXTURE_TEST_CASE(test_count_calendar, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "", "", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.calendars_size(), 2);
}
/*

    Test_forbidden_uris : forbidden_uris(line:B)
        => Result 1 calendar : calA

  */
BOOST_FIXTURE_TEST_CASE(test_forbidden_uris, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "", "", 1, 10, 0, "", {"line:B"});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.calendars_size(), 1);
    pbnavitia::Calendar pb_cal = resp.calendars(0);
    BOOST_REQUIRE_EQUAL(pb_cal.uri(), "calA");
    BOOST_REQUIRE_EQUAL(pb_cal.active_periods_size(), 2);
}
/*
    Test_filter : filter(line.uri=line:A)
        => Result 1 calendar : calA
*/
BOOST_FIXTURE_TEST_CASE(test_filter, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "", "", 1, 10, 0, "line.uri=line:A", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.calendars_size(), 1);
    pbnavitia::Calendar pb_cal = resp.calendars(0);
    BOOST_REQUIRE_EQUAL(pb_cal.uri(), "calA");
    BOOST_REQUIRE_EQUAL(pb_cal.active_periods_size(), 2);
}
/*
    test_filter_period_no_solution : Period filter  ("20140201", "20140210")
        => Result No solution
  */
BOOST_FIXTURE_TEST_CASE(test_filter_period_no_solution, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "20140201", "20140210", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}
/*
    Test_filter_period_1 : Period filter  ("20140302", "20140303")
        => Result 1 calendar : calA
  */

// Response calA
BOOST_FIXTURE_TEST_CASE(test_filter_period_1, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "20140302", "20140303", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.calendars_size(), 1);
    pbnavitia::Calendar pb_cal = resp.calendars(0);
    BOOST_REQUIRE_EQUAL(pb_cal.uri(), "calA");
    BOOST_REQUIRE_EQUAL(pb_cal.active_periods_size(), 2);
}
/*

    test_filter_period_2 : Period filter  ("20140307", "20140308")
        => Result 1 calendar : calB
  */
// Response calB
BOOST_FIXTURE_TEST_CASE(test_filter_period_2, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "20140307", "20140308", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.calendars_size(), 1);
    pbnavitia::Calendar pb_cal = resp.calendars(0);
    BOOST_REQUIRE_EQUAL(pb_cal.uri(), "calB");
    BOOST_REQUIRE_EQUAL(pb_cal.active_periods_size(), 3);
}
/*
    test_filter_period_3 : Period filter  ("20140312", "20140313")
        => Result 2 calendar : calA and calB
  */
// Response calA et calB
BOOST_FIXTURE_TEST_CASE(test_filter_period_3, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "20140312", "20140313", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.calendars_size(), 2);
    pbnavitia::Calendar pb_cal = resp.calendars(0);
    BOOST_REQUIRE_EQUAL(pb_cal.uri(), "calA");
    BOOST_REQUIRE_EQUAL(pb_cal.active_periods_size(), 2);
    pb_cal = resp.calendars(1);
    BOOST_REQUIRE_EQUAL(pb_cal.uri(), "calB");
    BOOST_REQUIRE_EQUAL(pb_cal.active_periods_size(), 3);
}
/*

    test_filter_period_4 : Period filter  ("20140322", "20140324")
        => Result 1 calendar : calB
*/
// Response calB
BOOST_FIXTURE_TEST_CASE(test_filter_period_4, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "20140322", "20140324", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.calendars_size(), 1);
    pbnavitia::Calendar pb_cal = resp.calendars(0);
    BOOST_REQUIRE_EQUAL(pb_cal.uri(), "calB");
    BOOST_REQUIRE_EQUAL(pb_cal.active_periods_size(), 3);
}
/*

    test_filter_period_5 : Period of one day ("20140301", "20140301")
        => Result 1 calendar : calA
  */
// Response calA
BOOST_FIXTURE_TEST_CASE(test_filter_period_5, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "20140301", "20140301", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.calendars_size(), 1);
    pbnavitia::Calendar pb_cal = resp.calendars(0);
    BOOST_REQUIRE_EQUAL(pb_cal.uri(), "calA");
    BOOST_REQUIRE_EQUAL(pb_cal.active_periods_size(), 2);
}

// Response Error
BOOST_FIXTURE_TEST_CASE(test_parse_start_date, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "201403AA", "20140301", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(
        resp.error().message(),
        "Unable to parse start_date, bad lexical cast: source type value could not be interpreted as target");
}

// Response Error
BOOST_FIXTURE_TEST_CASE(test_parse_end_date, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "20140301", "201403AA", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(
        resp.error().message(),
        "Unable to parse end_date, bad lexical cast: source type value could not be interpreted as target");
}

// Response Error
BOOST_FIXTURE_TEST_CASE(test_parse_start_end_date, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "0000", "1111", 1, 10, 0, "", {});
    pbnavitia::Response resp = pb_creator.get_response();
#if BOOST_VERSION > 106600
    BOOST_REQUIRE(resp.error().message() == "Unable to parse start_date, Day of month value is out of range 1..31"
                  || resp.error().message() == "Unable to parse start_date, Year is out of valid range: 1400..9999");
    // either are valid error (it seems that the order of the parse depends on the compiler)
#else
    BOOST_REQUIRE(resp.error().message() == "Unable to parse start_date, Day of month value is out of range 1..31"
                  || resp.error().message() == "Unable to parse start_date, Year is out of valid range: 1400..10000");
#endif
}

// Response Error
BOOST_FIXTURE_TEST_CASE(test_ptref_error, calendar_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    navitia::calendar::calendars(pb_creator, *(b.data), "", "", 1, 10, 0, "line:A", {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.error().message(), "Filter: unable to parse line:A");
}
