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
#include "time_tables/route_schedules.h"
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/sort.hpp>
#include "kraken/apply_disruption.h"
#include "kraken/make_disruption_from_chaos.h"

namespace ntt = navitia::timetables;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

static const std::string& get_vj(const pbnavitia::RouteSchedule& r, int i) {
    return r.table().headers(i).pt_display_informations().uris().vehicle_journey();
}

static void print_route_schedule(const pbnavitia::RouteSchedule& r) {
    std::cout << "\t|";
    for (const auto& row : r.table().headers()) {
        std::cout << row.pt_display_informations().uris().vehicle_journey() << "\t|";
    }
    std::cout << std::endl;
    for (const auto& row : r.table().rows()) {
        std::cout << row.stop_point().uri() << "\t|";
        for (const auto& elt : row.date_times()) {
            if (elt.time() > 48 * 3600) {
                std::cout << "-"
                          << "\t|";
            } else {
                std::cout << elt.time() / 3600 << ":" << (elt.time() % 3600) / 60. << "\t|";
            }
        }
        std::cout << std::endl;
    }
}

// for more concice test
static pt::ptime d(std::string str) {
    return boost::posix_time::from_iso_string(str);
}
/*
    Wanted schedules:
            VJ1    VJ2    VJ3    VJ4
        A          8h00   8h05
        B          8h10   8h20   8h25
        C   8h05
        D   9h30                 9h35

        But the thermometer algorithm is buggy, is doesn't give the shorest common superstring..
        So, actually we have this schedule
            VJ2    VJ1    VJ3    VJ4
        C          8h05
        D          9h30
        A   8h00          8h05
        B   8h10          8h20   8h25
        D                        9h35

With VJ1, and VJ2, we ensure that we are no more comparing the two first jpp of
each VJ, but we have a more human order.
VJ2 and VJ3, is a basic test.
The difficulty with VJ4 comes when we have to compare it to VJ1.
When comparing VJ1 and VJ4, we compare VJ1(stopC) with VJ4(stopD)
and when we compare VJ4 and VJ1, we compare VJ4(stopB) with VJ1(stopC).
*/
struct route_schedule_fixture {
    ed::builder b;

    route_schedule_fixture()
        : b("20120614", [](ed::builder& b) {
              const std::string a_name = "stopA", b_name = "stopB", c_name = "stopC", d_name = "stopD";
              b.vj("A", "1111111", "", true, "1", "hs_1", "1")(c_name, 8 * 3600 + 5 * 60)(d_name, 9 * 3600 + 30 * 60);
              b.vj("A", "1111111", "", true, "2", "hs_2", "2")(a_name, 8 * 3600)(b_name, 8 * 3600 + 10 * 60);
              b.vj("A", "1111111", "", true, "3", "hs_3", "3")(a_name, 8 * 3600 + 5 * 60)(b_name, 8 * 3600 + 20 * 60);
              b.vj("A", "1111111", "", true, "4", "hs_4", "4")(b_name, 8 * 3600 + 25 * 60)(d_name, 9 * 3600 + 35 * 60);
          }) {
        boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20120613");
        boost::gregorian::date end = boost::gregorian::date_from_iso_string("20120630");
        b.data->meta->production_date = boost::gregorian::date_period(begin, end);
    }
};

BOOST_FIXTURE_TEST_CASE(test1, route_schedule_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=A", {}, {}, d("20120615T070000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_REQUIRE_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:2");
    BOOST_REQUIRE_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:1");
    BOOST_REQUIRE_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:3");
    BOOST_REQUIRE_EQUAL(get_vj(route_schedule, 3), "vehicle_journey:4");
}

BOOST_FIXTURE_TEST_CASE(test_max_nb_stop_times, route_schedule_fixture) {
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=A", {}, {}, d("20120615T070000"), 86400, 0, 3, 10, 0,
                                        nt::RTLevel::Base);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_REQUIRE_EQUAL(route_schedule.table().headers().size(), 0);
}

/*
We have 3 vehicle journeys VJ5, VJ6, VJ7 and 3 stops S1, S2, S3 :
     VJ5     VJ6     VJ7
S1  10:00   11:00   13:00
S2  10:30   11:30   13:37
s3  11:00   12:00   14:00

We have 4 calendars, C1, C2, C3, C4 :
- C1 : week (monday to friday),
- C2 : week end,
- C3 : holiday (monday to sunday during holidays),
- C4 : random calendar nobody uses.

Each vehicle_journey has its respective meta_vj : MVJ5, MVJ6, MVJ7.
We have the following associations :
- MVJ5 => C1,C2
- MVJ6 => C2,C3
- MVJ7 => ∅
*/
struct route_schedule_calendar_fixture {
    // we complicate things a bit, we say that the vjs have a utc offset
    ed::builder b;
    navitia::type::Calendar *c1, *c2, *c3, *c4;
    navitia::type::VehicleJourney *vj5, *vj6, *vj7;

