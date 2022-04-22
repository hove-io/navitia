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
#include "tests/utils_test.h"
#include "routing/get_stop_times.h"
#include "ed/build_helper.h"
#include "routing/dataraptor.h"

#include "type/calendar.h"
#include "type/meta_vehicle_journey.h"

using namespace navitia;
using namespace navitia::routing;
using navitia::routing::StopEvent;

static std::pair<const JourneyPattern&, const JourneyPatternPoint&> get_first_jp_jpp(const ed::builder& b,
                                                                                     const std::string& sa) {
    const auto sp_idx = SpIdx(*b.data->pt_data->stop_areas_map[sa]->stop_point_list.front());
    const auto& jpp = b.data->dataRaptor->jpps_from_sp[sp_idx].front();
    return {b.data->dataRaptor->jp_container.get(jpp.jp_idx), b.data->dataRaptor->jp_container.get(jpp.idx)};
}

BOOST_AUTO_TEST_CASE(test1) {
    ed::builder b("20120614",
                  [&](ed::builder& b) { b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250); });

    std::vector<JppIdx> jpps;
    for (const auto jpp : b.data->dataRaptor->jp_container.get_jpps())
        jpps.push_back(jpp.first);

    auto result = get_stop_times(StopEvent::pick_up, jpps, navitia::DateTimeUtils::min,
                                 navitia::DateTimeUtils::set(1, 0), 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    BOOST_CHECK(std::any_of(result.begin(), result.end(),
                            [](datetime_stop_time& dt_st) { return dt_st.second->order() == nt::RankStopTime(0); }));
    BOOST_CHECK(std::any_of(result.begin(), result.end(),
                            [](datetime_stop_time& dt_st) { return dt_st.second->order() == nt::RankStopTime(1); }));

    result = get_stop_times(StopEvent::drop_off, jpps, navitia::DateTimeUtils::set(0, 86399),
                            navitia::DateTimeUtils::min, 100, *b.data, nt::RTLevel::Base, {});
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    BOOST_CHECK(std::any_of(result.begin(), result.end(),
                            [](datetime_stop_time& dt_st) { return dt_st.second->order() == nt::RankStopTime(1); }));
    BOOST_CHECK(std::any_of(result.begin(), result.end(),
                            [](datetime_stop_time& dt_st) { return dt_st.second->order() == nt::RankStopTime(2); }));
}

/**
 * Test get_all_stop_times for one calendar
 *
 * 3 vj going from sp1 to somewhere, 2 valid for our calendar, 1 invalid
 *
 * ==========     =====
 * stop point     sp1
 * === ========== =====
 * vj1 departure  8000
 * === ========== =====
 * vj2 departure  8100
 * === ========== =====
 * vj3 departure  9000
 * === ========== =====
 *
 */
BOOST_AUTO_TEST_CASE(test_calendar) {
    DateTime vj1_departure = 8000;
    DateTime vj2_departure = 8100;
    DateTime vj3_departure = 9000;
    std::string spa1 = "stop1";
    type::Calendar* cal = nullptr;

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A", "1010", "", true, "vj1")(spa1, vj1_departure, vj1_departure)("useless", 10000, 10000);
        b.vj("A", "1010", "", true, "vj2")(spa1, vj2_departure, vj2_departure)("useless", 11000, 11000);
        b.vj("A", "1111", "", true, "vj3")(spa1, vj3_departure, vj3_departure)("useless", 12000, 12000);

        cal = new type::Calendar(b.data->meta->production_date.begin());
        cal->uri = "cal1";

        for (auto vj_name : {"vj1", "vj2"}) {
            auto associated_cal = new type::AssociatedCalendar();
            associated_cal->calendar = cal;
            b.data->pt_data->meta_vjs.get_mut(vj_name)->associated_calendars.insert({cal->uri, associated_cal});
        }
    });

    auto jpp1 = get_first_jp_jpp(b, spa1);
    auto res = get_all_calendar_stop_times(jpp1.first, jpp1.second, cal->uri);

    BOOST_REQUIRE_EQUAL(res.size(), 2);

    // result are not sorted
    using p = std::pair<uint32_t, const type::StopTime*>;
    std::sort(res.begin(), res.end(), [](const p& p1, const p& p2) { return p1.first < p2.first; });

    auto first_elt = res[0];
    BOOST_CHECK_EQUAL(first_elt.first, vj1_departure);
    BOOST_REQUIRE(first_elt.second != nullptr);
    BOOST_CHECK_EQUAL(first_elt.second->departure_time, vj1_departure);
    BOOST_CHECK_EQUAL(first_elt.second->stop_point->stop_area->name, spa1);

    auto second_elt = res[1];
    BOOST_CHECK_EQUAL(second_elt.first, vj2_departure);
    BOOST_REQUIRE(second_elt.second != nullptr);
    BOOST_CHECK_EQUAL(second_elt.second->departure_time, vj2_departure);
    BOOST_CHECK_EQUAL(second_elt.second->stop_point->stop_area->name, spa1);
}

