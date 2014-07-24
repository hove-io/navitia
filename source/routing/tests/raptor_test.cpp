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
#include "ed/build_helper.h"


using namespace navitia;
using namespace routing;
namespace bt = boost::posix_time;

BOOST_AUTO_TEST_CASE(direct){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    b.data->pt_data->index();
    b.data->build_raptor();
    RAPTOR raptor(*b.data);

    auto res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0, DateTimeUtils::inf, false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0, DateTimeUtils::set(0, 8200), false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0, DateTimeUtils::set(0, 8100), false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, 0);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, 1);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    res1 = raptor.compute(b.data->pt_data->stop_areas[0], b.data->pt_data->stop_areas[1], 7900, 0, DateTimeUtils::set(0, 8099), false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(change){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("B")("stop4", 8000, 8050)("stop2", 8300, 8350)("stop5", 8400, 8450);
    b.data->pt_data->index();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::inf, false, true);
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

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::set(0,8500), false, true);
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

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::set(0, 8400), false, true);
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


    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[4], 7900, 0, DateTimeUtils::set(0, 8399), false, true);
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
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["A"], d.stop_areas_map["D"], 7*3600, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items.front().departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(to_datetime(res.items.front().departure, *(b.data))), 7*3600);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items.back().arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(to_datetime(res.items.back().arrival, *(b.data))), 7*3600 + 15*60);

    res1 = raptor.compute(d.stop_areas_map["D"], d.stop_areas_map["A"], 7*3600, 0, DateTimeUtils::inf, false, true);
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
    b.data->pt_data->index();
    b.data->build_raptor();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0, DateTimeUtils::inf, false, true);
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

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0, DateTimeUtils::set(1, 8500), false, true);
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

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0, DateTimeUtils::set(1, 20*60), false, true);
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

    res1 = raptor.compute(d.stop_areas[0], d.stop_areas[2], 22*3600, 0, DateTimeUtils::set(1, (20*60)-1), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(over_midnight_2){
    ed::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 23*3600 + 59*60);
    b.vj("B")("stop4", 23*3600 + 10*60)("stop2", 10*60)("stop3", 20*60);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::inf, false, true);

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

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 5000), false, true);

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

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 20*60), false, true);

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

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, (20*60)-1), false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(over_midnight_interne){
    ed::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 23*3600 + 30*60, 24*3600 + 30*60)("stop3", 24*3600 + 40 * 60);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::inf, false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 3600), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, 40*60), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22*3600, 0, DateTimeUtils::set(1, (40*60)-1), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(validity_pattern){
    ed::builder b("20120614");
    b.vj("D", "10", "", true)("stop1", 8000)("stop2", 8200);
    b.vj("D", "1", "", true)("stop1", 9000)("stop2", 9200);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::set(0, 10000), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9200);


    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::set(0, 9200), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7800, 0, DateTimeUtils::set(0, 9199), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 2, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

