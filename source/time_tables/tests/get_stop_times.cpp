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
#include "time_tables/get_stop_times.h"
#include "ed/build_helper.h"

using namespace navitia::timetables;
using namespace navitia;

BOOST_AUTO_TEST_CASE(test1){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150)("stop3", 8200, 8250);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();

    std::vector<navitia::type::idx_t> jpps;
    for(auto jpp : b.data->pt_data->journey_pattern_points)
        jpps.push_back(jpp->idx);

    auto result = get_stop_times(true, jpps, navitia::DateTimeUtils::min,
                                 navitia::DateTimeUtils::set(1, 0), 100, *b.data, false);
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    BOOST_CHECK(std::any_of(result.begin(), result.end(),
                            [](timetables::datetime_stop_time& dt_st) {return dt_st.second->journey_pattern_point->idx == 0;}));
    BOOST_CHECK(std::any_of(result.begin(), result.end(),
                            [](timetables::datetime_stop_time& dt_st) {return dt_st.second->journey_pattern_point->idx == 1;}));

    result = get_stop_times(false, jpps, navitia::DateTimeUtils::set(0, 86399),
                            navitia::DateTimeUtils::min, 100, *b.data, false, {});
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    BOOST_CHECK(std::any_of(result.begin(), result.end(),
                            [](timetables::datetime_stop_time& dt_st) {return dt_st.second->journey_pattern_point->idx == 1;}));
    BOOST_CHECK(std::any_of(result.begin(), result.end(),
                            [](timetables::datetime_stop_time& dt_st) {return dt_st.second->journey_pattern_point->idx == 2;}));


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
    ed::builder b("20120614");
    DateTime vj1_departure = 8000;
    DateTime vj2_departure = 8100;
    DateTime vj3_departure = 9000;
    std::string spa1 = "stop1";
    b.vj("A", "1010", "", true, "vj1")(spa1, vj1_departure, vj1_departure)("useless_stop", 1000, 1000);
    b.vj("A", "1010", "", true, "vj2")(spa1, vj2_departure, vj2_departure)("useless_stop", 1000, 1000);
    b.vj("A", "1111", "", true, "vj3")(spa1, vj3_departure, vj3_departure)("useless_stop", 1000, 1000);

    auto cal(new type::Calendar(b.data->meta->production_date.begin()));
    cal->uri="cal1";


    b.finish();

    for (auto vj_name: {"vj1", "vj2"}) {
        auto associated_cal = new type::AssociatedCalendar();
        associated_cal->calendar = cal;
        b.data->pt_data->meta_vj[vj_name]->associated_calendars.insert({cal->uri, associated_cal});
    }

    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();

    auto res = get_all_stop_times(jpp1, cal->uri);

    BOOST_REQUIRE_EQUAL(res.size(), 2);

    //result are not sorted
    using p = std::pair<uint32_t, const type::StopTime*>;
    std::sort(res.begin(), res.end(), [](const p& p1, const p& p2) { return p1.first < p2.first; });

    auto first_elt = res[0];
    BOOST_CHECK_EQUAL(first_elt.first, vj1_departure);
    BOOST_REQUIRE(first_elt.second != nullptr);
    BOOST_CHECK_EQUAL(first_elt.second->departure_time, vj1_departure);
    BOOST_CHECK_EQUAL(first_elt.second->journey_pattern_point->stop_point->stop_area->name, spa1);

    auto second_elt = res[1];
    BOOST_CHECK_EQUAL(second_elt.first, vj2_departure);
    BOOST_REQUIRE(second_elt.second != nullptr);
    BOOST_CHECK_EQUAL(second_elt.second->departure_time, vj2_departure);
    BOOST_CHECK_EQUAL(second_elt.second->journey_pattern_point->stop_point->stop_area->name, spa1);
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
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A", "1010", "", true)(spa1, sp1_departure, sp1_departure)
                             (spa2, sp2_arrival, sp2_arrival);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();

    const std::string calendar = "calendar_bob";
    //calendar is not associated to stop time => no answer
    auto res = get_all_stop_times(jpp1, calendar);
    BOOST_CHECK(res.empty());
}

/**
 * Test get_all_stop_times for one calendar
 *
 * 2 vj going from sp1 to somewhere, valid for our calendar
 * vj1 is a frequency vj
 * vj2 is a 'normal' vj
 *
 * ==========     =====
 * stop point     sp1
 * === ========== =====
 * vj1 departure  every 100s from 8000 to 9000
 * === ========== ===== =====
 * vj2 departure  8001
 * === ========== ===== =====
 *
 */
