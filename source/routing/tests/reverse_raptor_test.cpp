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

#include "routing/raptor.h"
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

BOOST_AUTO_TEST_CASE(direct) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 9100, 9150);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 9200, 0, DateTimeUtils::min,
                               type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 9200, 0,
                          DateTimeUtils::set(0, 8050 - 1000), type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 9200, 0,
                          DateTimeUtils::set(0, 8050 - 1), type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 9100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 9200, 0, DateTimeUtils::set(0, 8051),
                          type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(change) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250)("stop3", 8400, 8450);
    b.vj("B")("stop4", 9000, 9050)("stop2", 9500, 9550)("stop5", 10000, 10050);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.connection("stop5", "stop5", 120);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop5"], 13000, 0, DateTimeUtils::min,
                               type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop5"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].departure.time_of_day().total_seconds(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.time_of_day().total_seconds(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].departure.time_of_day().total_seconds(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].arrival.time_of_day().total_seconds(), 10000);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop5"], 13000, 0,
                          DateTimeUtils::set(0, 8050 - 1000), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop5"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].departure.time_of_day().total_seconds(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.time_of_day().total_seconds(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].departure.time_of_day().total_seconds(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].arrival.time_of_day().total_seconds(), 10000);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop5"], 13000, 0,
                          DateTimeUtils::set(0, 8050 - 1), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop5"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].departure.time_of_day().total_seconds(), 8050);
    BOOST_CHECK_EQUAL(res.items[0].arrival.time_of_day().total_seconds(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].departure.time_of_day().total_seconds(), 8200);
    BOOST_CHECK_EQUAL(res.items[1].arrival.time_of_day().total_seconds(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].departure.time_of_day().total_seconds(), 9550);
    BOOST_CHECK_EQUAL(res.items[2].arrival.time_of_day().total_seconds(), 10000);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop5"], 13000, 0,
                          DateTimeUtils::set(0, 8050 + 1), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 23 * 3600)("stop2", 24 * 3600 + 5 * 60);
    b.vj("B")("stop2", 10 * 60)("stop3", 20 * 60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22 * 3600, 1, DateTimeUtils::min,
                               type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22 * 3600, 1,
                          DateTimeUtils::set(0, 23 * 3600 - 1000), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);

    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22 * 3600, 1,
                          DateTimeUtils::set(0, 23 * 3600 - 1), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[1].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22 * 3600, 1,
                          DateTimeUtils::set(0, 23 * 3600 + 1), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit_2) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 23 * 3600)("stop2", 23 * 3600 + 59 * 60);
    b.vj("B")("stop4", 23 * 3600 + 10 * 60)("stop2", 10 * 60)("stop3", 20 * 60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22 * 3600, 1, DateTimeUtils::min,
                               type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[2].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22 * 3600, 1,
                          DateTimeUtils::set(0, 23 * 3600 - 1000), type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[2].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22 * 3600, 1,
                          DateTimeUtils::set(0, 23 * 3600 - 1), type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 3);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[1].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[0]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[2].stop_points[1]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[2].arrival, *(b.data))), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 22 * 3600, 1,
                          DateTimeUtils::set(0, 23 * 3600 + 1), type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit_interne) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 23 * 3600)("stop2", 23 * 3600 + 30 * 60, 24 * 3600 + 30 * 60)("stop3", 24 * 3600 + 40 * 60);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 50 * 60, 1, DateTimeUtils::min,
                               type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 50 * 60, 1,
                          DateTimeUtils::set(0, 23 * 3600 - 1000), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 50 * 60, 1,
                          DateTimeUtils::set(0, 23 * 3600 - 1), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[2]->idx, d.stop_areas_map["stop3"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 50 * 60, 1,
                          DateTimeUtils::set(0, 23 * 3600 + 1), type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(passe_minuit4) {
    ed::builder b("20120614");
    b.vj("A", "0001000", "", true)("stop1", 23 * 3600 + 55 * 60, 24 * 3600)("stop2", 24 * 3600 + 15 * 60);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 24 * 3600 + 15 * 60, 3,
                               DateTimeUtils::min, nt::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();

    BOOST_REQUIRE_EQUAL(res.items.size(), 1);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[0]->idx, d.stop_areas_map["stop1"]->idx);
    BOOST_CHECK_EQUAL(res.items[0].stop_points[1]->idx, d.stop_areas_map["stop2"]->idx);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].departure, *(b.data))), 4);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(to_datetime(res.items[0].departure, *(b.data))), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(to_datetime(res.items[0].arrival, *(b.data))), 4);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(to_datetime(res.items[0].arrival, *(b.data))), 15 * 60);
}