/**
 * Test calendars
 * ========== ===== =====
 * stop point sp1   sp2
 * ========== ===== =====
 * arrivee          8100
 * ========== ===== =====
 * depart     8000
 * ========== ===== =====
 *
 * Validity pattern: Not valid the first and third day, valid the second and fourth day
 *
 * no calendar associated => no answer
 *
 *
 */
BOOST_AUTO_TEST_CASE(test_no_calendar) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A", "1010", "", true)(spa1, sp1_departure, sp1_departure)(spa2, sp2_arrival, sp2_arrival);
    });

    auto jpp1 = get_first_jp_jpp(b, spa1);

    const std::string calendar = "calendar_bob";
    // calendar is not associated to stop time => no answer
    auto res = get_all_calendar_stop_times(jpp1.first, jpp1.second, calendar);
    BOOST_CHECK(res.empty());
}

/**
 * Test get_all_stop_times for one calendar
 *
 * 1 vj going from sp1 to somewhere, valid for our calendar
 * vj1 is a frequency vj
 *
 * ==========     =====
 * stop point     sp1
 * === ========== =====
 * vj1 departure  every 100s from 8000 to 9000
 * === ========== ===== =====
 *
 */
BOOST_AUTO_TEST_CASE(test_frequency_for_calendar) {
    std::string spa1 = "stop1";
    DateTime vj1_departure = 8000;
    size_t headway_sec = 100;
    type::Calendar* cal = nullptr;

    ed::builder b("20120614", [&](ed::builder& b) {
        b.frequency_vj("A", vj1_departure, 9000, headway_sec, "default_network", "1010", "", true, "vj1")(
            spa1, vj1_departure, vj1_departure)("useless_stop", 10000, 10000);

        cal = new type::Calendar(b.data->meta->production_date.begin());
        cal->uri = "cal1";

        auto associated_cal = new type::AssociatedCalendar();
        associated_cal->calendar = cal;
        b.data->pt_data->meta_vjs.get_mut("vj1")->associated_calendars.insert({cal->uri, associated_cal});
    });

    auto jpp1 = get_first_jp_jpp(b, spa1);
    auto res = get_all_calendar_stop_times(jpp1.first, jpp1.second, cal->uri);

    BOOST_REQUIRE_EQUAL(res.size(), 11);

    // result are not sorted
    using p = std::pair<uint32_t, const type::StopTime*>;
    std::sort(res.begin(), res.end(), [](const p& p1, const p& p2) { return p1.first < p2.first; });

    auto first_elt = res[0];
    BOOST_CHECK_EQUAL(first_elt.first, vj1_departure);
    BOOST_REQUIRE(first_elt.second != nullptr);
    BOOST_CHECK_EQUAL(first_elt.second->stop_point->stop_area->name, spa1);

    // then all vj1 departures
    for (size_t i = 1; i < res.size(); ++i) {
        auto departure = vj1_departure + headway_sec * i;
        BOOST_CHECK_EQUAL(res[i].first, departure);
        BOOST_REQUIRE(res[i].second != nullptr);
        BOOST_CHECK_EQUAL(res[i].second->stop_point->stop_area->name, spa1);
    }
}

