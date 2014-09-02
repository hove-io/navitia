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
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "ed/build_helper.h"
#include "type/type.h"
#include "departure_board_test_data.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

int32_t time_to_int(int h, int m, int s) {
    auto dur = navitia::time_duration(h, m, s);
    return dur.total_seconds(); //time are always number of seconds from midnight
}

using namespace navitia::timetables;

boost::gregorian::date date(std::string str) {
    return boost::gregorian::from_undelimited_string(str);
}

BOOST_AUTO_TEST_CASE(test1) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 36000, 36100)("stop2", 36150,362000);
    b.vj("B")("stop1", 36000, 36100)("stop2", 36150,36200)("stop3", 36250,36300);
    b.data->pt_data->index();
    b.data->build_raptor();

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20120613");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20120630");

    b.data->meta->production_date = boost::gregorian::date_period(begin, end);

    pbnavitia::Response resp = departure_board("stop_point.uri=stop2", {}, {}, "20120615T094500", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::terminus);
    stop_schedule = resp.stop_schedules(1);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),1);

    resp = departure_board("stop_point.uri=stop2", {}, {}, "20120615T094500", 800, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::terminus);
    stop_schedule = resp.stop_schedules(1);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::no_departure_this_day);

    resp = departure_board("stop_point.uri=stop2", {}, {}, "20120701T094500", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);
    BOOST_REQUIRE_EQUAL(resp.error().id(), pbnavitia::Error::date_out_of_bounds);
}



BOOST_FIXTURE_TEST_CASE(test_data_set, calendar_fixture) {
    //simple test on the data set creation

    //we check that each vj is associated with the right calendar
    //NOTE: this is better checked in the UT for associated cal

    BOOST_REQUIRE_EQUAL(b.data->pt_data->meta_vj["week"]->associated_calendars.size(), 1);
    BOOST_REQUIRE(b.data->pt_data->meta_vj["week"]->associated_calendars["week_cal"]);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->meta_vj["week_bis"]->associated_calendars.size(), 1);
    BOOST_REQUIRE(b.data->pt_data->meta_vj["week_bis"]->associated_calendars["week_cal"]);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->meta_vj["weekend"]->associated_calendars.size(), 1);
    BOOST_REQUIRE(b.data->pt_data->meta_vj["weekend"]->associated_calendars["weekend_cal"]);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->meta_vj["all"]->associated_calendars.size(), 2);
    BOOST_REQUIRE(b.data->pt_data->meta_vj["all"]->associated_calendars["week_cal"]);
    BOOST_REQUIRE(b.data->pt_data->meta_vj["all"]->associated_calendars["weekend_cal"]);
    BOOST_REQUIRE(b.data->pt_data->meta_vj["wednesday"]->associated_calendars.empty());
}
/*
 * unknown calendar in request => error
 */
BOOST_FIXTURE_TEST_CASE(test_no_weekend, calendar_fixture) {

    //when asked on non existent calendar, we get an error
    boost::optional<const std::string> calendar_id{"bob_the_calendar"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE(resp.has_error());
    BOOST_REQUIRE(! resp.error().message().empty());
}

/*
 * For this test we want to get the schedule for the week end
 * we thus will get the 'week end' vj + the 'all' vj
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_weekend, calendar_fixture) {
    boost::optional<const std::string> calendar_id{"weekend_cal"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 2);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(15, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date

    stop_date_time = stop_schedule.date_times(1);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(20, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    //the vj 'wednesday' is never matched
}

/*
 * For this test we want to get the schedule for the week
 * we thus will get the 2 'week' vj + the 'all' vj
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_week, calendar_fixture) {
    boost::optional<const std::string> calendar_id{"week_cal"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 3);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(10, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    stop_date_time = stop_schedule.date_times(1);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(11, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    stop_date_time = stop_schedule.date_times(2);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(15, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    //the vj 'wednesday' is never matched
}

/*
 * when asked with a calendar not associated with the line, we got an empty schedule
 */
BOOST_FIXTURE_TEST_CASE(test_not_associated_cal, calendar_fixture) {
    boost::optional<const std::string> calendar_id{"not_associated_cal"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_calendar_with_exception, calendar_fixture) {
    //we add a new calendar that nearly match a vj
    auto nearly_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
    nearly_cal->uri = "nearly_cal";
    nearly_cal->active_periods.push_back({beg, end_of_year});
    nearly_cal->week_pattern = std::bitset<7>{"1111100"};
    //we add 2 exceptions (2 add), one by week
    navitia::type::ExceptionDate exception_date;
    exception_date.date = date("20120618");
    exception_date.type = navitia::type::ExceptionDate::ExceptionType::add;
    nearly_cal->exceptions.push_back(exception_date);
    exception_date.date = date("20120619");
    exception_date.type = navitia::type::ExceptionDate::ExceptionType::add;
    nearly_cal->exceptions.push_back(exception_date);


    b.data->pt_data->calendars.push_back(nearly_cal);
    b.lines["line:A"]->calendar_list.push_back(nearly_cal);

    //call all the init again
    b.data->build_uri();
    b.data->pt_data->index();
    b.data->build_raptor();

    b.data->complete();

    boost::optional<const std::string> calendar_id{"nearly_cal"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    //it should match only the 'all' vj
    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 3);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(10, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date

    auto properties = stop_date_time.properties();
    BOOST_REQUIRE_EQUAL(properties.exceptions_size(), 2);
    auto exception = properties.exceptions(0);
    BOOST_REQUIRE_EQUAL(exception.uri(), "exception:120120618");
    BOOST_REQUIRE_EQUAL(exception.date(), "20120618");
    BOOST_REQUIRE_EQUAL(exception.type(), pbnavitia::ExceptionType::Remove);

    exception = properties.exceptions(1);
    BOOST_REQUIRE_EQUAL(exception.uri(), "exception:120120619");
    BOOST_REQUIRE_EQUAL(exception.date(), "20120619");
    BOOST_REQUIRE_EQUAL(exception.type(), pbnavitia::ExceptionType::Remove);

}