BOOST_AUTO_TEST_CASE(validity_pattern) {
    ed::builder b("20120614");
    b.vj("D", "00", "", true)("stop1", 8000)("stop2", 8200);
    b.vj("C", "10", "", true)("stop1", 9000)("stop2", 9200);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 7000, 1, DateTimeUtils::min,
                              type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res.size(), 0);
}

BOOST_AUTO_TEST_CASE(marche_a_pied_milieu) {
    ed::builder b("20120614");
    b.vj("A", "11111111", "", true)("stop1", 8000, 8050)("stop2", 8200, 8250);
    b.vj("B", "11111111", "", true)("stop3", 10000, 19050)("stop4", 19200, 19250);
    b.connection("stop2", "stop3", 10 * 60);
    b.connection("stop3", "stop2", 10 * 60);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 27900, 0, DateTimeUtils::min,
                               type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 19200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 27900, 0,
                          DateTimeUtils::set(0, 8050 - 1000), type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 19200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 27900, 0,
                          DateTimeUtils::set(0, 8050 - 1), type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res = res1.back();
    BOOST_REQUIRE_EQUAL(res.items.size(), 4);
    BOOST_CHECK_EQUAL(res.items[3].arrival.time_of_day().total_seconds(), 19200);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop4"], 27900, 0,
                          DateTimeUtils::set(0, 8050 + 1), type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(sn_fin) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8 * 3600)("stop2", 8 * 3600 + 20 * 60);
    b.vj("B")("stop1", 9 * 3600)("stop2", 9 * 3600 + 20 * 60);

    routing::map_stop_point_duration departs, destinations;
    departs[SpIdx(0)] = 0_s;
    destinations[SpIdx(1)] = 10_min;
    b.make();
    RAPTOR raptor(*(b.data));

    auto res1 =
        raptor.compute_all(departs, destinations, DateTimeUtils::set(0, 9 * 3600 + 20 * 60), nt::RTLevel::Base, 2_min);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].departure.time_of_day().total_seconds(), 8 * 3600);
}

BOOST_AUTO_TEST_CASE(stay_in_basic) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8 * 3600)("stop2", 8 * 3600 + 10 * 60);
    b.vj("B", "1111111", "block1", true)("stop4", 8 * 3600 + 15 * 60)("stop3", 8 * 3600 + 20 * 60);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 8 * 3600 + 25 * 60, 0,
                               DateTimeUtils::min, nt::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 1);
}

BOOST_AUTO_TEST_CASE(stay_in_and_one_earlier_with_connection) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8 * 3600)("stop2", 8 * 3600 + 10 * 60);
    b.vj("B", "1111111", "block1", true)("stop4", 8 * 3600 + 15 * 60)("stop3", 8 * 3600 + 20 * 60);
    b.vj("C", "1111111", "", true)("stop1", 8 * 3600 + 10 * 60)("stop2", 8 * 3600 + 15 * 60);
    b.vj("D", "1111111", "", true)("stop2", 8 * 3600 + 20 * 60)("stop3", 8 * 3600 + 25 * 60);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 8 * 3600 + 25 * 60, 0,
                               DateTimeUtils::min, nt::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 2);
    BOOST_CHECK(std::any_of(res1.begin(), res1.end(), [](Path path) {
        return path.items[2].arrival.time_of_day().total_seconds() == 8 * 3600 + 25 * 60;
    }));
    BOOST_CHECK(std::any_of(res1.begin(), res1.end(), [](Path path) {
        return path.items[2].arrival.time_of_day().total_seconds() == 8 * 3600 + 20 * 60;
    }));
}

