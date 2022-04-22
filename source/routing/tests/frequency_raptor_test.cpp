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
#define BOOST_TEST_MODULE test_raptor
#include <boost/test/unit_test.hpp>
#include "routing/raptor.h"
#include "routing/routing.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "utils/logger.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

using namespace navitia;
using namespace routing;
namespace bt = boost::posix_time;

BOOST_AUTO_TEST_CASE(freq_vj) {
    ed::builder b("20120614");
    b.frequency_vj("A1", "8:00"_t, "18:00"_t, "00:05"_t)("stop1", "8:00"_t)("stop2", "8:10"_t);

    b.make();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop2"), 8 * 3600, 0,
                               DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), 8 * 3600 + 10 * 60);

    res1 = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop2"), 9 * 3600, 0, DateTimeUtils::inf,
                          type::RTLevel::Base, 2_min, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), 9 * 3600 + 10 * 60);
}

BOOST_AUTO_TEST_CASE(freq_vj_pam) {
    ed::builder b("20120614");
    b.frequency_vj("A1", 8 * 3600, 26 * 3600, 5 * 60)("stop1", 8 * 3600)("stop2", 8 * 3600 + 10 * 60);

    b.make();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop2"), 23 * 3600, 0,
                               DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), 23 * 3600 + 10 * 60);

    /*auto*/ res1 = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop2"), 24 * 3600, 0,
                                   DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res1[0].items[0].arrival, *(b.data))), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(),
                      (24 * 3600 + 10 * 60) % DateTimeUtils::SECONDS_PER_DAY);

    /*auto*/ res1 = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop2"), 25 * 3600, 0,
                                   DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res1[0].items[0].arrival, *(b.data))), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(),
                      (25 * 3600 + 10 * 60) % DateTimeUtils::SECONDS_PER_DAY);
}

BOOST_AUTO_TEST_CASE(freq_vj_stop_times) {
    ed::builder b("20120614");
    b.frequency_vj("A1", 8 * 3600, 18 * 3600, 60 * 60)("stop1", 8 * 3600, 8 * 3600 + 10 * 60)(
        "stop2", 8 * 3600 + 15 * 60, 8 * 3600 + 25 * 60)("stop3", 8 * 3600 + 30 * 60, 8 * 3600 + 40 * 60);

    b.make();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    const auto check_journey = [](const Path& p) {
        const auto& section = p.items[0];
        BOOST_CHECK_EQUAL(section.departure.time_of_day().total_seconds(), 9 * 3600 + 10 * 60);
        BOOST_CHECK_EQUAL(section.arrival.time_of_day().total_seconds(), 9 * 3600 + 30 * 60);
        BOOST_REQUIRE_EQUAL(section.stop_times.size(), 3);
        BOOST_REQUIRE_EQUAL(section.departures.size(), 3);
        BOOST_REQUIRE_EQUAL(section.arrivals.size(), 3);
        BOOST_CHECK_EQUAL(section.arrivals[0], "20120614T090000"_dt);
        BOOST_CHECK_EQUAL(section.departures[0], "20120614T091000"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[1], "20120614T091500"_dt);
        BOOST_CHECK_EQUAL(section.departures[1], "20120614T092500"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[2], "20120614T093000"_dt);
        BOOST_CHECK_EQUAL(section.departures[2], "20120614T094000"_dt);
    };

    auto res_earliest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop3"), 9 * 3600, 0,
                                       DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res_earliest.size(), 1);
    check_journey(res_earliest[0]);

    // same but tardiest departure
    auto res_tardiest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop3"), 9 * 3600 + 30 * 60,
                                       0, DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res_tardiest.size(), 1);
    check_journey(res_tardiest[0]);
}

/*
 * Test simple frequency trip with different stops duration
 */
