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
#define BOOST_TEST_MODULE test_raptor
#include <boost/test/unit_test.hpp>
#include "routing/raptor.h"
#include "routing/routing.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"

struct logger_initialized {
    logger_initialized() { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

using namespace navitia;
using namespace routing;
namespace bt = boost::posix_time;

BOOST_AUTO_TEST_CASE(direct){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    RAPTOR raptor(*b.data);

    auto res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0,
            DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    //check the departure/arrival in each stop
    // remember that those are the arrival/departure of the vehicle in each stops
    // so arrival < departure
    BOOST_REQUIRE_EQUAL(res.items[0].departures.size(), 2);
    BOOST_REQUIRE_EQUAL(res.items[0].arrivals.size(), 2);
    BOOST_CHECK_EQUAL(res.items[0].arrivals[0], "20120614T021320"_dt);
    BOOST_CHECK_EQUAL(res.items[0].departures[0], "20120614T021410"_dt);
    BOOST_CHECK_EQUAL(res.items[0].arrivals[1], "20120614T021500"_dt);
    BOOST_CHECK_EQUAL(res.items[0].departures[1], "20120614T021550"_dt);

    res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0,
            DateTimeUtils::set(0, 8200), type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0,
            DateTimeUtils::set(0, 8101), type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0,
            DateTimeUtils::set(0, 8100), type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(change){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("B")("stop4", 8000, 8050)("stop2", 8300, 8350)("stop5", 8400, 8450);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.connection("stop5", "stop5", 120);
    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::inf,
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);

    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, 1);

    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, 4);

    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);

    BOOST_CHECK_EQUAL(res.items[1].departure.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(res.items[1].arrival.time_of_day().total_seconds(), 8350);

    BOOST_CHECK_EQUAL(res.items[2].departure.time_of_day().total_seconds(), 8350);
    BOOST_CHECK_EQUAL(res.items[2].arrival.time_of_day().total_seconds(), 8400);

    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::set(0,8500),
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, 4);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);

    BOOST_CHECK_EQUAL(res.items[1].departure.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(res.items[1].arrival.time_of_day().total_seconds(), 8350);

    BOOST_CHECK_EQUAL(res.items[2].departure.time_of_day().total_seconds(), 8350);
    BOOST_CHECK_EQUAL(res.items[2].arrival.time_of_day().total_seconds(), 8400);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::set(0, 8401),
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, 4);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);

    BOOST_CHECK_EQUAL(res.items[1].departure.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(res.items[1].arrival.time_of_day().total_seconds(), 8350);

    BOOST_CHECK_EQUAL(res.items[2].departure.time_of_day().total_seconds(), 8350);
    BOOST_CHECK_EQUAL(res.items[2].arrival.time_of_day().total_seconds(), 8400);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 0);


    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::set(0, 8400),
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}
/****
 *              connection 5mins
 *   line1      ----------------->  line 2
 * A----------->B                 C----------->D
 *              <-----------------
 *              connection 15 mins
 *
 * Time tables
 * Line 1
 *    VJ1     VJ2            VJ3     VJ4
 * A  07h00  07h55        B 07h10   07h20
 * B  07h05  08h00        A 07h15   07h25
 *
 * Line 2
 *    VJ5    VJ6             VJ7
 * C 07h10  07h20         D 07h00
 * D 07h15  07h35         C 07h05
 *
 */
BOOST_AUTO_TEST_CASE(different_connection_time) {
    ed::builder b("20120614");
    b.vj("line1-1")("A", 7*3600)("B", 7*3600 + 5*60);
    b.vj("line1-1")("A", 7*3600 + 55*60)("B", 8*3600);
    b.vj("line1-2")("B", 7*3600 + 10*60)("A", 7*3600 + 15*60);
    b.vj("line1-2")("B", 7*3600 + 20*60)("A", 7*3600 + 25*60);
    b.vj("line2-1")("C", 7*3600 + 10*60)("D", 7*3600 + 15*60);
    b.vj("line2-1")("C", 7*3600 + 20*60)("D", 7*3600 + 35*60);
    b.vj("line2-2")("D", 7*3600)("C", 7*3600 + 5*60);
    b.connection("B", "C", 300);
    b.connection("C", "B", 900);
    b.connection("A", "A", 120);
    b.connection("B", "B", 120);
    b.connection("C", "C", 120);
    b.connection("D", "D", 120);
    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["A"], d.stop_areas_map["D"], 7*3600, 0, DateTimeUtils::inf,
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items.front().departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(to_datetime(res.items.front().departure, *(b.data))), 7*3600);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items.back().arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(to_datetime(res.items.back().arrival, *(b.data))), 7*3600 + 15*60);

    res1 = raptor.compute(d.stop_areas_map["D"], d.stop_areas_map["A"], 7*3600, 0, DateTimeUtils::inf,
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items.front().departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(to_datetime(res.items.front().departure, *(b.data))), 7*3600);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items.back().arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(to_datetime(res.items.back().arrival, *(b.data))), 7*3600 + 25*60);
}


BOOST_AUTO_TEST_CASE(over_midnight){
    ed::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 5*60);
    b.vj("B")("stop2", 10*60)("stop3", 20*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0, DateTimeUtils::inf,
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0, DateTimeUtils::set(1, 8500),
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0, DateTimeUtils::set(1, 20*60 + 1),
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0, DateTimeUtils::set(1, 20*60),
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(over_midnight_2){
    ed::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 23*3600 + 59*60);
    b.vj("B")("stop4", 23*3600 + 10*60)("stop2", 10*60)("stop3", 20*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::inf,
            type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_points_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_points_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 5000),
            type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_points_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_points_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 20*60 + 1),
            type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_points_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_points_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_points_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 20*60),
            type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(over_midnight_interne){
    ed::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 23*3600 + 30*60, 24*3600 + 30*60)("stop3", 24*3600 + 40 * 60);
    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::inf,
            type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 3600),
            type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 40*60 + 1), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 40*60), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(validity_pattern){
    ed::builder b("20120614");
    b.vj("D", "10", "", true)("stop1", 8000)("stop2", 8200);
    b.vj("D", "1", "", true)("stop1", 9000)("stop2", 9200);

    b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::set(0, 10000), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9200);


    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::set(0, 9201), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::set(0, 9200), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 2, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

/*BOOST_AUTO_TEST_CASE(validit_pattern_2) {
    ed::builder b("20120614");
    b.vj("D", "10", "", true)("stop1", 8000)("stop2", 8200);

   b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::inf);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.day(), 1);
}
*/


BOOST_AUTO_TEST_CASE(forbidden_uri){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8100,8150);
    b.vj("B")("stop3", 9500)("stop4", 10000);
    b.vj("C")("stop1", 8000, 8050)("stop4", 18000);

    b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    RAPTOR raptor(*b.data);

    auto res1 = raptor.compute(b.data->pt_data->stop_areas[0],
            b.data->pt_data->stop_areas[3], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min,
            true, {}, std::numeric_limits<uint32_t>::max(), {"stop2"});

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), 18000);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_milieu){
    ed::builder b("20120614");
    b.vj("A", "11111111", "", true)("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B", "11111111", "", true)("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);

    b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(0,10000), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(0,9201), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(0,9200), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(marche_a_pied_pam){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 23*3600+59*60);
    b.vj("B")("stop3", 2*3600)("stop4",2*3600+20);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);


    b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1]->idx, d.stop_areas_map["stop4"]->idx);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 2*3600+20);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[3].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(1 , (3*3600+20)), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1]->idx, d.stop_areas_map["stop4"]->idx);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 2*3600+20);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[3].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(1 , 2*3600+20+1), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1]->idx, d.stop_areas_map["stop4"]->idx);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 2*3600+20);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[3].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(1 , 2*3600+20), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(test_rattrapage) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8*3600 + 10*60)("stop2", 8*3600 + 15*60)("stop3", 8*3600 + 35*60)("stop4", 8*3600 + 45*60);
    b.vj("B")("stop1", 8*3600 + 20*60)("stop2", 8*3600 + 25*60)("stop3", 8*3600 + 40*60)("stop4", 8*3600 + 50*60);
    b.vj("C")("stop2", 8*3600 + 30*60)("stop5", 8*3600 + 31*60)("stop3", 8*3600 + 32*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.connection("stop5", "stop5", 120);

    b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 8*3600 + 15*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);


    BOOST_REQUIRE_EQUAL(res1.size(), 2);

    BOOST_CHECK(std::any_of(res1.begin(), res1.end(),
                            [&](const Path& p){
        return   p.items.size() == 5 &&
                p.items[0].stop_points.size() == 2 &&
                p.items[1].stop_points.size() == 1 &&
                p.items[2].stop_points.size() == 3 &&
                p.items[3].stop_points.size() == 1 &&
                p.items[4].stop_points.size() == 2 &&
                p.items[0].stop_points[0]->idx == d.stop_points_map["stop1"]->idx &&
                p.items[0].stop_points[1]->idx == d.stop_points_map["stop2"]->idx &&
                p.items[1].stop_points[0]->idx == d.stop_points_map["stop2"]->idx &&
                p.items[2].stop_points[0]->idx == d.stop_points_map["stop2"]->idx &&
                p.items[2].stop_points[2]->idx == d.stop_points_map["stop3"]->idx &&
                p.items[3].stop_points[0]->idx == d.stop_points_map["stop3"]->idx &&
                p.items[4].stop_points[0]->idx == d.stop_points_map["stop3"]->idx &&
                p.items[4].stop_points[1]->idx == d.stop_points_map["stop4"]->idx &&
                p.items[0].departure.time_of_day().total_seconds() == 8*3600+20*60 &&
                p.items[4].arrival.time_of_day().total_seconds() == 8*3600+45*60;
    }));

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 8*3600 + 15*60, 0, DateTimeUtils::set(0, (9*3600+45*60)), type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 2);
    BOOST_CHECK(std::any_of(res1.begin(), res1.end(),
                            [&](const Path& p){
        return  p.items.size(), 5 &&
                p.items[0].stop_points.size() == 2 &&
                p.items[1].stop_points.size() == 1 &&
                p.items[2].stop_points.size() == 3 &&
                p.items[3].stop_points.size() == 1 &&
                p.items[4].stop_points.size() == 2 &&
                p.items[0].stop_points[0]->idx == d.stop_points_map["stop1"]->idx &&
                p.items[0].stop_points[1]->idx == d.stop_points_map["stop2"]->idx &&
                p.items[1].stop_points[0]->idx == d.stop_points_map["stop2"]->idx &&
                p.items[2].stop_points[0]->idx == d.stop_points_map["stop2"]->idx &&
                p.items[2].stop_points[2]->idx == d.stop_points_map["stop3"]->idx &&
                p.items[3].stop_points[0]->idx == d.stop_points_map["stop3"]->idx &&
                p.items[4].stop_points[0]->idx == d.stop_points_map["stop3"]->idx &&
                p.items[4].stop_points[1]->idx == d.stop_points_map["stop4"]->idx &&
                p.items[0].departure.time_of_day().total_seconds() == 8*3600+20*60 &&
                p.items[4].arrival.time_of_day().total_seconds() == 8*3600+45*60;
    }
    ));
    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 8*3600 + 15*60, 0, DateTimeUtils::set(0, 8*3600+45*60+1), type::RTLevel::Base, 2_min, true);

    // As the bound is tight, we now only have the shortest trip.
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK(std::any_of(res1.begin(), res1.end(),
                            [&](const Path& p){
        return p.items.size() == 5  &&
                p.items[0].stop_points.size() == 2  &&
                p.items[1].stop_points.size() == 1  &&
                p.items[2].stop_points.size() == 3  &&
                p.items[3].stop_points.size() == 1  &&
                p.items[4].stop_points.size() == 2  &&
                p.items[0].stop_points[0]->idx == d.stop_points_map["stop1"]->idx  &&
                p.items[0].stop_points[1]->idx == d.stop_points_map["stop2"]->idx  &&
                p.items[1].stop_points[0]->idx == d.stop_points_map["stop2"]->idx  &&
                p.items[2].stop_points[0]->idx == d.stop_points_map["stop2"]->idx  &&
                p.items[2].stop_points[2]->idx == d.stop_points_map["stop3"]->idx  &&
                p.items[3].stop_points[0]->idx == d.stop_points_map["stop3"]->idx  &&
                p.items[4].stop_points[0]->idx == d.stop_points_map["stop3"]->idx  &&
                p.items[4].stop_points[1]->idx == d.stop_points_map["stop4"]->idx  &&
                p.items[0].departure.time_of_day().total_seconds() == 8*3600+20*60  &&
                p.items[4].arrival.time_of_day().total_seconds() == 8*3600+45*60;
    }));



    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 8*3600 + 15*60, 0, DateTimeUtils::set(0, 8*3600+45*60), type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(pam_veille) {
    ed::builder b("20120614");
    b.vj("A", "11111111", "", true)("stop1", 3*60)("stop2", 20*60);
    b.vj("B", "01", "", true)("stop0", 23*3600)("stop2", 24*3600 + 30*60)("stop3", 24*3600 + 40*60);

    b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop2"], d.stop_areas_map["stop3"], 50*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
}


