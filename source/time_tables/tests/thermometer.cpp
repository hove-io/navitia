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
#include "time_tables/thermometer.h"

#include "ed/build_helper.h"

#include <ctime>
#include <cstdlib>

using namespace navitia::timetables;

BOOST_AUTO_TEST_CASE(t1) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2});
    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 3);

    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), 0));
    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), 1));
    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), 2));

    BOOST_REQUIRE_LT(posA, posB);
    BOOST_REQUIRE_LT(posB, posC);
}

BOOST_AUTO_TEST_CASE(topt) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2});
    req.push_back({3, 4, 5});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 6);

    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), 0));
    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), 1));
    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), 2));

    BOOST_REQUIRE_LT(posA, posB);
    BOOST_REQUIRE_LT(posB, posC);
}

BOOST_AUTO_TEST_CASE(t2) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2, 1, 3});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 5);

    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), 0));
    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), 1));
    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), 2));
    auto posB1 = distance(result.begin(), std::find(result.begin() + posB + 1, result.end(), 1));
    auto posE = distance(result.begin(), std::find(result.begin(), result.end(), 3));

    BOOST_REQUIRE_LT(posA, posB);
    BOOST_REQUIRE_LT(posB, posC);
    BOOST_REQUIRE_LT(posC, posB1);
    BOOST_REQUIRE_LT(posB1, posE);
}

BOOST_AUTO_TEST_CASE(t3) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2, 3, 0});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 5);

    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), 0));
    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), 1));
    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), 2));
    auto posD = distance(result.begin(), std::find(result.begin(), result.end(), 3));
    auto posA1 = distance(result.begin(), std::find(result.begin() + posA + 1, result.end(), 0));

    BOOST_REQUIRE_LT(posA, posB);
    BOOST_REQUIRE_LT(posB, posC);
    BOOST_REQUIRE_LT(posC, posD);
    BOOST_REQUIRE_LT(posD, posA1);
}

BOOST_AUTO_TEST_CASE(t4) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2, 3, 0, 4});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 6);

    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), 0));
    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), 1));
    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), 2));
    auto posD = distance(result.begin(), std::find(result.begin(), result.end(), 3));
    auto posA1 = distance(result.begin(), std::find(result.begin() + posA + 1, result.end(), 0));
    auto posE = distance(result.begin(), std::find(result.begin(), result.end(), 4));

    BOOST_REQUIRE_LT(posA, posB);
    BOOST_REQUIRE_LT(posB, posC);
    BOOST_REQUIRE_LT(posC, posD);
    BOOST_REQUIRE_LT(posD, posA1);
    BOOST_REQUIRE_LT(posA1, posE);
}

BOOST_AUTO_TEST_CASE(t5) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2, 1, 3, 0, 4});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 7);

    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), 0));
    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), 1));
    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), 2));
    auto posB1 = distance(result.begin(), std::find(result.begin() + posB + 1, result.end(), 1));
    auto posD = distance(result.begin(), std::find(result.begin(), result.end(), 3));
    auto posA1 = distance(result.begin(), std::find(result.begin() + posA + 1, result.end(), 0));
    auto posE = distance(result.begin(), std::find(result.begin(), result.end(), 4));

    BOOST_REQUIRE_LT(posA, posB);
    BOOST_REQUIRE_LT(posB, posC);
    BOOST_REQUIRE_LT(posC, posB1);
    BOOST_REQUIRE_LT(posB1, posD);
    BOOST_REQUIRE_LT(posD, posA1);
    BOOST_REQUIRE_LT(posA1, posE);
}

BOOST_AUTO_TEST_CASE(t6) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2});
    req.push_back({0, 2, 1});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 4);

    auto posA = distance(result.begin(), std::find(result.begin(), result.end(), 0));
    auto posB = distance(result.begin(), std::find(result.begin(), result.end(), 1));
    auto posC = distance(result.begin(), std::find(result.begin(), result.end(), 2));

    BOOST_REQUIRE_LT(posA, posB);
    BOOST_REQUIRE_LT(posA, posC);
    BOOST_REQUIRE_NE(posB, posC);

    if (posB < posC) {
        auto posB1 = distance(result.begin(), std::find(result.begin() + posB + 1, result.end(), 1));
        BOOST_REQUIRE_LT(posC, posB1);
    } else {
        auto posC1 = distance(result.begin(), std::find(result.begin() + posC + 1, result.end(), 2));
        BOOST_REQUIRE_LT(posB, posC1);
    }
}

BOOST_AUTO_TEST_CASE(t7) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2, 3, 4});
    req.push_back({4, 5, 2, 6, 0});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 9);
}

BOOST_AUTO_TEST_CASE(t8) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    req.push_back({4, 5, 2, 6, 0, 11, 5, 47, 9});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 15);
}

BOOST_AUTO_TEST_CASE(t9) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({
        5,
        1,
        2,
        35,
        4,
        5,
        6,
        7,
        8,
        59,
    });
    req.push_back({5, 4, 2, 6, 0, 115, 5, 47, 9});
    req.push_back({4, 5, 2, 65, 0, 11, 5, 47, 9});
    req.push_back({4, 5, 2, 6, 05, 11, 5, 457, 9});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 21);
}

