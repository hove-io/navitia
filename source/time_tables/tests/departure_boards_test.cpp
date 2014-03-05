#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "time_tables/departure_boards.h"
#include "ed/build_helper.h"
#include "type/type.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )


using namespace navitia::timetables;

BOOST_AUTO_TEST_CASE(test1) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 36000, 36100)("stop2", 36150,362000);
    b.vj("B")("stop1", 36000, 36100)("stop2", 36150,36200)("stop3", 36250,36300);
    b.data.pt_data.index();
    b.data.build_raptor();

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20120613");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20120630");

    b.data.meta.production_date = boost::gregorian::date_period(begin, end);

    pbnavitia::Response resp = departure_board("stop_point.uri=stop2", {}, {}, "20120615T094500", 86400, std::numeric_limits<int>::max(), 1, 10, 0, b.data, false);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::terminus);
    stop_schedule = resp.stop_schedules(1);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),1);

    resp = departure_board("stop_point.uri=stop2", {}, {}, "20120615T094500", 800, std::numeric_limits<int>::max(), 1, 10, 0, b.data, false);

    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::terminus);
    stop_schedule = resp.stop_schedules(1);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::no_departure_this_day);

    resp = departure_board("stop_point.uri=stop2", {}, {}, "20120701T094500", 86400, std::numeric_limits<int>::max(), 1, 10, 0, b.data, false);
    BOOST_REQUIRE_EQUAL(resp.error().id(), pbnavitia::Error::date_out_of_bounds);
}

BOOST_AUTO_TEST_CASE(test_calendar) {
    ed::builder b("20120614");
    b.vj("network:R", "line:A", "", "", true, "week")("stop1", 36000, 36100)("stop2", 36150, 36200);

    b.vj("network:R", "line:A", "", "", true, "weekend")("stop1", 36000, 36100)("stop2", 36150, 36200)("stop3", 36250, 36300);

    b.vj("network:R", "line:A", "", "", true, "all")("stop1", 40000, 36100)("stop2", 40150, 40200)("stop3", 40250, 40300);

    b.data.build_uri();
    boost::gregorian::date beg = b.data.meta.production_date.begin();
    auto end_of_year = beg + boost::gregorian::years(1);
    boost::posix_time::ptime beg_time (beg, {});
    boost::posix_time::ptime eoy_time (end_of_year, {});

    navitia::type::VehicleJourney* vj_week = b.data.pt_data.vehicle_journeys_map["week"];
    vj_week->validity_pattern->add(beg, end_of_year, std::bitset<7>{"0111110"});
    navitia::type::VehicleJourney* vj_weekend = b.data.pt_data.vehicle_journeys_map["weekend"];
    vj_weekend->validity_pattern->add(beg, end_of_year, std::bitset<7>{"1000001"});

    //we now add 2 similar calendars
    auto week_cal = new navitia::type::Calendar(b.data.meta.production_date.begin());
    week_cal->uri = "week_cal";
    week_cal->active_periods.push_back({beg_time, eoy_time});
    week_cal->week_pattern = std::bitset<7>{"0111110"};
    b.data.pt_data.calendars.push_back(week_cal);

    auto weekend_cal = new navitia::type::Calendar(b.data.meta.production_date.begin());
    weekend_cal->uri = "weekend_cal";
    weekend_cal->active_periods.push_back({beg_time, eoy_time});
    weekend_cal->week_pattern = std::bitset<7>{"1000001"};
    b.data.pt_data.calendars.push_back(weekend_cal);

    b.data.pt_data.index();
    b.data.build_raptor();

    b.data.complete();

    boost::optional<const std::string> calendar_id{"weekend_cal"};
    pbnavitia::Response resp = departure_board("stop_point.uri=stop2", calendar_id, {}, "20120615T094500", 86400, std::numeric_limits<int>::max(), 1, 10, 0, b.data, false);

}