BOOST_AUTO_TEST_CASE(stay_in_3_vj) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8 * 3600)("stop2", 8 * 3600 + 10 * 60);
    b.vj("B", "1111111", "block1", true)("stop4", 8 * 3600 + 15 * 60)("stop5", 8 * 3600 + 20 * 60);
    b.vj("C", "1111111", "block1", true)("stop6", 8 * 3600 + 25 * 60)("stop3", 8 * 3600 + 30 * 60);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 8 * 3600 + 35 * 60, 0,
                               DateTimeUtils::min, nt::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[4].arrival.time_of_day().total_seconds(), 8 * 3600 + 30 * 60);
}

BOOST_AUTO_TEST_CASE(stay_in_loop) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8 * 3600)("stop2", 8 * 3600 + 10 * 60);
    b.vj("B", "1111111", "block1", true)("stop4", 8 * 3600 + 15 * 60)("stop3", 8 * 3600 + 20 * 60);
    b.vj("C", "1111111", "block1", true)("stop5", 8 * 3600 + 25 * 60)("stop1", 8 * 3600 + 30 * 60);
    b.vj("D", "1111111", "block1", true)("stop4", 8 * 3600 + 35 * 60)("stop3", 8 * 3600 + 40 * 60);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 8 * 3600 + 25 * 60, 0,
                               DateTimeUtils::min, nt::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[2].arrival.time_of_day().total_seconds(), 8 * 3600 + 20 * 60);
}

BOOST_AUTO_TEST_CASE(stay_in_invalid_vp) {
    ed::builder b("20120614");
    b.vj("A", "1111111", "block1", true)("stop1", 8 * 3600)("stop2", 8 * 3600 + 10 * 60);
    b.vj("B", "0000", "block1", true)("stop4", 8 * 3600 + 15 * 60)("stop3", 8 * 3600 + 20 * 60);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 5 * 60, 0, DateTimeUtils::inf,
                               nt::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(itl) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 10 * 60, 1)("stop2", 8 * 3600 + 15 * 60, 8 * 3600 + 15 * 60, 1)(
        "stop3", 8 * 3600 + 20 * 60);
    b.vj("B")("stop1", 9 * 3600)("stop2", 10 * 3600);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 9 * 3600 + 15 * 60, 0,
                               DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(res1.size(), 0);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 8 * 3600 + 20 * 60, 0,
                          DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.back().items[0].departure.time_of_day().total_seconds(), 8 * 3600 + 10 * 60);
}

BOOST_AUTO_TEST_CASE(mdi) {
    ed::builder b("20120614");

    b.vj("B")("stop1", 17 * 3600, 17 * 3600, std::numeric_limits<uint16_t>::max(), true, false)(
        "stop2", 17 * 3600 + 15 * 60)("stop3", 17 * 3600 + 30 * 60, 17 * 3600 + 30 * 60,
                                      std::numeric_limits<uint16_t>::max(), true, true);
    b.vj("C")("stop4", 16 * 3600, 16 * 3600, std::numeric_limits<uint16_t>::max(), true, true)(
        "stop5", 16 * 3600 + 15 * 60)("stop6", 16 * 3600 + 30 * 60, 16 * 3600 + 30 * 60,
                                      std::numeric_limits<uint16_t>::max(), false, true);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop3"], 17 * 3600 + 30 * 60, 0,
                               DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_CHECK_EQUAL(res1.size(), 0);
    res1 = raptor.compute(d.stop_areas_map["stop2"], d.stop_areas_map["stop3"], 17 * 3600 + 30 * 60, 0,
                          DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_CHECK_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop4"], d.stop_areas_map["stop6"], 17 * 3600 + 30 * 60, 0,
                          DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_CHECK_EQUAL(res1.size(), 0);
    res1 = raptor.compute(d.stop_areas_map["stop4"], d.stop_areas_map["stop5"], 17 * 3600 + 30 * 60, 0,
                          DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_CHECK_EQUAL(res1.size(), 1);
}

BOOST_AUTO_TEST_CASE(max_duration) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100, 8150);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 8200, 0, DateTimeUtils::min,
                               type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 8200, 0, DateTimeUtils::set(0, 8049),
                          type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 8200, 0, DateTimeUtils::set(0, 8051),
                          type::RTLevel::Base, 2_min, 2_min, false);
    BOOST_REQUIRE_EQUAL(res1.size(), 0);
}