    route_schedule_calendar_fixture()
        : b("20120614",
            [&](ed::builder& b) {
                const std::string S1_name = "S1", S2_name = "S2", S3_name = "S3";
                boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20120613");
                boost::gregorian::date end = boost::gregorian::date_from_iso_string("20120630");

                vj5 = b.vj("B", "1111111", "", true, "VJ5", "hs_VJ5", "MVJ5")(S1_name, "10:00"_t)(S2_name, "10:30"_t)(
                           S3_name, "11:00"_t)
                          .make();
                vj6 = b.vj("B", "1111111", "", true, "VJ6", "hs_VJ6", "MVJ6")(S1_name, "11:00"_t)(S2_name, "11:30"_t)(
                           S3_name, "12:00"_t)
                          .make();
                vj7 = b.vj("B", "1111111", "", true, "VJ7", "hs_VJ7", "MVJ7")(S1_name, "13:00"_t)(S2_name, "13:37"_t)(
                           S3_name, "14:00"_t)
                          .make();

                auto save_cal = [&](navitia::type::Calendar* cal) {
                    b.data->pt_data->calendars.push_back(cal);
                    b.data->pt_data->calendars_map[cal->uri] = cal;
                };

                c1 = new navitia::type::Calendar(begin);
                c1->uri = "C1";
                c1->active_periods.emplace_back(begin, end);
                c1->week_pattern = std::bitset<7>("1111100");
                save_cal(c1);
                c2 = new navitia::type::Calendar(begin);
                c2->uri = "C2";
                c2->active_periods.emplace_back(begin, end);
                c2->week_pattern = std::bitset<7>("0000011");
                save_cal(c2);
                c3 = new navitia::type::Calendar(begin);
                c3->uri = "C3";
                c3->active_periods.emplace_back(begin, end);
                c3->week_pattern = std::bitset<7>("1111111");
                save_cal(c3);
                c4 = new navitia::type::Calendar(begin);
                c4->uri = "C4";
                c4->active_periods.emplace_back(begin, end);
                c4->week_pattern = std::bitset<7>("0000000");
                save_cal(c4);

                auto a1 = new navitia::type::AssociatedCalendar;
                a1->calendar = c1;
                b.data->pt_data->associated_calendars.push_back(a1);
                auto a2 = new navitia::type::AssociatedCalendar;
                a2->calendar = c2;
                b.data->pt_data->associated_calendars.push_back(a2);
                auto a3 = new navitia::type::AssociatedCalendar;
                a3->calendar = c3;
                b.data->pt_data->associated_calendars.push_back(a3);

                b.data->pt_data->meta_vjs.get_mut("MVJ5")->associated_calendars.insert({c1->uri, a1});
                b.data->pt_data->meta_vjs.get_mut("MVJ5")->associated_calendars.insert({c2->uri, a2});
                b.data->pt_data->meta_vjs.get_mut("MVJ6")->associated_calendars.insert({c2->uri, a2});
                b.data->pt_data->meta_vjs.get_mut("MVJ6")->associated_calendars.insert({c3->uri, a3});

                b.data->meta->production_date = boost::gregorian::date_period(begin, end);
            },
            false,
            "hove",
            "paris",
            {{"02:00"_t, {{"20120614"_d, "20130614"_d}}}}) {}

    void check_calendar_results(boost::optional<const std::string> calendar, std::vector<std::string> expected_vjs) {
        auto* data_ptr = b.data.get();
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        navitia::timetables::route_schedule(pb_creator, "line.uri=B", calendar, {}, d("20120615T070000"), 86400, 100, 3,
                                            10, 0, nt::RTLevel::Base);

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
        pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
        print_route_schedule(route_schedule);
        BOOST_REQUIRE_EQUAL(route_schedule.table().headers_size(), expected_vjs.size());
        for (int i = 0; i < route_schedule.table().headers_size(); i++) {
            BOOST_REQUIRE_EQUAL(get_vj(route_schedule, i), expected_vjs[i]);
        }
    }
};

BOOST_FIXTURE_TEST_CASE(test_calendar_filter, route_schedule_calendar_fixture) {
    // No filter, all 3 VJs expected
    check_calendar_results({}, {vj5->uri, vj6->uri, vj7->uri});
    // For each calendar only the VJs explicitly linked to it expected
    check_calendar_results(c1->uri, {vj5->uri});
    check_calendar_results(c2->uri, {vj5->uri, vj6->uri});
    check_calendar_results(c3->uri, {vj6->uri});
    // No results for calendar C4
    check_calendar_results(c4->uri, {});
}

namespace ba = boost::adaptors;
using vec_dt = std::vector<navitia::DateTime>;
static navitia::DateTime get_dt(const navitia::routing::datetime_stop_time& p) {
    return p.first;
}

/*
 * Test get_all_route_stop_times with a calendar
 *
 * wich the C2 calendar, we should have the vj5 and vj6, thus we should have this schedule:
 *        VJ5     VJ6
 *    S1  10:00   11:00
 *    S2  10:30   11:30
 *    S3  11:00   12:00
 * NOTE: this is in UTC and since we ask with a calendar we want local time
 * Thus the schedule is:
 *        VJ5     VJ6
 *    S1  12:00   13:00
 *    S2  12:30   13:30
 *    S3  13:00   14:00
 */