BOOST_AUTO_TEST_CASE(pam_3) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 5 * 60);
    b.vj("B")("stop1", 23*3600)("stop3", 23*3600 + 5 * 60);
    b.vj("C1")("stop2", 10*60)("stop3", 20*60)("stop4", 30*60);
    b.vj("C1")("stop2", 23*3600)("stop3", 23*3600 + 10*60)("stop4", 23*3600 + 40*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);

    b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    BOOST_CHECK_EQUAL(res1[0].items[2].arrival.time_of_day().total_seconds(), 23*3600 + 40 * 60);
}

BOOST_AUTO_TEST_CASE(sn_debut) {
    ed::builder b("20120614");
    b.vj("A","11111111", "", true)("stop1", 8*3600)("stop2", 8*3600 + 20*60);
    b.vj("B","11111111", "", true)("stop1", 9*3600)("stop2", 9*3600 + 20*60);

    routing::map_stop_point_duration departs, destinations;
    departs[SpIdx(0)] = 10_min;
    destinations[SpIdx(1)] = 0_s;

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));

    auto res1 = raptor.compute_all(departs, destinations, DateTimeUtils::set(0, 8*3600), type::RTLevel::Base, 2_min);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.time_of_day().total_seconds(), 9*3600 + 20 * 60);
}


BOOST_AUTO_TEST_CASE(stay_in_basic) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop2", 8*3600+15*60)("stop3", 8*3600 + 20*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type, ItemType::stay_in);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}

BOOST_AUTO_TEST_CASE(stay_in_short) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop2", 8*3600+10*60)("stop3", 8*3600 + 20*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type, ItemType::stay_in);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}

BOOST_AUTO_TEST_CASE(stay_in_nl) {
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true) ("ehv", 60300,60600)
            ("ehb", 60780,60780)
            ("bet", 61080,61080)
            ("btl", 61560,61560)
            ("vg",  61920,61920)
            ("ht",  62340,62340);
    b.vj("4462", "1111111", "block1", true) ("ht",  62760,62760)
            ("hto", 62940,62940)
            ("rs",  63180,63180);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["bet"], d.stop_areas_map["rs"], 60780, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type, ItemType::stay_in);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 63180);
}

BOOST_AUTO_TEST_CASE(stay_in_nl_counterclock) {
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true) ("ehv", 60300,60600)
            ("ehb", 60780,60780)
            ("bet", 61080,61080)
            ("btl", 61560,61560)
            ("vg",  61920,61920)
            ("ht",  62340,62340);
    b.vj("4462", "1111111", "block1", true) ("ht",  62760,62760)
            ("hto", 62940,62940)
            ("rs",  63180,63180);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["bet"], d.stop_areas_map["rs"], 63200, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type,  ItemType::stay_in);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 63180);
}


BOOST_AUTO_TEST_CASE(stay_in_teleport) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type,  ItemType::stay_in);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}


BOOST_AUTO_TEST_CASE(stay_in_departure_last_of_first_vj) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop2"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}

BOOST_AUTO_TEST_CASE(stay_in_complex) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "", true)("stop1", 8*3600)("stop2", 9*3600)("stop3", 10*3600);
    b.vj("B", "1111111", "block1", true)("stop3", 10*3600+5*60)("stop2", 11*3600);
    b.vj("C", "1111111", "block1", true)("stop4", 11*3600+5*60)("stop5", 11*3600+10*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.connection("stop5", "stop5", 120);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop5"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[4].arrival.time_of_day().total_seconds(), 11*3600+10*60);
}


BOOST_AUTO_TEST_CASE(stay_in_and_one_earlier_with_connection) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);
    b.vj("C", "1111111", "", true)("stop1", 8*3600)("stop2", 8*3600+5*60);
    b.vj("D", "1111111", "", true)("stop2", 8*3600+10*60)("stop3", 8*3600+15*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 2);
    BOOST_CHECK(std::any_of(res1.begin(), res1.end(), [](Path path){
                    return path.items[2].arrival.time_of_day().total_seconds() == 8*3600 + 15*60;}));
    BOOST_CHECK(std::any_of(res1.begin(), res1.end(), [](Path path){
                    return path.items[2].arrival.time_of_day().total_seconds() == 8*3600 + 20*60;}));
}



BOOST_AUTO_TEST_CASE(stay_in_3_vj) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop5", 8*3600 + 20*60);
    b.vj("C", "1111111", "block1", true)("stop6", 8*3600+25*60)("stop3", 8*3600 + 30*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[4].arrival.time_of_day().total_seconds(), 8*3600 + 30*60);
}


BOOST_AUTO_TEST_CASE(stay_in_loop) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);
    b.vj("C", "1111111", "block1", true)("stop5", 8*3600+25*60)("stop1", 8*3600 + 30*60);
    b.vj("D", "1111111", "block1", true)("stop4", 8*3600+35*60)("stop3", 8*3600 + 40*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}

BOOST_AUTO_TEST_CASE(stay_in_invalid_vp) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "0000", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

/*
 * This case isn't handle yet, because we only apply the next_vj of the last taken vehicle_journey.

BOOST_AUTO_TEST_CASE(first_vj_stay_in) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8*3600+15*60)("stop2", 8*3600+20*60);
    b.vj("B")("stop1", 8*3600)("stop3", 8*3600 + 5*60);
    b.vj("C")("stop2", 8*3600)("stop3", 8*3600+10*60)("stop4", 8*3600+15*60);
    b.vj("C", "111111", "block1")("stop2", 8*3600+25*60)("stop3", 8*3600+30*60)("stop4", 8*3600+45*60);
    b.vj("D", "111111", "block1")("stop5", 8*3600+50*60);

   b.data->pt_data->index(); b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop5"], 5*60, 0, DateTimeUtils::inf, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items.back().arrival.time_of_day().total_seconds(), 8*3600 + 50*60);
}
*/

BOOST_AUTO_TEST_CASE(itl) {
    ed::builder b("20120614");
    b.vj("A")("stop1",8*3600+10*60, 8*3600 + 10*60,1)("stop2",8*3600+15*60,8*3600+15*60,1)("stop3", 8*3600+20*60);
    b.vj("B")("stop1",9*3600)("stop2",10*3600);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.time_of_day().total_seconds(), 10*3600);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.time_of_day().total_seconds(), 8*3600+20*60);
}