BOOST_AUTO_TEST_CASE(freq_vj_different_departure_arrival_duration) {
    // we want to test that everything is fine for a frequency vj
    // with different duration in each stops
    ed::builder b("20120614");
    b.frequency_vj("A1", 8 * 3600, 18 * 3600, 60 * 60)("stop1", 8 * 3600, 8 * 3600 + 10 * 60)  // 10min
        ("stop2", 8 * 3600 + 15 * 60, 8 * 3600 + 40 * 60)                                      // 25mn
        ("stop3", 9 * 3600, 9 * 3600 + 5 * 60);                                                // 5mn

    b.make();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    const auto check_journey = [](const Path& p) {
        const auto& section = p.items[0];
        BOOST_CHECK_EQUAL(section.departure.time_of_day().total_seconds(), 9 * 3600 + 10 * 60);
        BOOST_CHECK_EQUAL(section.arrival.time_of_day().total_seconds(), 10 * 3600);
        BOOST_REQUIRE_EQUAL(section.stop_times.size(), 3);
        BOOST_REQUIRE_EQUAL(section.departures.size(), 3);
        BOOST_REQUIRE_EQUAL(section.arrivals.size(), 3);
        BOOST_CHECK_EQUAL(section.arrivals[0], "20120614T090000"_dt);
        BOOST_CHECK_EQUAL(section.departures[0], "20120614T091000"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[1], "20120614T091500"_dt);
        BOOST_CHECK_EQUAL(section.departures[1], "20120614T094000"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[2], "20120614T100000"_dt);
        BOOST_CHECK_EQUAL(section.departures[2], "20120614T100500"_dt);
    };

    auto res_earliest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop3"), 9 * 3600 + 5 * 60, 0,
                                       DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res_earliest.size(), 1);
    check_journey(res_earliest[0]);

    // same but tardiest departure
    auto res_tardiest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop3"), 10 * 3600, 0,
                                       DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res_tardiest.size(), 1);
    check_journey(res_tardiest[0]);
}

/*
 * Test overmidnight cases with different arrival/departure time on the stops
 */
BOOST_AUTO_TEST_CASE(freq_vj_overmidnight_different_dep_arr) {
    ed::builder b("20120612");
    b.frequency_vj("A1", 8 * 3600, 26 * 3600, 60 * 60)("stop1", 8 * 3600, 8 * 3600 + 10 * 60)(
        "stop2", 8 * 3600 + 30 * 60, 8 * 3600 + 35 * 60)("stop3", 9 * 3600 + 20 * 60, 9 * 3600 + 40 * 60);

    b.make();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    const auto check_journey = [](const Path& p) {
        const auto& section = p.items[0];
        BOOST_CHECK_EQUAL(section.departure, "20120614T231000"_dt);
        // we arrive at 00:20
        BOOST_CHECK_EQUAL(section.arrival, "20120615T002000"_dt);
        BOOST_REQUIRE_EQUAL(section.stop_times.size(), 3);
        BOOST_REQUIRE_EQUAL(section.departures.size(), 3);
        BOOST_REQUIRE_EQUAL(section.arrivals.size(), 3);
        BOOST_CHECK_EQUAL(section.arrivals[0], "20120614T230000"_dt);
        BOOST_CHECK_EQUAL(section.departures[0], "20120614T231000"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[1], "20120614T233000"_dt);
        BOOST_CHECK_EQUAL(section.departures[1], "20120614T233500"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[2], "20120615T002000"_dt);
        BOOST_CHECK_EQUAL(section.departures[2], "20120615T004000"_dt);
    };

    // leaving at 22:15, we have to wait for the next departure at 23:10
    auto res_earliest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop3"), 22 * 3600 + 15 * 60,
                                       2, DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res_earliest.size(), 1);
    check_journey(res_earliest[0]);

    // wwe want to arrive before 01:00 we'll take the same trip
    auto res_tardiest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop3"), 1 * 3600, 3,
                                       DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res_earliest.size(), 1);
    check_journey(res_earliest[0]);
}

