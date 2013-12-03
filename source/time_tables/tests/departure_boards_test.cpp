#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "time_tables/departure_boards.h"
#include "ed/build_helper.h"

using namespace navitia::timetables;

BOOST_AUTO_TEST_CASE(test1){
    ed::builder b("20120614");
    b.vj("A")("stop1", 36000, 36100)("stop2", 36150,362000);
    b.vj("B")("stop1", 36000, 36100)("stop2", 36150,36200)("stop3", 36250,36300);
    b.data.pt_data.index();
    b.data.build_raptor();

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20120613");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20120630");

    b.data.meta.production_date = boost::gregorian::date_period(begin, end);

    pbnavitia::Response resp = departure_board("stop_point.uri=stop2", {""}, "20120615T094500", 86400, std::numeric_limits<int>::max(), 1, 10, 0, b.data);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.status(), pbnavitia::ResponseStatus::terminus);
    stop_schedule = resp.stop_schedules(1);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),1);

    resp = departure_board("stop_point.uri=stop2", {""}, "20120615T094500", 800, std::numeric_limits<int>::max(), 1, 10, 0, b.data);

    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.status(), pbnavitia::ResponseStatus::terminus);
    stop_schedule = resp.stop_schedules(1);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.status(), pbnavitia::ResponseStatus::no_departure_this_day);

    resp = departure_board("stop_point.uri=stop2", {""}, "20120701T094500", 86400, std::numeric_limits<int>::max(), 1, 10, 0, b.data);
    BOOST_REQUIRE_EQUAL(resp.error().id(), pbnavitia::Error::date_out_of_bounds);
}