/*BOOST_AUTO_TEST_CASE(validit_pattern_2) {
    ed::builder b("20120614");
    b.vj("D", "10", "", true)("stop1", 8000)("stop2", 8200);
    b.data->pt_data->index();
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
    b.data->pt_data->index();
    b.data->build_raptor();
    RAPTOR raptor(*b.data);

    auto res1 = raptor.compute(b.data->pt_data->stop_areas[0],
            b.data->pt_data->stop_areas[3], 7900, 0, DateTimeUtils::inf, false,
            true, true, {}, std::numeric_limits<uint32_t>::max(), {"stop2"});

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), 18000);
}


BOOST_AUTO_TEST_CASE(marche_a_pied_milieu){
    ed::builder b("20120614");
    b.vj("A", "11111111", "", true)("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B", "11111111", "", true)("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(0,10000), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(0,9200), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 9200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(0,9199), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(marche_a_pied_pam){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 23*3600+59*60);
    b.vj("B")("stop3", 2*3600)("stop4",2*3600+20);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);

    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1]->idx, d.stop_areas_map["stop4"]->idx);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 2*3600+20);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[3].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(1 , (3*3600+20)), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1]->idx, d.stop_areas_map["stop4"]->idx);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 2*3600+20);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[3].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(1 , (2*3600+20)), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].stop_points[1]->idx, d.stop_areas_map["stop4"]->idx);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 2*3600+20);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[3].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 7900, 0, DateTimeUtils::set(1 , (2*3600+20)-1), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(test_rattrapage) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8*3600 + 10*60)("stop2", 8*3600 + 15*60)("stop3", 8*3600 + 35*60)("stop4", 8*3600 + 45*60);
    b.vj("B")("stop1", 8*3600 + 20*60)("stop2", 8*3600 + 25*60)("stop3", 8*3600 + 40*60)("stop4", 8*3600 + 50*60);
    b.vj("C")("stop2", 8*3600 + 30*60)("stop5", 8*3600 + 31*60)("stop3", 8*3600 + 32*60);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 8*3600 + 15*60, 0, DateTimeUtils::inf, false, true);


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

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 8*3600 + 15*60, 0, DateTimeUtils::set(0, (9*3600+45*60)), false, true);

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
    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 8*3600 + 15*60, 0, DateTimeUtils::set(0, (8*3600+45*60)), false, true);

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



    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 8*3600 + 15*60, 0, DateTimeUtils::set(0, (8*3600+45*60)-1), false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}


BOOST_AUTO_TEST_CASE(pam_veille) {
    ed::builder b("20120614");
    b.vj("A", "11111111", "", true)("stop1", 3*60)("stop2", 20*60);
    b.vj("B", "01", "", true)("stop0", 23*3600)("stop2", 24*3600 + 30*60)("stop3", 24*3600 + 40*60);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop2"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
}


BOOST_AUTO_TEST_CASE(pam_3) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 23*3600)("stop2", 24*3600 + 5 * 60);
    b.vj("B")("stop1", 23*3600)("stop3", 23*3600 + 5 * 60);
    b.vj("C1")("stop2", 10*60)("stop3", 20*60)("stop4", 30*60);
    b.vj("C1")("stop2", 23*3600)("stop3", 23*3600 + 10*60)("stop4", 23*3600 + 40*60);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 5*60, 0, DateTimeUtils::inf, false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    BOOST_CHECK_EQUAL(res1[0].items[2].arrival.time_of_day().total_seconds(), 23*3600 + 40 * 60);
}

BOOST_AUTO_TEST_CASE(sn_debut) {
    ed::builder b("20120614");
    b.vj("A","11111111", "", true)("stop1", 8*3600)("stop2", 8*3600 + 20*60);
    b.vj("B","11111111", "", true)("stop1", 9*3600)("stop2", 9*3600 + 20*60);

    std::vector<std::pair<navitia::type::idx_t, navitia::time_duration>> departs, destinations;
    departs.push_back(std::make_pair(0, navitia::seconds(10 * 60)));
    destinations.push_back(std::make_pair(1,navitia::seconds(0)));

    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));

    auto res1 = raptor.compute_all(departs, destinations, DateTimeUtils::set(0, 8*3600), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.time_of_day().total_seconds(), 9*3600 + 20 * 60);
}


BOOST_AUTO_TEST_CASE(stay_in_basic) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop2", 8*3600+15*60)("stop3", 8*3600 + 20*60);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type,stay_in);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}

BOOST_AUTO_TEST_CASE(stay_in_short) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop2", 8*3600+10*60)("stop3", 8*3600 + 20*60);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type,stay_in);
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
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["bet"], d.stop_areas_map["rs"], 60780, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type,stay_in);
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
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["bet"], d.stop_areas_map["rs"], 63200, 0, DateTimeUtils::inf, false, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type,stay_in);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 63180);
}


BOOST_AUTO_TEST_CASE(stay_in_teleport) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.back().items[1].type,stay_in);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}


BOOST_AUTO_TEST_CASE(stay_in_departure_last_of_first_vj) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop2"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}

BOOST_AUTO_TEST_CASE(stay_in_complex) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "", true)("stop1", 8*3600)("stop2", 9*3600)("stop3", 10*3600);
    b.vj("B", "1111111", "block1", true)("stop3", 10*3600+5*60)("stop2", 11*3600);
    b.vj("C", "1111111", "block1", true)("stop4", 11*3600+5*60)("stop5", 11*3600+10*60);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop5"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[4].arrival.time_of_day().total_seconds(), 11*3600+10*60);
}


BOOST_AUTO_TEST_CASE(stay_in_and_one_earlier_with_connection) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);
    b.vj("C", "1111111", "", true)("stop1", 8*3600)("stop2", 8*3600+5*60);
    b.vj("D", "1111111", "", true)("stop2", 8*3600+10*60)("stop3", 8*3600+15*60);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
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
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[4].arrival.time_of_day().total_seconds(), 8*3600 + 30*60);
}


BOOST_AUTO_TEST_CASE(stay_in_loop) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "1111111", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);
    b.vj("C", "1111111", "block1", true)("stop5", 8*3600+25*60)("stop1", 8*3600 + 30*60);
    b.vj("D", "1111111", "block1", true)("stop4", 8*3600+35*60)("stop3", 8*3600 + 40*60);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8*3600 + 20*60);
}

BOOST_AUTO_TEST_CASE(stay_in_invalid_vp) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8*3600)("stop2", 8*3600+10*60);
    b.vj("B", "0000", "block1", true)("stop4", 8*3600+15*60)("stop3", 8*3600 + 20*60);
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
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
    b.finish();
    b.data->pt_data->index();
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
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 5*60, 0, DateTimeUtils::inf, false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.time_of_day().total_seconds(), 10*3600);


    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].arrival.time_of_day().total_seconds(), 8*3600+20*60);
}


BOOST_AUTO_TEST_CASE(mdi) {
    ed::builder b("20120614");

    b.vj("B")("stop1",17*3600, 17*3600,std::numeric_limits<uint16_t>::max(), true, false)("stop2", 17*3600+15*60)("stop3",17*3600+30*60, 17*3600+30*60,std::numeric_limits<uint16_t>::max(), true, false);
    b.vj("C")("stop4",16*3600, 16*3600,std::numeric_limits<uint16_t>::max(), true, true)("stop5", 16*3600+15*60)("stop6",16*3600+30*60, 16*3600+30*60,std::numeric_limits<uint16_t>::max(), false, true);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_CHECK_EQUAL(res1.size(), 0);
    res1 = raptor.compute(d.stop_areas_map["stop2"], d.stop_areas_map["stop3"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_CHECK_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop4"], d.stop_areas_map["stop6"], 5*60, 0, DateTimeUtils::inf, false, true);
    BOOST_CHECK_EQUAL(res1.size(), 0);
    res1 = raptor.compute(d.stop_areas_map["stop4"],d.stop_areas_map["stop5"], 5*60, 0, DateTimeUtils::inf, false, true);
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

    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;


    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 5*60, 0, DateTimeUtils::inf, false, true);

    BOOST_CHECK_EQUAL(res1.size(), 1);
}


BOOST_AUTO_TEST_CASE(freq_vj) {
    ed::builder b("20120614");
    b.vj("A1")("stop1", 8*3600)("stop2", 8*3600+10*60).frequency(8*3600,18*3600,5*60);

    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;


    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 8*3600, 0, DateTimeUtils::inf, false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), 8*3600 + 10*60);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 9*3600, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), 9*3600 + 10*60);
}


BOOST_AUTO_TEST_CASE(freq_vj_pam) {
    ed::builder b("20120614");
    b.vj("A1")("stop1", 8*3600)("stop2", 8*3600+10*60).frequency(8*3600,26*3600,5*60);

    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 23*3600, 0, DateTimeUtils::inf, false, true);



    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), 23*3600 + 10*60);

    /*auto*/ res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 24*3600, 0, DateTimeUtils::inf, false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res1[0].items[0].arrival, *(b.data))), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), (24*3600 + 10*60)% DateTimeUtils::SECONDS_PER_DAY);

    /*auto*/ res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 25*3600, 0, DateTimeUtils::inf, false, true);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res1[0].items[0].arrival, *(b.data))), 1);
    BOOST_CHECK_EQUAL(res1[0].items[0].arrival.time_of_day().total_seconds(), (25*3600 + 10*60)% DateTimeUtils::SECONDS_PER_DAY);
}


BOOST_AUTO_TEST_CASE(max_duration){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::inf, false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::set(0, 8101), false, true);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::set(0, 8099), false, true);
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
    b.data->build_raptor();
    b.data->build_uri();
    RAPTOR raptor(*(b.data));
    type::PT_Data & d = *b.data->pt_data;

    for(uint32_t nb_transfers=0; nb_transfers<=2; ++nb_transfers) {
//        type::Properties p;
        auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7900, 0, DateTimeUtils::inf, false, true, true, type::AccessibiliteParams(), nb_transfers);
        BOOST_REQUIRE(res1.size()>=1);
        for(auto r : res1) {
            BOOST_REQUIRE(r.nb_changes <= nb_transfers);
        }
    }
}

