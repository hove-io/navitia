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

#include "ed/data.h"
#include "ed/types.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include <boost/date_time/gregorian/greg_date.hpp>
#include <string>

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

BOOST_AUTO_TEST_CASE(do_not_shift) {
    ed::Data d;
    auto vj = new ed::types::VehicleJourney();
    d.vehicle_journeys.push_back(vj);
    auto st = new ed::types::StopTime();
    vj->stop_time_list.push_back(st);
    st->arrival_time = 50000;
    st->departure_time = 49000;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 50000);
    BOOST_CHECK_EQUAL(st->departure_time, 49000);
}


BOOST_AUTO_TEST_CASE(shift_before) {
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
    st->arrival_time = -50000;
    st->departure_time = -49000;
    d.shift_stop_times();
    BOOST_CHECK_EQUAL(st->arrival_time, 36400);
    BOOST_CHECK_EQUAL(st->departure_time, 37400);
}
