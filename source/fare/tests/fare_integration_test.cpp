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
#define BOOST_TEST_MODULE test_fares
#include <boost/test/unit_test.hpp>
#include "fare/fare.h"
#include "type/data.h"
#include "routing/raptor_api.h"

#include "routing/raptor.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

using namespace navitia;
using namespace routing;

BOOST_AUTO_TEST_CASE(test_protobuff) {
    ed::builder b("20120614");
    b.vj("RATP", "A", "11111111", "", true, "")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("RATP", "B", "11111111", "", true, "")("stop4", 8000, 8050)("stop2", 8300, 8350)("stop5", 8400, 8450);
    b.generate_dummy_basis();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));

    RAPTOR raptor(*b.data);

    //fare data initialization
    boost::gregorian::date start_date(boost::gregorian::from_undelimited_string("20110101"));
    boost::gregorian::date end_date(boost::gregorian::from_undelimited_string("20350101"));
    b.data->fare->fare_map["price1"].add(start_date, end_date, fare::Ticket("price1", "Ticket vj 1", 100, "125"));
    b.data->fare->fare_map["price2"].add(start_date, end_date, fare::Ticket("price2", "Ticket vj 2", 200, "175"));

    // dummy transition
    fare::Transition transitionA;
    fare::State endA;
    endA.line = "A";
    transitionA.ticket_key = "price1";
    auto endA_v = boost::add_vertex(endA, b.data->fare->g);
    boost::add_edge(b.data->fare->begin_v, endA_v, transitionA, b.data->fare->g);

    fare::Transition transitionB;
    fare::State endB;
    endB.line = "B";
    transitionB.ticket_key = "price2";
    auto endB_v = boost::add_vertex(endB, b.data->fare->g);
    boost::add_edge(b.data->fare->begin_v, endB_v, transitionB, b.data->fare->g);

    //call to raptor
    type::EntryPoint origin(type::Type_e::StopArea, "stop1");
    type::EntryPoint destination(type::Type_e::StopArea, "stop5");

    georef::StreetNetwork sn_worker(*b.data->geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {test::to_posix_timestamp("20120614T080000")},
                                             true, type::AccessibiliteParams(), {}, sn_worker, true, true);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    const pbnavitia::Journey& journey = resp.journeys(0);

    //fare structures check
    const pbnavitia::Fare& pb_fare = journey.fare();

    BOOST_REQUIRE_EQUAL(pb_fare.ticket_id_size(), 2);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 5);
    // The first section is the crowfly
    auto section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.id(), "section_1");
    BOOST_CHECK_EQUAL(section.origin().name(), "stop1");
    BOOST_CHECK_EQUAL(section.destination().name(), "stop2");
    section = journey.sections(2);
    BOOST_CHECK_EQUAL(section.id(), "section_2");//section_1 is the transfer
    section = journey.sections(3);
    BOOST_CHECK_EQUAL(section.id(), "section_3");
    BOOST_CHECK_EQUAL(section.origin().name(), "stop2");
    BOOST_CHECK_EQUAL(section.destination().name(), "stop5");

    BOOST_REQUIRE_EQUAL(resp.tickets_size(), 2);
    std::map<std::string, pbnavitia::Ticket> ticket;
    for (int i = 0; i < resp.tickets_size() ; ++i) {
        auto t = resp.tickets(i);
        ticket[t.id()] = t;
    }
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(0)].name(), "price1");
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(0)].cost().value(), 100);
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(0)].cost().currency(), "euro");
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(0)].section_id_size(), 1);
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(0)].section_id(0), "section_1");
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(1)].name(), "price2");
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(1)].cost().value(), 200);
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(1)].cost().currency(), "euro");
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(1)].section_id(0), "section_3");

}

BOOST_AUTO_TEST_CASE(test_protobuff_no_data) {
    ed::builder b("20120614");
    b.vj("RATP", "A", "11111111", "", true, "")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("RATP", "B", "11111111", "", true, "")("stop4", 8000, 8050)("stop2", 8300, 8350)("stop5", 8400, 8450);
    b.generate_dummy_basis();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));

    RAPTOR raptor(*b.data);

    //call to raptor
    type::EntryPoint origin(type::Type_e::StopArea, "stop1");
    type::EntryPoint destination(type::Type_e::StopArea, "stop5");

    georef::StreetNetwork sn_worker(*b.data->geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {test::to_posix_timestamp("20120614T080000")},
                                             true, type::AccessibiliteParams(), {}, sn_worker, true, true);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    const pbnavitia::Journey& journey = resp.journeys(0);

    //fare structures check
    const pbnavitia::Fare& pb_fare = journey.fare();

    BOOST_REQUIRE_EQUAL(pb_fare.ticket_id_size(), 1);
    BOOST_REQUIRE_EQUAL(pb_fare.ticket_id(0), "unknown_ticket");
    BOOST_CHECK_EQUAL(pb_fare.found(), false);

    BOOST_REQUIRE_EQUAL(resp.tickets_size(), 1);
    BOOST_REQUIRE_EQUAL(resp.tickets(0).id(), "unknown_ticket");
}
