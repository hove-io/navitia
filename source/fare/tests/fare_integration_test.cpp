#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_fares
#include <boost/test/unit_test.hpp>
#include "fare/fare.h"
#include "type/data.h"
#include "routing/raptor_api.h"

#include "routing/raptor.h"
#include "ed/build_helper.h"

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
    b.data.pt_data.index();
    b.data.build_raptor();
    b.data.build_uri();
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));

    RAPTOR raptor(b.data);

    //fare data initialization
    boost::gregorian::date start_date(boost::gregorian::from_undelimited_string("20110101"));
    boost::gregorian::date end_date(boost::gregorian::from_undelimited_string("20350101"));
    b.data.fare.fare_map["price1"].add(start_date, end_date, fare::Ticket("price1", "Ticket vj 1", 100, "125"));
    b.data.fare.fare_map["price2"].add(start_date, end_date, fare::Ticket("price2", "Ticket vj 2", 200, "175"));

    std::vector<std::string> price1 {"price1"};
    std::vector<std::string> price2 {"price2"};
    b.data.fare.od_tickets[fare::OD_key(fare::OD_key::StopArea, "stop1")][fare::OD_key(fare::OD_key::StopArea, "stop2")] = price1;
    b.data.fare.od_tickets[fare::OD_key(fare::OD_key::StopArea, "stop2")][fare::OD_key(fare::OD_key::StopArea, "stop5")] = price2;

    // if the graph is empty, the algorithm do not work, so we add a dummy transition
    // that for any start to any arrival it's free (we want the OD the take precedence)
    fare::Transition transition;
    transition.start_conditions = {};
    transition.end_conditions = {};
    transition.ticket_key = "price1";
    transition.global_condition = fare::Transition::GlobalCondition::with_changes;
    fare::State start;
    fare::State end;
    auto start_v = boost::add_vertex(start, b.data.fare.g);
    auto end_v = boost::add_vertex(end, b.data.fare.g);
    boost::add_edge(start_v, end_v, transition, b.data.fare.g);

    //call to raptor
    type::EntryPoint origin(type::Type_e::StopArea, "stop1");
    type::EntryPoint destination(type::Type_e::StopArea, "stop5");

    georef::StreetNetwork sn_worker(b.data.geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20120614T080000"}, true, type::AccessibiliteParams(), {}, sn_worker);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    const pbnavitia::Journey& journey = resp.journeys(0);

    //fare structures check
    const pbnavitia::Fare& pb_fare = journey.fare();

    BOOST_REQUIRE_EQUAL(pb_fare.ticket_id_size(), 2);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    auto section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.id(), "section_0");
    BOOST_CHECK_EQUAL(section.origin().name(), "stop1");
    BOOST_CHECK_EQUAL(section.destination().name(), "stop2");
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.id(), "section_1");//section_1 is the transfert
    section = journey.sections(2);
    BOOST_CHECK_EQUAL(section.id(), "section_2");
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
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(0)].section_id(0), "section_0");
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(1)].name(), "price2");
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(1)].cost().value(), 200);
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(1)].cost().currency(), "euro");
    BOOST_CHECK_EQUAL(ticket[pb_fare.ticket_id(1)].section_id(0), "section_2");

}

BOOST_AUTO_TEST_CASE(test_protobuff_no_data) {
    ed::builder b("20120614");
    b.vj("RATP", "A", "11111111", "", true, "")("stop1", 8000, 8050)("stop2", 8100, 8150)("stop3", 8200, 8250);
    b.vj("RATP", "B", "11111111", "", true, "")("stop4", 8000, 8050)("stop2", 8300, 8350)("stop5", 8400, 8450);
    b.generate_dummy_basis();
    b.data.pt_data.index();
    b.data.build_raptor();
    b.data.build_uri();
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));

    RAPTOR raptor(b.data);

    //call to raptor
    type::EntryPoint origin(type::Type_e::StopArea, "stop1");
    type::EntryPoint destination(type::Type_e::StopArea, "stop5");

    georef::StreetNetwork sn_worker(b.data.geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20120614T080000"}, true, type::AccessibiliteParams(), {}, sn_worker);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    const pbnavitia::Journey& journey = resp.journeys(0);

    //fare structures check
    const pbnavitia::Fare& pb_fare = journey.fare();

    BOOST_REQUIRE_EQUAL(pb_fare.ticket_id_size(), 0);
    BOOST_CHECK_EQUAL(pb_fare.found(), false);

    BOOST_REQUIRE_EQUAL(resp.tickets_size(), 0);
}
