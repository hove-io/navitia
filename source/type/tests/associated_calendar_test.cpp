#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE associated_calendar_test
#include <boost/test/unit_test.hpp>
#include "type/type.h"
#include "ed/build_helper.h"

#define BOOST_CHECK_NOT(value) BOOST_CHECK(!value)

BOOST_AUTO_TEST_CASE(calendar2validity_pattern){

    ed::builder b("20140210");
    navitia::type::Calendar* cal = new navitia::type::Calendar();
    cal->uri="cal1";
    boost::posix_time::ptime start = boost::posix_time::from_iso_string("20140210T000010");
    boost::posix_time::ptime end = boost::posix_time::from_iso_string("20140213T235959");
    cal->active_periods.push_back(boost::posix_time::time_period(start, end));
    b.data.pt_data.calendars.push_back(cal);
    {
        /*
        Periodes :
               10/02/2014                    13/02/2014
                   +-----------------------------+
        */
        navitia::type::Calendar* cal = b.data.pt_data.calendars.front();

        navitia::type::ValidityPattern vp = cal->calendar2validity_pattern(b.data.meta.production_date.begin());
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140210")));
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140211")));
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140212")));
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140213")));
    }
    {
        /*
        Periodes :
               10/02/2014                   13/02/2014
                   +----------------------------+
        */
        navitia::type::Calendar* cal = b.data.pt_data.calendars.front();
        navitia::type::ExceptionDate exd;
        exd.date = boost::gregorian::from_undelimited_string("20140212");
        exd.type = navitia::type::ExceptionDate::ExceptionType::sub;
        cal->exceptions.push_back(exd);

        navitia::type::ValidityPattern vp = cal->calendar2validity_pattern(b.data.meta.production_date.begin());
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140210")));
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140211")));
        BOOST_CHECK_NOT(vp.check(boost::gregorian::from_undelimited_string("20140212")));
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140213")));
    }
    {
        /*
        Periodes :
               10/02/2014                   13/02/2014
                   +----------------------------+
        */
        navitia::type::Calendar* cal = b.data.pt_data.calendars.front();
        navitia::type::ExceptionDate& exd = cal->exceptions.front();
        exd.type = navitia::type::ExceptionDate::ExceptionType::add;

        navitia::type::ValidityPattern vp = cal->calendar2validity_pattern(b.data.meta.production_date.begin());
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140210")));
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140211")));
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140212")));
        BOOST_CHECK(vp.check(boost::gregorian::from_undelimited_string("20140213")));
        cal->exceptions.clear();
    }
}

BOOST_AUTO_TEST_CASE(associated_calendar){

    ed::builder b("20140210");
    navitia::type::Calendar* cal = new navitia::type::Calendar();
    cal->uri="cal1";
    boost::posix_time::ptime start = boost::posix_time::from_iso_string("20140210T000010");
    boost::posix_time::ptime end = boost::posix_time::from_iso_string("20140213T235959");
    cal->active_periods.push_back(boost::posix_time::time_period(start, end));
    b.data.pt_data.calendars.push_back(cal);
    b.vj("network:R", "line:A", "1111", "", true, "vj1")("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
    {
        /*
        Periodes :
               10/02/2014                   13/02/2014
                   +----------------------------+
        */
        // VehicleJourney à un validitypattern equivalent au validitypattern du calendrier
        b.data.build_associated_calendar();
        navitia::type::Calendar* cal = b.data.pt_data.calendars.front();
        navitia::type::VehicleJourney* vj = b.data.pt_data.vehicle_journeys.front();
        BOOST_CHECK((vj->associated_calendar->calendar == cal));
        BOOST_CHECK((vj->associated_calendar->calendar->exceptions.size() == 0));
        BOOST_CHECK((vj->associated_calendar->exceptions.size() == 0));
    }

    {
        /*
        Periodes :
               10/02/2014                   13/02/2014
                   +----------------------------+
        */
        // VehicleJourney à un validitypattern equivalent au validitypattern du calendrier

        b.data.build_associated_calendar();
        navitia::type::Calendar* cal = b.data.pt_data.calendars.front();
        navitia::type::VehicleJourney* vj = b.data.pt_data.vehicle_journeys.front();
        BOOST_CHECK((vj->associated_calendar->calendar == cal));
        BOOST_CHECK((vj->associated_calendar->calendar->exceptions.size() == 0));
        BOOST_CHECK((vj->associated_calendar->exceptions.size() == 0));
    }
}