BOOST_AUTO_TEST_CASE(mdi) {
    ed::builder b("20120614");

    b.vj("B")("stop1",17*3600, 17*3600,std::numeric_limits<uint16_t>::max(), true, false)("stop2", 17*3600+15*60)("stop3",17*3600+30*60, 17*3600+30*60,std::numeric_limits<uint16_t>::max(), true, false);
    b.vj("C")("stop4",16*3600, 16*3600,std::numeric_limits<uint16_t>::max(), true, true)("stop5", 16*3600+15*60)("stop6",16*3600+30*60, 16*3600+30*60,std::numeric_limits<uint16_t>::max(), false, true);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_CHECK_EQUAL(res1.size(), 0);
    res1 = raptor.compute(d.stop_areas_map["stop2"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_CHECK_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop4"], d.stop_areas_map["stop6"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_CHECK_EQUAL(res1.size(), 0);
    res1 = raptor.compute(d.stop_areas_map["stop4"],d.stop_areas_map["stop5"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_CHECK_EQUAL(res1.size(), 1);
}


BOOST_AUTO_TEST_CASE(multiples_vj) {
    ed::builder b("20120614");
    b.vj("A1")("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("A2")("stop1", 8*3600 + 15*60 )("stop2", 8*3600+20*60);
    b.vj("B1")("stop2", 8*3600 + 25*60)("stop3", 8*3600+30*60);
    b.vj("B2")("stop2", 8*3600 + 35*60)("stop3", 8*3600+40*60);
    b.vj("C")("stop3", 8*3600 + 45*60)("stop4", 8*3600+50*60);
    b.vj("E1")("stop1", 8*3600)("stop5", 8*3600 + 5*60);
    b.vj("E2")("stop5", 8*3600 + 10 * 60)("stop1", 8*3600 + 12*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.connection("stop5", "stop5", 120);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);

    BOOST_CHECK_EQUAL(res1.size(), 1);
}


BOOST_AUTO_TEST_CASE(max_duration){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::set(0, 8101), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::set(0, 8099), type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(max_transfers){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 81000,81500);
    b.vj("B")("stop1",8000)("stop3",8500);
    b.vj("C")("stop3",9000)("stop2",11000);
    b.vj("D")("stop3",9000)("stop4",9500);
    b.vj("E")("stop4",10000)("stop2",10500);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    for(uint32_t nb_transfers=0; nb_transfers<=2; ++nb_transfers) {
        //        type::Properties p;
        auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::inf,
                nt::RTLevel::Base, 2_min, true, type::AccessibiliteParams(), nb_transfers);
        BOOST_REQUIRE(res1.size()>=1);
        for(auto r : res1) {
            BOOST_REQUIRE(r.nb_changes <= nb_transfers);
        }
    }
}

BOOST_AUTO_TEST_CASE(destination_over_writing) {
    /*
     * In this test case we reach the destination with a transfer, and we don't want that.
     * This can seem silly, but we actually want to be able to say that we arrive at stop 2
     * at 30000
     */
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 30000);
    b.vj("B")("stop1", 9000)("stop3", 9200);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 30000);
}


BOOST_AUTO_TEST_CASE(over_midnight_special) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B")("stop1", 7*3600)("stop3", 7*3600+5*60);
    b.vj("C")("stop2", 7*3600+10*60)("stop3", 7*3600+15*60)("stop4", 7*3600+20*60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);


    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 6*3600, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[2].arrival.time_of_day().total_seconds(), 7*3600+20*60);
    BOOST_CHECK_EQUAL(res.items[2].arrival.date().day(), 14);

}


BOOST_AUTO_TEST_CASE(invalid_stay_in_overmidnight) {
    ed::builder b("20120614");
    b.vj("A", "111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "010", "block1", true)("stop2", 8*3600+15*60)("stop3", 24*3600 + 20*60, 24*3600+25*60)("stop4", 24*3600+30*60, 24*3600+35*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    // Here we want to check if the second vehicle_journey is not taken on the
    // first day
    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 6*3600, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
    
    // There must be a journey the second day
    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 6*3600, 1, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_CHECK_EQUAL(res1.size(), 1);
}

BOOST_AUTO_TEST_CASE(no_departure_before_given_date) {
    // 06k   08k   10k   12k   14k   16k   18k
    //       1|---A----|3 |------B-------|5
    //       1|--C--|2 |--D--|4 |--E--|5
    //    1|-------F---------|4
    //
    // we take 1.5k to go to 1
    // we want to go to 5 with 6k as departure date
    // 2 valid path: A->B and C->D->E
    // invalid path F->E must not be returned because we can't be at 1 at 7k

    ed::builder b("20120614");
    b.vj("A")("stop1",  8000)("stop3", 11000);
    b.vj("B")("stop3", 12000)("stop5", 17000);
    b.vj("C")("stop1",  8000)("stop2", 10000);
    b.vj("D")("stop2", 11000)("stop4", 13000);
    b.vj("E")("stop4", 14000)("stop5", 16000);
    b.vj("F")("stop1",  7000)("stop4", 13000);
    b.connection("stop1", "stop1", 100);
    b.connection("stop2", "stop2", 100);
    b.connection("stop3", "stop3", 100);
    b.connection("stop4", "stop4", 100);
    b.connection("stop5", "stop5", 100);


    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*b.sps["stop1"])] = 1500_s;
    arrivals[SpIdx(*b.sps["stop5"])] = 0_s;

    auto results = raptor.compute_all(departures, arrivals, 6000, type::RTLevel::Base, 2_min);

    BOOST_CHECK_EQUAL(results.size(), 2);
    for (const auto& res: results) {
        // we can't begin the journey before 7.5k
        BOOST_CHECK_GE(res.items.front().departure, to_posix_time(7500, *b.data));
    }
}
/***
 *
 *  I---A-----C-----B--J
 * walking times:
 *  I->A : 0   secs
 *  C->J : 321 secs
 *  B->J : 0   secs
 *
 * Schedules:
 *  A     C      B
 *  8h00  8h01   8h07
 *
 * We want to have two answers
 * J1: I->C->J: arriving at 8h01 at C so at 8h05 at J with a walking time of 321 seconds
 * J2: I->B->J: arriving at 8h07 at B so at 8h07 at J with a walking time of 0 seconds
 */
BOOST_AUTO_TEST_CASE(less_fallback) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8*3600)("stop2", 8*3600 + 1*60)("stop3", 8*3600 + 12*60);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    routing::map_stop_point_duration departs, destinations;
    departs[SpIdx(*d.stop_areas_map["stop1"]->stop_point_list.front())] = 0_s;
    destinations[SpIdx(*d.stop_areas_map["stop1"]->stop_point_list.front())] = 560_s;
    destinations[SpIdx(*d.stop_areas_map["stop2"]->stop_point_list.front())] = 320_s;
    destinations[SpIdx(*d.stop_areas_map["stop3"]->stop_point_list.front())] = 0_s;
    auto res1 = raptor.compute_all(departs, destinations, DateTimeUtils::set(0, 8*3600), type::RTLevel::Base, 2_min);

    BOOST_REQUIRE_EQUAL(res1.size(), 2);
    BOOST_CHECK(std::any_of(res1.begin(), res1.end(),
                            [](const routing::Path& path) {
        return path.items.back().arrival.time_of_day().total_seconds() == 8*3600 + 12*60;
    }));

    BOOST_CHECK(std::any_of(res1.begin(), res1.end(),
                            [](const routing::Path& path) {
        return path.items.back().arrival.time_of_day().total_seconds() == 8*3600 + 60;
    }));
}


/*
 * Schedules:
 *      Line 1:
 *      stop1   stop2
 *      9h      9h50
 *
 *      Line 2:
 *      stop2   stop3
 *      9h55    10h00
 *
 * walking times:
 *  stop2->J: 15 minutes
 *  stop3->J: 0 minute
 *
 * We want to leave from stop1 after 8h, and we want to go to J
 * So we want to have two answers, one arriving without transfers in stop2.
 * The other with one transfer in stop3.
 *
 */
BOOST_AUTO_TEST_CASE(pareto_front) {
    ed::builder b("20120614");
    b.vj("line1")("stop1", 9*3600)("stop2", 9*3600 + 50*60);
    b.vj("line2")("stop2", 9*3600 + 55*60)("stop3", 10*3600);
    b.connection("stop2", "stop2", 120);


    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    routing::map_stop_point_duration departs, destinations;
    departs[SpIdx(*d.stop_areas_map["stop1"]->stop_point_list.front())] = 0_s;
    // We are going to J point, so we add the walking times
    destinations[SpIdx(*d.stop_areas_map["stop2"]->stop_point_list.front())] = 15_min;
    destinations[SpIdx(*d.stop_areas_map["stop3"]->stop_point_list.front())] = 0_s;
    auto res1 = raptor.compute_all(departs, destinations, DateTimeUtils::set(0, 8*3600), type::RTLevel::Base, 2_min);

    BOOST_REQUIRE_EQUAL(res1.size(), 2);
    BOOST_CHECK(std::any_of(res1.begin(), res1.end(),
                            [](const routing::Path& path) {
        return path.items.size() == 1 &&
                path.items.back().arrival.time_of_day().total_seconds() == 9*3600 + 50*60;
    }));

    BOOST_CHECK(std::any_of(res1.begin(), res1.end(),
                            [](const routing::Path& path) {
        return path.items.size() == 3 &&
                path.items.back().arrival.time_of_day().total_seconds() == 10*3600;
    }));
}

BOOST_AUTO_TEST_CASE(overlapping_on_first_st) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8200)("stop2", 8500);
    b.vj("A")("stop1", 8100, 8300)("stop2", 8600);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    RAPTOR raptor(*b.data);
    auto res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8200);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8500);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

}

BOOST_AUTO_TEST_CASE(stay_in_unnecessary) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop2", 8*3600)("stop3", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop1", 7*3600)("stop2", 8*3600);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items.size(), 1);
}


BOOST_AUTO_TEST_CASE(stay_in_unnecessary2) {
    ed::builder b("20120614");
    b.vj("B", "1111111", "block1", true)("stop2", 8*3600)("stop3", 9*3600);
    b.vj("A", "1111111", "block1", true)("stop1", 7*3600)("stop2", 8*3600);
    b.vj("line2")("stop2", 9*3600 + 55*60)("stop4", 10*3600);
    b.connection("stop2", "stop2", 120);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 5*60, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items.size(), 3);
}

