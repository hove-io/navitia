#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "routing/raptor_api.h"
#include "naviMake/build_helper.h"


using namespace navitia;
using namespace routing::raptor;
using namespace boost::posix_time;

BOOST_AUTO_TEST_CASE(simple_journey){
    std::vector<std::string> forbidden;
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
    type::Data data;
    b.generate_dummy_basis();
    b.build(data.pt_data);
    data.build_raptor();
    data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    RAPTOR raptor(data);

    type::EntryPoint origin("stop_area:stop1");
    type::EntryPoint destination("stop_area:stop2");

    streetnetwork::StreetNetwork sn_worker(data.geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20120614T021000"}, true, 1.38, 1000, false, forbidden, sn_worker);

    BOOST_REQUIRE(resp.has_requested_api());
    BOOST_CHECK_EQUAL(resp.requested_api(), pbnavitia::PLANNER);

    BOOST_REQUIRE(resp.has_planner());
    pbnavitia::Planner planner = resp.planner();
    BOOST_REQUIRE_EQUAL(planner.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(planner.journeys_size(), 1);
    pbnavitia::Journey journey = planner.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_point:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_point:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), "20120614T081100");
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), "20120614T082000");
}

BOOST_AUTO_TEST_CASE(journey_array){
    std::vector<std::string> forbidden;
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
    b.vj("A")("stop1", 9*3600 +10*60, 9*3600 + 11 * 60)("stop2",  9*3600 + 20 * 60 ,9*3600 + 21*60);
    type::Data data;
    b.generate_dummy_basis();
    b.build(data.pt_data);
    data.build_raptor();
    data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    RAPTOR raptor(data);

    type::EntryPoint origin("stop_area:stop1");
    type::EntryPoint destination("stop_area:stop2");

    streetnetwork::StreetNetwork sn_worker(data.geo_ref);

    // On met les horaires dans le desordre pour voir s'ils sont bien trié comme attendu
    std::vector<std::string> datetimes{"20120614T080000", "20120614T090000"};
    pbnavitia::Response resp = make_response(raptor, origin, destination, datetimes, true, 1.38, 1000, false, forbidden, sn_worker);

    BOOST_REQUIRE(resp.has_requested_api());
    BOOST_CHECK_EQUAL(resp.requested_api(), pbnavitia::PLANNER);

    BOOST_REQUIRE(resp.has_planner());
    pbnavitia::Planner planner = resp.planner();
    BOOST_REQUIRE_EQUAL(planner.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(planner.journeys_size(), 2);

    pbnavitia::Journey journey = planner.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_point:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_point:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), "20120614T081100");
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), "20120614T082000");

    journey = planner.journeys(1);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    st1 = section.stop_date_times(0);
    st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_point:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_point:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), "20120614T091100");
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), "20120614T092000");
}
