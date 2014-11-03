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
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/routing.h"

using namespace navitia;
namespace pt = boost::posix_time;

BOOST_AUTO_TEST_CASE(ctor){
    DateTime d = DateTimeUtils::set(1,2);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 1);
}

BOOST_AUTO_TEST_CASE(date_update){
    DateTime d = DateTimeUtils::set(10, 10);
    DateTimeUtils::update(d, 100);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 10);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 100);

    DateTimeUtils::update(d,50);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 11);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 50);
}

BOOST_AUTO_TEST_CASE(increment){
    DateTime d = DateTimeUtils::set(0, 0);
    d += 10;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 10);
}

BOOST_AUTO_TEST_CASE(passe_minuit){
    DateTime d = DateTimeUtils::set(10, 24*3600 - 1);
    d += 10;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 11);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 9);
}


BOOST_AUTO_TEST_CASE(passe_minuit_update){
    DateTime d = DateTimeUtils::set(10, 23*3600);
    d += 3600 + 10;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 11);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 10);
}

BOOST_AUTO_TEST_CASE(passe_minuit_update_reverse){
    DateTime d = DateTimeUtils::set(1, 300);
    DateTimeUtils::update(d, 23*3600, false);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 0);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 23*3600);
}

BOOST_AUTO_TEST_CASE(decrement){
    DateTime d = DateTimeUtils::set(2, 100);
    d -= 50;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 50);
    d -= 100;
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 1);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 24*3600 - 50);
}

BOOST_AUTO_TEST_CASE(init2) {
    DateTime d = DateTimeUtils::set(1, 24*3600 + 10);
    BOOST_CHECK_EQUAL(DateTimeUtils::date(d), 2);
    BOOST_CHECK_EQUAL(DateTimeUtils::hour(d), 10);
}

BOOST_AUTO_TEST_CASE(moins) {
    DateTime d1 = DateTimeUtils::set(10, 8*3600);
    DateTime d2 = DateTimeUtils::set(10, 9*3600);
    DateTime d3 = DateTimeUtils::set(11, 5*3600);
    uint32_t t1 = d2 - d1;
    uint32_t t2 = d3 - d1;
    BOOST_CHECK_EQUAL(t1, 3600);
    BOOST_CHECK_EQUAL(t2, 86400 - 3 * 3600);
}

BOOST_AUTO_TEST_CASE(freq_stop_time_validation){
    type::StopTime st;
    st.vehicle_journey = new type::VehicleJourney();
    st.vehicle_journey->start_time = 8000;
    st.vehicle_journey->end_time = 9000;
    st.properties.set(type::StopTime::IS_FREQUENCY);
    BOOST_CHECK(st.valid_hour(7999, true));
    BOOST_CHECK(st.valid_hour(8000, true));
    BOOST_CHECK(st.valid_hour(8500, true));
    BOOST_CHECK(st.valid_hour(9000, true));
    BOOST_CHECK(!st.valid_hour(9001, true));
    BOOST_CHECK(!st.valid_hour(7999, false));
    BOOST_CHECK(st.valid_hour(8000, false));
    BOOST_CHECK(st.valid_hour(8500, false));
    BOOST_CHECK(st.valid_hour(9000, false));
    BOOST_CHECK(st.valid_hour(9001, false));

    st.vehicle_journey->start_time = 23 * 3600;
    st.vehicle_journey->end_time = 26 * 3600;
    BOOST_CHECK(st.valid_hour(3600, true));
    BOOST_CHECK(st.valid_hour(22*3600, true));
    BOOST_CHECK(!st.valid_hour(22*3600 + 24*3600, true));
    BOOST_CHECK(st.valid_hour(23*3600, true));
    BOOST_CHECK(st.valid_hour(25*3600, true));
    BOOST_CHECK(!st.valid_hour(27*3600, true));

    BOOST_CHECK(!st.valid_hour(3600, false));
    BOOST_CHECK(!st.valid_hour(22*3600, false));
    BOOST_CHECK(st.valid_hour(22*3600 + 24*3600, false));
    BOOST_CHECK(st.valid_hour(23*3600, false));
    BOOST_CHECK(st.valid_hour(25*3600, false));
    BOOST_CHECK(st.valid_hour(27*3600, false));
}