BOOST_AUTO_TEST_CASE(forbid_transfer_between_2_odt){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("B")("stop4", 8000, 8050)("stop2", 8300, 8350)("stop5", 8400, 8450);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.connection("stop5", "stop5", 120);
    for (auto* vj: b.data->pt_data->vehicle_journeys) {
        for (auto& st: vj->stop_time_list) {
            st.set_date_time_estimated(true);
        }
    }
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->aggregate_odt();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

// Simple ODT test, we should find a journey
// but there should only be 2 stop times in the response, because it's odt and the inter stops are not relevant
BOOST_AUTO_TEST_CASE(simple_odt){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250)("stop4", 8500, 8650);
    for (auto* vj: b.data->pt_data->vehicle_journeys) {
        for (auto& st: vj->stop_time_list) {
            st.set_date_time_estimated(true);
        }
    }
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->aggregate_odt();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas[0], d.stop_areas[3], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    const auto& journey = res1.front();
    BOOST_REQUIRE_EQUAL(journey.items.size(), 1);
    const auto& section = journey.items.front();
    BOOST_REQUIRE_EQUAL(section.stop_times.size(), 2);
    BOOST_CHECK_EQUAL(section.stop_times[0]->stop_point->uri, "stop1");
    BOOST_CHECK_EQUAL(section.stop_times[1]->stop_point->uri, "stop4");
}

// Same as previous test, but with a virtual_with_stop_time
// we should find a journey and there should be all the stop times (they are relevant)
BOOST_AUTO_TEST_CASE(simple_odt_virtual_with_stop_time){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250)("stop4", 8500, 8650);
    for (auto vj : b.data->pt_data->vehicle_journeys) {
        for (auto& st: vj->stop_time_list) {
            st.set_odt(true);
        }
    }
    b.data->pt_data->index();
    b.data->aggregate_odt();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas[0], d.stop_areas[3], 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    const auto& journey = res1.front();
    BOOST_REQUIRE_EQUAL(journey.items.size(), 1);
    const auto& section = journey.items.front();
    BOOST_REQUIRE_EQUAL(section.stop_times.size(), 4);
    BOOST_CHECK_EQUAL(section.stop_times[0]->stop_point->uri, "stop1");
    BOOST_CHECK_EQUAL(section.stop_times[1]->stop_point->uri, "stop2");
    BOOST_CHECK_EQUAL(section.stop_times[2]->stop_point->uri, "stop3");
    BOOST_CHECK_EQUAL(section.stop_times[3]->stop_point->uri, "stop4");
}

// 1
//  \  A
//   2 --> 3
//   2 <-- 3
//  /  B
// 4
//
// We want 1->2->4 and not 1->3->4 (see the 7-pages ppt that explain the bug)
static void
check_y_line_the_ultimate_political_blocking_bug(const std::vector<navitia::routing::Path>& res) {
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items.size(), 3);
    BOOST_CHECK_EQUAL(res[0].items[0].departure.time_of_day().total_seconds(), 8000);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(res[0].items[1].departure.time_of_day().total_seconds(), 8100);// A stop2
    BOOST_CHECK_EQUAL(res[0].items[1].arrival.time_of_day().total_seconds(), 8500);// B stop2
    BOOST_CHECK_EQUAL(res[0].items[2].departure.time_of_day().total_seconds(), 8500);
    BOOST_CHECK_EQUAL(res[0].items[2].arrival.time_of_day().total_seconds(), 8600);
}
BOOST_AUTO_TEST_CASE(y_line_the_ultimate_political_blocking_bug){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8000)("stop2", 8100, 8100)("stop3", 8200, 8200);
    b.vj("B")("stop3", 8400, 8400)("stop2", 8500, 8500)("stop4", 8600, 8600);
    b.connection("stop1", "stop1", 10);
    b.connection("stop2", "stop2", 10);
    b.connection("stop3", "stop3", 10);
    b.connection("stop4", "stop4", 10);
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop4"), 7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    check_y_line_the_ultimate_political_blocking_bug(res1);
    auto res2 = raptor.compute(d.stop_areas_map.at("stop1"), d.stop_areas_map.at("stop4"), 9000, 0, DateTimeUtils::min, type::RTLevel::Base, 2_min, false);
    check_y_line_the_ultimate_political_blocking_bug(res2);
}

/*
 *    A --------------- B --------------- C --------------- D
 *
 * l1   ----------------x-----------------x----------------->
 *
 * l2                    -----------------
 *                                       | service extension
 * l3                                    ------------------->
 *
 * We want to test that raptor stops at the second iteration (because the service extension does not improve the solution)
 * */
BOOST_AUTO_TEST_CASE(finish_on_service_extension) {
    ed::builder b("20120614");
    b.vj("l1", "1", "", true)("A", 8000, 8000)("B", 8100, 8100)("C", 8200, 8200)("D", 8500, 8500);
    b.vj("l2", "1", "toto", true)("B", 8400, 8400)("C", 8600, 8600);
    b.vj("l3", "1", "toto", true)("C", 8600, 8600)("D", 10000, 10000);

    b.connection("B", "B", 10);
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departs;
    departs[routing::SpIdx(*d.stop_points_map["A"])] = {};

    raptor.first_raptor_loop(departs, DateTimeUtils::set(0, 7900), nt::RTLevel::Base, DateTimeUtils::inf,
                             std::numeric_limits<uint32_t>::max(), {}, {}, true);

    //and raptor has to stop on count 2
    BOOST_CHECK_EQUAL(raptor.count, 2);
}

/*
 *    A --------------- B --------------- C --------------- D
 *
 * l1   ----------------x-----------------x----------------->
 *
 * l2                    --------E========
 *
 * We want to test that raptor stops at the second iteration
 *
 * C can connect with itself, so C has been 'best_labeled' with a better transfers than E->C
 * */
BOOST_AUTO_TEST_CASE(finish_on_foot_path) {
    ed::builder b("20120614");
    b.vj("l1", "1", "", true)("A", 8000, 8000)("B", 8100, 8100)("C", 8300, 8300)("D", 8500, 8500);
    b.vj("l2", "1", "toto", true)("B", 8130, 8130)("E", 8200, 8200);

    b.connection("B", "B", 10);
    b.connection("C", "C", 0); // -> C can connect with itself
    b.connection("E", "C", 150);

    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departs;
    departs[routing::SpIdx(*d.stop_points_map["A"])] = {};

    raptor.first_raptor_loop(departs, DateTimeUtils::set(0, 7900), type::RTLevel::Base, DateTimeUtils::inf,
                             std::numeric_limits<uint32_t>::max(), {}, {}, true);

    //and raptor has to stop on count 2
    BOOST_CHECK_EQUAL(raptor.count, 2);
}

// instance:
// A---1---B=======B
//         A---1---B=======B
//                         B---2---C=======C
//                                 B---2---C=======C
//                                                 C---3---D
//                                                         C---3---D
//
// We want:
//         A---1---B=======B
//                         B---2---C=======CzzzzzzzC
//                                                 C---3---D
// ie: earlier arrival, latest departure and left aligned
static void test_2nd_and_3rd_pass_checks(const std::vector<navitia::routing::Path>& res) {
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items.size(), 5);

    // A---1---B
    BOOST_CHECK_EQUAL(res[0].items[0].departure.time_of_day().total_seconds(), 9000);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival.time_of_day().total_seconds(), 10000);

    //         B=======B
    BOOST_CHECK_EQUAL(res[0].items[1].departure.time_of_day().total_seconds(), 10000);
    BOOST_CHECK_EQUAL(res[0].items[1].arrival.time_of_day().total_seconds(), 11000);

    //                 B---2---C
    BOOST_CHECK_EQUAL(res[0].items[2].departure.time_of_day().total_seconds(), 11000);
    BOOST_CHECK_EQUAL(res[0].items[2].arrival.time_of_day().total_seconds(), 12000);

    //                         C=======CzzzzzzzC
    BOOST_CHECK_EQUAL(res[0].items[3].departure.time_of_day().total_seconds(), 12000);
    BOOST_CHECK_EQUAL(res[0].items[3].arrival.time_of_day().total_seconds(), 14000);

    //                                         C---3---D
    BOOST_CHECK_EQUAL(res[0].items[4].departure.time_of_day().total_seconds(), 14000);
    BOOST_CHECK_EQUAL(res[0].items[4].arrival.time_of_day().total_seconds(), 15000);
}
BOOST_AUTO_TEST_CASE(test_2nd_and_3rd_pass) {
    ed::builder b("20120614");
    b.vj("1", "1")("A", 8000, 8000)("B", 9000, 9000);
    b.vj("1", "1")("A", 9000, 9000)("B", 10000, 10000);
    b.vj("2", "1")("B", 11000, 11000)("C", 12000, 12000);
    b.vj("2", "1")("B", 12000, 12000)("C", 13000, 13000);
    b.vj("3", "1")("C", 14000, 14000)("D", 15000, 15000);
    b.vj("3", "1")("C", 15000, 15000)("D", 16000, 16000);

    b.connection("B", "B", 1000);
    b.connection("C", "C", 1000);

    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("D"),
                               7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    test_2nd_and_3rd_pass_checks(res1);

    // non clockwise test
    auto res2 = raptor.compute(d.stop_areas[0], d.stop_areas[3], 16100, 0, 0, type::RTLevel::Base, 2_min, false);
    test_2nd_and_3rd_pass_checks(res2);
}