BOOST_FIXTURE_TEST_CASE(test_get_all_route_stop_times_with_cal, route_schedule_calendar_fixture) {
    const auto* route = b.data->pt_data->routes_map.at("B:0");

    auto res = navitia::timetables::get_all_route_stop_times(
        route, "00:00"_t, "00:00"_t + "24:00"_t, std::numeric_limits<size_t>::max(), *b.data, nt::RTLevel::Base,
        boost::optional<const std::string>(c2->uri));

    BOOST_REQUIRE_EQUAL(res.size(), 2);
    boost::sort(res);
    BOOST_CHECK_EQUAL_RANGE(res[0] | ba::transformed(get_dt), vec_dt({"12:00"_t, "12:30"_t, "13:00"_t}));
    BOOST_CHECK_EQUAL_RANGE(res[1] | ba::transformed(get_dt), vec_dt({"13:00"_t, "13:30"_t, "14:00"_t}));
}

/*
 * Test get_all_route_stop_times with a calendar and with a custom time (used only for the sort)
 * Schedule should be
 *        VJ5        VJ6
 *    S1  12:00+1   13:00
 *    S2  12:30+1   13:30
 *    s3  13:00+1   14:00
 *
 * the +1 day is used so the following sort will sort the result in a correct way
 */
BOOST_FIXTURE_TEST_CASE(test_get_all_route_stop_times_with_cal_and_time, route_schedule_calendar_fixture) {
    const auto* route = b.data->pt_data->routes_map.at("B:0");

    auto res = navitia::timetables::get_all_route_stop_times(
        route, "12:37"_t, "12:37"_t + "24:00"_t, std::numeric_limits<size_t>::max(), *b.data, nt::RTLevel::Base,
        boost::optional<const std::string>(c2->uri));

    BOOST_REQUIRE_EQUAL(res.size(), 2);

    auto one_day = "24:00"_t;
    boost::sort(res);
    BOOST_CHECK_EQUAL_RANGE(res[0] | ba::transformed(get_dt), vec_dt({"13:00"_t, "13:30"_t, "14:00"_t}));
    BOOST_CHECK_EQUAL_RANGE(res[1] | ba::transformed(get_dt),
                            vec_dt({"12:00"_t + one_day, "12:30"_t + one_day, "13:00"_t + one_day}));
}

/*
 * Test get_all_route_stop_times with not a calendar but only a dt (the classic route_schedule)
 *
 * Note: the returned dt should be in UTC, not local time
 *
 * thus the schedule after 11h37 should be:
 *
 *      VJ5       VJ6       VJ7
 * S1  10:00+1   11:00+1   13:00
 * S2  10:30+1   11:30+1   13:37
 * s3  11:00+1   12:00+1   14:00
 *
 */
BOOST_FIXTURE_TEST_CASE(test_get_all_route_stop_times_with_time, route_schedule_calendar_fixture) {
    const auto* route = b.data->pt_data->routes_map.at("B:0");

    auto res = navitia::timetables::get_all_route_stop_times(
        route, "11:37"_t, "11:37"_t + "24:00"_t, std::numeric_limits<size_t>::max(), *b.data, nt::RTLevel::Base, {});

    BOOST_REQUIRE_EQUAL(res.size(), 3);

    auto one_day = "24:00"_t;
    boost::sort(res);
    BOOST_CHECK_EQUAL_RANGE(res[0] | ba::transformed(get_dt), vec_dt({"13:00"_t, "13:37"_t, "14:00"_t}));
    BOOST_CHECK_EQUAL_RANGE(res[1] | ba::transformed(get_dt),
                            vec_dt({"10:00"_t + one_day, "10:30"_t + one_day, "11:00"_t + one_day}));
    BOOST_CHECK_EQUAL_RANGE(res[2] | ba::transformed(get_dt),
                            vec_dt({"11:00"_t + one_day, "11:30"_t + one_day, "12:00"_t + one_day}));
}

/*
 * Test with a dataset close to https://github.com/hove-io/navitia/issues/1161
 *
 * The dataset in LOCAL TIME (france) is:
 *
 *      vj1    vj2    vj3
 * S1  00:50  01:50  02:50
 * S2  01:05  02:05  03:05
 * S3  01:15  02:15  03:15
 *
 * The small catch is that there is 2 hours UTC shift, thus vj1 and vj2 validity pattern's are shifted the day before:
 *
 *      vj1    vj2    vj3
 * S1  22:50  23:50  00:50
 * S2  23:05  00:05  01:05
 * S3  23:15  00:15  01:15
 *
 */
struct CalWithDSTFixture {
    ed::builder b;

