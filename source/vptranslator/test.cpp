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
#define BOOST_TEST_MODULE vptranslator_test

#include <boost/test/unit_test.hpp>
#include "vptranslator/vptranslator.h"
#include "tests/utils_test.h"

using namespace navitia::vptranslator;
using navitia::type::ExceptionDate;
using navitia::type::ValidityPattern;
using Week = navitia::type::WeekPattern;
using boost::gregorian::date;
using boost::gregorian::date_period;

//       July 2012
// Mo Tu We Th Fr Sa Su
//                    1
//  2  3  4  5  6  7  8
//  9 10 11 12 13 14 15
// 16 17 18 19 20 21 22
// 23 24 25 26 27 28 29
// 30 31

// /!\ the 0-1 strings used in these tests are bitset strings.  Thus,
// they should be readed right to left to match chronological order.

BOOST_AUTO_TEST_CASE(nb_weeks) {
    BlockPattern block;

    // one week
    block.validity_periods = {date_period(date(2012, 7, 2), date(2012, 7, 9))};
    BOOST_CHECK_EQUAL(block.nb_weeks(), 1);

    // partial + one week
    block.validity_periods = {date_period(date(2012, 7, 4), date(2012, 7, 16))};
    BOOST_CHECK_EQUAL(block.nb_weeks(), 2);

    // one week + partial
    block.validity_periods = {date_period(date(2012, 7, 2), date(2012, 7, 13))};
    BOOST_CHECK_EQUAL(block.nb_weeks(), 2);

    // partial + one week + partial with nb partial = 6
    block.validity_periods = {date_period(date(2012, 7, 4), date(2012, 7, 17))};
    BOOST_CHECK_EQUAL(block.nb_weeks(), 3);

    // partial + one week + partial with nb partial = 7
    block.validity_periods = {date_period(date(2012, 7, 4), date(2012, 7, 18))};
    BOOST_CHECK_EQUAL(block.nb_weeks(), 3);

    // partial + one week + partial with nb partial = 8
    block.validity_periods = {date_period(date(2012, 7, 4), date(2012, 7, 19))};
    BOOST_CHECK_EQUAL(block.nb_weeks(), 3);
}

BOOST_AUTO_TEST_CASE(only_first_day) {
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2), "1"));
    BOOST_CHECK_EQUAL(response.week, Week("0000001"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 2), date(2012, 7, 3))}));
}

BOOST_AUTO_TEST_CASE(bound_cut) {
    BlockPattern response;

    response = translate_one_block(ValidityPattern(date(2012, 7, 16),
                                                   "000"
                                                   "1011100"));
    BOOST_CHECK_EQUAL(response.week, Week("1011100"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 18), date(2012, 7, 23))}));

    response = translate_one_block(ValidityPattern(date(2012, 7, 16),
                                                   "00"
                                                   "0100000"));
    BOOST_CHECK_EQUAL(response.week, Week("0100000"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 21), date(2012, 7, 22))}));
}

BOOST_AUTO_TEST_CASE(empty_vp) {
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 16), "0000000"));
    BOOST_CHECK_EQUAL(response.week, Week("0"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK(response.validity_periods.empty());
}

BOOST_AUTO_TEST_CASE(only_one_thursday) {
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2),
                                                              "0001000"
                                                              "0000000"));
    BOOST_CHECK_EQUAL(response.week, Week("0001000"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 12), date(2012, 7, 13))}));
}

BOOST_AUTO_TEST_CASE(only_one_monday) {
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2),
                                                              "00001"
                                                              "0000000"));
    BOOST_CHECK_EQUAL(response.week, Week("0000001"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 9), date(2012, 7, 10))}));
}

BOOST_AUTO_TEST_CASE(only_one_sunday) {
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2),
                                                              "000000"
                                                              "1000000"));
    BOOST_CHECK_EQUAL(response.week, Week("1000000"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      std::set<date_period>{date_period(date(2012, 7, 8), date(2012, 7, 9))});
}

// only one friday saturday sunday
BOOST_AUTO_TEST_CASE(only_one_fss) {
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2),
                                                              "1111000"
                                                              "0000000"));
    BOOST_CHECK_EQUAL(response.week, Week("1111000"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 12), date(2012, 7, 16))}));
}

BOOST_AUTO_TEST_CASE(three_complete_weeks) {
    const auto days =
        "1111111"
        "1111111"
        "1111111";
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2), days));
    BOOST_CHECK_EQUAL(response.week, Week("1111111"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 2), date(2012, 7, 23))}));
}

BOOST_AUTO_TEST_CASE(MoTuThFrX3_MoTuWeThFrX2_MoTuThFrX1_SaSuX2) {
    const std::string mttf = "0011011", mtwtf = "0011111", ss = "1100000";
    const std::string pattern = ss + ss + mttf + mtwtf + mtwtf + mttf + mttf + mttf;
    const auto response = translate_no_exception(ValidityPattern(date(2012, 7, 2), pattern));
    for (const auto& bp : response) {
        BOOST_CHECK(bp.exceptions.empty());
    }
    BOOST_REQUIRE_EQUAL(response.size(), 3);
    BOOST_CHECK_EQUAL(response.at(0).week, Week(mttf));
    BOOST_CHECK_EQUAL(response.at(0).validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 2), date(2012, 7, 23)),
                                             date_period(date(2012, 8, 6), date(2012, 8, 13))}));
    BOOST_CHECK_EQUAL(response.at(1).week, Week(mtwtf));
    BOOST_CHECK_EQUAL(response.at(1).validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 23), date(2012, 8, 6))}));
    BOOST_CHECK_EQUAL(response.at(2).week, Week(ss));
    BOOST_CHECK_EQUAL(response.at(2).validity_periods,
                      (std::set<date_period>{date_period(date(2012, 8, 13), date(2012, 8, 27))}));
}