// same as test_2nd_and_3rd_pass with vj extentions
static void test_2nd_and_3rd_pass_ext_checks(const std::vector<navitia::routing::Path>& res) {
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items.size(), 9);

    // A---1---B
    BOOST_CHECK_EQUAL(res[0].items[0].departure.time_of_day().total_seconds(), 9000);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival.time_of_day().total_seconds(), 10000);

    //         B=======B
    BOOST_CHECK_EQUAL(res[0].items[1].departure.time_of_day().total_seconds(), 10000);
    BOOST_CHECK_EQUAL(res[0].items[1].arrival.time_of_day().total_seconds(), 11000);

    //                 B-2E4F5-C
    BOOST_CHECK_EQUAL(res[0].items[2].departure.time_of_day().total_seconds(), 11000);
    BOOST_CHECK_EQUAL(res[0].items[2].arrival.time_of_day().total_seconds(), 11100);

    BOOST_CHECK_EQUAL(res[0].items[3].departure.time_of_day().total_seconds(), 11100);
    BOOST_CHECK_EQUAL(res[0].items[3].arrival.time_of_day().total_seconds(), 11100);
    BOOST_CHECK_EQUAL(res[0].items[3].type, ItemType::stay_in);

    BOOST_CHECK_EQUAL(res[0].items[4].departure.time_of_day().total_seconds(), 11100);
    BOOST_CHECK_EQUAL(res[0].items[4].arrival.time_of_day().total_seconds(), 11200);

    BOOST_CHECK_EQUAL(res[0].items[5].departure.time_of_day().total_seconds(), 11200);
    BOOST_CHECK_EQUAL(res[0].items[5].arrival.time_of_day().total_seconds(), 11200);
    BOOST_CHECK_EQUAL(res[0].items[5].type, ItemType::stay_in);

    BOOST_CHECK_EQUAL(res[0].items[6].departure.time_of_day().total_seconds(), 11200);
    BOOST_CHECK_EQUAL(res[0].items[6].arrival.time_of_day().total_seconds(), 12000);

    //                         C=======CzzzzzzzC
    BOOST_CHECK_EQUAL(res[0].items[7].departure.time_of_day().total_seconds(), 12000);
    BOOST_CHECK_EQUAL(res[0].items[7].arrival.time_of_day().total_seconds(), 14000);

    //                                         C---3---D
    BOOST_CHECK_EQUAL(res[0].items[8].departure.time_of_day().total_seconds(), 14000);
    BOOST_CHECK_EQUAL(res[0].items[8].arrival.time_of_day().total_seconds(), 15000);
}
BOOST_AUTO_TEST_CASE(test_2nd_and_3rd_pass_ext) {
    ed::builder b("20120614");
    b.vj("1", "1", "", true)("A", 8000, 8000)("B", 9000, 9000);
    b.vj("1", "1", "", true)("A", 9000, 9000)("B", 10000, 10000);

    b.vj("2", "1", "block1", true)("B", 11000, 11000)("E", 11100, 11100);
    b.vj("4", "1", "block1", true)("E", 11100, 11100)("F", 11200, 11200);
    b.vj("5", "1", "block1", true)("F", 11200, 11200)("C", 12000, 12000);

    b.vj("2", "1", "block2", true)("B", 12000, 12000)("E", 12100, 12100);
    b.vj("4", "1", "block2", true)("E", 12100, 12100)("F", 12200, 12200);
    b.vj("5", "1", "block2", true)("F", 12200, 12200)("C", 13000, 13000);

    b.vj("3", "1", "", true)("C", 14000, 14000)("D", 15000, 15000);
    b.vj("3", "1", "", true)("C", 15000, 15000)("D", 16000, 16000);

    b.connection("B", "B", 100);
    b.connection("C", "C", 100);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("D"),
                               7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    test_2nd_and_3rd_pass_ext_checks(res1);

    // non clockwise test
    auto res2 = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("D"),
                               16100, 0, 0, type::RTLevel::Base, 2_min, false);
    test_2nd_and_3rd_pass_ext_checks(res2);
}

// 1: A-----------B---C
// 2:     A---B
// We want:
//    A-------1-------C
//        A-2-BzzzB-1-C
BOOST_AUTO_TEST_CASE(direct_and_1trans_at_same_dt) {
    ed::builder b("20120614");
    b.vj("1", "1")("A", 8000, 8000)("B", 11000, 11000)("C", 12000, 12000);
    b.vj("2", "1")("A", 9000, 9000)("B", 10000, 10000);
    b.connection("B", "B", 100);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("C"),
                              7900, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);

    BOOST_REQUIRE_EQUAL(res.size(), 2);
    const auto& direct = res[0].items.size() == 1 ? res[0] : res[1];
    const auto& fastest = res[0].items.size() == 1 ? res[1] : res[0];
    BOOST_REQUIRE_EQUAL(direct.items.size(), 1);
    BOOST_REQUIRE_EQUAL(fastest.items.size(), 3);

    //    A-------1-------C
    BOOST_CHECK_EQUAL(direct.items[0].departure.time_of_day().total_seconds(), 8000);
    BOOST_CHECK_EQUAL(direct.items[0].arrival.time_of_day().total_seconds(), 12000);

    //        A-2-BzzzB-1-C
    BOOST_CHECK_EQUAL(fastest.items[0].departure.time_of_day().total_seconds(), 9000);
    BOOST_CHECK_EQUAL(fastest.items[0].arrival.time_of_day().total_seconds(), 10000);
    BOOST_CHECK_EQUAL(fastest.items[1].departure.time_of_day().total_seconds(), 10000);
    BOOST_CHECK_EQUAL(fastest.items[1].arrival.time_of_day().total_seconds(), 11000);
    BOOST_CHECK_EQUAL(fastest.items[2].departure.time_of_day().total_seconds(), 11000);
    BOOST_CHECK_EQUAL(fastest.items[2].arrival.time_of_day().total_seconds(), 12000);
}

// we have:
//   ====A-----1-----B-----1-----C
//===========D-2-B
//                       B-3-C
//
// we want:
//   ====A-----1-----B-----1-----C
//   ====A-----1-----BzzzB-3-C
//
// and not:
//===========D-2-BzzzB-----1-----C
BOOST_AUTO_TEST_CASE(dont_return_dominated_solutions) {
    ed::builder b("20120614");
    b.vj("1", "1")("A", 8000, 8000)("B", 11000, 11000)("C", 14000, 14000);
    b.vj("2", "1")("D", 9000, 9000)("B", 10000, 10000);
    b.vj("3", "1")("B", 12000, 12000)("C", 13000, 13000);
    b.connection("B", "B", 0);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("A")->stop_point_list.front())] = 1000_s;
    departures[SpIdx(*d.stop_areas_map.at("D")->stop_point_list.front())] = 3000_s;
    arrivals[SpIdx(*d.stop_areas_map.at("C")->stop_point_list.front())] = 0_s;
    auto res = raptor.compute_all(departures, arrivals, 1000, type::RTLevel::Base, 2_min);

    BOOST_REQUIRE_EQUAL(res.size(), 2);
    const auto& direct = res[0].items.size() == 1 ? res[0] : res[1];
    const auto& fastest = res[0].items.size() == 1 ? res[1] : res[0];

    //   ====A-----1-----B-----1-----C
    BOOST_REQUIRE_EQUAL(direct.items.size(), 1);
    BOOST_CHECK_EQUAL(direct.items[0].departure.time_of_day().total_seconds(), 8000);
    BOOST_CHECK_EQUAL(direct.items[0].arrival.time_of_day().total_seconds(), 14000);

    //   ====A-----1-----BzzzB-3-C
    BOOST_REQUIRE_EQUAL(fastest.items.size(), 3);
    BOOST_CHECK_EQUAL(fastest.items[0].departure.time_of_day().total_seconds(), 8000);
    BOOST_CHECK_EQUAL(fastest.items[0].arrival.time_of_day().total_seconds(), 11000);
    BOOST_CHECK_EQUAL(fastest.items[1].departure.time_of_day().total_seconds(), 11000);
    BOOST_CHECK_EQUAL(fastest.items[1].arrival.time_of_day().total_seconds(), 12000);
    BOOST_CHECK_EQUAL(fastest.items[2].departure.time_of_day().total_seconds(), 12000);
    BOOST_CHECK_EQUAL(fastest.items[2].arrival.time_of_day().total_seconds(), 13000);
}

//       DC1     1J      GM1     1J           C1
//     /->+<--------------+<-------------------+
//     |  +-------------->+                     + C2
//     | DC2     1R      GM2                    ^
//     |                                        |
//     |                                        |2
//     |                                        |
//     |                    42                  |
//     \--------------------<-------------------+ VG
//
// We ask for VG to GM. We want VG--2->C2=zC2--1J->GM1
BOOST_AUTO_TEST_CASE(second_pass) {
    ed::builder b("20150101");

    b.vj("2")("VG", "8:31"_t)("C2", "8:36"_t);
    b.vj("1J")("C1", "8:42"_t)("GM1", "8:47"_t)("DC1", "8:49"_t);
    b.vj("1R")("DC2", "8:51"_t)("GM2", "8:53"_t);
    b.vj("42")("VG", "8:35"_t)("DC1", "8:45"_t);

    b.connection("C1", "C2", "00:02"_t);
    b.connection("C2", "C1", "00:02"_t);
    b.connection("GM1", "GM2", "00:02"_t);
    b.connection("DC1", "DC2", "00:02"_t);
    b.connection("DC2", "DC1", "00:02"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("VG")->stop_point_list.front())] = 0_s;
    arrivals[SpIdx(*d.stop_areas_map.at("GM1")->stop_point_list.front())] = 0_s;
    arrivals[SpIdx(*d.stop_areas_map.at("GM2")->stop_point_list.front())] = 0_s;

    auto res = raptor.compute_all(departures, arrivals, DateTimeUtils::set(2, "08:30"_t), type::RTLevel::Base, 2_min,
                                  DateTimeUtils::inf, 10, {}, {}, true);

    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items.size(), 4);

    BOOST_CHECK_EQUAL(res[0].items[0].departure, "20150103T083100"_dt);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150103T083600"_dt);
    BOOST_CHECK_EQUAL(res[0].items[1].departure, "20150103T083600"_dt);
    BOOST_CHECK_EQUAL(res[0].items[1].arrival, "20150103T083800"_dt);
    BOOST_CHECK_EQUAL(res[0].items[2].departure, "20150103T083800"_dt);
    BOOST_CHECK_EQUAL(res[0].items[2].arrival, "20150103T084200"_dt);
    BOOST_CHECK_EQUAL(res[0].items[3].departure, "20150103T084200"_dt);
    BOOST_CHECK_EQUAL(res[0].items[3].arrival, "20150103T084700"_dt);
}