    CalWithDSTFixture()
        : b("20150614",
            [](ed::builder& b) {
                auto normal_vp = "111111";
                auto shifted_vp = "111110";
                b.vj("B", shifted_vp)("S1", "22:50"_t)("S2", "23:05"_t)("S3", "23:15"_t);
                b.vj("B", shifted_vp)("S1", "23:50"_t)("S2", "00:05"_t)("S3", "00:15"_t);
                b.vj("B", normal_vp)("S1", "00:50"_t)("S2", "01:05"_t)("S3", "01:15"_t);

                auto cal = new navitia::type::Calendar();
                cal->uri = "cal";
                b.data->pt_data->calendars.push_back(cal);
                b.data->pt_data->calendars_map[cal->uri] = cal;

                auto a1 = new navitia::type::AssociatedCalendar;
                a1->calendar = cal;
                b.data->pt_data->associated_calendars.push_back(a1);

                for (auto& mvj : b.data->pt_data->meta_vjs) {
                    mvj->associated_calendars.insert({cal->uri, a1});
                }
            },
            true,
            "hove",
            "paris",
            {{"02:00"_t, {{"20150614"_d, "20160614"_d}}}}) {}
};

BOOST_FIXTURE_TEST_CASE(test_get_all_route_stop_times_with_different_vp, CalWithDSTFixture) {
    const auto* route = b.data->pt_data->routes.at(0);

    auto res = navitia::timetables::get_all_route_stop_times(route, "00:00"_t, "00:00"_t + "24:00"_t,
                                                             std::numeric_limits<size_t>::max(), *b.data,
                                                             nt::RTLevel::Base, std::string("cal"));

    BOOST_REQUIRE_EQUAL(res.size(), 3);

    boost::sort(res);
    BOOST_CHECK_EQUAL_RANGE(res[0] | ba::transformed(get_dt), vec_dt({"00:50"_t, "01:05"_t, "01:15"_t}));
    BOOST_CHECK_EQUAL_RANGE(res[1] | ba::transformed(get_dt), vec_dt({"01:50"_t, "02:05"_t, "02:15"_t}));
    BOOST_CHECK_EQUAL_RANGE(res[2] | ba::transformed(get_dt), vec_dt({"02:50"_t, "03:05"_t, "03:15"_t}));
}

BOOST_FIXTURE_TEST_CASE(test_get_all_route_stop_times_with_different_vp_and_hour, CalWithDSTFixture) {
    const auto* route = b.data->pt_data->routes.at(0);

    auto res = navitia::timetables::get_all_route_stop_times(route, "01:00"_t, "01:00"_t + "24:00"_t,
                                                             std::numeric_limits<size_t>::max(), *b.data,
                                                             nt::RTLevel::Base, std::string("cal"));

    BOOST_REQUIRE_EQUAL(res.size(), 3);

    auto one_day = "24:00"_t;
    boost::sort(res);
    // both are after the asked time (1h), so they are on the following day
    BOOST_CHECK_EQUAL_RANGE(res[0] | ba::transformed(get_dt), vec_dt({"01:50"_t, "02:05"_t, "02:15"_t}));
    BOOST_CHECK_EQUAL_RANGE(res[1] | ba::transformed(get_dt), vec_dt({"02:50"_t, "03:05"_t, "03:15"_t}));
    BOOST_CHECK_EQUAL_RANGE(res[2] | ba::transformed(get_dt),
                            vec_dt({"00:50"_t + one_day, "01:05"_t + one_day, "01:15"_t + one_day}));
}

// We want: (for a end of service at 2h00)
//      A      B      C
// S1 23:30  23:50  00:10
// S2 23:40  00:00  00:20
// S3 23:50  00:10  00:30
//
// Detail in associated PR https://github.com/hove-io/navitia/pull/1304
BOOST_AUTO_TEST_CASE(test_route_schedule_with_different_vp_over_midnight) {
    navitia::type::Calendar* c1;

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20150101");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20160101");

    ed::builder b("20151127", [&](ed::builder& b) {
        b.vj("L", "111111", "", true, "A", "hs_A", "A")("st1", "23:30"_t)("st2", "23:40"_t)("st3", "23:50"_t);
        b.vj("L", "111111", "", true, "B", "hs_B", "B")("st1", "23:50"_t)("st2", "24:00"_t)("st3", "24:10"_t);
        b.vj("L", "111110", "", true, "C", "hs_C", "C")("st1", "24:10"_t)("st2", "24:20"_t)("st3", "24:30"_t);

        auto save_cal = [&](navitia::type::Calendar* cal) {
            b.data->pt_data->calendars.push_back(cal);
            b.data->pt_data->calendars_map[cal->uri] = cal;
        };

        c1 = new navitia::type::Calendar(begin);
        c1->uri = "C1";
        c1->active_periods.emplace_back(begin, end);
        c1->week_pattern = std::bitset<7>("1111111");
        save_cal(c1);

        auto a1 = new navitia::type::AssociatedCalendar;
        a1->calendar = c1;
        b.data->pt_data->associated_calendars.push_back(a1);

        b.data->pt_data->meta_vjs.get_mut("A")->associated_calendars.insert({c1->uri, a1});
        b.data->pt_data->meta_vjs.get_mut("B")->associated_calendars.insert({c1->uri, a1});
        b.data->pt_data->meta_vjs.get_mut("C")->associated_calendars.insert({c1->uri, a1});
    });

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", c1->uri, {}, d("20151201T020000"), 86400, 100, 3, 10,
                                        0, nt::RTLevel::Base);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:A");
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:B");
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:C");
}

