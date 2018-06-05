/* Copyright © 2001-2018, Canal TP and/or its affiliates. All rights reserved.

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
#define BOOST_TEST_MODULE test_journey

#include "routing/journey.h"
#include "tests/utils_test.h"
#include "ed/build_helper.h"

#include <boost/test/unit_test.hpp>

using namespace navitia;
using namespace routing;

struct logger_initialized {
    logger_initialized() { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );

namespace {
    const std::vector<type::StopTime> make_stop_times(ed::builder &b)
    {
        auto vj1 = b.vj("VJ1");
        auto vj2 = b.vj("VJ2");
        auto vj3 = b.vj("VJ3");

        vj1.make();
        vj2.make();
        vj3.make();

        vj1("StopPoint1", "09:42:00"_t)("StopPoint2", "10:00:00"_t);
        vj2("StopPoint1", "09:37:00"_t)("StopPoint2", "09:55:00"_t);
        vj3("StopPoint1", "09:42:00"_t)("StopPoint2", "10:00:00"_t);

        std::vector<type::StopTime> stop_times;

        stop_times.push_back(vj1.stop_times[0].st);
        stop_times.push_back(vj1.stop_times[1].st);

        stop_times.push_back(vj2.stop_times[0].st);
        stop_times.push_back(vj2.stop_times[1].st);

        stop_times.push_back(vj3.stop_times[0].st);
        stop_times.push_back(vj3.stop_times[1].st);

        return stop_times;
    }
}

BOOST_AUTO_TEST_CASE(similar_sections_should_be_equal) {

    ed::builder b("20180101");
    auto sts = make_stop_times(b);

    Journey::Section section1(sts[0], 1, sts[1], 2);

    Journey j1; j1.sections.push_back(section1);
    Journey j2; j2.sections.push_back(section1);

    BOOST_CHECK_EQUAL(j1, j2);
}

BOOST_AUTO_TEST_CASE(similar_sections_should_not_be_equal_with_different_VJ) {

    ed::builder b("20180101");
    auto sts = make_stop_times(b);

    Journey::Section section1(sts[0], 1, sts[1], 2);
    Journey::Section section2(sts[4], 1, sts[5], 2);

    Journey j1; j1.sections.push_back(section1);
    Journey j2; j2.sections.push_back(section2);

    BOOST_CHECK_NE(j1, j2);
}

BOOST_AUTO_TEST_CASE(different_sections_should_NOT_be_equal) {

    ed::builder b("20180101");
    auto sts = make_stop_times(b);

    Journey::Section section1(sts[0], 1, sts[1], 2);
    Journey::Section section2(sts[1], 1, sts[0], 2);

    Journey j1; j1.sections.push_back(section1);
    Journey j2; j2.sections.push_back(section2);

    BOOST_CHECK_NE(j1, j2);
}

BOOST_AUTO_TEST_CASE(journeys_should_be_usable_in_a_set) {

    ed::builder b("20180101");
    auto sts = make_stop_times(b);

    Journey::Section section1(sts[0], 1, sts[1], 2);
    Journey::Section section2(sts[2], 1, sts[3], 2);

    Journey j1; j1.sections.push_back(section1);
    Journey j2; j2.sections.push_back(section2);

    JourneySet journey_set;
    journey_set.insert(j1);
    BOOST_CHECK_EQUAL(journey_set.size(), 1);

    journey_set.insert(j1);
    BOOST_CHECK_EQUAL(journey_set.size(), 1);

    journey_set.insert(j2);
    BOOST_CHECK_EQUAL(journey_set.size(), 2);
}
