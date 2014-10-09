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
#define BOOST_TEST_MODULE raptor_adapted_test
#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "ed/build_helper.h"
#include "routing/raptor.h"
#include "ed/adapted.h"
#include "routing/raptor_api.h"
#include "utils/functions.h"
#include "tests/utils_test.h"
#include "georef/street_network.h"


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
    disruption_active=false

Result: The raptor propose vehicle journey vj2 with departure at 10h30 and arrival at 11h10.
This is the optimized solution.

Call-2:
-------
parameters :
    date: 20140114T101000
    disruption_active=true

Result: The raptor propose vehicle journey vj1 with departure at 10h10 and arrival at 11h10.
This is not the optimized solution but another proposition avoiding vehiclejourney with disruption (Impact-1).

Call-3:
-------
parameters :
    date: 20140115T101000
    disruption_active=true

Result: The raptor propose vehicle journey vj2 because there is no disruption for this day.
This is the optimized solution.

Call-4:
-------
parameters :
    date: 20140116T101000
    disruption_active=true

Result: The raptor propose vehicle journey vj1 with departure at 10h10 and arrival at 11h10.
This is not the optimized solution but another proposition avoiding vehiclejourney with disruption (Impact-2).

Call-5:
-------
parameters :
    date: 20140118T101000
    disruption_active=true

Result: The raptor propose vehicle journey vj2 because there is no disruption active between 10h30
and 11h10 for this day. Impact-3 is active between 08h00 and 10h29 for 2014-01-18.
This is the optimized solution.
*/