BOOST_AUTO_TEST_CASE(freq_vj_transfer_with_regular_vj) {
    ed::builder b("20120614");

    b.frequency_vj("A", "8:00"_t, "26:00"_t, "1:00"_t)("stop1", "8:00"_t, "8:10"_t)("stop2", "8:30"_t, "8:35"_t)(
        "stop3", "9:20"_t, "9:40"_t);

    b.vj("B")("stop4", "12:00"_t, "12:05"_t)("stop2", "12:10"_t, "12:11"_t)("stop5", "12:30"_t, "12:35"_t);

    b.connection("stop2", "stop2", 120);

    b.make();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    const auto check_journey = [](const Path& p) {
        BOOST_REQUIRE_EQUAL(p.items.size(), 3);

        const auto& section = p.items[0];
        // we arrive at 11:30
        BOOST_CHECK_EQUAL(section.departure, "20120616T111000"_dt);
        BOOST_CHECK_EQUAL(section.arrival, "20120616T113000"_dt);
        BOOST_REQUIRE_EQUAL(section.stop_times.size(), 2);
        BOOST_REQUIRE_EQUAL(section.departures.size(), 2);
        BOOST_REQUIRE_EQUAL(section.arrivals.size(), 2);
        BOOST_CHECK_EQUAL(section.arrivals[0], "20120616T110000"_dt);
        BOOST_CHECK_EQUAL(section.departures[0], "20120616T111000"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[1], "20120616T113000"_dt);
        BOOST_CHECK_EQUAL(section.departures[1], "20120616T113500"_dt);

        // waiting section
        const auto& waiting_section = p.items[1];
        BOOST_CHECK_EQUAL(waiting_section.departure, "20120616T113000"_dt);
        BOOST_CHECK_EQUAL(waiting_section.arrival, "20120616T121100"_dt);
        BOOST_REQUIRE_EQUAL(waiting_section.stop_times.size(), 0);
        BOOST_REQUIRE_EQUAL(waiting_section.departures.size(), 0);
        BOOST_REQUIRE_EQUAL(waiting_section.arrivals.size(), 0);

        const auto& second_section = p.items[2];
        BOOST_CHECK_EQUAL(second_section.departure, "20120616T121100"_dt);
        BOOST_CHECK_EQUAL(second_section.arrival, "20120616T123000"_dt);
        BOOST_REQUIRE_EQUAL(second_section.stop_times.size(), 2);
        BOOST_REQUIRE_EQUAL(second_section.departures.size(), 2);
        BOOST_REQUIRE_EQUAL(second_section.arrivals.size(), 2);
        BOOST_CHECK_EQUAL(second_section.arrivals[0], "20120616T121000"_dt);
        BOOST_CHECK_EQUAL(second_section.departures[0], "20120616T121100"_dt);
        BOOST_CHECK_EQUAL(second_section.arrivals[1], "20120616T123000"_dt);
        BOOST_CHECK_EQUAL(second_section.departures[1], "20120616T123500"_dt);
    };

    // leaving after 10:20, we have to wait for the next bus at 11:15
    // then we can catch the bus B at 12:10 and finish at 12:30
    auto res_earliest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop5"), "10:20"_t, 2,
                                       DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res_earliest.size(), 1);
    check_journey(res_earliest[0]);

    auto res_tardiest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop5"), "12:40"_t, 2,
                                       DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res_tardiest.size(), 1);
    check_journey(res_tardiest[0]);
}

BOOST_AUTO_TEST_CASE(transfer_between_freq) {
    ed::builder b("20120614");

    b.frequency_vj("A", "8:00"_t, "26:00"_t, "1:00"_t)("stop1", "8:00"_t, "8:10"_t)("stop2", "8:30"_t, "8:30"_t)(
        "stop3", "9:20"_t, "9:40"_t);

    b.frequency_vj("B", "14:00"_t, "20:00"_t, "00:10"_t)("stop4", "8:00"_t, "8:14"_t)("stop5", "8:35"_t, "8:44"_t)(
        "stop6", "9:25"_t, "9:34"_t);

    b.connection("stop3", "stop4", "00:20"_t);

    b.make();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    const auto check_journey = [](const Path& p) {
        BOOST_REQUIRE_EQUAL(p.items.size(), 4);

        const auto& section = p.items[0];
        BOOST_CHECK_EQUAL(section.departure, "20120616T121000"_dt);
        BOOST_CHECK_EQUAL(section.arrival, "20120616T132000"_dt);
        BOOST_REQUIRE_EQUAL(section.stop_times.size(), 3);
        BOOST_REQUIRE_EQUAL(section.departures.size(), 3);
        BOOST_REQUIRE_EQUAL(section.arrivals.size(), 3);
        BOOST_CHECK_EQUAL(section.arrivals[0], "20120616T120000"_dt);
        BOOST_CHECK_EQUAL(section.departures[0], "20120616T121000"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[1], "20120616T123000"_dt);
        BOOST_CHECK_EQUAL(section.departures[1], "20120616T123000"_dt);
        BOOST_CHECK_EQUAL(section.arrivals[2], "20120616T132000"_dt);
        BOOST_CHECK_EQUAL(section.departures[2], "20120616T134000"_dt);

        // transfer section
        const auto& transfer_section = p.items[1];
        BOOST_CHECK_EQUAL(transfer_section.departure, "20120616T132000"_dt);
        BOOST_CHECK_EQUAL(transfer_section.arrival, "20120616T134000"_dt);
        BOOST_REQUIRE_EQUAL(transfer_section.stop_times.size(), 0);
        BOOST_REQUIRE_EQUAL(transfer_section.departures.size(), 0);
        BOOST_REQUIRE_EQUAL(transfer_section.arrivals.size(), 0);

        // waiting section
        const auto& waiting_section = p.items[2];
        BOOST_CHECK_EQUAL(waiting_section.departure, "20120616T134000"_dt);
        BOOST_CHECK_EQUAL(waiting_section.arrival, "20120616T141400"_dt);
        BOOST_REQUIRE_EQUAL(waiting_section.stop_times.size(), 0);
        BOOST_REQUIRE_EQUAL(waiting_section.departures.size(), 0);
        BOOST_REQUIRE_EQUAL(waiting_section.arrivals.size(), 0);

        const auto& second_section = p.items[3];
        BOOST_CHECK_EQUAL(second_section.departure, "20120616T141400"_dt);
        BOOST_CHECK_EQUAL(second_section.arrival, "20120616T143500"_dt);
        BOOST_REQUIRE_EQUAL(second_section.stop_times.size(), 2);
        BOOST_REQUIRE_EQUAL(second_section.departures.size(), 2);
        BOOST_REQUIRE_EQUAL(second_section.arrivals.size(), 2);
        BOOST_CHECK_EQUAL(second_section.arrivals[0], "20120616T140000"_dt);
        BOOST_CHECK_EQUAL(second_section.departures[0], "20120616T141400"_dt);
        BOOST_CHECK_EQUAL(second_section.arrivals[1], "20120616T143500"_dt);
        BOOST_CHECK_EQUAL(second_section.departures[1], "20120616T144400"_dt);
    };

    // leaving after 11:10, we would have to wait for the bus B after so the 2nd pass makes us wait
    // so we leave at 12:10 to arrive at 13:20
    // then we have to wait for the begin of the bust B at 14:04 and finish at 14:35
    auto res_earliest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop5"), "11:10"_t, 2,
                                       DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);

    BOOST_REQUIRE_EQUAL(res_earliest.size(), 1);
    check_journey(res_earliest[0]);

    auto res_tardiest = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop5"), "14:35"_t, 2,
                                       DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res_tardiest.size(), 1);
    check_journey(res_tardiest[0]);
}