// We want:
//     C A B
// st1 2   1
// st2     3
// st3     5
// st4 3 5 6
// st5 4 6 7
// st6 5 7 8
BOOST_AUTO_TEST_CASE(complicated_order_1) {
    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("L", "1111111", "", true, "A", "hs_A", "A")("st4", "5:00"_t)("st5", "6:00"_t)("st6", "7:00"_t);
        b.vj("L", "1111111", "", true, "B", "hs_B", "B")("st1", "1:00"_t)("st2", "3:00"_t)("st3", "5:00"_t)(
            "st4", "6:00"_t)("st5", "7:00"_t)("st6", "8:00"_t);
        b.vj("L", "1111111", "", true, "C", "hs_C", "C")("st1", "2:00"_t)("st4", "3:00"_t)("st5", "4:00"_t)("st6",
                                                                                                            "5:00"_t);
    });

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", {}, {}, d("20120615T000000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:C");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(0).time(), "3:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:A");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(1).time(), "5:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:B");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(2).time(), "6:00"_t);
}

// We want:
//      C  A B
// st1     1 2
// st2     2 3
// st3     3 4
// st4     5
// st5     7
// st6  8  9 7
// st7  9 10
// st8 10 11
BOOST_AUTO_TEST_CASE(complicated_order_2) {
    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("L", "1111111", "", true, "A", "hs_A", "A")("st1", "1:00"_t)("st2", "2:00"_t)("st3", "3:00"_t)(
            "st4", "5:00"_t)("st5", "7:00"_t)("st6", "9:00"_t)("st7", "10:00"_t)("st8", "11:00"_t);
        b.vj("L", "1111111", "", true, "B", "hs_B", "B")("st1", "2:00"_t)("st2", "3:00"_t)("st3", "4:00"_t)("st6",
                                                                                                            "7:00"_t);
        b.vj("L", "1111111", "", true, "C", "hs_C", "C")("st6", "8:00"_t)("st7", "9:00"_t)("st8", "10:00"_t);
    });

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", {}, {}, d("20120615T000000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:C");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(5).date_times(0).time(), "8:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:A");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(5).date_times(1).time(), "9:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:B");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(5).date_times(2).time(), "7:00"_t);
}

// We want:
//     A B C D E
// st1   1 2 3 4
// st2   2 3 4 5
// st3 6 7
// st4 7 8
BOOST_AUTO_TEST_CASE(complicated_order_3) {
    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("L", "1111111", "", true, "A", "hs_A", "A")("st3", "6:00"_t)("st4", "7:00"_t);
        b.vj("L", "1111111", "", true, "B", "hs_B", "B")("st1", "1:00"_t)("st2", "2:00"_t)("st3", "7:00"_t)("st4",
                                                                                                            "8:00"_t);
        b.vj("L", "1111111", "", true, "C", "hs_C", "C")("st1", "2:00"_t)("st2", "3:00"_t);
        b.vj("L", "1111111", "", true, "D", "hs_D", "D")("st1", "3:00"_t)("st2", "4:00"_t);
        b.vj("L", "1111111", "", true, "E", "hs_E", "E")("st1", "4:00"_t)("st2", "5:00"_t);
    });

    using btp = boost::posix_time::time_period;
    b.impact(nt::RTLevel::Adapted, "Disruption 1")
        .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
        .on(nt::Type_e::StopPoint, "st1", *b.data->pt_data)
        .application_periods(btp("20120614T010000"_dt, "20150625T235900"_dt))
        .publish(btp("20120614T010000"_dt, "20150625T235900"_dt))
        .msg("Disruption on stop_point st1");
    // current_datetime out of bounds
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", {}, {}, d("20120615T000000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:A");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(0).time(), "6:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:B");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).time(), "1:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:C");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).time(), "2:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 3), "vehicle_journey:D");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).time(), "3:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 4), "vehicle_journey:E");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(4).time(), "4:00"_t);

    // current_datetime
    pb_creator.init(data_ptr, "20120615T080000"_dt, null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", {}, {}, d("20120615T000000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);

    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    route_schedule = resp.route_schedules(0);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).stop_point().uri(), "st1");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).stop_point().impact_uris_size(), 1);
    const auto* impact = navitia::test::get_impact(route_schedule.table().rows(0).stop_point().impact_uris(0), resp);
    BOOST_REQUIRE(impact);
    BOOST_CHECK_EQUAL(impact->messages_size(), 1);
    BOOST_CHECK_EQUAL(impact->messages(0).text(), "Disruption on stop_point st1");
}

// We want: without impact
//     A B C D E
// st1   1 2 3 4
// st2   2 3 4 5
// st3 6 7
// st4 7 8