BOOST_AUTO_TEST_CASE(max_transfers) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 81000, 81500);
    b.vj("B")("stop1", 8000)("stop3", 8500);
    b.vj("C")("stop3", 9000)("stop2", 11000);
    b.vj("D")("stop3", 9000)("stop4", 9500);
    b.vj("E")("stop4", 10000)("stop2", 10500);
    b.make();
    RAPTOR raptor(*(b.data));
    type::PT_Data& d = *b.data->pt_data;

    for (uint32_t nb_transfers = 0; nb_transfers <= 2; ++nb_transfers) {
        auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 86400, 0, DateTimeUtils::inf,
                                   nt::RTLevel::Base, 2_min, 2_min, true, type::AccessibiliteParams(), nb_transfers);
        BOOST_REQUIRE(res1.size() >= 1);
        for (auto r : res1) {
            BOOST_REQUIRE(r.nb_changes <= nb_transfers);
        }
    }
}

BOOST_AUTO_TEST_CASE(with_boarding_alighting_time_and_stay_in) {
    ed::builder b("20170101");
    b.vj("A", "1111111", "block1")
        .name("vj:A1")("S1", "09:00"_t, "09:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 600)(
            "S2", "09:05"_t, "09:05"_t, std::numeric_limits<uint16_t>::max(), true, true, 1200, 1200)(
            "S3", "09:10"_t, "09:10"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)(
            "S4", "09:15"_t, "09:15"_t, std::numeric_limits<uint16_t>::max(), true, false, 600, 0);

    b.vj("B", "1111111", "block1")
        .name("vj:B1")("S4", "09:30"_t, "09:30"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
            "S5", "09:35"_t, "09:35"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 0);

    b.vj("B").name("vj:B2")("S4", "09:35"_t, "09:35"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
        "S5", "09:40"_t, "09:40"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 0);

    b.connection("S4", "S4", 120);

    b.make();
    RAPTOR raptor(*b.data);

    auto result = raptor.compute(b.data->pt_data->stop_areas_map["S1"], b.data->pt_data->stop_areas_map["S5"],
                                 "09:45"_t, 0, DateTimeUtils::min, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 4);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20170101T085000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20170101T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20170101T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20170101T091500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].type, ItemType::stay_in);
    BOOST_CHECK_EQUAL(result.at(0).items[2].departure, "20170101T091500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].arrival, "20170101T093000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[3].departure, "20170101T093000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[3].arrival, "20170101T093500"_dt);
}

/*
 * departure                                        the shortest journey is :
 *     |         |--- B --- 5min \
 *     A --------|                arrival               A ----------> C
 *               |--- C --- 0min /
 *
 * Bug found on a journey request where the datetime is an arrival, involving alighting time.
 * The best solution won't be selected if :
 *  - the requested arrival_time < (arrival_time + arrival access time) (not sure at 100%)
 *  - the same journey pattern is available one day before the requested date
 *
 *  Stop | Departure | Arrival | Alighting | Foot path
 *    A  |   9:00    |   9:00  |   9:00    |     X
 *    B  |   9:10    |   9:10  |   9:25    |   5 min
 *    C  |   9:10    |   9:10  |   9:25    |   0 min
 */