// A---1---B
//         B===C===D
//             C---D---2---E
//
// We want
// A---1---B===C---D---2---E
// and not
// A---1---B===C===D---2---E
static void test_good_connection_when_walking_as_fast_as_bus(const std::vector<navitia::routing::Path>& res) {
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items.size(), 3);

    // A---1---B===C---D---2---E
    using boost::posix_time::time_from_string;
    BOOST_CHECK_EQUAL(res[0].items[0].departure, time_from_string("2015-01-01 08:00:00"));
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, time_from_string("2015-01-01 09:00:00"));
    BOOST_CHECK_EQUAL(res[0].items[1].departure, time_from_string("2015-01-01 09:00:00"));
    BOOST_CHECK_EQUAL(res[0].items[1].arrival, time_from_string("2015-01-01 10:00:00"));
    BOOST_CHECK_EQUAL(res[0].items[2].departure, time_from_string("2015-01-01 10:00:00"));
    BOOST_CHECK_EQUAL(res[0].items[2].arrival, time_from_string("2015-01-01 12:00:00"));
}
BOOST_AUTO_TEST_CASE(good_connection_when_walking_as_fast_as_bus) {
    ed::builder b("20150101");

    b.vj("1")("A", "8:00"_t)("B", "9:00"_t);
    b.vj("2")("C", "10:00"_t)("D", "11:00"_t)("E", "12:00"_t);
    b.connection("B", "C", "01:00"_t);
    b.connection("B", "D", "02:00"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("E"),
                               "8:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true);
    test_good_connection_when_walking_as_fast_as_bus(res1);

    // non clockwise test
    auto res2 = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("E"),
                               "12:00"_t, 0, 0, type::RTLevel::Base, 2_min, false);
    test_good_connection_when_walking_as_fast_as_bus(res2);
}

// We have 2 sp at the sa A: A1 not wheelchair, and A2 wheelchair. A
// vj do A1->B and arrive at 9, and a vj do A2->B and arrive at 10.
// We should not take the first vj as we can't take it with a
// wheelchair.
BOOST_AUTO_TEST_CASE(accessible_on_first_sp) {
    using navitia::type::hasProperties;
    using boost::posix_time::time_from_string;

    ed::builder b("20150101");

    b.sa("A")("A1", 0, 0, false)("A2", 0, 0, true);
    b.sa("B")("B1", 0, 0, true);
    b.vj("1")("A1", "8:30"_t)("B1", "9:00"_t);
    b.vj("2")("A2", "8:00"_t)("B1", "10:00"_t);
    auto params = type::AccessibiliteParams();
    params.properties.set(hasProperties::WHEELCHAIR_BOARDING, true);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("B"),
                              "8:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true, params);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items.back().arrival, time_from_string("2015-01-01 10:00:00"));

    // non clockwise test
    res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("B"),
                         "10:00"_t, 0, 0, type::RTLevel::Base, 2_min, false, params);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items.back().arrival, time_from_string("2015-01-01 10:00:00"));
}

// If the direct path duration is better than everything, no journey is returned
//
// Here, we have a 10s walk + 1 min pt + 10s walk. We ask with a 15s
// direct path, thus we must not have any solution. With a 25s direct
// path, we have the journey.
BOOST_AUTO_TEST_CASE(direct_path_filter) {
    ed::builder b("20150101");

    b.vj("1")("A", "8:02"_t)("B", "8:03"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("A")->stop_point_list.front())] = 10_s;
    arrivals[SpIdx(*d.stop_areas_map.at("B")->stop_point_list.front())] = 10_s;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(2, "08:00"_t),
                                  type::RTLevel::Base,
                                  2_min,
                                  DateTimeUtils::inf,
                                  10,
                                  {},
                                  {},
                                  true,
                                  135_s); // 135s direct path
    BOOST_CHECK_EQUAL(res.size(), 0);

    res = raptor.compute_all(departures,
                             arrivals,
                             DateTimeUtils::set(2, "08:00"_t),
                             type::RTLevel::Base,
                             2_min,
                             DateTimeUtils::inf,
                             10,
                             {},
                             {},
                             true,
                             145_s); // 145s direct path
    BOOST_CHECK_EQUAL(res.size(), 1);

    // reverse clockwise
    res = raptor.compute_all(departures,
                             arrivals,
                             DateTimeUtils::set(2, "08:05"_t),
                             type::RTLevel::Base,
                             2_min,
                             0,
                             10,
                             {},
                             {},
                             false,
                             135_s); // 135s direct path
    BOOST_CHECK_EQUAL(res.size(), 0);

    res = raptor.compute_all(departures,
                             arrivals,
                             DateTimeUtils::set(2, "08:05"_t),
                             type::RTLevel::Base,
                             2_min,
                             0,
                             10,
                             {},
                             {},
                             false,
                             145_s); // 145s direct path
    BOOST_CHECK_EQUAL(res.size(), 1);
}

// A1 and A2 are in the stop area A. We don't want to take pt to go
// from a stop area to the same stop area to win 2 walking
// seconds. Related to http://jira.canaltp.fr/browse/NAVITIAII-1708
BOOST_AUTO_TEST_CASE(no_iti_from_to_same_sa) {
    using navitia::type::hasProperties;
    using boost::posix_time::time_from_string;

    ed::builder b("20150101");

    b.sa("A")("A1")("A2");
    b.sa("B")("B1");
    b.vj("1")("A1", "8:00"_t)("B1", "8:10"_t);
    b.vj("2")("B1", "8:20"_t)("A2", "8:30"_t);
    b.connection("B1", "B1", "00:01"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_points_map.at("A1"))] = 4_s;
    arrivals[SpIdx(*d.stop_points_map.at("A2"))] = 4_s;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(2, "07:50"_t),
                                  type::RTLevel::Base,
                                  2_min,
                                  0,
                                  10,
                                  {},
                                  {},
                                  false,
                                  10_s);
    BOOST_CHECK_EQUAL(res.size(), 0);
}

// l1: A--->--B1--->--C
// l2: A--<---B2
//            x me
//
// We don't want B1-2->A-1->C even if there is less walking as we pass
// by the starting stop area B. See
// http://jira.canaltp.fr/browse/NAVITIAII-1685
BOOST_AUTO_TEST_CASE(no_going_backward) {
    using navitia::type::hasProperties;
    using boost::posix_time::time_from_string;

    ed::builder b("20150101");

    b.sa("B")("B1")("B2");
    b.vj("l1")("A", "8:09"_t)("B1", "8:10"_t)("C", "8:20"_t);
    b.vj("l2")("B2", "8:07"_t)("A", "8:08"_t);
    b.connection("A", "A", "00:00"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_points_map.at("B1"))] = 5_s;
    departures[SpIdx(*d.stop_points_map.at("B2"))] = 0_s;
    arrivals[SpIdx(*d.stop_points_map.at("C"))] = 0_s;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(0, "08:30"_t),
                                  type::RTLevel::Base,
                                  2_min,
                                  0,
                                  10,
                                  {},
                                  {},
                                  false);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items.size(), 1);
    using boost::posix_time::time_from_string;
    BOOST_CHECK_EQUAL(res[0].items[0].departure, time_from_string("2015-01-01 08:10:00"));
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, time_from_string("2015-01-01 08:20:00"));
}

// VJ1: A--->B
// VJ2:       B---->C
//
// Journey from A to C. We can walk to B, but then we arrive to late
// for VJ2 and no solution is possible.  So we want
// A--VJ1-->B--VJ2-->C
//
// Related to http://jira.canaltp.fr/browse/NAVITIAII-1776
BOOST_AUTO_TEST_CASE(walking_doesnt_break_connection) {
    ed::builder b("20150101");

    b.vj("1", "1")("A", "8:00"_t)("B", "8:10"_t);
    b.vj("2", "1")("B", "8:11"_t)("C", "9:00"_t);
    b.connection("B", "B", "00:00"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_points_map.at("A"))] = 0_s;
    departures[SpIdx(*d.stop_points_map.at("B"))] = 15_min;
    arrivals[SpIdx(*d.stop_points_map.at("C"))] = 0_s;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(0, "08:00"_t),
                                  type::RTLevel::Base, 2_min);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items.size(), 3);
    using boost::posix_time::time_from_string;
    BOOST_CHECK_EQUAL(res[0].items[0].departure, time_from_string("2015-01-01 08:00:00"));
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, time_from_string("2015-01-01 08:10:00"));
    BOOST_CHECK_EQUAL(res[0].items[1].departure, time_from_string("2015-01-01 08:10:00"));
    BOOST_CHECK_EQUAL(res[0].items[1].arrival, time_from_string("2015-01-01 08:11:00"));
    BOOST_CHECK_EQUAL(res[0].items[2].departure, time_from_string("2015-01-01 08:11:00"));
    BOOST_CHECK_EQUAL(res[0].items[2].arrival, time_from_string("2015-01-01 09:00:00"));
}


/**
  *
  * l1:  A ----> B1*-----> C1
  *              |         |
  *              |         |
  * l2:          B2 -----> C2 -----> D2
  *
  *
  * B1 is not accessible, we should do the cnx on C1-C2 even if it is less interresting
  **/