// We want: with impact on line(NO_SERVICE)
//     A B C D E
// st1
// st2
// st3
// st4

BOOST_AUTO_TEST_CASE(complicated_order_with_impacts) {
    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("L", "1111111", "", true, "A", "hs_A", "A")("st3", "6:00"_t)("st4", "7:00"_t);
        b.vj("L", "1111111", "", true, "B", "hs_B", "B")("st1", "1:00"_t)("st2", "2:00"_t)("st3", "7:00"_t)("st4",
                                                                                                            "8:00"_t);
        b.vj("L", "1111111", "", true, "C", "hs_C", "C")("st1", "2:00"_t)("st2", "3:00"_t);
        b.vj("L", "1111111", "", true, "D", "hs_D", "D")("st1", "3:00"_t)("st2", "4:00"_t);
        b.vj("L", "1111111", "", true, "E", "hs_E", "E")("st1", "4:00"_t)("st2", "5:00"_t);
    });

    using btp = boost::posix_time::time_period;
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Disruption 1")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::Line, "L", *b.data->pt_data)
                                  .application_periods(btp("20120614T010000"_dt, "20150625T235900"_dt))
                                  .publish(btp("20120614T010000"_dt, "20150625T235900"_dt))
                                  .msg("Disruption on line")
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", {}, {}, d("20120615T000000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:A");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(0).time(), "6:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:B");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).time(), "1:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:C");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).time(), "2:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 3), "vehicle_journey:D");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).time(), "3:00"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 4), "vehicle_journey:E");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(4).time(), "4:00"_t);

    // Call with RealTime:
    pb_creator.init(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", {}, {}, d("20120615T000000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::RealTime);

    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(route_schedule.response_status(), pbnavitia::ResponseStatus::active_disruption);
}

// Test that frequency are correctly displayed and that we get departure_times and not boarding times
BOOST_AUTO_TEST_CASE(route_schedule_with_boarding_time_frequency_and_calendar) {
    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20170101");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20180101");
    auto* c = new navitia::type::Calendar(begin);

    ed::builder b("20170101", [&](ed::builder& b) {
        b.vj("L1").name("vj:0")("stop1", "8:00"_t, "8:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
            "stop2", "8:05"_t, "8:05"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 900)(
            "stop3", "8:10"_t, "8:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

        b.vj("L1").name("vj:2")("stop1", "23:50"_t, "23:50"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 0)(
            "stop2", "23:55"_t, "23:55"_t, std::numeric_limits<uint16_t>::max(), true, true, 600, 1800)(
            "stop3", "24:10"_t, "24:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 900);

        b.frequency_vj("L1", "18:00"_t, "19:00"_t, "00:30"_t)
            .name("vj:1")("stop1", "18:00"_t, "18:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
                "stop2", "18:05"_t, "18:05"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 900)(
                "stop3", "18:10"_t, "18:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

        c->uri = "C1";
        c->active_periods.emplace_back(begin, end);
        c->week_pattern = std::bitset<7>("1111111");
        b.data->pt_data->calendars.push_back(c);
        b.data->pt_data->calendars_map[c->uri] = c;

        auto a1 = new navitia::type::AssociatedCalendar;
        a1->calendar = c;
        b.data->pt_data->associated_calendars.push_back(a1);
        b.data->pt_data->meta_vjs.get_mut("vj:0")->associated_calendars.insert({c->uri, a1});
        b.data->pt_data->meta_vjs.get_mut("vj:1")->associated_calendars.insert({c->uri, a1});
    });

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator_cal(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator_cal, "line.uri=L1", c->uri, {}, d("20170103T070000"), 86400, 100, 3,
                                        10, 0, nt::RTLevel::Base);
    pbnavitia::Response resp = pb_creator_cal.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:vj:0");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).time(), "8:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).time(), "8:05"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(0).time(), "8:10"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 3), "vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).time(), "19:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(3).time(), "19:05"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(3).time(), "19:10"_t);

    // Same thing with no calendar, should get vj:2 too
    navitia::PbCreator pb_creator_nocal(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator_nocal, "line.uri=L1", {}, {}, d("20170103T070000"), 86400, 100, 3,
                                        10, 0, nt::RTLevel::Base);
    resp = pb_creator_nocal.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:vj:0");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).time(), "8:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).time(), "8:05"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(0).time(), "8:10"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 3), "vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).time(), "19:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(3).time(), "19:05"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(3).time(), "19:10"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 4), "vehicle_journey:vj:2");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(4).time(), "23:50"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(4).time(), "23:55"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(4).time(), "0:10"_t);
}