BOOST_AUTO_TEST_CASE(arrival_with_alighting) {
    ed::builder b("20190101");

    b.sa("stop_area:A", 0, 0, false)("stop_point:A");
    b.sa("stop_area:B", 10, 10, false)("stop_point:B");
    b.sa("stop_area:C", 10, 8, false)("stop_point:C");

    b.vj("1", "011", "block_1")
        .name("vj:1")("stop_point:A", "09:00"_t, "09:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0)(
            "stop_point:B", "09:10"_t, "09:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 900)(
            "stop_point:C", "09:10"_t, "09:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 900);

    b.make();
    RAPTOR raptor(*b.data);
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("stop_area:A")->stop_point_list.front())] = 0_min;
    arrivals[SpIdx(*d.stop_areas_map.at("stop_area:B")->stop_point_list.front())] = 5_min;
    arrivals[SpIdx(*d.stop_areas_map.at("stop_area:C")->stop_point_list.front())] = 0_min;

    // The journey pattern is not available the day before the requested date
    // the result is OK
    auto result = raptor.compute_all(departures, arrivals, DateTimeUtils::set(0, "09:25"_t), type::RTLevel::Base, 2_min,
                                     2_min, DateTimeUtils::min, 10, type::AccessibiliteParams(), {}, {}, false);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 2);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20190101T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20190101T091000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20190101T091000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20190101T092500"_dt);

    // The journey pattern is available the day before the requested date
    // but the requested arrival time >= arrival time + access time
    // the result is OK
    result = raptor.compute_all(departures, arrivals, DateTimeUtils::set(1, "09:30"_t), type::RTLevel::Base, 2_min,
                                2_min, DateTimeUtils::min, 10, type::AccessibiliteParams(), {}, {}, false);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 2);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20190102T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20190102T091000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20190102T091000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20190102T092500"_dt);

    // The journey pattern is available the day before the requested date
    // the requested arrival time < arrival time + access time
    // the result is KO
    result = raptor.compute_all(departures, arrivals, DateTimeUtils::set(1, "09:25"_t), type::RTLevel::Base, 2_min,
                                2_min, DateTimeUtils::min, 10, type::AccessibiliteParams(), {}, {}, false);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 2);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20190102T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20190102T091000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20190102T091000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20190102T092500"_dt);
}

/*
 *                                 arrival         the shortest journey is :
 *          / 0 min --- A ---|        |
 * departure                 |------- C               A ----------> C
 *          \ 5 min --- B ---|
 *
 * Bug found on a journey request where the datetime is a departure, involving boarding time.
 * The best solution won't be selected if :
 *  - the requested departure_time > (departure_time - departure access time) (not sure at 100%)
 *  - the same journey pattern is available one day after the requested date
 *
 *  Stop | Departure | Arrival | Boarding | Foot path
 *    A  |   9:00    |   9:00  |   8:45   |   0 min
 *    B  |   9:00    |   9:00  |   8:45   |   5 min
 *    C  |   9:30    |   9:30  |   9:30   |    X
 */

BOOST_AUTO_TEST_CASE(departure_with_boarding) {
    ed::builder b("20190101");

    b.sa("stop_area:A", 0, 0, false)("stop_point:A");
    b.sa("stop_area:B", 0, 5, false)("stop_point:B");
    b.sa("stop_area:C", 5, 2.5, false)("stop_point:C");

    b.vj("1", "011", "block_1")
        .name("vj:1")("stop_point:A", "09:00"_t, "09:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 900)(
            "stop_point:B", "09:00"_t, "09:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 900)(
            "stop_point:C", "09:30"_t, "09:30"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 0);

    b.make();
    RAPTOR raptor(*b.data);
    const type::PT_Data& d = *b.data->pt_data;

    routing::map_stop_point_duration departures, arrivals;
    departures[SpIdx(*d.stop_areas_map.at("stop_area:A")->stop_point_list.front())] = 0_min;
    departures[SpIdx(*d.stop_areas_map.at("stop_area:B")->stop_point_list.front())] = 5_min;
    arrivals[SpIdx(*d.stop_areas_map.at("stop_area:C")->stop_point_list.front())] = 0_min;

    // The journey pattern is not available the day after the requested date
    // the result is OK
    auto result =
        raptor.compute_all(departures, arrivals, DateTimeUtils::set(1, "08:45"_t), type::RTLevel::Base, 2_min);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 2);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20190102T084500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20190102T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20190102T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20190102T093000"_dt);

    // The journey pattern is available the day after the requested date
    // but the requested departure time <= (departure time - access time)
    // the result is OK
    result = raptor.compute_all(departures, arrivals, DateTimeUtils::set(0, "08:40"_t), type::RTLevel::Base, 2_min);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 2);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20190101T084500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20190101T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20190101T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20190101T093000"_dt);

    // The journey pattern is available the day after the requested date
    // the requested departure time > (departure time - access time)
    // the result is KO
    result = raptor.compute_all(departures, arrivals, DateTimeUtils::set(0, "08:45"_t), type::RTLevel::Base, 2_min);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 2);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20190101T084500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20190101T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20190101T090000"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20190101T093000"_dt);
}