BOOST_AUTO_TEST_CASE(accessibility_drop_off_forbidden) {
    ed::builder b("20150101");

    b.sa("A", 0, 0, false)("A1", 0, 0, true);
    b.sa("B", 0, 0, false)("B1", 0, 0, false)("B2", 0, 0, true);
    b.sa("C", 0, 0, false)("C1", 0, 0, true)("C2", 0, 0, true);
    b.sa("D", 0, 0, false)("D2", 0, 0, true);

    b.vj("l1", "11111", "", true)
            ("A1", "8:00"_t)("B1", "11:00"_t)("C1", "11:30"_t);
    b.vj("l2", "11111", "", true)
            ("B2", "11:00"_t)("C2", "12:31"_t)("D2", "13:00"_t);

    b.connection("B1", "B2", "00:00"_t);
    b.connection("C1", "C2", "01:00"_t);

    auto params = type::AccessibiliteParams();
    params.properties.set(type::hasProperties::WHEELCHAIR_BOARDING, true);
    params.vehicle_properties.set(type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE, true);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("D"),
                              "8:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true, params);

    BOOST_REQUIRE_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    const auto j = res[0];
    BOOST_CHECK_EQUAL(j.items.front().departure, time_from_string("2015-01-01 08:00:00"));
    BOOST_CHECK_EQUAL(j.items.back().arrival, time_from_string("2015-01-01 13:00:00"));

    // we should have 2 pt section + 1 transfer and one waiting
    BOOST_REQUIRE_EQUAL(j.items.size(), 4);

    BOOST_REQUIRE(! j.items[0].stop_times.empty());
    // we should do the cnx on D1
    BOOST_CHECK_EQUAL(j.items[0].stop_points.back()->uri, "C1");

    // the 2th path item should be the transfer
    BOOST_CHECK(j.items[1].stop_times.empty());
    BOOST_CHECK_EQUAL(j.items[1].stop_points.front()->uri, "C1");
    BOOST_CHECK_EQUAL(j.items[1].stop_points.back()->uri, "C2");
}

/**
  *
  * l1:  A ----> B1 -----> C1*-----> D1
  *              |         |
  *              |         |
  * l2:          B2*-----> C2 -----> D2 -----> E
  *
  *
  * B2 and C1 are not accessible
  *
  * We should not be able to find any journey
  **/
BOOST_AUTO_TEST_CASE(accessibility_on_departure_cnx_no_solution) {
    ed::builder b("20150101");

    b.sa("A", 0, 0, false)("A1", 0, 0, true);
    b.sa("B", 0, 0, false)("B1", 0, 0, true)("B2", 0, 0, false);
    b.sa("C", 0, 0, false)("C1", 0, 0, false)("C2", 0, 0, true);
    b.sa("D", 0, 0, false)("D1", 0, 0, true)("D2", 0, 0, true);
    b.sa("E", 0, 0, false)("E1", 0, 0, true);

    b.vj("l1", "11111", "", true)
            ("A1", "8:00"_t)("B1", "9:00"_t)("C1", "11:00"_t)("D1", "11:30"_t);
    b.vj("l2", "11111", "", true)
            ("B2", "9:01"_t)("C2", "11:01"_t)("D2", "12:31"_t)("E1", "13:00"_t);

    b.connection("B1", "B2", "00:00"_t);
    b.connection("C1", "C2", "00:00"_t);
    //no cnx between D1 and D2

    auto params = type::AccessibiliteParams();
    params.properties.set(type::hasProperties::WHEELCHAIR_BOARDING, true);
    params.vehicle_properties.set(type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE, true);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("E"),
                              "8:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true, params);

    BOOST_REQUIRE_EQUAL(res.size(), 0);
}


/**
  *
  * l1:  A ----> B1 -----> C1*-----> D1
  *              |         |         |
  *              |         |         |
  * l2:          B2*-----> C2 -----> D2 -----> E
  *
  *
  * B2 and C1 are not accessible. D1 and D2 are both accessible.
  *
  * Even if the connexion D1-D2 is not good (1h of connexion), it should be the one used
  **/
BOOST_AUTO_TEST_CASE(accessibility_on_departure_cnx) {
    ed::builder b("20150101");

    b.sa("A", 0, 0, false)("A1", 0, 0, true);
    b.sa("B", 0, 0, false)("B1", 0, 0, true)("B2", 0, 0, false);
    b.sa("C", 0, 0, false)("C1", 0, 0, false)("C2", 0, 0, true);
    b.sa("D", 0, 0, false)("D1", 0, 0, true)("D2", 0, 0, true);
    b.sa("E", 0, 0, false)("E1", 0, 0, true);

    b.vj("l1", "11111", "", true)
            ("A1", "8:00"_t)("B1", "9:00"_t)("C1", "11:00"_t)("D1", "11:30"_t);
    b.vj("l2", "11111", "", true)
            ("B2", "9:01"_t)("C2", "11:01"_t)("D2", "12:41"_t)("E1", "13:00"_t);

    b.connection("B1", "B2", "00:00"_t);
    b.connection("C1", "C2", "00:00"_t);
    b.connection("D1", "D2", "01:00"_t);

    auto params = type::AccessibiliteParams();
    params.properties.set(type::hasProperties::WHEELCHAIR_BOARDING, true);
    params.vehicle_properties.set(type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE, true);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("E"),
                              "8:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true, params);

    BOOST_REQUIRE_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    const auto j = res[0];
    BOOST_CHECK_EQUAL(j.items.front().departure, time_from_string("2015-01-01 08:00:00"));
    BOOST_CHECK_EQUAL(j.items.back().arrival, time_from_string("2015-01-01 13:00:00"));

    // we should have 2 pt section + 1 transfer and one waiting
    BOOST_REQUIRE_EQUAL(j.items.size(), 4);

    BOOST_REQUIRE(! j.items[0].stop_times.empty());
    // we should do the cnx on D1
    BOOST_CHECK_EQUAL(j.items[0].stop_points.back()->uri, "D1");

    // the 2th path item should be the transfer
    BOOST_CHECK(j.items[1].stop_times.empty());
    BOOST_CHECK_EQUAL(j.items[1].stop_points.front()->uri, "D1");
    BOOST_CHECK_EQUAL(j.items[1].stop_points.back()->uri, "D2");
}


/**
  *
  * l1:  A ----> B1*-----> C1
  *              |         |
  *              |         |
  * l2:          B2 -----> C2 -----> D2
  *
  *
  * B1 is forbidden, we should do the cnx on C1-C2 even if it is less interresting
  **/
BOOST_AUTO_TEST_CASE(forbidden_uri_cnx) {
    ed::builder b("20150101");

    b.sa("A", 0, 0, false)("A1", 0, 0, true);
    b.sa("B", 0, 0, false)("B1", 0, 0, false)("B2", 0, 0, true);
    b.sa("C", 0, 0, false)("C1", 0, 0, true)("C2", 0, 0, true);
    b.sa("D", 0, 0, false)("D2", 0, 0, true);

    b.vj("l1", "11111", "", true)
            ("A1", "8:00"_t)("B1", "11:00"_t)("C1", "11:30"_t);
    b.vj("l2", "11111", "", true)
            ("B2", "11:00"_t)("C2", "12:31"_t)("D2", "13:00"_t);

    b.connection("B1", "B2", "00:00"_t);
    b.connection("C1", "C2", "01:00"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    const std::vector<std::string> forbid = {"B1"};

    auto res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("D"),
                              "8:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min,
                              true, type::AccessibiliteParams(), 10, forbid);

    BOOST_REQUIRE_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    const auto j = res[0];
    BOOST_CHECK_EQUAL(j.items.front().departure, time_from_string("2015-01-01 08:00:00"));
    BOOST_CHECK_EQUAL(j.items.back().arrival, time_from_string("2015-01-01 13:00:00"));

    // we should have 2 pt section + 1 transfer and one waiting
    BOOST_REQUIRE_EQUAL(j.items.size(), 4);

    BOOST_REQUIRE(! j.items[0].stop_times.empty());
    // we should do the cnx on D1
    BOOST_CHECK_EQUAL(j.items[0].stop_points.back()->uri, "C1");

    // the 2th path item should be the transfer
    BOOST_CHECK(j.items[1].stop_times.empty());
    BOOST_CHECK_EQUAL(j.items[1].stop_points.front()->uri, "C1");
    BOOST_CHECK_EQUAL(j.items[1].stop_points.back()->uri, "C2");
}

/**
  *
  * l1:  A ----> B1-----> C1*
  *              |        |
  *              |        |
  * l2:          B2 -----> C2 -----> D2
  *
  *
  * C1 is forbidden, we should do the cnx on B1-B2 even if it is less interresting
  **/
BOOST_AUTO_TEST_CASE(accessibility_on_departure_cnx_2) {
    ed::builder b("20150101");

    b.sa("A", 0, 0, false)("A1", 0, 0, true);
    b.sa("B", 0, 0, false)("B1", 0, 0, true)("B2", 0, 0, true);
    b.sa("C", 0, 0, false)("C1", 0, 0, false)("C2", 0, 0, true);
    b.sa("D", 0, 0, false)("D2", 0, 0, true);

    b.vj("l1", "11111", "", true)
            ("A1", "8:00"_t)("B1", "11:00"_t)("C1", "11:30"_t);
    b.vj("l2", "11111", "", true)
            ("B2", "12:01"_t)("C2", "12:31"_t)("D2", "13:00"_t);

    b.connection("B1", "B2", "01:00"_t);
    b.connection("C1", "C2", "00:00"_t);

    type::AccessibiliteParams params;
    params.properties.set(type::hasProperties::WHEELCHAIR_BOARDING, true);
    params.vehicle_properties.set(type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE, true);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("D"),
                              "8:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true, params);

    BOOST_REQUIRE_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    const auto j = res[0];
    BOOST_CHECK_EQUAL(j.items.front().departure, time_from_string("2015-01-01 08:00:00"));
    BOOST_CHECK_EQUAL(j.items.back().arrival, time_from_string("2015-01-01 13:00:00"));

    // we should have 2 pt section + 1 transfer and one waiting
    BOOST_REQUIRE_EQUAL(j.items.size(), 4);

    BOOST_REQUIRE(! j.items[0].stop_times.empty());
    // we should do the cnx on D1
    BOOST_CHECK_EQUAL(j.items[0].stop_points.back()->uri, "B1");

    // the 2th path item should be the transfer
    BOOST_CHECK(j.items[1].stop_times.empty());
    BOOST_CHECK_EQUAL(j.items[1].stop_points.front()->uri, "B1");
    BOOST_CHECK_EQUAL(j.items[1].stop_points.back()->uri, "B2");
}


