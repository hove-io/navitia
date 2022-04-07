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

#include "ed/data.h"
#include "ed/types.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include <boost/date_time/gregorian/greg_date.hpp>
#include <string>

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(do_not_shift) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    st->arrival_time = 50000;
    st->departure_time = 49000;
    st->alighting_time = 50000;
    st->boarding_time = 49000;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 50000);
    BOOST_CHECK_EQUAL(st->departure_time, 49000);
}

BOOST_AUTO_TEST_CASE(do_not_shift_freq) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    vj->start_time = 40000;
    vj->end_time = 60000;
    vj->headway_secs = 10;
    st->arrival_time = 50000;
    st->departure_time = 49000;
    st->alighting_time = 50000;
    st->boarding_time = 49000;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 50000);
    BOOST_CHECK_EQUAL(st->departure_time, 49000);
    BOOST_CHECK_EQUAL(vj->start_time, 40000);
    BOOST_CHECK_EQUAL(vj->end_time, 60000);
}

BOOST_AUTO_TEST_CASE(do_not_shift_freq_end_before) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    vj->start_time = 40000;
    vj->end_time = 30000;
    vj->headway_secs = 10;
    st->arrival_time = 50000;
    st->departure_time = 49000;
    st->alighting_time = 50000;
    st->boarding_time = 49000;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 50000);
    BOOST_CHECK_EQUAL(st->departure_time, 49000);
    BOOST_CHECK_EQUAL(vj->start_time, 40000);
    BOOST_CHECK_EQUAL(vj->end_time, 30000);
}

BOOST_AUTO_TEST_CASE(do_not_shift_freq_end_neg) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    vj->start_time = 40000;
    vj->end_time = -10000;
    vj->headway_secs = 10;
    st->arrival_time = 50000;
    st->departure_time = 49000;
    st->alighting_time = 50000;
    st->boarding_time = 49000;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 50000);
    BOOST_CHECK_EQUAL(st->departure_time, 49000);
    BOOST_CHECK_EQUAL(vj->start_time, 40000);
    BOOST_CHECK_EQUAL(vj->end_time, 76400);
}

BOOST_AUTO_TEST_CASE(shift_before) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    auto vp = new ed::types::ValidityPattern();
    vp->beginning_date = boost::gregorian::date(2014, 10, 9);
    vp->add(boost::gregorian::date(2014, 10, 10));
    vp->add(boost::gregorian::date(2014, 10, 11));
    vj->validity_pattern = vp;
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    st->arrival_time = -50000;
    st->departure_time = -49000;
    st->alighting_time = -50000;
    st->boarding_time = -49000;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 36400);
    BOOST_CHECK_EQUAL(st->departure_time, 37400);
    BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 9)));
    BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 10)));
    BOOST_CHECK(!vj->validity_pattern->check(boost::gregorian::date(2014, 10, 11)));
}

BOOST_AUTO_TEST_CASE(shift_before_freq) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    auto vp = new ed::types::ValidityPattern();
    vp->beginning_date = boost::gregorian::date(2014, 10, 9);
    vp->add(boost::gregorian::date(2014, 10, 10));
    vp->add(boost::gregorian::date(2014, 10, 11));
    vj->validity_pattern = vp;
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    vj->start_time = -60000;
    vj->end_time = 0;
    vj->headway_secs = 10;
    st->arrival_time = -50000;
    st->departure_time = -49000;
    st->alighting_time = -50000;
    st->boarding_time = -49000;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 36400);
    BOOST_CHECK_EQUAL(st->departure_time, 37400);
    BOOST_CHECK_EQUAL(vj->start_time, 26400);
    BOOST_CHECK_EQUAL(vj->end_time, 0);
    BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 9)));
    BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 10)));
    BOOST_CHECK(!vj->validity_pattern->check(boost::gregorian::date(2014, 10, 11)));
}