// Check that results are in the right order even if the boarding_time are in a different one
BOOST_AUTO_TEST_CASE(route_schedule_with_boarding_time_order_check) {
    ed::builder b("20170101", [&](ed::builder& b) {
        b.vj("L1").name("vj:0")("stop1", "8:00"_t, "8:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 900, 0)(
            "stop2", "8:05"_t, "8:05"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 0)(
            "stop3", "8:10"_t, "8:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 900, 0);

        b.vj("L1").name("vj:1")("stop1", "8:05"_t, "8:05"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 900)(
            "stop2", "8:10"_t, "8:10"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 900)(
            "stop3", "8:15"_t, "8:15"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 900);
    });

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L1", {}, {}, d("20170103T070000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);

    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:vj:0");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).time(), "8:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).time(), "8:05"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(0).time(), "8:10"_t);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).time(), "8:05"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).time(), "8:10"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(1).time(), "8:15"_t);
}

BOOST_AUTO_TEST_CASE(route_schedule_multiple_days) {
    ed::builder b("20180101", [&](ed::builder& b) {
        b.vj("L1", "10000001").name("vj:0")("stop1", "8:00"_t, "8:00"_t)("stop2", "8:05"_t, "8:05"_t);
        b.frequency_vj("L1", "10:00:00"_t, "11:00:00"_t, "00:30:00"_t, "", "10000001")
            .name("vj:1")("stop1", "10:00:00"_t, "10:00:00"_t)("stop2", "10:05:00"_t, "10:05:00"_t);
    });

    auto* data_ptr = b.data.get();
    auto first_sunday = navitia::to_posix_timestamp("20180101T000000"_dt);
    auto last_sunday = navitia::to_posix_timestamp("20180108T000000"_dt);

    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        navitia::timetables::route_schedule(pb_creator, "line.uri=L1", {}, {}, d("20180101T080000"), 86400, 100, 3, 10,
                                            0, nt::RTLevel::Base);
        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
        pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
        print_route_schedule(route_schedule);

        BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:vj:0");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).time(), "8:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).time(), "8:05"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).time(), "10:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).time(), "10:05"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).time(), "10:30"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(2).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(2).time(), "10:35"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 3), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).time(), "11:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(3).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(3).time(), "11:05"_t);
    }
    {
        // No departure from next sundays yet
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        navitia::timetables::route_schedule(pb_creator, "line.uri=L1", {}, {}, d("20180101T080000"), 86400 * 7 - 1, 100,
                                            3, 10, 0, nt::RTLevel::Base);
        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
        pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
        print_route_schedule(route_schedule);

        BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:vj:0");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).time(), "8:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).time(), "8:05"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).time(), "10:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).time(), "10:05"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).time(), "10:30"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(2).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(2).time(), "10:35"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 3), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).time(), "11:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(3).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(3).time(), "11:05"_t);
    }
    {
        // No departure from next sundays yet
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        navitia::timetables::route_schedule(pb_creator, "line.uri=L1", {}, {}, d("20180101T080000"), 86400 * 7, 100, 3,
                                            10, 0, nt::RTLevel::Base);
        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
        pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
        print_route_schedule(route_schedule);

        BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:vj:0");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).time(), "8:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).time(), "8:05"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).time(), "10:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).time(), "10:05"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).time(), "10:30"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(2).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(2).time(), "10:35"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 3), "vehicle_journey:vj:1");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(3).time(), "11:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(3).date(), first_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(3).time(), "11:05"_t);
        BOOST_CHECK_EQUAL(get_vj(route_schedule, 4), "vehicle_journey:vj:0");
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(4).date(), last_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(4).time(), "08:00"_t);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(4).date(), last_sunday);
        BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(4).time(), "08:05"_t);
    }
}

// stop_times with skipped_stops from the beginning (*)
//      VJ_A   VJ_B   VJ_C  VJ_D
// st1  1       4       3   5 *
// st2  2       4 *     4   5 *
// st3  3       4 *     5   6
// st4  4       5       6   7
// st5  5       6       7   8

// wanted but not obtained:
//      VJ_A   VJ_B   VJ_C  VJ_D
// st1  1       4       3   -
// st2  2       -       4   -
// st3  3       -       5   6
// st4  4       5       6   7
// st5  5       6       7   8

// obtained: here vehicle_journey VJ_D disappeared as the start date_time is skipped_stop
//      VJ_A   VJ_C   VJ_B
// st1  1       3       4
// st2  2       4       -
// st3  3       5       -
// st4  4       6       5
// st5  5       7       6

