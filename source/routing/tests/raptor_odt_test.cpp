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
#define BOOST_TEST_MODULE raptor_odt_test
#include <boost/test/unit_test.hpp>
#include "routing/raptor_api.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"



struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

class Params{
public:
    std::vector<std::string> forbidden;
    ed::builder b;
    navitia::type::EntryPoint origin;
    navitia::type::EntryPoint destination;
    std::unique_ptr<navitia::routing::RAPTOR> raptor;
    std::unique_ptr<navitia::georef::StreetNetwork> street_network;
    pbnavitia::Response resp;
    pbnavitia::Journey journey;
    pbnavitia::Section section;
    pbnavitia::PtDisplayInfo displ;
    navitia::type::VehicleJourney* vj1;
    navitia::type::VehicleJourney* vj2;

    Params():b("20140113") {

        b.vj("network:R", "line:A", "1111111111111", "", true, "vj1")("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
        b.vj("network:R", "line:C", "1111111111111", "", true, "vj2")("stop_area:stop1", 10 * 3600 + 30 * 60, 10 * 3600 + 30 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);

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
        vj1 = b.data->pt_data->vehicle_journeys_map["vj1"];
        vj2 = b.data->pt_data->vehicle_journeys_map["vj2"];

    }
    pbnavitia::Response make_response(bool allow_odt) {
        return navitia::routing::make_response(*raptor, origin, destination,
                               {navitia::test::to_posix_timestamp("20140114T101000")}, true,
                               navitia::type::AccessibiliteParams(),
                               forbidden,
                               *street_network, false, allow_odt);
    }
    void unset_odt_jp(){
        for(navitia::type::JourneyPattern* jp : b.data->pt_data->journey_patterns){
            jp->odt_level = navitia::type::OdtLevel_e::none;
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(allow_odt, Params)


/*
   Testing the behavior if :
    Not ODT
*/
BOOST_AUTO_TEST_CASE(test1){
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500")); //date are now posix timestamp
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && not estimated_date_time
   VJ2 : Not ODT, Not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test2){
    vj1->stop_time_list.front()->set_odt(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time
   VJ2 : Not ODT, Not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test3){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time, last stoptime is odt && not estimated_date_time
   VJ2 : Not ODT, Not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test4){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    vj1->stop_time_list.back()->set_odt(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time, last stoptime is odt && estimated_date_time
   VJ2 : Not ODT, Not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test5){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    vj1->stop_time_list.back()->set_odt(true);
    vj1->stop_time_list.back()->set_date_time_estimated(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj2");
    BOOST_CHECK_EQUAL(journey.duration(), 2400);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T103000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    vj1->stop_time_list.front()->set_odt(false);
    vj1->stop_time_list.front()->set_date_time_estimated(false);
    vj1->stop_time_list.back()->set_odt(false);
    vj1->stop_time_list.back()->set_date_time_estimated(false);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : Not ODT, Not estimated_date_time
   VJ2 : first stoptime is odt && not estimated_date_time, last stoptime is not odt && not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test6){
    vj2->stop_time_list.front()->set_odt(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : Not ODT, Not estimated_date_time
   VJ2 : first stoptime is odt && estimated_date_time, last stoptime is not odt && not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test7){
    vj2->stop_time_list.front()->set_odt(true);
    vj2->stop_time_list.front()->set_date_time_estimated(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : Not ODT, Not estimated_date_time
   VJ2 : first stoptime is odt && estimated_date_time, last stoptime is odt && not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test8){
    vj2->stop_time_list.front()->set_odt(true);
    vj2->stop_time_list.front()->set_date_time_estimated(true);
    vj2->stop_time_list.back()->set_odt(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : Not ODT, Not estimated_date_time
   VJ2 : first stoptime is odt && estimated_date_time, last stoptime is odt && estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test9){
    vj2->stop_time_list.front()->set_odt(true);
    vj2->stop_time_list.front()->set_date_time_estimated(true);
    vj2->stop_time_list.back()->set_odt(true);
    vj2->stop_time_list.back()->set_date_time_estimated(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && not estimated_date_time, last stoptime is not odt && not estimated_date_time
   VJ2 : first stoptime is odt && not estimated_date_time, last stoptime is not odt && not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test10){
    vj1->stop_time_list.front()->set_odt(true);
    vj2->stop_time_list.front()->set_odt(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time, last stoptime is not odt && not estimated_date_time
   VJ2 : first stoptime is odt && estimated_date_time, last stoptime is not odt && not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test11){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    vj2->stop_time_list.front()->set_odt(true);
    vj2->stop_time_list.front()->set_date_time_estimated(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time, last stoptime is odt && not estimated_date_time
   VJ2 : first stoptime is odt && estimated_date_time, last stoptime is odt && not estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test12){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    vj1->stop_time_list.back()->set_odt(true);
    vj2->stop_time_list.front()->set_odt(true);
    vj2->stop_time_list.front()->set_date_time_estimated(true);
    vj2->stop_time_list.back()->set_odt(true);
    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time, last stoptime is odt && estimated_date_time
   VJ2 : first stoptime is odt && estimated_date_time, last stoptime is odt && estimated_date_time
*/
BOOST_AUTO_TEST_CASE(test13){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    vj1->stop_time_list.back()->set_odt(true);
    vj1->stop_time_list.back()->set_date_time_estimated(true);

    vj2->stop_time_list.front()->set_odt(true);
    vj2->stop_time_list.front()->set_date_time_estimated(true);
    vj2->stop_time_list.back()->set_odt(true);
    vj2->stop_time_list.back()->set_date_time_estimated(true);

    b.data->build_odt();
    resp = make_response(true);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);
    section = journey.sections(0);
    displ = section.pt_display_informations();
    BOOST_CHECK_EQUAL(displ.uris().vehicle_journey(), "vj1");
    BOOST_CHECK_EQUAL(journey.duration(), 3300);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20140114T101500"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20140114T111000"));

    resp = make_response(false);
    BOOST_CHECK_EQUAL(resp.response_type(), pbnavitia::NO_SOLUTION);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && not estimated_date_time, last stoptime is not odt && not estimated_date_time
   VJ2 : not odt
*/

BOOST_AUTO_TEST_CASE(test14){
    vj1->stop_time_list.front()->set_odt(true);
    b.data->build_odt();
    BOOST_CHECK_EQUAL((vj1->get_odt_level() == navitia::type::OdtLevel_e::none), true);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time, last stoptime is not odt && not estimated_date_time
   VJ2 : not odt
*/

BOOST_AUTO_TEST_CASE(test15){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    b.data->build_odt();
    BOOST_CHECK_EQUAL((vj1->get_odt_level() == navitia::type::OdtLevel_e::mixt), true);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time, last stoptime is odt && not estimated_date_time
   VJ2 : not odt
*/

BOOST_AUTO_TEST_CASE(test16){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    vj1->stop_time_list.back()->set_odt(true);
    b.data->build_odt();
    BOOST_CHECK_EQUAL((vj1->get_odt_level() == navitia::type::OdtLevel_e::mixt), true);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && estimated_date_time, last stoptime is odt && estimated_date_time
   VJ2 : not odt
*/

BOOST_AUTO_TEST_CASE(test17){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    vj1->stop_time_list.back()->set_odt(true);
    vj1->stop_time_list.back()->set_date_time_estimated(true);
    b.data->build_odt();
    BOOST_CHECK_EQUAL((vj1->get_odt_level() == navitia::type::OdtLevel_e::zonal), true);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is not odt && not estimated_date_time, last stoptime is odt && not estimated_date_time
   VJ2 : not odt
*/

BOOST_AUTO_TEST_CASE(test18){
    vj1->stop_time_list.back()->set_odt(true);
    b.data->build_odt();
    BOOST_CHECK_EQUAL((vj1->get_odt_level() == navitia::type::OdtLevel_e::none), true);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is not odt && not estimated_date_time, last stoptime is odt && estimated_date_time
   VJ2 : not odt
*/

BOOST_AUTO_TEST_CASE(test19){
    vj1->stop_time_list.back()->set_odt(true);
    vj1->stop_time_list.back()->set_date_time_estimated(true);
    b.data->build_odt();
    BOOST_CHECK_EQUAL((vj1->get_odt_level() == navitia::type::OdtLevel_e::mixt), true);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is odt && not estimated_date_time, last stoptime is odt && estimated_date_time
   VJ2 : not odt
*/

BOOST_AUTO_TEST_CASE(test20){
    vj1->stop_time_list.front()->set_odt(true);
    vj1->stop_time_list.back()->set_odt(true);
    vj1->stop_time_list.back()->set_date_time_estimated(true);
    b.data->build_odt();
    BOOST_CHECK_EQUAL((vj1->get_odt_level() == navitia::type::OdtLevel_e::mixt), true);
    unset_odt_jp();
}

/*
   Testing the behavior if :
   VJ1 : first stoptime is not odt && estimated_date_time, last stoptime is odt && estimated_date_time
   VJ2 : not odt
*/

BOOST_AUTO_TEST_CASE(test21){
    vj1->stop_time_list.front()->set_date_time_estimated(true);
    vj1->stop_time_list.back()->set_odt(true);
    vj1->stop_time_list.back()->set_date_time_estimated(true);
    b.data->build_odt();
    BOOST_CHECK_EQUAL((vj1->get_odt_level() == navitia::type::OdtLevel_e::mixt), true);
    unset_odt_jp();
    std::cout<<std::endl<<b.data->pt_data->routes.front()->uri<<std::endl;
    std::cout<<std::endl<<b.data->pt_data->routes.back()->uri<<std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