/**
 * Test get_all_stop_times for one calendar
 * We want to test in this test a frequency vj 'looping' through the day
 *
 * 2 vj going from sp1 to somewhere, valid for our calendar
 * vj1 is a frequency vj, it's end is lower than it's begin
 *
 * ==========     =====
 * stop point     sp1
 * === ========== =====
 * vj1 departure  every 100s from 70000 to 1000
 *
 */
BOOST_AUTO_TEST_CASE(test_looping_frequency_for_calendar) {
    std::string spa1 = "stop1";
    DateTime vj1_departure = 70000;
    size_t headway_sec = 1000;
    type::Calendar* cal = nullptr;

    ed::builder b("20120614", [&](ed::builder& b) {
        b.frequency_vj("A", vj1_departure, 2001, headway_sec, "default_network", "1010", "", true, "vj1")(
            spa1, vj1_departure, vj1_departure)("useless_stop", 87400, 87400);

        cal = new type::Calendar(b.data->meta->production_date.begin());
        cal->uri = "cal1";

        auto associated_cal = new type::AssociatedCalendar();
        associated_cal->calendar = cal;
        b.data->pt_data->meta_vjs.get_mut("vj1")->associated_calendars.insert({cal->uri, associated_cal});
    });

    auto jpp1 = get_first_jp_jpp(b, spa1);
    auto res = get_all_calendar_stop_times(jpp1.first, jpp1.second, cal->uri);

    // end of day is 86400, so we have (86400 - 70000) / 1000 + the stop on the morning (2001 / 1000)
    BOOST_REQUIRE_EQUAL(res.size(),
                        (DateTimeUtils::SECONDS_PER_DAY - vj1_departure) / headway_sec + 2001 / headway_sec);
}

/**
 * Test get_all_stop_times for one calendar
 * We want to test in this test a frequency vj with end > 24:00
 *
 * 2 vj going from sp1 to somewhere, valid for our calendar
 * vj1 is a frequency vj
 *
 * ==========     =====
 * stop point     sp1
 * === ========== =====
 * vj1 departure  every 100s from 70000 to 90000
 *
 */
BOOST_AUTO_TEST_CASE(test_frequency_over_midnight_for_calendar) {
    std::string spa1 = "stop1";
    DateTime vj1_departure = 70000;
    size_t headway_sec = 1000;
    type::Calendar* cal = nullptr;

    ed::builder b("20120614", [&](ed::builder& b) {
        b.frequency_vj("A", vj1_departure, 90001, headway_sec, "default_network", "1010", "", true, "vj1")(
            spa1, vj1_departure, vj1_departure)("useless_stop", 87400, 87400);

        cal = new type::Calendar(b.data->meta->production_date.begin());
        cal->uri = "cal1";

        auto associated_cal = new type::AssociatedCalendar();
        associated_cal->calendar = cal;
        b.data->pt_data->meta_vjs.get_mut("vj1")->associated_calendars.insert({cal->uri, associated_cal});
    });

    auto jpp1 = get_first_jp_jpp(b, spa1);
    auto res = get_all_calendar_stop_times(jpp1.first, jpp1.second, cal->uri);

    // end of day is 86400, so we have (90001 - 70000) / 1000 + 1 (for the last second)
    BOOST_REQUIRE_EQUAL(res.size(), (90001 - 70000) / headway_sec + 1);
}