BOOST_AUTO_TEST_CASE(xfail_stop_times_with_skipped_stops_from_the_beginning) {
    ed::builder b("20220614", [&](ed::builder& b) {
        b.vj("L", "1111111", "", true, "VJ_A", "hs_VJ_A", "VJ_A")("st1", "1:00"_t)("st2", "2:00"_t)("st3", "3:00"_t)(
            "st4", "4:00"_t)("st5", "5:00"_t);
        b.vj("L", "1111111", "", true, "VJ_B", "hs_VJ_B", "VJ_B")("st1", "4:00"_t)(
            "st2", "4:00"_t, "5:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
            "st3", "4:00"_t, "5:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
            "st4", "5:00"_t, "5:00"_t, std::numeric_limits<uint16_t>::max(), false, true)("st5", "6:00"_t);
        b.vj("L", "1111111", "", true, "VJ_C", "hs_VJ_C", "VJ_C")("st1", "3:00"_t)("st2", "4:00"_t)("st3", "5:00"_t)(
            "st4", "6:00"_t)("st5", "7:00"_t);
        b.vj("L", "1111111", "", true, "VJ_D", "hs_VJ_D", "VJ_D")(
            "st1", "5:00"_t, "5:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
            "st2", "5:00"_t, "5:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
            "st3", "6:00"_t, "6:00"_t, std::numeric_limits<uint16_t>::max(), false, true)("st4", "7:00"_t)("st5",
                                                                                                           "8:00"_t);
    });

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", {}, {}, d("20220615T000000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:VJ_A");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).time(), "1:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).time(), "2:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(0).time(), "3:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(0).time(), "4:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(4).date_times(0).time(), "5:00"_t);

    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:VJ_C");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).time(), "3:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).time(), "4:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(1).time(), "5:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(1).time(), "6:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(4).date_times(1).time(), "7:00"_t);

    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:VJ_B");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).time(), "4:00"_t);
    // Empty date_time at stop point with skipped_stop = true
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(2).time(), std::numeric_limits<u_int64_t>::max());
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(2).time(), std::numeric_limits<u_int64_t>::max());

    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(2).time(), "5:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(4).date_times(2).time(), "6:00"_t);
}

// stop_times with skipped_stops toward the end (*)
//      VJ_A   VJ_B   VJ_C
// st1  1       2       3
// st2  2       3       4
// st3  3       4 *     4 *
// st4  4       4 *     4 *
// st5  5       4       4 *

// wanted and obtained:
//      VJ_A   VJ_B   VJ_C
// st1  1       2       3
// st2  2       3       4
// st3  3       -       -
// st4  4       -       -
// st5  5       4       -

BOOST_AUTO_TEST_CASE(stop_times_with_skipped_stops_toward_the_end) {
    ed::builder b("20220614", [&](ed::builder& b) {
        b.vj("L", "1111111", "", true, "VJ_A", "hs_VJ_A", "VJ_A")("st1", "1:00"_t)("st2", "2:00"_t)("st3", "3:00"_t)(
            "st4", "4:00"_t)("st5", "5:00"_t);
        b.vj("L", "1111111", "", true, "VJ_B", "hs_VJ_B", "VJ_B")("st1", "2:00"_t)(
            "st2", "3:00"_t, "3:00"_t, std::numeric_limits<uint16_t>::max(), true, false)(
            "st3", "4:00"_t, "4:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
            "st4", "4:00"_t, "4:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
            "st5", "4:00"_t, "4:00"_t, std::numeric_limits<uint16_t>::max(), false, true);
        b.vj("L", "1111111", "", true, "VJ_C", "hs_VJ_C", "VJ_C")("st1", "3:00"_t)(
            "st2", "4:00"_t, "4:00"_t, std::numeric_limits<uint16_t>::max(), true, false)(
            "st3", "4:00"_t, "4:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
            "st4", "4:00"_t, "4:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
            "st5", "4:00"_t, "4:00"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true);
    });

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    navitia::timetables::route_schedule(pb_creator, "line.uri=L", {}, {}, d("20220615T000000"), 86400, 100, 3, 10, 0,
                                        nt::RTLevel::Base);

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.route_schedules().size(), 1);
    pbnavitia::RouteSchedule route_schedule = resp.route_schedules(0);
    print_route_schedule(route_schedule);
    BOOST_CHECK_EQUAL(get_vj(route_schedule, 0), "vehicle_journey:VJ_A");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(0).time(), "1:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(0).time(), "2:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(0).time(), "3:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(0).time(), "4:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(4).date_times(0).time(), "5:00"_t);

    BOOST_CHECK_EQUAL(get_vj(route_schedule, 1), "vehicle_journey:VJ_B");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(1).time(), "2:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(1).time(), "3:00"_t);
    // Empty date_time at stop point with skipped_stop = true
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(1).time(), std::numeric_limits<u_int64_t>::max());
    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(1).time(), std::numeric_limits<u_int64_t>::max());
    BOOST_CHECK_EQUAL(route_schedule.table().rows(4).date_times(1).time(), "4:00"_t);

    BOOST_CHECK_EQUAL(get_vj(route_schedule, 2), "vehicle_journey:VJ_C");
    BOOST_CHECK_EQUAL(route_schedule.table().rows(0).date_times(2).time(), "3:00"_t);
    BOOST_CHECK_EQUAL(route_schedule.table().rows(1).date_times(2).time(), "4:00"_t);
    // Empty date_time at stop point with skipped_stop = true
    BOOST_CHECK_EQUAL(route_schedule.table().rows(2).date_times(2).time(), std::numeric_limits<u_int64_t>::max());
    BOOST_CHECK_EQUAL(route_schedule.table().rows(3).date_times(2).time(), std::numeric_limits<u_int64_t>::max());
    BOOST_CHECK_EQUAL(route_schedule.table().rows(4).date_times(2).time(), std::numeric_limits<u_int64_t>::max());
}
