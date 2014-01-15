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
        |------------------------|----------------------------|----
VJ1 :   |*********************** |****************************|****
VJ2 :   |*********************** |****************************|****

                10h15              11h10
VJ1             A------------------>B
                    10h30          11h10
VJ2                 A-------------->B

----------------------
--List of impacts:
----------------------

start_publication_date  = 2014-01-14 07:00:00
end_publication_date    = 2014-01-30 23:59:00
active_days = 11111111


            08h00                                   18h00
Impact-1:   |----------------------------------------| for 2014-01-14

            08h00     10h35
Impact-2:   |----------|    between 2014-01-16 et 2014-01-17

            08h00 10h29
Impact-3:   |------|    for 2014-01-18

Call-1:
-------
parameters :
    date: 20140114T101000
    without_disrupt=false

Result: The raptor propose vehicle journey vj2 with departure at 10h30 and arrival at 11h10.
This is the optimized solution.

Call-2:
-------
parameters :
    date: 20140114T101000
    without_disrupt=true

Result: The raptor propose vehicle journey vj1 with departure at 10h10 and arrival at 11h10.
This is not the optimized solution but another proposition avoiding vehiclejourney with disruption (Impact-1).

Call-3:
-------
parameters :
    date: 20140115T101000
    without_disrupt=true

Result: The raptor propose vehicle journey vj2 because there is no disruption for this day.
This is the optimized solution.

Call-4:
-------
parameters :
    date: 20140116T101000
    without_disrupt=true

Result: The raptor propose vehicle journey vj1 with departure at 10h10 and arrival at 11h10.
This is not the optimized solution but another proposition avoiding vehiclejourney with disruption (Impact-2).

Call-5:
-------
parameters :
    date: 20140118T101000
    without_disrupt=true

Result: The raptor propose vehicle journey vj2 because there is no disruption active between 10h30
and 11h10 for this day. Impact-3 is active between 08h00 and 10h29 for 2014-01-18.
This is the optimized solution.
*/