BOOST_AUTO_TEST_CASE(shift_before_overmidnight) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    auto vp = new ed::types::ValidityPattern();
    vp->beginning_date = boost::gregorian::date(2014, 10, 9);
    vp->add(boost::gregorian::date(2014, 10, 10));
    vp->add(boost::gregorian::date(2014, 10, 11));
    vj->validity_pattern = vp;
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    st->arrival_time = -100;
    st->departure_time = 100;
    st->alighting_time = -100;
    st->boarding_time = 100;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 86300);
    BOOST_CHECK_EQUAL(st->departure_time, 86500);
    BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 9)));
    BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 10)));
    BOOST_CHECK(!vj->validity_pattern->check(boost::gregorian::date(2014, 10, 11)));
}

BOOST_AUTO_TEST_CASE(shift_before_change_beginning_date) {
    ed::Data d;
    d.meta.production_date = {boost::gregorian::date(2014, 10, 9), boost::gregorian::date(2015, 10, 9)};
    for (auto times : {std::pair<int, int>(-50000, -49000), std::pair<int, int>(49000, 50000)}) {
        auto vp = new ed::types::ValidityPattern();
        vp->beginning_date = d.meta.production_date.begin();
        vp->add(boost::gregorian::date(2014, 10, 9));
        vp->add(boost::gregorian::date(2014, 10, 11));
        auto vj = new ed::types::VehicleJourney();
        vj->uri = "vj : " + std::to_string(times.first);
        vj->validity_pattern = vp;
        d.vehicle_journeys.push_back(vj);
        d.validity_patterns.push_back(vp);
        auto st = new ed::types::StopTime();
        vj->stop_time_list.push_back(st);
        st->arrival_time = times.first;
        st->departure_time = times.second;
        st->alighting_time = times.first;
        st->boarding_time = times.second;
    }

    d.shift_stop_times();
    {
        // First vj is shift
        auto vj = d.vehicle_journeys.front();
        auto st = vj->stop_time_list.front();
        BOOST_CHECK_EQUAL(st->arrival_time, 36400);
        BOOST_CHECK_EQUAL(st->departure_time, 37400);
        BOOST_CHECK_EQUAL(vj->validity_pattern->beginning_date, boost::gregorian::date(2014, 10, 8));
        BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 8)));
        BOOST_CHECK(!vj->validity_pattern->check(boost::gregorian::date(2014, 10, 9)));
        BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 10)));
        BOOST_CHECK(!vj->validity_pattern->check(boost::gregorian::date(2014, 10, 11)));
    }

    {
        // Second vj isn't
        auto vj = d.vehicle_journeys.back();
        auto st = vj->stop_time_list.front();
        BOOST_CHECK_EQUAL(st->arrival_time, 49000);
        BOOST_CHECK_EQUAL(st->departure_time, 50000);
        BOOST_CHECK_EQUAL(vj->validity_pattern->beginning_date, boost::gregorian::date(2014, 10, 8));
        BOOST_CHECK(!vj->validity_pattern->check(boost::gregorian::date(2014, 10, 8)));
        BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 9)));
        BOOST_CHECK(!vj->validity_pattern->check(boost::gregorian::date(2014, 10, 10)));
        BOOST_CHECK(vj->validity_pattern->check(boost::gregorian::date(2014, 10, 11)));
    }
}

BOOST_AUTO_TEST_CASE(shift_after) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    auto vp = new ed::types::ValidityPattern();
    vp->beginning_date = boost::gregorian::date(2014, 10, 9);
    vp->add(1);
    vp->add(2);
    vj->validity_pattern = vp;
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    st->arrival_time = 86500;
    st->departure_time = 86600;
    st->alighting_time = 86500;
    st->boarding_time = 86600;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 100);
    BOOST_CHECK_EQUAL(st->departure_time, 200);
}

BOOST_AUTO_TEST_CASE(shift_after_freq) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    auto vp = new ed::types::ValidityPattern();
    vp->beginning_date = boost::gregorian::date(2014, 10, 9);
    vp->add(1);
    vp->add(2);
    vj->validity_pattern = vp;
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    st->arrival_time = 86500;
    st->departure_time = 86600;
    st->alighting_time = 86500;
    st->boarding_time = 86600;
    vj->start_time = 86400;
    vj->end_time = 1000;
    vj->headway_secs = 10;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 100);
    BOOST_CHECK_EQUAL(st->departure_time, 200);
    BOOST_CHECK_EQUAL(vj->start_time, 0);
    BOOST_CHECK_EQUAL(vj->end_time, 1000);
}