BOOST_AUTO_TEST_CASE(t10) {
    Thermometer t;
    std::vector<vector_idx> req;

    std::vector<vector_idx> vec_tmp = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 188, 19, 80},
        {4, 5, 2, 6, 0, 11, 5, 47, 1, 23, 1, 10, 5, 2, 3, 17, 55, 12, 13, 14, 48, 5, 2, 3, 17, 3, 0},
        {4, 5, 82, 6, 0, 11, 700, 10, 12, 13, 14, 48, 5, 2, 3, 17, 3, 0},
        {84,  5, 82, 6, 0,  11, 5,  0,   118, 5, 47, 1, 23, 1, 10, 12,
         777, 1, 23, 1, 10, 12, 13, 184, 48,  5, 2,  3, 17, 3, 0},
        {4, 5, 287, 6, 0, 11, 5, 47, 61, 23, 1, 108, 12, 13, 14, 48, 5, 2, 3, 17, 3, 0},
        {4, 5, 86, 0, 11, 889, 1, 782, 1, 10, 12, 13, 14, 48, 5, 2, 3, 17, 3, 0},
        {4, 5, 2, 6, 0, 878, 5, 47, 1, 783, 1, 140, 560, 14, 48, 5, 2, 3, 17, 3, 0},
        {4, 5, 62, 6, 0, 11, 5, 47, 12, 13, 14, 4, 5, 2, 3, 17, 5, 2, 3, 17, 8, 5, 2, 3, 17, 3, 0},
        {4, 5, 2, 6, 880, 11, 58, 47, 1, 23, 1, 10, 12, 173, 14, 48, 5, 2, 3, 17, 3, 0},
        {4, 5, 2, 65, 677, 1, 23, 1, 10, 12, 13, 14, 48, 5, 2, 3, 17, 3, 0},
        {4, 45, 2, 6, 0, 11, 5, 47, 1, 2378, 1, 656, 12, 0, 118, 5, 47, 1, 23, 871, 13, 14, 48, 5, 2, 3, 17, 3, 0},
        {14, 5, 2, 66, 0, 118, 5, 47, 1, 23, 1, 10, 12, 13, 814, 48, 5, 2, 3, 17, 3, 0},
        {4, 15, 2, 6, 0, 11, 5, 677, 1, 23, 1, 10, 12, 13, 14, 48, 5, 2, 3, 17, 3, 0},
        {4, 5, 2, 6, 0, 11, 5, 47, 1, 378, 1, 446, 12, 888, 13, 14, 48, 5, 2, 3, 17, 3, 0},
        {4, 55, 2, 66, 0, 118, 5, 47, 1, 23, 1, 10, 12, 13, 814, 48, 5, 2, 3, 17, 3, 0},
        {489, 5, 2, 6, 0, 11, 5, 47, 1, 23, 1, 810, 12, 30, 14, 989, 5, 2, 3, 17, 3, 0}};

    t.generate_thermometer(vec_tmp);
    auto result = t.get_thermometer();
    bool error = false;
    for (auto journey_pattern : vec_tmp) {
        try {
            t.stop_times_order_helper(journey_pattern);
        } catch (const Thermometer::cant_match&) {
            error = true;
        }
    }

    BOOST_CHECK_EQUAL(t.get_thermometer().size(), 96);
    BOOST_CHECK_EQUAL(error, false);
}

BOOST_AUTO_TEST_CASE(t12) {
    srand(time(nullptr));
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({4, 5, 6});
    req.push_back({5, 7, 4});

    t.generate_thermometer(req);
    auto tp = t.get_thermometer();
    req.clear();
    req.push_back(tp);
    req.push_back({4, 8, 9});

    t.generate_thermometer(req);
    tp = t.get_thermometer();
    req.clear();
    req.push_back(tp);
    req.push_back({8, 7, 5});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_LT(result.size(), 87 * 111);
}

BOOST_AUTO_TEST_CASE(tmp) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({0, 1, 2, 3, 4});
    req.push_back({4, 5, 2, 6, 0, 11});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 10);
}

BOOST_AUTO_TEST_CASE(t13) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({1, 2});
    req.push_back({3, 2});
    req.push_back({0, 3, 2});
    req.push_back({0, 1, 2});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result.size(), 4);
    BOOST_CHECK_EQUAL(result[0], 0);
    BOOST_CHECK_EQUAL(result[1], 1);
    BOOST_CHECK_EQUAL(result[2], 3);
    BOOST_CHECK_EQUAL(result[3], 2);
}

BOOST_AUTO_TEST_CASE(t14) {
    Thermometer t;
    std::vector<vector_idx> req;
    req.push_back({1, 2});
    req.push_back({3, 2});
    req.push_back({0, 1, 2});
    req.push_back({0, 3, 2});

    t.generate_thermometer(req);
    auto result = t.get_thermometer();

    BOOST_REQUIRE_EQUAL(result[0], 0);
    BOOST_REQUIRE_EQUAL(result[1], 3);
    BOOST_REQUIRE_EQUAL(result[2], 1);
    BOOST_REQUIRE_EQUAL(result[3], 2);
}

//        BOOST_AUTO_TEST_CASE(lower_bound){
//            BOOST_CHECK_LE(get_lower_bound({}), 0);
//            BOOST_CHECK_LE(get_lower_bound({{1}}), 1);
//            BOOST_CHECK_LE(get_lower_bound({{1,2}}), 2);
//            BOOST_CHECK_LE(get_lower_bound({{1,2,1}}), 3);
//            BOOST_CHECK_LE(get_lower_bound({{1,2}, {1,2}}), 2);
//            BOOST_CHECK_LE(get_lower_bound({{1,2,1}, {1,2}}), 3);
//            BOOST_CHECK_LE(get_lower_bound({{1,2,1}, {1,2,3}}), 4);
//        }