BOOST_AUTO_TEST_CASE(three_mtwss_excluding_one_day) {
    const auto days =
        "1110011"
        "1100011"
        "1110011";
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2), days));
    BOOST_CHECK_EQUAL(response.week, Week("1110011"));
    BOOST_CHECK_EQUAL(response.exceptions,
                      (std::set<ExceptionDate>{{ExceptionDate::ExceptionType::sub, date(2012, 07, 13)}}));
    BOOST_CHECK_EQUAL(response.validity_periods,
                      std::set<date_period>{date_period(date(2012, 7, 2), date(2012, 7, 23))});
}

BOOST_AUTO_TEST_CASE(three_mtwss_including_one_day) {
    const auto days =
        "1110011"
        "1111011"
        "1110011";
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2), days));
    BOOST_CHECK_EQUAL(response.week, Week("1110011"));
    BOOST_CHECK_EQUAL(response.exceptions,
                      (std::set<ExceptionDate>{{ExceptionDate::ExceptionType::add, date(2012, 07, 12)}}));
    BOOST_CHECK_EQUAL(response.validity_periods,
                      std::set<date_period>{date_period(date(2012, 7, 2), date(2012, 7, 23))});
}

BOOST_AUTO_TEST_CASE(mwtfss_mttfss_mtwfss) {
    const auto days =
        "1110111"
        "1111011"
        "1111101";
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2), days));
    BOOST_CHECK_EQUAL(response.week, Week("1111111"));
    BOOST_CHECK_EQUAL(response.exceptions,
                      (std::set<ExceptionDate>{{ExceptionDate::ExceptionType::sub, date(2012, 7, 3)},
                                               {ExceptionDate::ExceptionType::sub, date(2012, 7, 11)},
                                               {ExceptionDate::ExceptionType::sub, date(2012, 7, 19)}}));
    BOOST_CHECK_EQUAL(response.validity_periods,
                      std::set<date_period>{date_period(date(2012, 7, 2), date(2012, 7, 23))});
}

BOOST_AUTO_TEST_CASE(t_w_t) {
    const auto days =
        "0001000"
        "0000100"
        "0000010";
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2), days));
    BOOST_CHECK_EQUAL(response.week, Week("0000000"));
    BOOST_CHECK_EQUAL(response.exceptions,
                      (std::set<ExceptionDate>{{ExceptionDate::ExceptionType::add, date(2012, 7, 3)},
                                               {ExceptionDate::ExceptionType::add, date(2012, 7, 11)},
                                               {ExceptionDate::ExceptionType::add, date(2012, 7, 19)}}));
    BOOST_CHECK_EQUAL(response.validity_periods,
                      std::set<date_period>{date_period(date(2012, 7, 3), date(2012, 7, 20))});
}

BOOST_AUTO_TEST_CASE(one_full_week_except_monday_black_week_two_full_week) {
    const auto days =
        "1111111"
        "1111111"
        "0000000"
        "1111110";
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2), days));
    BOOST_CHECK_EQUAL(response.week, Week("1111111"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 3), date(2012, 7, 9)),
                                             date_period(date(2012, 7, 16), date(2012, 7, 30))}));
}

BOOST_AUTO_TEST_CASE(bound_compression) {
    const auto days =
        "0111000"
        "1111000"
        "1110000";
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 2), days));
    BOOST_CHECK_EQUAL(response.week, Week("1111000"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 6), date(2012, 7, 22))}));
}

BOOST_AUTO_TEST_CASE(SaSu_blank_week_MoTuWeThFr) {
    const auto days =
        "1111100"
        "0000011";
    const auto response = translate_one_block(ValidityPattern(date(2012, 7, 7), days));
    BOOST_CHECK_EQUAL(response.week, Week("1111111"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2012, 7, 7), date(2012, 7, 9)),
                                             date_period(date(2012, 7, 16), date(2012, 7, 21))}));
}

// ROADEF 2015 example
BOOST_AUTO_TEST_CASE(may2015) {
    const auto dates =
        "0111110"
        "0011111"
        "0010111"
        "0001111"
        "0001111";
    const auto response = translate_one_block(ValidityPattern(date(2015, 4, 27), dates));
    BOOST_CHECK_EQUAL(response.week, Week("0011111"));
    BOOST_CHECK_EQUAL(response.exceptions, (std::set<ExceptionDate>{
                                               {ExceptionDate::ExceptionType::sub, date(2015, 5, 1)},
                                               {ExceptionDate::ExceptionType::sub, date(2015, 5, 8)},
                                               {ExceptionDate::ExceptionType::sub, date(2015, 5, 14)},
                                               {ExceptionDate::ExceptionType::sub, date(2015, 5, 25)},
                                               {ExceptionDate::ExceptionType::add, date(2015, 5, 30)},
                                           }));
    BOOST_CHECK_EQUAL(response.validity_periods,
                      (std::set<date_period>{date_period(date(2015, 4, 27), date(2015, 5, 31))}));
}

// TODO: with real bound compression, this must work.
/*
BOOST_AUTO_TEST_CASE(Su_MoWeSu_Mo) {
    const auto response = translate_one_block(ValidityPattern(date(2012,7,1), "1" "1000101" "1"));
    std::cout << response << std::endl;
    BOOST_CHECK_EQUAL(response.week, Week("1000101"));
    BOOST_CHECK(response.exceptions.empty());
    BOOST_CHECK_EQUAL(response.validity_periods, (std::set<date_period>{
        date_period(date(2012,7,1), date(2012,7,10))
    }));
}
*/