BOOST_AUTO_TEST_CASE(test_frequency_for_calendar) {
    ed::builder b("20120614");
    std::string spa1 = "stop1";
    DateTime vj1_departure = 8000;
    DateTime vj2_departure = 8001;
    size_t headway_sec = 100;
    b.frequency_vj("A", vj1_departure, 9000, headway_sec,
                   "default_network", "1010", "", true, "vj1")
                    (spa1, vj1_departure, vj1_departure)
                    ("useless_stop", 1000, 1000);
    b.vj("A", "1010", "", true, "vj2")
        (spa1, vj2_departure, vj2_departure)
        ("useless_stop", 1000, 1000);

    auto cal(new type::Calendar(b.data->meta->production_date.begin()));
    cal->uri="cal1";


    b.finish();

    for (auto vj_name: {"vj1", "vj2"}) {
        auto associated_cal = new type::AssociatedCalendar();
        associated_cal->calendar = cal;
        b.data->pt_data->meta_vj[vj_name]->associated_calendars.insert({cal->uri, associated_cal});
    }

    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();

    auto res = get_all_stop_times(jpp1, cal->uri);

    BOOST_REQUIRE_EQUAL(res.size(), 11+1);

    //result are not sorted
    using p = std::pair<uint32_t, const type::StopTime*>;
    std::sort(res.begin(), res.end(), [](const p& p1, const p& p2) { return p1.first < p2.first; });

    auto first_elt = res[0];
    BOOST_CHECK_EQUAL(first_elt.first, vj1_departure);
    BOOST_REQUIRE(first_elt.second != nullptr);
    BOOST_CHECK_EQUAL(first_elt.second->journey_pattern_point->stop_point->stop_area->name, spa1);

    //second is the 'normal' vj departure
    auto second_elt = res[1];
    BOOST_CHECK_EQUAL(second_elt.first, vj2_departure);
    BOOST_REQUIRE(second_elt.second != nullptr);
    BOOST_CHECK_EQUAL(second_elt.second->departure_time, vj2_departure);
    BOOST_CHECK_EQUAL(second_elt.second->journey_pattern_point->stop_point->stop_area->name, spa1);

    //then all vj1 departures
    for (size_t i = 2; i < res.size(); ++i) {
        auto departure = vj1_departure + headway_sec * (i - 1);
        BOOST_CHECK_EQUAL(res[i].first, departure);
        BOOST_REQUIRE(res[i].second != nullptr);
        BOOST_CHECK_EQUAL(res[i].second->journey_pattern_point->stop_point->stop_area->name, spa1);
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
    ed::builder b("20120614");
    std::string spa1 = "stop1";
    DateTime vj1_departure = 70000;
    size_t headway_sec = 1000;
    b.frequency_vj("A", vj1_departure, 2001, headway_sec, "default_network", "1010", "", true, "vj1")
        (spa1, vj1_departure, vj1_departure)
        ("useless_stop", 1000, 1000);

    auto cal(new type::Calendar(b.data->meta->production_date.begin()));
    cal->uri="cal1";

    b.finish();

    auto associated_cal = new type::AssociatedCalendar();
    associated_cal->calendar = cal;
    b.data->pt_data->meta_vj["vj1"]->associated_calendars.insert({cal->uri, associated_cal});

    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();

    auto res = get_all_stop_times(jpp1, cal->uri);

    //end of day is 86400, so we have (86400 - 70000) / 1000 + the stop on the morning (2001 / 1000)
    BOOST_REQUIRE_EQUAL(res.size(),
                        (DateTimeUtils::SECONDS_PER_DAY - vj1_departure) /headway_sec
                        + 2001 / headway_sec);

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
    ed::builder b("20120614");
    std::string spa1 = "stop1";
    DateTime vj1_departure = 70000;
    size_t headway_sec = 1000;
    b.frequency_vj("A", vj1_departure, 90001, headway_sec, "default_network", "1010", "", true, "vj1")
        (spa1, vj1_departure, vj1_departure)
        ("useless_stop", 1000, 1000);

    auto cal(new type::Calendar(b.data->meta->production_date.begin()));
    cal->uri="cal1";

    b.finish();

    auto associated_cal = new type::AssociatedCalendar();
    associated_cal->calendar = cal;
    b.data->pt_data->meta_vj["vj1"]->associated_calendars.insert({cal->uri, associated_cal});

    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();

    auto res = get_all_stop_times(jpp1, cal->uri);

    //end of day is 86400, so we have (90001 - 70000) / 1000 + 1 (for the last second)
    BOOST_REQUIRE_EQUAL(res.size(), (90001 - 70000) / headway_sec + 1 );

}