/*
 * In this case, we test the case where a freq vj's end_time is smaller than the start_time
 * due to the UTC conversion
 *
 * 0-------------------------------------86400(midnight)
 *  -------end                 start--------
 *         6:00                20:00
 * */
BOOST_AUTO_TEST_CASE(freq_vj_end_time_is_smaller_than_start_time) {
    ed::builder b("20120614");

    b.frequency_vj("A", "20:00"_t, "6:00"_t, "1:00"_t, "network_bob", "00111100")("stop1", "20:00"_t, "20:10"_t)(
        "stop2", "21:00"_t, "21:10"_t)("stop3", "22:00"_t, "22:10"_t);

    b.make();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto compute = [&](int day) {
        return raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop3"), "21:30"_t, day,
                              DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min, true);
    };

    auto res = compute(1);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    res = compute(2);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    res = compute(3);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    res = compute(5);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    res = compute(6);
    BOOST_REQUIRE_EQUAL(res.size(), 0);
}

BOOST_AUTO_TEST_CASE(freq_vj_with_boarding_alighting) {
    ed::builder b("20170101");
    b.frequency_vj("A", "8:00"_t, "18:00"_t, "00:30"_t)("stop1", "8:00"_t, "8:02"_t,
                                                        std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
        "stop2", "8:05"_t, "8:07"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 900)(
        "stop3", "8:10"_t, "8:12"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

    b.make();
    RAPTOR raptor(*(b.data));

    auto result = raptor.compute(b.data->pt_data->stop_areas_map["stop1"], b.data->pt_data->stop_areas_map["stop3"],
                                 "07:58"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 3);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20170101T082700"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20170101T083200"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20170101T083200"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20170101T084000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[2].departure, "20170101T084000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].arrival, "20170101T084500"_dt);

    result = raptor.compute(b.data->pt_data->stop_areas_map["stop1"], b.data->pt_data->stop_areas_map["stop3"],
                            "08:43"_t, 0, DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 3);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20170101T075700"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20170101T080200"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20170101T080200"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20170101T081000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[2].departure, "20170101T081000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].arrival, "20170101T081500"_dt);
}