// The vp should be shifted to the previous day since the boarding time will be before midnight
BOOST_AUTO_TEST_CASE(reverse_pass_midnight_with_boardings) {
    ed::builder b("20170101");
    b.vj("A", "0111100")
        .name("vj:A1")("S1", "00:05"_t, "00:05"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 0)(
            "S2", "00:15"_t, "00:15"_t, std::numeric_limits<uint16_t>::max(), true, true, 1200, 1200)(
            "S3", "00:25"_t, "00:25"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)(
            "S4", "00:35"_t, "00:35"_t, std::numeric_limits<uint16_t>::max(), true, false, 600, 0);

    b.make();
    RAPTOR raptor(*b.data);
    const type::PT_Data& d = *b.data->pt_data;

    auto compute = [&](int day) {
        return raptor.compute(d.stop_areas_map.at("S2"), d.stop_areas_map.at("S4"), "01:00"_t, day, DateTimeUtils::min,
                              type::RTLevel::Base, 2_min, 2_min, false);
    };

    auto result = compute(0);
    BOOST_REQUIRE_EQUAL(result.size(), 0);

    result = compute(1);
    BOOST_REQUIRE_EQUAL(result.size(), 0);

    result = compute(2);
    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE_EQUAL(result.at(0).items.size(), 3);
    BOOST_CHECK_EQUAL(result.at(0).items[0].type, ItemType::boarding);
    BOOST_CHECK_EQUAL(result.at(0).items[0].departure, "20170102T235500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[0].arrival, "20170103T001500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].type, ItemType::public_transport);
    BOOST_CHECK_EQUAL(result.at(0).items[1].departure, "20170103T001500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[1].arrival, "20170103T003500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].type, ItemType::alighting);
    BOOST_CHECK_EQUAL(result.at(0).items[2].departure, "20170103T003500"_dt);
    BOOST_CHECK_EQUAL(result.at(0).items[2].arrival, "20170103T004500"_dt);

    result = compute(5);
    BOOST_REQUIRE_EQUAL(result.size(), 1);

    result = compute(6);
    BOOST_REQUIRE_EQUAL(result.size(), 0);
}

// Check that we have a valid journey with a stay_in that pass midnight
BOOST_AUTO_TEST_CASE(stay_in_pass_midnight) {
    ed::builder b("20170101");
    auto* vj1 = b.vj("A")
                    .name("vj:1")
                    .block_id("42")("S1", "23:45"_t, "23:45"_t)("S2", "23:50"_t, "23:50"_t)("S3", "24:00"_t, "24:00"_t)
                    .make();

    auto* vj2 = b.vj("A")
                    .name("vj:2")
                    .block_id("42")("S3", "24:45"_t, "24:45"_t)("S4", "24:50"_t, "24:50"_t)("S5", "24:55"_t, "24:55"_t)
                    .make();

    b.make();
    vj1->next_vj = vj2;
    vj1->prev_vj = nullptr;
    vj2->prev_vj = vj1;
    vj2->next_vj = nullptr;
    RAPTOR raptor(*b.data);

    auto result = raptor.compute(b.data->pt_data->stop_areas_map["S1"], b.data->pt_data->stop_areas_map["S5"],
                                 "01:00"_t, 2, 0, type::RTLevel::Base, 2_min, 2_min, false);

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    // as we arrive the 20170103, we must begin the day before
    BOOST_CHECK_EQUAL(result.front().items.front().departure, "20170102T234500"_dt);
}