BOOST_AUTO_TEST_CASE(weekday_conversion) {
    boost::gregorian::date today(2014, 03, 5);
    BOOST_CHECK_EQUAL(navitia::get_weekday(today), navitia::Wednesday);
    BOOST_CHECK_EQUAL(today.day_of_week(), boost::date_time::Wednesday);

    {
        boost::gregorian::date after(2014, 03, 6);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Thursday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Thursday);
    }
    {
        boost::gregorian::date after(2014, 03, 7);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Friday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Friday);
    }
    {
        boost::gregorian::date after(2014, 3, 8);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Saturday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Saturday);
    }
    {
        boost::gregorian::date after(2014, 3, 9);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Sunday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Sunday);
    }
    {
        boost::gregorian::date after(2014, 3, 10);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Monday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Monday);
    }
    {
        boost::gregorian::date after(2014, 3, 11);
        BOOST_CHECK_EQUAL(navitia::get_weekday(after), navitia::Tuesday);
        BOOST_CHECK_EQUAL(after.day_of_week(), boost::date_time::Tuesday);
    }

}

BOOST_AUTO_TEST_CASE(simple_duration_construction) {
    // we can 'cast' a boost posix duration to a navitia duration with the from_boost_duration method
    pt::time_duration boost_dur(12,24,49, 1);

    navitia::time_duration nav_dur(navitia::time_duration::from_boost_duration(boost_dur));

    BOOST_CHECK_EQUAL(nav_dur.hours(), 12);
    BOOST_CHECK_EQUAL(nav_dur.minutes(), 24);
    BOOST_CHECK_EQUAL(nav_dur.seconds(), 49);
    BOOST_CHECK_EQUAL(nav_dur.fractional_seconds(), 1);
}

BOOST_AUTO_TEST_CASE(time_dur_no_overflow_with_infinity) {
    pt::time_duration big_dur = pt::pos_infin;

    pt::time_duration other_dur (big_dur);//no problem to copy the duration with boost

    navitia::time_duration nav_dur = navitia::time_duration::from_boost_duration(big_dur);

    //and no problem with navitia either, it is recognised as infinity
    BOOST_CHECK_EQUAL(nav_dur, pt::pos_infin);
}

BOOST_AUTO_TEST_CASE(time_dur_overflow) {
    //we build a very big duration
    pt::time_duration big_dur (0,0,0,std::numeric_limits<int64_t>::max() - 21);

    pt::time_duration other_dur (big_dur);//no problem to copy the duration with boost

    //but the max should be to high for navitia duration
    BOOST_CHECK_THROW(navitia::time_duration::from_boost_duration(big_dur), navitia::exception);
}

namespace pt = boost::posix_time;
BOOST_AUTO_TEST_CASE(expand_calendar_full_days) {
    auto beg = pt::from_iso_string("20140101T120000");
    auto end = pt::from_iso_string("20140108T100000");

    auto days = std::bitset<7>("1111111");
    auto start_in_days = pt::duration_from_string("00:00");
    auto end_in_days = pt::duration_from_string("24:00");
    //all days and all day, we don't want to split

    auto periods = navitia::expand_calendar(beg, end, start_in_days, end_in_days, days);

    BOOST_REQUIRE_EQUAL(periods.size(), 1);
    BOOST_CHECK_EQUAL(periods[0].begin(), beg);
    BOOST_CHECK_EQUAL(periods[0].end(), end);

}

BOOST_AUTO_TEST_CASE(expand_calendar_test) {
    auto beg = pt::from_iso_string("20140101T120000");
    auto end = pt::from_iso_string("20140108T100000");

    auto days = std::bitset<7>("1111111");
    auto start_in_days = pt::duration_from_string("11:00");
    auto end_in_days = pt::duration_from_string("14:00");

    auto periods = navitia::expand_calendar(beg, end, start_in_days, end_in_days, days);

    BOOST_REQUIRE_EQUAL(periods.size(), 8);

    //first starts from 12:00 (because it's beg's time) and finish at 14:00 (it's the regular end of a day)
    BOOST_CHECK_EQUAL(periods[0].begin(), pt::from_iso_string("20140101T120000"));
    BOOST_CHECK_EQUAL(periods[0].end(), pt::from_iso_string("20140101T140000") + pt::seconds(1));

    for (int i = 1; i < 7 ; ++i) {
        BOOST_CHECK_EQUAL(periods[i].begin(), pt::ptime(boost::gregorian::date(2014,01,i+1),
                                                        pt::duration_from_string("11:00")));
        BOOST_CHECK_EQUAL(periods[i].end(), pt::ptime(boost::gregorian::date(2014,01,i+1),
                                                      pt::duration_from_string("14:00") + pt::seconds(1)));
    }

    //last is null since the end is at 10:00 (because it's beg's time) and start is a 11:00
    BOOST_CHECK(periods.back().is_null());
}
