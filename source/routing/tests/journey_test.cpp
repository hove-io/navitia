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
#define BOOST_TEST_MODULE test_journey

#include "routing/journey.h"
#include "tests/utils_test.h"
#include "ed/build_helper.h"
#include "utils/logger.h"

#include <boost/test/unit_test.hpp>

using namespace navitia;
using namespace routing;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

using boost::posix_time::duration_from_string;

class JourneyTest {
    ed::builder b;
    std::vector<type::StopTime> stop_times;
    std::vector<Journey::Section> sections;

public:
    JourneyTest() : b("20180101") {}

    Journey::Section make_section(const std::string& vj_name,
                                  const std::string& departure_time,
                                  const std::string& arrival_time) {
        b.vj(vj_name, "11111111", vj_name)("StopPoint1", departure_time)("StopPoint2", arrival_time);

        auto it = b.block_vjs.find(vj_name);
        auto& vj = *it->second;

        return Journey::Section(vj.stop_time_list[0], 1, vj.stop_time_list[1], 2);
    }

    Journey make_journey(const std::string& vj_name,
                         const std::string& departure_time,
                         const std::string& arrival_time) {
        Journey j;
        Journey::Section section = make_section(vj_name, departure_time, arrival_time);

        j.sections.push_back(section);
        j.departure_dt = duration_from_string(departure_time).total_seconds();
        j.arrival_dt = duration_from_string(arrival_time).total_seconds();

        return j;
    }
};

BOOST_FIXTURE_TEST_SUITE(journey, JourneyTest)

BOOST_AUTO_TEST_CASE(similar_sections_should_be_equal) {
    Journey::Section section = make_section("vj1", "09:42:00", "10:00:00");

    Journey j1;
    j1.sections.push_back(section);
    Journey j2;
    j2.sections.push_back(section);

    BOOST_CHECK_EQUAL(j1, j2);
}

BOOST_AUTO_TEST_CASE(similar_sections_should_not_be_equal_with_different_VJ) {
    Journey j1 = make_journey("vj1", "09:42:00", "10:00:00");
    Journey j2 = make_journey("vj2", "09:42:00", "10:00:00");

    BOOST_CHECK_NE(j1, j2);
}

BOOST_AUTO_TEST_CASE(different_sections_should_NOT_be_equal) {
    Journey::Section section = make_section("vj1", "09:42:00", "10:00:00");
    Journey j1;
    j1.sections.push_back(section);

    std::swap(section.get_in_dt, section.get_out_dt);
    Journey j2;
    j2.sections.push_back(section);

    BOOST_CHECK_NE(j1, j2);
}

BOOST_AUTO_TEST_CASE(journeys_should_be_usable_in_a_set) {
    Journey j1 = make_journey("vj1", "09:42:00", "10:00:00");
    Journey j2 = make_journey("vj2", "09:55:00", "10:10:00");

    JourneySet journey_set;

    journey_set.insert(j1);
    BOOST_CHECK_EQUAL(journey_set.size(), 1);

    journey_set.insert(j1);
    BOOST_CHECK_EQUAL(journey_set.size(), 1);

    journey_set.insert(j2);
    BOOST_CHECK_EQUAL(journey_set.size(), 2);
}

BOOST_AUTO_TEST_CASE(journeys_should_get_best_journey) {
    Journey j1 = make_journey("vj1", "09:00:00", "10:00:00");
    Journey j2 = make_journey("vj2", "08:00:00", "09:30:00");
    Journey j3 = make_journey("vj3", "10:00:00", "10:10:00");

    std::vector<Journey> journeys = {j1, j2, j3};

    auto best = get_pseudo_best_journey(journeys, true);
    BOOST_CHECK_EQUAL(best.departure_dt, j2.departure_dt);
}

BOOST_AUTO_TEST_CASE(journeys_should_get_best_journey_clockwise) {
    Journey j1 = make_journey("vj1", "09:00:00", "10:00:00");
    Journey j2 = make_journey("vj2", "08:00:00", "09:30:00");
    Journey j3 = make_journey("vj3", "10:00:00", "10:10:00");

    std::vector<Journey> journeys = {j1, j2, j3};

    auto best = get_pseudo_best_journey(journeys, false);
    BOOST_CHECK_EQUAL(best.arrival_dt, j3.arrival_dt);
}

BOOST_AUTO_TEST_CASE(get_best_journey_should_throw) {
    std::vector<Journey> journeys = {};
    BOOST_CHECK_THROW(get_pseudo_best_journey(journeys, true), recoverable_exception);
}

BOOST_AUTO_TEST_SUITE_END()