BOOST_AUTO_TEST_CASE(transfer_between_freq_with_boarding_alighting) {
    ed::builder b("20170101");
    b.frequency_vj("A", "8:00"_t, "18:00"_t, "00:30"_t)("stop1", "8:00"_t, "8:00"_t,
                                                        std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
        "stop2", "8:05"_t, "8:05"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 900)(
        "stop3", "8:10"_t, "8:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

    b.frequency_vj("B", "12:15"_t, "16:15"_t, "00:45"_t)("stop3", "12:15"_t, "12:15"_t,
                                                         std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
        "stop4", "12:30"_t, "12:30"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)(
        "stop5", "12:45"_t, "12:45"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

    b.connection("stop3", "stop3", 120);

    b.make();
    RAPTOR raptor(*(b.data));

    auto result = raptor.compute(b.data->pt_data->stop_areas_map["stop1"], b.data->pt_data->stop_areas_map["stop5"],
                                 "08:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 7);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20170101T112500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20170101T113000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20170101T113000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20170101T114000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[2].departure, "20170101T114000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].arrival, "20170101T114500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].type, ItemType::waiting);
    BOOST_CHECK_EQUAL(result.at(0).items[3].departure, "20170101T114500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].arrival, "20170101T121000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[4].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[4].departure, "20170101T121000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[4].arrival, "20170101T121500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[5].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[5].departure, "20170101T121500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[5].arrival, "20170101T124500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[6].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[6].departure, "20170101T124500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[6].arrival, "20170101T125000"_dt);

    result = raptor.compute(b.data->pt_data->stop_areas_map["stop1"], b.data->pt_data->stop_areas_map["stop5"],
                            "18:00"_t, 0, DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 7);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20170101T152500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20170101T153000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20170101T153000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20170101T154000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[2].departure, "20170101T154000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].arrival, "20170101T154500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].type, ItemType::waiting);
    BOOST_CHECK_EQUAL(result.at(0).items[3].departure, "20170101T154500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].arrival, "20170101T155500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[4].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[4].departure, "20170101T155500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[4].arrival, "20170101T160000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[5].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[5].departure, "20170101T160000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[5].arrival, "20170101T163000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[6].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[6].departure, "20170101T163000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[6].arrival, "20170101T163500"_dt);
}

BOOST_AUTO_TEST_CASE(transfer_pass_midnight_freq_vj_to_vj_with_boarding_time) {
    ed::builder b("20170101");

    b.frequency_vj("A", "22:00"_t, "29:00"_t, "01:00"_t, "network:1", "0000001")(
        "stop1", "22:00"_t, "22:10"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 600)(
        "stop2", "22:30"_t, "22:40"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 600)(
        "stop3", "23:00"_t, "23:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 600);

    b.vj("B", "1111111", "block1")
        .name("vj:B1")("stop4", "06:15"_t, "06:15"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
            "stop5", "06:30"_t, "06:30"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 0)(
            "stop4", "06:45"_t, "06:45"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

    b.vj("B", "1111111", "block1")
        .name("vj:B2")("stop4", "08:15"_t, "08:15"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
            "stop5", "08:30"_t, "08:30"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 0)(
            "stop4", "08:45"_t, "08:45"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

    b.connection("stop3", "stop4", "00:15"_t);

    b.make();
    RAPTOR raptor(*(b.data));

    // We should take the last possible vj on A in order to get vj:B1
    auto result = raptor.compute(b.data->pt_data->stop_areas_map["stop1"], b.data->pt_data->stop_areas_map["stop5"],
                                 "22:10"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, 2_min);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 6);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20170102T040000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20170102T041000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20170102T041000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20170102T050000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].type, ItemType::walking);
    BOOST_CHECK_EQUAL(result.at(0).items[2].departure, "20170102T050000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].arrival, "20170102T051500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].type, ItemType::waiting);
    BOOST_CHECK_EQUAL(result.at(0).items[3].departure, "20170102T051500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].arrival, "20170102T061000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[4].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[4].departure, "20170102T061000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[4].arrival, "20170102T061500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[5].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[5].departure, "20170102T061500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[5].arrival, "20170102T063000"_dt);

    // We should take the last vj of the frequency (at 5am) and wait two hours to get vj:B2
    result = raptor.compute(b.data->pt_data->stop_areas_map["stop1"], b.data->pt_data->stop_areas_map["stop5"],
                            "09:00"_t, 1, DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 6);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20170102T050000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20170102T051000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20170102T051000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20170102T060000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].type, ItemType::walking);
    BOOST_CHECK_EQUAL(result.at(0).items[2].departure, "20170102T060000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].arrival, "20170102T061500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].type, ItemType::waiting);
    BOOST_CHECK_EQUAL(result.at(0).items[3].departure, "20170102T061500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].arrival, "20170102T081000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[4].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[4].departure, "20170102T081000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[4].arrival, "20170102T081500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[5].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[5].departure, "20170102T081500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[5].arrival, "20170102T083000"_dt);
}
