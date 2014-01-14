#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE raptor_adapted_test
#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "ed/build_helper.h"
#include "routing/raptor.h"
#include "ed/adapted.h"
#include "routing/raptor_api.h"

#include <google/protobuf/text_format.h>



/*
                14/01/2014              15/01/2014
        |------------------------|----------------------------|
VJ1 :   |*********************** |****************************|
VJ2 :   |*********************** |****************************|

     10h15              11h10
VJ1  A------------------>B
    10h30               11h10
VJ2  A------------------>B

start_publication_date  = 2014-01-14 07:00:00
end_publication_date    = 2014-01-14 23:59:00

start_application_date  = 2014-01-14 08:32:00
end_application_date    = 2014-01-14 18:32:00

start_application_daily_hour = 08h40
end_application_daily_hour = 18h00
active_days = 11111111

  */

namespace pt = boost::posix_time;
class Params{
public:
    std::vector<std::string> forbidden;
    ed::builder b;
    std::vector<navitia::type::AtPerturbation> perturbations;

    Params():b("20140113"){
        std::vector<std::string> forbidden;
        b.vj("network:R", "line:A", "11111111", "", true, "vj1")("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
        b.vj("network:R", "line:A", "11111111", "", true, "vj2")("stop_area:stop1", 10 * 3600 + 30 * 60, 10 * 3600 + 30 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
        b.generate_dummy_basis();
        b.data.pt_data.index();
        b.data.build_raptor();
        b.data.build_uri();

        navitia::type::VehicleJourney* vj2 =  b.data.pt_data.vehicle_journeys[1];
        boost::shared_ptr<navitia::type::Message> message;
        message = boost::make_shared<navitia::type::Message>();
        message->uri = "mess1";
        message->object_uri="vj2";
        message->object_type = navitia::type::Type_e::VehicleJourney;
        message->application_period = pt::time_period(pt::time_from_string("2014-01-14 00:00:00"),
                                                      pt::time_from_string("2014-01-14 23:59:59"));
        message->publication_period = pt::time_period(pt::time_from_string("2014-01-14 00:00:00"),
                                                      pt::time_from_string("2014-01-30 23:59:00"));
        message->active_days = std::bitset<8>("11111111");
        message->application_daily_start_hour = pt::duration_from_string("08:00");
        message->application_daily_end_hour = pt::duration_from_string("18:00");
        vj2->messages.push_back(message);

        navitia::type::AtPerturbation perturbation;
        perturbation.uri = message->uri;
        perturbation.object_uri = message->object_uri;
        perturbation.object_type = message->object_type;
        perturbation.application_daily_start_hour = message->application_daily_start_hour;
        perturbation.application_daily_end_hour = message->application_daily_end_hour;
        perturbation.application_period = message->application_period;
        perturbation.active_days = message->active_days;
        perturbations.push_back(perturbation);
        ed::AtAdaptedLoader adapter;
        adapter.apply(perturbations, b.data.pt_data);
    }
};

BOOST_FIXTURE_TEST_CASE(Test_without_disrupt_false, Params) {
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2014,01,13), boost::gregorian::days(7));
    navitia::routing::RAPTOR raptor(b.data);

    navitia::type::Type_e origin_type = b.data.get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data.get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");
    navitia::georef::StreetNetwork street_network(b.data.geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20140114T101000"},
                                true, navitia::type::AccessibiliteParams(), forbidden, street_network, false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj2");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);
}

BOOST_FIXTURE_TEST_CASE(Test_without_disrupt_true, Params) {
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2014,01,13), boost::gregorian::days(7));
    navitia::routing::RAPTOR raptor(b.data);

    navitia::type::Type_e origin_type = b.data.get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data.get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");
    navitia::georef::StreetNetwork street_network(b.data.geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20140114T101000"},
                                true, navitia::type::AccessibiliteParams(), forbidden, street_network, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
}
