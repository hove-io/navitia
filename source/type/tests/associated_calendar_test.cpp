#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE associated_calendar_test
#include <boost/test/unit_test.hpp>
#include "type/type.h"
#include "ed/build_helper.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

//helper for lazyness
boost::gregorian::date date(std::string str) {
    return boost::gregorian::from_undelimited_string(str);
}

struct calendar_fixture {
    calendar_fixture() : b("20140210"),
        cal(new navitia::type::Calendar(b.data.meta.production_date.begin())) {
        cal->uri="cal1";
        boost::posix_time::ptime start = boost::posix_time::from_iso_string("20140210T000010");
        boost::posix_time::ptime end = boost::posix_time::from_iso_string("20140214T000000"); //end is not in the period
        cal->active_periods.push_back({start, end});
        cal->week_pattern = std::bitset<7>("1111111");
        b.data.pt_data.calendars.push_back(cal);
    }
    ed::builder b;
    navitia::type::Calendar* cal;
};

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test1, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                    13/02/2014
               +-----------------------------+
    */
    cal->build_validity_pattern();
    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140212")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140214")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140514")));
}

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test2, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                   13/02/2014
               +----------------------------+
    */
    navitia::type::ExceptionDate exd;
    exd.date = date("20140212");
    exd.type = navitia::type::ExceptionDate::ExceptionType::sub;
    cal->exceptions.push_back(exd);

    cal->build_validity_pattern();

    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140212")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
}

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test3, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                   13/02/2014
               +----------------------------+
    */
    navitia::type::ExceptionDate exd;
    exd.date = date("20140212");
    exd.type = navitia::type::ExceptionDate::ExceptionType::add;
    cal->exceptions.push_back(exd);

    cal->build_validity_pattern();

    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
}

struct associated_cal_fixture : public calendar_fixture {
    associated_cal_fixture() {
        b.vj("network:R", "line:A", "1111", "", true, "vj1")
                ("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)
                ("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
        cal->build_validity_pattern();
    }
};

BOOST_FIXTURE_TEST_CASE(associated_val_test1, associated_cal_fixture) {

    /*
    Periodes :
           10/02/2014                   13/02/2014
               +----------------------------+
    */
    // VehicleJourney à un validitypattern equivalent au validitypattern du calendrier
    b.data.build_associated_calendar();
//    navitia::type::Calendar* cal = b.data.pt_data.calendars.front();
//    navitia::type::VehicleJourney* vj = b.data.pt_data.vehicle_journeys.front();
//    BOOST_REQUIRE(vj->associated_calendars);
//    BOOST_CHECK((vj->associated_calendars->calendar == cal));
//    BOOST_CHECK((vj->associated_calendars->calendar->exceptions.size() == 0));
//    BOOST_CHECK((vj->associated_calendars->exceptions.size() == 0));

}