using namespace navitia;
using namespace routing;

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
        //Impact-1 on vj2 from 2014-01-14 08:32:00 à 08h40 to 2014-01-14 18:32:00 à 18h00
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

        //Impact-2 on vj2 from 2014-01-16 00:00:00 à 08h00 to 2014-01-17 23:59:59 à 10h35
        boost::shared_ptr<navitia::type::Message> message1;
        message1 = boost::make_shared<navitia::type::Message>();
        message1->uri = "mess2";
        message1->object_uri="vj2";
        message1->object_type = navitia::type::Type_e::VehicleJourney;
        message1->application_period = pt::time_period(pt::time_from_string("2014-01-16 00:00:00"),
                                                      pt::time_from_string("2014-01-17 23:59:59"));
        message1->publication_period = pt::time_period(pt::time_from_string("2014-01-14 00:00:00"),
                                                      pt::time_from_string("2014-01-30 23:59:00"));
        message1->active_days = std::bitset<8>("11111111");
        message1->application_daily_start_hour = pt::duration_from_string("08:30");
        message1->application_daily_end_hour = pt::duration_from_string("10:35");
        vj2->messages.push_back(message1);

        navitia::type::AtPerturbation perturbation1;
        perturbation1.uri = message1->uri;
        perturbation1.object_uri = message1->object_uri;
        perturbation1.object_type = message1->object_type;
        perturbation1.application_daily_start_hour = message1->application_daily_start_hour;
        perturbation1.application_daily_end_hour = message1->application_daily_end_hour;
        perturbation1.application_period = message1->application_period;
        perturbation1.active_days = message1->active_days;
        perturbations.push_back(perturbation1);

        //Impact-3 on vj2 from 2014-01-17 00:00:00 à 11h00 to 2014-01-17 23:59:59 à 14h00
        boost::shared_ptr<navitia::type::Message> message2;
        message2 = boost::make_shared<navitia::type::Message>();
        message2->uri = "mess3";
        message2->object_uri="vj2";
        message2->object_type = navitia::type::Type_e::VehicleJourney;
        message2->application_period = pt::time_period(pt::time_from_string("2014-01-18 00:00:00"),
                                                      pt::time_from_string("2014-01-18 23:59:59"));
        message2->publication_period = pt::time_period(pt::time_from_string("2014-01-14 00:00:00"),
                                                      pt::time_from_string("2014-01-30 23:59:00"));
        message2->active_days = std::bitset<8>("11111111");
        message2->application_daily_start_hour = pt::duration_from_string("08:30");
        message2->application_daily_end_hour = pt::duration_from_string("10:29");
        vj2->messages.push_back(message2);

        navitia::type::AtPerturbation perturbation2;
        perturbation2.uri = message2->uri;
        perturbation2.object_uri = message2->object_uri;
        perturbation2.object_type = message2->object_type;
        perturbation2.application_daily_start_hour = message2->application_daily_start_hour;
        perturbation2.application_daily_end_hour = message2->application_daily_end_hour;
        perturbation2.application_period = message2->application_period;
        perturbation2.active_days = message2->active_days;
        perturbations.push_back(perturbation2);

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

    //vehiclejourney vj2 impacted (impact-1) from 08h40 to 18h00 for the day of travel
    //We must get vehiclejourney=vj1 in the result.
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20140114T101000"},
                                true, navitia::type::AccessibiliteParams(), forbidden, street_network, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);

    //vehiclejourney vj2 not impacted for 2014-01-15
    //We must get vehiclejourney=vj2 in the result because vj2 takes less time (optimized).
    resp = make_response(raptor, origin, destination, {"20140115T101000"},
                                    true, navitia::type::AccessibiliteParams(), forbidden, street_network, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj2");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);


    //vehiclejourney vj2 impacted (impact-2) from 2014-01-16 00:00:00 à 08h30 to 2014-01-17 23:59:59 à 10h35
    //We must get vehiclejourney=vj1 in the result
    resp = make_response(raptor, origin, destination, {"20140116T101000"},
                                    true, navitia::type::AccessibiliteParams(), forbidden, street_network, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);

    //vehiclejourney vj2 impacted (impact-3) from 2014-01-18 00:00:00 à 08h30 to 2014-01-18 23:59:59 à 10h29
    //We must get vehiclejourney=vj2 in the result
    resp = make_response(raptor, origin, destination, {"20140118T101000"},
                                    true, navitia::type::AccessibiliteParams(), forbidden, street_network, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj2");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);
}


BOOST_AUTO_TEST_CASE(journey_simple){
    //Parameters initialized
    std::vector<std::string> forbidden;
    std::vector<navitia::type::AtPerturbation> perturbations;
    ed::builder b("20140113");
    b.vj("network:R", "line:A", "11111111", "", true, "vj1")("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
    b.vj("network:R", "line:A", "11111111", "", true, "vj2")("stop_area:stop1", 10 * 3600 + 30 * 60, 10 * 3600 + 30 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
    b.generate_dummy_basis();
    b.data.pt_data.index();
    b.data.build_raptor();
    b.data.build_uri();
    b.data.geo_ref.init();

    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2014,01,13), boost::gregorian::days(7));
    RAPTOR raptor(b.data);

    //get origin and destination point
    navitia::type::Type_e origin_type = b.data.get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data.get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");
    navitia::georef::StreetNetwork street_network(b.data.geo_ref);

    //call of raptor with parameter "without_disrupt=false"
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20140114T101000"},
                                    true, navitia::type::AccessibiliteParams(), forbidden, street_network, false);
    //We must have vehiclejourney "vj2" which leaves at 10h30 and arrives at 11h10
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
//    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj2");
//    BOOST_CHECK_EQUAL(journey.duration(), 2400);

    //Lets impact hehiclejourney vj2 from 2014-01-14 08:00:00 to 2014-01-14 18:00:00
    navitia::type::VehicleJourney* vj2 =  b.data.pt_data.vehicle_journeys[1];
    //Message general
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
    //Perturbation de type disrupt
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

    //call of raptor with parameter "without_disrupt=true"
    resp = make_response(raptor, origin, destination, {"20140114T101000"},
                                true, navitia::type::AccessibiliteParams(), forbidden, street_network, true);

//    std::string str;
//    google::protobuf::TextFormat::PrintToString(resp, &str);
//    std::cout<<str<<std::endl;


    //We must have vehiclejourney "vj1" which leaves at 10h15 and arrives at 11h10 because
    //other

//    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
//    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);

}