struct departure_helper {
    departure_helper() : b("20150907") {}
    std::vector<JppIdx> get_jpp_idx(const std::string& stop_point_id) {
        std::vector<JppIdx> jpp_stop;
        const auto sp_idx = SpIdx(*b.data->pt_data->stop_points_map.at(stop_point_id));
        const auto& vec_jpp_stop = b.data->dataRaptor->jpps_from_sp[sp_idx];
        for (const auto& jpp : vec_jpp_stop) {
            jpp_stop.push_back(jpp.idx);
        }
        return jpp_stop;
    }

    ed::builder b;
    const navitia::DateTime yesterday = "00:00"_t;
    const navitia::DateTime yesterday_8h45 = "8:45"_t;
    const navitia::DateTime today = "24:00"_t;
    const navitia::DateTime today_8h45 = "24:00"_t + "8:45"_t;
    const navitia::DateTime tomorrow = "48:00"_t;
};

/**
 *       stop1            stop2            stop3            stop4
 * A: 08:00-08:01      09:00-09:01      10:00-10:01          ---
 * B:     ---          08:30-08:31      09:30-09:31      10:30-10:31
 *
 * First arrivals and last departures are useless though
 * Checking that next arrivals and previous departures are ok at each stop_point
 */
BOOST_FIXTURE_TEST_CASE(test_discrete_next_arrivals_prev_departures, departure_helper) {
    b.vj("A")("stop1", "8:00"_t, "8:01"_t)("stop2", "9:00"_t, "9:01"_t)("stop3", "10:00"_t, "10:01"_t);
    b.vj("B")("stop2", "8:30"_t, "8:31"_t)("stop3", "9:30"_t, "9:31"_t)("stop4", "10:30"_t, "10:31"_t);
    b.make();

    auto result_next_arrivals =
        get_stop_times(StopEvent::drop_off, get_jpp_idx("stop1"), today, tomorrow, 100, *b.data, nt::RTLevel::Base);
    BOOST_CHECK_EQUAL(result_next_arrivals.size(), 0);
    auto result_prev_departures =
        get_stop_times(StopEvent::pick_up, get_jpp_idx("stop1"), today, yesterday, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result_prev_departures.size(), 1);
    BOOST_CHECK_EQUAL(result_prev_departures.at(0).first, "8:01"_t);

    result_next_arrivals =
        get_stop_times(StopEvent::drop_off, get_jpp_idx("stop2"), today, tomorrow, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result_next_arrivals.size(), 1);
    BOOST_CHECK_EQUAL(result_next_arrivals.at(0).first, "24:00"_t + "9:00"_t);
    result_prev_departures =
        get_stop_times(StopEvent::pick_up, get_jpp_idx("stop2"), today, yesterday, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result_prev_departures.size(), 2);
    BOOST_CHECK_EQUAL(result_prev_departures.at(0).first, "9:01"_t);
    BOOST_CHECK_EQUAL(result_prev_departures.at(1).first, "8:31"_t);
    result_prev_departures =
        get_stop_times(StopEvent::pick_up, get_jpp_idx("stop2"), today_8h45, today, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result_prev_departures.size(), 1);
    BOOST_CHECK_EQUAL(result_prev_departures.at(0).first, "24:00"_t + "8:31"_t);
    result_prev_departures = get_stop_times(StopEvent::pick_up, get_jpp_idx("stop2"), today_8h45, yesterday_8h45, 100,
                                            *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result_prev_departures.size(), 2);
    BOOST_CHECK_EQUAL(result_prev_departures.at(0).first, "24:00"_t + "8:31"_t);
    BOOST_CHECK_EQUAL(result_prev_departures.at(1).first, "9:01"_t);

    result_next_arrivals =
        get_stop_times(StopEvent::drop_off, get_jpp_idx("stop3"), today, tomorrow, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result_next_arrivals.size(), 2);
    BOOST_CHECK_EQUAL(result_next_arrivals.at(0).first, "24:00"_t + "09:30"_t);
    BOOST_CHECK_EQUAL(result_next_arrivals.at(1).first, "24:00"_t + "10:00"_t);
    result_prev_departures =
        get_stop_times(StopEvent::pick_up, get_jpp_idx("stop3"), today, yesterday, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result_prev_departures.size(), 1);
    BOOST_CHECK_EQUAL(result_prev_departures.at(0).first, "9:31"_t);

    result_next_arrivals =
        get_stop_times(StopEvent::drop_off, get_jpp_idx("stop4"), today, tomorrow, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(result_next_arrivals.size(), 1);
    BOOST_CHECK_EQUAL(result_next_arrivals.at(0).first, "24:00"_t + "10:30"_t);
    result_prev_departures =
        get_stop_times(StopEvent::pick_up, get_jpp_idx("stop4"), today, yesterday, 100, *b.data, nt::RTLevel::Base);
    BOOST_CHECK_EQUAL(result_prev_departures.size(), 0);
}

/**
 * test departures on lolipop VJ
 *
 * s1-----------s2------------s3
 *              |              |
 *              |              |
 *              ----------------
 *
 *
 */
BOOST_FIXTURE_TEST_CASE(test_discrete_lolipop, departure_helper) {
    b.vj("A")("s1", "8:00"_t, "8:01"_t)("s2", "9:00"_t, "9:01"_t)("s3", "10:00"_t, "10:01"_t)("s2", "11:00"_t,
                                                                                              "11:01"_t);

    b.make();

    // at s2, we have 2 arrivals
    auto next_arrivals =
        get_stop_times(StopEvent::drop_off, get_jpp_idx("s2"), today, tomorrow, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(next_arrivals.size(), 2);
    BOOST_CHECK_EQUAL(next_arrivals.at(0).first, "24:00"_t + "9:00"_t);
    BOOST_CHECK_EQUAL(next_arrivals.at(1).first, "24:00"_t + "11:00"_t);

    // b ut only one departure (the last does not count, it is the terminus)
    auto next_departures =
        get_stop_times(StopEvent::pick_up, get_jpp_idx("s2"), today, tomorrow, 100, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(next_departures.size(), 1);
    BOOST_CHECK_EQUAL(next_departures.at(0).first, "24:00"_t + "9:01"_t);
}

/*
 * small test to check the priority queue used in get_stop_times
 */
BOOST_AUTO_TEST_CASE(queue_comp_test_clockwise) {
    bool clockwise{true};
    JppStQueue q({clockwise});
    q.push({JppIdx(0), nullptr, 12});
    q.push({JppIdx(1), nullptr, 42});
    q.push({JppIdx(2), nullptr, 6});

    // for clockwise, we want the smallest first
    BOOST_CHECK_EQUAL(q.top().dt, 6);
    q.pop();
    BOOST_CHECK_EQUAL(q.top().dt, 12);
}

BOOST_AUTO_TEST_CASE(queue_comp_test_anti_clockwise) {
    bool clockwise{false};
    JppStQueue q({clockwise});
    q.push({JppIdx(0), nullptr, 12});
    q.push({JppIdx(1), nullptr, 42});
    q.push({JppIdx(2), nullptr, 6});

    // for clockwise, we want the greatest first
    BOOST_CHECK_EQUAL(q.top().dt, 42);
    q.pop();
    BOOST_CHECK_EQUAL(q.top().dt, 12);
}
/**
 *
 * There are 3 lines with multiple vj that pass through the 'center' station.
 *
 *                                        Center
 *
 * A: A1   9H
 *    A2                 10h30
 *    A3                      11h
 *
 * B: B1                                     19H
 *    B2                                             20H30
 *    B3                                                 21H
 *
 * C: C1          9H45
 *    C2             9H55
 *    C3                                                             23h
 *
 * Check that the departures and arrivals at that station are correctly ordered and filtered
 *
 */
BOOST_FIXTURE_TEST_CASE(dep_arr_filtering, departure_helper) {
    b.vj("A", "1111", "", true, "A1")("x", "08:00"_t, "08:01"_t)("center", "09:00"_t, "09:01"_t)("y", "10:00"_t,
                                                                                                 "10:01"_t);
    b.vj("A", "1111", "", true, "A2")("x", "10:00"_t, "10:01"_t)("center", "10:30"_t, "10:31"_t)("y", "11:00"_t,
                                                                                                 "11:01"_t);
    b.vj("A", "1111", "", true, "A3")("x", "10:30"_t, "10:31"_t)("center", "11:00"_t, "11:01"_t)("y", "11:30"_t,
                                                                                                 "11:31"_t);

    b.vj("B", "1111", "", true, "B1")("x", "18:00"_t, "18:01"_t)("center", "19:00"_t, "19:01"_t)("y", "20:00"_t,
                                                                                                 "20:01"_t);
    b.vj("B", "1111", "", true, "B2")("x", "20:00"_t, "20:01"_t)("center", "20:30"_t, "20:31"_t)("y", "21:00"_t,
                                                                                                 "21:01"_t);
    b.vj("B", "1111", "", true, "B3")("x", "20:30"_t, "20:31"_t)("center", "21:00"_t, "21:01"_t)("y", "21:30"_t,
                                                                                                 "21:31"_t);

    b.vj("C", "1111", "", true, "C1")("x", "08:00"_t, "08:01"_t)("center", "09:45"_t, "09:46"_t)("y", "10:00"_t,
                                                                                                 "10:01"_t);
    b.vj("C", "1111", "", true, "C2")("x", "08:10"_t, "08:11"_t)("center", "09:55"_t, "09:56"_t)("y", "11:00"_t,
                                                                                                 "11:01"_t);
    b.vj("C", "1111", "", true, "C3")("x", "10:30"_t, "10:31"_t)("center", "23:45"_t, "23:56"_t)("y", "23:55"_t,
                                                                                                 "23:56"_t);

    b.make();

    auto next_arrivals =
        get_stop_times(StopEvent::drop_off, get_jpp_idx("center"), today, tomorrow, 5, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(next_arrivals.size(), 5);
    BOOST_CHECK_EQUAL(next_arrivals.at(0).first, today + "9:00"_t);
    BOOST_CHECK_EQUAL(next_arrivals.at(1).first, today + "9:45"_t);
    BOOST_CHECK_EQUAL(next_arrivals.at(2).first, today + "9:55"_t);
    BOOST_CHECK_EQUAL(next_arrivals.at(3).first, today + "10:30"_t);
    BOOST_CHECK_EQUAL(next_arrivals.at(4).first, today + "11:00"_t);

    auto next_departures =
        get_stop_times(StopEvent::pick_up, get_jpp_idx("center"), today, tomorrow, 5, *b.data, nt::RTLevel::Base);
    BOOST_CHECK_EQUAL(next_departures.at(0).first, today + "9:01"_t);
    BOOST_CHECK_EQUAL(next_departures.at(1).first, today + "9:46"_t);
    BOOST_CHECK_EQUAL(next_departures.at(2).first, today + "9:56"_t);
    BOOST_CHECK_EQUAL(next_departures.at(3).first, today + "10:31"_t);
    BOOST_CHECK_EQUAL(next_departures.at(4).first, today + "11:01"_t);

    auto prev_departures =
        get_stop_times(StopEvent::pick_up, get_jpp_idx("center"), today, yesterday, 5, *b.data, nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(prev_departures.size(), 5);
    BOOST_CHECK_EQUAL(prev_departures.at(0).first, "23:56"_t);
    BOOST_CHECK_EQUAL(prev_departures.at(1).first, "21:01"_t);
    BOOST_CHECK_EQUAL(prev_departures.at(2).first, "20:31"_t);
    BOOST_CHECK_EQUAL(prev_departures.at(3).first, "19:01"_t);
    BOOST_CHECK_EQUAL(prev_departures.at(4).first, "11:01"_t);
}