/**
  *
  * l1:  A ----> E3-----> D2*
  *               \      /
  *                \    /
  * l2:              E1 -----> O -----> Arr
  *
  *
  * D2 is not accessible, we should do the cnx on E3-E1
  **/
BOOST_AUTO_TEST_CASE(accessibility_on_departure_cnx_3) {
    ed::builder b("20150101");

    b.sa("A", 0, 0, false)("A1", 0, 0, true);
    b.sa("commerce", 0, 0, false)
            ("E3", 0, 0, true)
            ("D2", 0, 0, false)
            ("E1", 0, 0, true);
    b.sa("0")("01", 0, 0, true);

    b.vj("l1", "11111", "", true)
            ("A1", "8:00"_t)("E3", "11:34"_t)("D2", "11:34"_t);
    b.vj("l2", "11111", "", true)
            ("E1", "11:37"_t)("O", "12:31"_t);
    b.vj("l3", "11111", "", true)
            ("E1", "12:37"_t)("O", "13:31"_t);
    // we add 2 lines on E1 -> O to have some concurency
    b.vj("l4", "11111", "", true)
            ("O", "13:31"_t)("Arr", "14:31"_t);

    b.connection("E3", "E1", "00:04:41"_t);
    b.connection("D2", "E1", "00:01:49"_t);
    b.connection("O", "O", "00:00"_t);

    type::AccessibiliteParams params;
    params.properties.set(type::hasProperties::WHEELCHAIR_BOARDING, true);
    params.vehicle_properties.set(type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE, true);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    auto res = raptor.compute(d.stop_areas_map.at("A"), d.stop_areas_map.at("Arr"),
                              "8:00"_t, 0, DateTimeUtils::inf, type::RTLevel::Base, 2_min, true, params);

    BOOST_REQUIRE_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    const auto j = res[0];
    BOOST_CHECK_EQUAL(j.items.front().departure, time_from_string("2015-01-01 08:00:00"));
    BOOST_CHECK_EQUAL(j.items.back().arrival, time_from_string("2015-01-01 14:31:00"));

    // we should have 2 pt section + 1 transfer and one waiting
    BOOST_REQUIRE_EQUAL(j.items.size(), 6);

    BOOST_REQUIRE(! j.items[0].stop_times.empty());
    // we should do the cnx on D1
    BOOST_CHECK_EQUAL(j.items[0].stop_points.back()->uri, "E3");

    // the 2th path item should be the transfer
    BOOST_CHECK(j.items[1].stop_times.empty());
    BOOST_CHECK_EQUAL(j.items[1].stop_points.front()->uri, "E3");
    BOOST_CHECK_EQUAL(j.items[1].stop_points.back()->uri, "E1");
}

// zone 1 | zone 2
//   A----+---B-------C
//
// We can pickup at A and B, and we want to go to C. We want to find
// only A->C as it's the only possible solution
BOOST_AUTO_TEST_CASE(begin_different_zone1) {
    ed::builder b("20150101");
    b.vj("1")("A", "8:00"_t, "8:00"_t, 1)("B", "8:10"_t, "8:10"_t, 2)("C", "8:20"_t, "8:20"_t, 2);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("A")->stop_point_list.front())] = 0_s;
    departures[SpIdx(*d.stop_areas_map.at("B")->stop_point_list.front())] = 0_s;
    arrivals[SpIdx(*d.stop_areas_map.at("C")->stop_point_list.front())] = 0_s;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(2, "08:00"_t),
                                  type::RTLevel::Base, 2_min);
    BOOST_CHECK_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    BOOST_CHECK_EQUAL(res.at(0).items.front().departure, time_from_string("2015-01-03 08:00:00"));
    BOOST_CHECK_EQUAL(res.at(0).items.back().arrival, time_from_string("2015-01-03 08:20:00"));
}

// zone 1 | zone 2 | zone 1
//   A----+---B----+---C
//
// We can pickup at A and B, and we want to go to C. We want to find
// only B->C as it's the only possible solution
BOOST_AUTO_TEST_CASE(begin_different_zone2) {
    ed::builder b("20150101");
    b.vj("1")("A", "8:00"_t, "8:00"_t, 1)("B", "8:10"_t, "8:10"_t, 2)("C", "8:20"_t, "8:20"_t, 1);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("A")->stop_point_list.front())] = 0_s;
    departures[SpIdx(*d.stop_areas_map.at("B")->stop_point_list.front())] = 0_s;
    arrivals[SpIdx(*d.stop_areas_map.at("C")->stop_point_list.front())] = 0_s;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(2, "08:00"_t),
                                  type::RTLevel::Base, 2_min);
    BOOST_CHECK_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    BOOST_CHECK_EQUAL(res.at(0).items.front().departure, time_from_string("2015-01-03 08:10:00"));
    BOOST_CHECK_EQUAL(res.at(0).items.back().arrival, time_from_string("2015-01-03 08:20:00"));
}

// zone 1 | zone 2
//   A----+---B-------C
//
// We can pickup at A and B, and we want to go to C. We want to find
// only A->C as it's the only possible solution
BOOST_AUTO_TEST_CASE(begin_different_zone3) {
    ed::builder b("20150101");
    b.vj("1")("A", "8:00"_t, "8:00"_t, 1)("B", "8:10"_t, "8:10"_t, 2)("C", "8:20"_t, "8:20"_t, 2);
    b.vj("1")("A", "8:05"_t, "8:05"_t, 1)("B", "8:15"_t, "8:15"_t, 2)("C", "8:25"_t, "8:25"_t, 2);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    // we can't catch the first vj at A
    departures[SpIdx(*d.stop_areas_map.at("A")->stop_point_list.front())] = 2_min;
    departures[SpIdx(*d.stop_areas_map.at("B")->stop_point_list.front())] = 0_s;
    arrivals[SpIdx(*d.stop_areas_map.at("C")->stop_point_list.front())] = 0_s;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(2, "08:00"_t),
                                  type::RTLevel::Base, 2_min);

    // We should find this solution, but...
    /*
    BOOST_CHECK_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    BOOST_CHECK_EQUAL(res.at(0).items.front().departure, time_from_string("2015-01-03 08:05:00"));
    BOOST_CHECK_EQUAL(res.at(0).items.back().arrival, time_from_string("2015-01-03 08:25:00"));
    */
    BOOST_CHECK_EQUAL(res.size(), 0);
    // We don't find the solution because the first vj is better, but
    // with a bad local zone. For the moment, we don't fix that as we
    // don't have a good solution and this case should not happen
    // often.
}

// test that raptor is able to find "hidden" pathes (improving sn, but only because of the departure)
//
//                                        9h10
//       __________ A ===================> B ________________
//      /   1 min            vj 1              3 min        /
//     /                                               arrival
// departure                                 9h00         /
//    \_________________ C ==================> D ________/
//           10 min              vj 2            1 min
//
BOOST_AUTO_TEST_CASE(exhaustive_second_pass) {
    ed::builder b("20150101");
    b.vj("1")("A", "8:00"_t, "8:00"_t)("B", "9:10"_t, "9:10"_t);
    b.vj("2")("C", "8:00"_t, "8:01"_t)("D", "9:00"_t, "9:00"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("A")->stop_point_list.front())] = 1_min;
    departures[SpIdx(*d.stop_areas_map.at("C")->stop_point_list.front())] = 10_min;
    arrivals[SpIdx(*d.stop_areas_map.at("B")->stop_point_list.front())] = 3_min;
    arrivals[SpIdx(*d.stop_areas_map.at("D")->stop_point_list.front())] = 1_min;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(2, "07:00"_t),
                                  type::RTLevel::Base,
                                  2_min,
                                  DateTimeUtils::inf,
                                  10,
                                  type::AccessibiliteParams(),
                                  std::vector<std::string>(),
                                  true,
                                  boost::none,
                                  10); // only extra second passes can find that A-B has less sn

    BOOST_REQUIRE_EQUAL(res.size(), 2);
    using boost::posix_time::time_from_string;
    BOOST_CHECK_EQUAL(res.at(0).items.front().departure, time_from_string("2015-01-03 08:01:00"));
    BOOST_CHECK_EQUAL(res.at(0).items.back().arrival, time_from_string("2015-01-03 09:00:00"));
    BOOST_CHECK_EQUAL(res.at(1).items.front().departure, time_from_string("2015-01-03 08:00:00"));
    BOOST_CHECK_EQUAL(res.at(1).items.back().arrival, time_from_string("2015-01-03 09:10:00"));
}

// A=======B=======C
//        /        + <- vj extention
//       / D=======C
// Dest +--'
//
// As dist(D, Dest) < dist(B, Dest) < dist(D, Dest) + penalty, we
// don't want A=C+C=D as a solution, only A=B
BOOST_AUTO_TEST_CASE(penalty_on_vj_extentions) {
    ed::builder b("20150101");
    b.vj("1").block_id("42")("A", "09:00"_t)("B", "10:00"_t)("C", "11:00"_t);
    b.vj("2").block_id("42")("C", "11:00"_t)("D", "12:00"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("A")->stop_point_list.front())] = 0_min;
    arrivals[SpIdx(*d.stop_areas_map.at("B")->stop_point_list.front())] = 2_min;
    arrivals[SpIdx(*d.stop_areas_map.at("D")->stop_point_list.front())] = 1_min;

    auto res = raptor.compute_all(departures,
                                  arrivals,
                                  DateTimeUtils::set(2, "08:00"_t),
                                  type::RTLevel::Base,
                                  2_min);

    BOOST_REQUIRE_EQUAL(res.size(), 1);
    using boost::posix_time::time_from_string;
    BOOST_CHECK_EQUAL(res.at(0).items.front().departure, time_from_string("2015-01-03 09:00:00"));
    BOOST_CHECK_EQUAL(res.at(0).items.back().arrival, time_from_string("2015-01-03 10:00:00"));
}