using namespace navitia;
using namespace routing;
/*
namespace pt = boost::posix_time;
class Params{
public:
    std::vector<std::string> forbidden;
    ed::builder b;
    std::vector<navitia::type::AtPerturbation> perturbations;
    navitia::type::EntryPoint origin;
    navitia::type::EntryPoint destination;
    std::unique_ptr<navitia::routing::RAPTOR> raptor;
    std::unique_ptr<navitia::georef::StreetNetwork> street_network;

    void add_perturbation(boost::shared_ptr<navitia::type::Message> & message){
        navitia::type::AtPerturbation perturbation;
        perturbation.uri = message->uri;
        perturbation.object_uri = message->object_uri;
        perturbation.object_type = message->object_type;
        perturbation.application_daily_start_hour = message->application_daily_start_hour;
        perturbation.application_daily_end_hour = message->application_daily_end_hour;
        perturbation.application_period = message->application_period;
        perturbation.active_days = message->active_days;
        perturbations.push_back(perturbation);
    }

    Params():b("20140113") {
        /*
VP
Validity_pattern principale :
                        Janvier 2014
                    di lu ma me je ve sa
                       13 14 15 16 17 18
                    19 20 21 22 23 24 25
                    26 27 28 29 30 31
Impact-1
                        Janvier 2014
                    di lu ma me je ve sa
                       13 ** 15 16 17 18
                    19 20 21 22 23 24 25
                    26 27 28 29 30 31
impact-2
                        Janvier 2014
                    di lu ma me je ve sa
                       13 14 15 ** ** 18
                    19 20 21 22 23 24 25
                    26 27 28 29 30 31
impact-3
                        Janvier 2014
                    di lu ma me je ve sa
                       13 14 15 16 17 **
                    19 20 21 22 23 24 25
                    26 27 28 29 30 31
*

        b.vj("network:R", "line:A", "1111111111111", "", true, "vj1")("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
        b.vj("network:R", "line:A", "1111111111111", "", true, "vj2")("stop_area:stop1", 10 * 3600 + 30 * 60, 10 * 3600 + 30 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
        b.vj("network:S", "line:S", "1111111111111", "", true, "vj3")("stop_area:stop01", 15 * 3600 + 30 * 60, 15 * 3600 + 30 * 60)
                                                                ("stop_area:stop02", 16 * 3600 + 10 * 60 ,16 * 3600 + 10 * 60)
                                                                ("stop_area:stop03", 17 * 3600 + 10 * 60 ,17 * 3600 + 10 * 60);

        b.vj("network:PASS", "line:PASS", "11111", "", true, "vj4")("stop_area:stopP1", 22 * 3600 + 30 * 60, 22 * 3600 + 30 * 60)
                                                                   ("stop_area:stopP2", 23 * 3600 + 10 * 60 ,23 * 3600 + 10 * 60)
                                                                   ("stop_area:stopP3", 24 * 3600 + 10 * 60 ,24 * 3600 + 10 * 60);

        navitia::type::VehicleJourney* vj2 =  b.data->pt_data->vehicle_journeys[1];
        //Impact-1 on vj2 from 2014-01-14 08:32:00 à 08h40 to 2014-01-14 18:32:00 à 18h00
        boost::shared_ptr<navitia::type::Message> message;
        navitia::type::StopPoint* spt;
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
        add_perturbation(message);

        //Impact-2 on vj2 from 2014-01-16 00:00:00 à 08h00text_format to 2014-01-17 23:59:59 à 10h35
        message = boost::make_shared<navitia::type::Message>();
        message->uri = "mess2";
        message->object_uri="vj2";
        message->object_type = navitia::type::Type_e::VehicleJourney;
        message->application_period = pt::time_period(pt::time_from_string("2014-01-16 00:00:00"),
                                                      pt::time_from_string("2014-01-17 23:59:59"));
        message->publication_period = pt::time_period(pt::time_from_string("2014-01-14 00:00:00"),
                                                      pt::time_from_string("2014-01-30 23:59:00"));
        message->active_days = std::bitset<8>("11111111");
        message->application_daily_start_hour = pt::duration_from_string("08:30");
        message->application_daily_end_hour = pt::duration_from_string("10:35");
        vj2->messages.push_back(message);
        add_perturbation(message);

        //Impact-3 on vj2 from 2014-01-18 00:00:00 à 11h00 to 2014-01-18 23:59:59 à 14h00
        message = boost::make_shared<navitia::type::Message>();
        message->uri = "mess3";
        message->object_uri="vj2";
        message->object_type = navitia::type::Type_e::VehicleJourney;
        message->application_period = pt::time_period(pt::time_from_string("2014-01-18 00:00:00"),
                                                      pt::time_from_string("2014-01-18 23:59:59"));
        message->publication_period = pt::time_period(pt::time_from_string("2014-01-14 00:00:00"),
                                                      pt::time_from_string("2014-01-30 23:59:00"));
        message->active_days = std::bitset<8>("11111111");
        message->application_daily_start_hour = pt::duration_from_string("08:30");
        message->application_daily_end_hour = pt::duration_from_string("10:29");
        vj2->messages.push_back(message);
        add_perturbation(message);

        //Impact-4 on stop_area:stop02 from 2014-01-18 00:00:00 à 11h00 to 2014-01-18 23:59:59 à 18h00
        spt =  b.data->pt_data->stop_points[3];
        message = boost::make_shared<navitia::type::Message>();
        message->uri = "mess4";
        message->object_uri="stop_area:stop02";
        message->object_type = navitia::type::Type_e::StopPoint;
        message->application_period = pt::time_period(pt::time_from_string("2014-01-18 00:00:00"),
                                                      pt::time_from_string("2014-01-18 23:59:59"));
        message->publication_period = pt::time_period(pt::time_from_string("2014-01-14 00:00:00"),
                                                      pt::time_from_string("2014-01-30 23:59:00"));
        message->active_days = std::bitset<8>("11111111");
        message->application_daily_start_hour = pt::duration_from_string("00:00");
        message->application_daily_end_hour = pt::duration_from_string("23:59");
        spt->messages.push_back(message);
        add_perturbation(message);
// PASS MINUIT
        spt =  b.data->pt_data->stop_points.back();
        message = boost::make_shared<navitia::type::Message>();
        message->uri = "mess5";
        message->object_uri="stop_area:stopP3";
        message->object_type = navitia::type::Type_e::StopPoint;
        message->application_period = pt::time_period(pt::time_from_string("2014-01-16 00:00:00"),
                                                      pt::time_from_string("2014-01-16 23:59:59"));
        message->publication_period = pt::time_period(pt::time_from_string("2014-01-14 00:00:00"),
                                                      pt::time_from_string("2014-01-30 23:59:00"));
        message->active_days = std::bitset<8>("11111111");
        message->application_daily_start_hour = pt::duration_from_string("00:00");
        message->application_daily_end_hour = pt::duration_from_string("23:59");
        spt->messages.push_back(message);
        add_perturbation(message);

        ed::AtAdaptedLoader adapter;
        adapter.apply(perturbations, *b.data->pt_data);
        b.build_relations(*b.data->pt_data);
        b.generate_dummy_basis();
        b.data->pt_data->index();
        b.data->pt_data->sort();
        b.data->build_raptor();
        b.data->build_uri();

        origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stop1");
        destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stop2");
        street_network = std::make_unique<navitia::georef::StreetNetwork>(*b.data->geo_ref);
        raptor = std::make_unique<navitia::routing::RAPTOR>(*b.data);

        b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2014,01,13), boost::gregorian::date(2014,01,25));
    }
    pbnavitia::Response make_response(const std::vector<std::string> &datetimes_str, bool disruption_active) {
        std::vector<uint64_t> timestamps;
        for (const auto& str: datetimes_str) {
            timestamps.push_back(navitia::test::to_posix_timestamp(str));
        }
        return ::make_response(*raptor, origin, destination,
                               timestamps, true,
                               type::AccessibiliteParams(),
                               forbidden,
                               *street_network, disruption_active, true);
    }
};
BOOST_FIXTURE_TEST_SUITE(disruption_active, Params)

BOOST_AUTO_TEST_CASE(Test_disruption_active_false) {
    {
    pbnavitia::Response resp = make_response({"20140114T101000"}, false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj2");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);
}
{

    //vehiclejourney vj2 impacted (impact-1) from 08h40 to 18h00 for the day of travel
    //We must get vehiclejourney=vj1 in the result.
    pbnavitia::Response resp = make_response({"20140114T101000"}, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);

    //vehiclejourney vj2 not impacted for 2014-01-15
    //We must get vehiclejourney=vj2 in the result because vj2 takes less time (optimized).
    resp = make_response({"20140115T101000"}, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj2");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);


    //vehiclejourney vj2 impacted (impact-2) from 2014-01-16 00:00:00 à 08h30 to 2014-01-17 23:59:59 à 10h35
    //We must get vehiclejourney=vj1 in the result
    resp = make_response({"20140116T101000"}, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);

    //vehiclejourney vj2 impacted (impact-3) from 2014-01-18 00:00:00 à 08h30 to 2014-01-18 23:59:59 à 10h29
    //We must get vehiclejourney=vj2 in the result
    resp = make_response({"20140118T101000"}, true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj2");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);
}
{
    // Tests impacts on StopPoint
    origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stop02");
    destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stop03");
    pbnavitia::Response resp = make_response({"20140118T150000"}, true);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj3");
    BOOST_CHECK_EQUAL(journey.duration(), 3600);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140119T161000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140119T171000"));
    BOOST_CHECK_EQUAL(journey.requested_date_time(), navitia::test::to_posix_timestamp("20140118T150000"));
}
{
    // Tests impacts on StopPoint
    origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stop02");
    destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stop03");
    pbnavitia::Response resp = make_response({"20140118T150000"}, false);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj3");
    BOOST_CHECK_EQUAL(journey.duration(), 3600);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140118T161000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140118T171000"));
    BOOST_CHECK_EQUAL(journey.requested_date_time(), navitia::test::to_posix_timestamp("20140118T150000"));
}
{
    // Tests passe-minuit
    origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stopP1");
    destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stopP3");
    pbnavitia::Response resp = make_response({"20140114T210000"}, true);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj4");
    BOOST_CHECK_EQUAL(journey.duration(), 6000);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T223000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140115T001000"));
}
{
    // Tests passe-minuit
    origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stopP1");
    destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stopP3");
    pbnavitia::Response resp = make_response({"20140114T210000"}, false);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj4");
    BOOST_CHECK_EQUAL(journey.duration(), 6000);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T223000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140115T001000"));
}
{
    // Tests passe-minuit
    origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stopP1");
    destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stopP3");
    pbnavitia::Response resp = make_response({"20140115T210000"}, false);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj4");
    BOOST_CHECK_EQUAL(journey.duration(), 6000);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140115T223000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140116T001000"));
}
{
    // Tests passe-minuit
    origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stopP1");
    destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stopP3");
    pbnavitia::Response resp = make_response({"20140115T210000"}, true);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj4");
    BOOST_CHECK_EQUAL(journey.duration(), 6000);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140116T223000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140117T001000"));
}
{
    // Tests passe-minuit
    origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stopP1");
    destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stopP2");
    pbnavitia::Response resp = make_response({"20140115T210000"}, true);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj4:adapted:0");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140115T223000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140115T231000"));
}
{
    // Tests passe-minuit
    origin = navitia::type::EntryPoint(navitia::type::Type_e::StopPoint, "stop_area:stopP1");
    destination = navitia::type::EntryPoint (navitia::type::Type_e::StopPoint, "stop_area:stopP2");
    pbnavitia::Response resp = make_response({"20140115T210000"}, false);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    pbnavitia::Section section = journey.sections(0);
    pbnavitia::PtDisplayInfo displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj4");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140115T223000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140115T231000"));
}
}

BOOST_AUTO_TEST_SUITE_END()
*/
