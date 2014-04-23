#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "routing/raptor_api.h"
#include "ed/build_helper.h"
#include "routing_api_test_data.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace nr = navitia::routing;
namespace bt = boost::posix_time;

void dump_response(pbnavitia::Response resp, std::string test_name, bool debug_info = false) {
    if (! debug_info)
        return;
    pbnavitia::Journey journey = resp.journeys(0);
    std::cout << test_name << ": " << std::endl;
    for (int idx_section = 0; idx_section < journey.sections().size(); ++idx_section) {
        auto& section = journey.sections(idx_section);
        std::cout << "section " << (int)(section.type()) << std::endl
                     << " -- coordinates :" << std::endl;
        for (int i = 0; i < section.street_network().coordinates_size(); ++i)
            std::cout << "coord: " << section.street_network().coordinates(i).lon() / navitia::type::GeographicalCoord::N_M_TO_DEG
                      << ", " << section.street_network().coordinates(i).lat() / navitia::type::GeographicalCoord::N_M_TO_DEG
                      << std::endl;

        std::cout << "dump item : " << std::endl;
        for (int i = 0; i < section.street_network().path_items_size(); ++i)
            std::cout << "- " << section.street_network().path_items(i).name()
                      << " with " << section.street_network().path_items(i).length()
                      << "m | " << section.street_network().path_items(i).duration() << "s"
                      << std::endl;
    }


}

BOOST_AUTO_TEST_CASE(simple_journey) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("A")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
    navitia::type::Data data;
    b.generate_dummy_basis();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");

    navitia::georef::StreetNetwork sn_worker(*data.geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20120614T021000"}, true, navitia::type::AccessibiliteParams()/*false*/, forbidden, sn_worker, false);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_area:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_area:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), "20120614T081100");
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), "20120614T082000");
}

BOOST_AUTO_TEST_CASE(journey_array){
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("A")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
    b.vj("A")("stop_area:stop1", 9*3600 +10*60, 9*3600 + 11 * 60)("stop_area:stop2",  9*3600 + 20 * 60 ,9*3600 + 21*60);
    navitia::type::Data data;
    b.generate_dummy_basis();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->geo_ref->init();
    b.data->build_proximity_list();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");

    navitia::georef::StreetNetwork sn_worker(*data.geo_ref);

    // On met les horaires dans le desordre pour voir s'ils sont bien triÃ© comme attendu
    std::vector<std::string> datetimes({"20120614T080000", "20120614T090000"});
    pbnavitia::Response resp = nr::make_response(raptor, origin, destination, datetimes, true, navitia::type::AccessibiliteParams()/*false*/, forbidden, sn_worker, false);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 2);

    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_area:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_area:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), "20120614T081100");
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), "20120614T082000");

    journey = resp.journeys(1);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    st1 = section.stop_date_times(0);
    st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_area:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_area:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), "20120614T091100");
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), "20120614T092000");
}


template <typename speed_provider_trait>
struct streetnetworkmode_fixture : public routing_api_data<speed_provider_trait> {

    pbnavitia::Response make_response() {
        navitia::georef::StreetNetwork sn_worker(*(this->b.data->geo_ref));
        nr::RAPTOR raptor(*(this->b.data));
        return nr::make_response(raptor, this->origin, this->destination, this->datetimes, true, navitia::type::AccessibiliteParams(), this->forbidden, sn_worker, false);
    }
};

// Walking
BOOST_FIXTURE_TEST_CASE(walking_test, streetnetworkmode_fixture<test_speed_provider>) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.max_duration = bt::seconds(15*60);//bt::seconds(200 / get_default_speed()[navitia::type::Mode_e::Walking]);
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.max_duration = bt::seconds(15*60);//bt::seconds(50 / get_default_speed()[navitia::type::Mode_e::Walking]);

    pbnavitia::Response resp = make_response();

    dump_response(resp, "walking");

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 2); //1 direct path by date and 1 path with bus
    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20120614T080000");
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20120614T080510");

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);

    BOOST_CHECK_EQUAL(section.street_network().coordinates_size(), 4);
    BOOST_CHECK(! section.id().empty());
    BOOST_CHECK_EQUAL(section.street_network().duration(), 310);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 3);
    BOOST_CHECK_EQUAL(section.street_network().path_items(0).name(), "rue bs");
    BOOST_CHECK_EQUAL(section.street_network().path_items(0).duration(), 20);
    BOOST_CHECK_EQUAL(section.street_network().path_items(1).name(), "rue ab");
    BOOST_CHECK_EQUAL(section.street_network().path_items(1).duration(), 200);
    BOOST_CHECK_EQUAL(section.street_network().path_items(2).name(), "rue ar");
    BOOST_CHECK_EQUAL(section.street_network().path_items(2).duration(), 90);
}

//biking
BOOST_FIXTURE_TEST_CASE(biking, streetnetworkmode_fixture<test_speed_provider>) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    origin.streetnetwork_params.max_duration = bt::minutes(15);
    origin.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    destination.streetnetwork_params.max_duration = bt::minutes(15);
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 2); //1 direct path by date and 1 path with bus
    auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    auto section = journey.sections(0);

    dump_response(resp, "biking");

    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ag");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 8);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_CHECK_EQUAL(section.street_network().duration(), 130); //it's the biking distance / biking speed (but there can be rounding pb)
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 7);

    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(pathitem.direction(), 0); //first direction is always 0°
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb"); //after that we went strait so still 0°
    BOOST_CHECK_EQUAL(pathitem.direction(), 0);
    pathitem = section.street_network().path_items(2);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    BOOST_CHECK_EQUAL(pathitem.direction(), 90); //then we turned right
    pathitem = section.street_network().path_items(3);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    BOOST_CHECK_EQUAL(pathitem.direction(), 90); //then we turned right
    pathitem = section.street_network().path_items(4);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    BOOST_CHECK_EQUAL(pathitem.direction(), -90); //then we turned left
    pathitem = section.street_network().path_items(5);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    pathitem = section.street_network().path_items(6);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
}

// Biking with a different speed
BOOST_FIXTURE_TEST_CASE(biking_with_different_speed, streetnetworkmode_fixture<test_speed_provider>) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    origin.streetnetwork_params.max_duration = bt::pos_infin;
    origin.streetnetwork_params.speed_factor = .5;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    destination.streetnetwork_params.max_duration = bt::pos_infin;
    destination.streetnetwork_params.speed_factor = .5;

    auto resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 2); //1 direct path by date and 1 path with bus
    auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    auto section = journey.sections(0);

    dump_response(resp, "biking with different speed");

    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ag");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 8);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_CHECK_EQUAL(section.street_network().duration(), 130*2);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 7);

    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    //NOTE: we cannot check length here, it has to be checked in a separate test (below), since the used faked speed in the test
    BOOST_CHECK_CLOSE(pathitem.duration(), S.distance_to(B) / (get_default_speed()[nt::Mode_e::Bike] * .5), 1);
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    pathitem = section.street_network().path_items(2);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    pathitem = section.street_network().path_items(3);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    pathitem = section.street_network().path_items(4);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    pathitem = section.street_network().path_items(5);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    pathitem = section.street_network().path_items(6);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    BOOST_CHECK_CLOSE(pathitem.duration(), A.distance_to(G) / (get_default_speed()[nt::Mode_e::Bike] * .5), 1);
}

// Car
BOOST_FIXTURE_TEST_CASE(car, streetnetworkmode_fixture<test_speed_provider>) {
    auto total_distance = S.distance_to(B) + B.distance_to(C) + C.distance_to(F) + F.distance_to(E) + E.distance_to(A) + 1;

    origin.streetnetwork_params.mode = navitia::type::Mode_e::Car;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Car];
    origin.streetnetwork_params.max_duration = bt::seconds(total_distance / get_default_speed()[navitia::type::Mode_e::Car]);
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = navitia::type::Mode_e::Car;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Car];
    destination.streetnetwork_params.max_duration = bt::seconds(total_distance / get_default_speed()[navitia::type::Mode_e::Car]);
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1); //1 direct path by date only because the car is faster than the bus
    auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    auto section = journey.sections(0);

    dump_response(resp, "car");

    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ef");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 5);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Car);
    BOOST_CHECK_EQUAL(section.street_network().duration(), 18); // (20+50+20)/5
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 4);
    //since R is not accessible by car, we project R in the closest edge in the car graph
    //this edge is F-C, so this is the end of the journey (the rest of it is as the crow flies)
    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue cb");
    pathitem = section.street_network().path_items(2);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue fc");
    pathitem = section.street_network().path_items(3);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ef");
    BOOST_CHECK_EQUAL(pathitem.length(), 0);
}

// BSS test
BOOST_FIXTURE_TEST_CASE(bss_test, streetnetworkmode_fixture<test_speed_provider>) {

    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bss;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bss];
    origin.streetnetwork_params.max_duration = bt::minutes(15);
    origin.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bss;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bss];
    destination.streetnetwork_params.max_duration = bt::minutes(15);
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    auto journey = resp.journeys(0);
    dump_response(resp, "bss");

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 5);
    //we should have 5 sections
    //1 walk, 1 boarding, 1 bike, 1 landing, and 1 final walking section
    auto section = journey.sections(0);

    //walk
    BOOST_CHECK(! section.id().empty());
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);

    //getting the bss bike
    section = journey.sections(1);
    BOOST_CHECK(! section.id().empty());
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::BSS_RENT);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    BOOST_CHECK_EQUAL(pathitem.duration(), bike_sharing_pickup.total_seconds());
    BOOST_CHECK_EQUAL(pathitem.length(), 0);

    //bike
    section = journey.sections(2);
    BOOST_CHECK(! section.id().empty());
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue kb");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue gh");
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 5);
    int cpt_item(0);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    BOOST_CHECK_EQUAL(pathitem.duration(), 45);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    BOOST_CHECK_EQUAL(pathitem.duration(), 30);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    BOOST_CHECK_EQUAL(pathitem.duration(), 10);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    BOOST_CHECK_EQUAL(pathitem.duration(), 15);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    BOOST_CHECK_EQUAL(pathitem.duration(), 10);

    //putting back the bss bike
    section = journey.sections(3);
    BOOST_CHECK(! section.id().empty());
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::BSS_PUT_BACK);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.duration(), bike_sharing_return.total_seconds());
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    BOOST_CHECK_EQUAL(pathitem.length(), 0);

    //walking
    section = journey.sections(4);
    BOOST_CHECK(! section.id().empty());
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue ag");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ar");
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);

    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 2);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ar");
//    BOOST_CHECK_EQUAL(pathitem.duration(), 0); //projection
    BOOST_CHECK_CLOSE(pathitem.duration(), A.distance_to(R) / (get_default_speed()[nt::Mode_e::Walking]), 1);
}

/**
  * With the test_default_speed it's difficult to test the length of the journeys, so here is a new fixture with classical default speed
 */

BOOST_FIXTURE_TEST_CASE(biking_length_test, streetnetworkmode_fixture<normal_speed_provider>) {
    auto mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.mode = mode;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.max_duration = bt::pos_infin;
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = mode;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.max_duration = bt::pos_infin;

    pbnavitia::Response resp = make_response();

    BOOST_REQUIRE(resp.journeys_size() != 0);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE(journey.sections_size() );
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);

    dump_response(resp, "biking length test");

    std::vector<pbnavitia::PathItem> path_items;
    for (int s = 0; s < journey.sections_size() ; s++) {
        for (int i = 0; i < journey.sections(s).street_network().path_items_size(); ++i) {
            path_items.push_back(journey.sections(s).street_network().path_items(i));
        }
    }

    BOOST_REQUIRE_EQUAL(path_items.size(), 7);
    int cpt(0);
    auto pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(pathitem.duration(), to_duration(B.distance_to(S), navitia::type::Mode_e::Bike).total_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), B.distance_to(S), 1);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    BOOST_CHECK_EQUAL(pathitem.duration(), to_duration(K.distance_to(B), navitia::type::Mode_e::Bike).total_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), B.distance_to(K), 1);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    BOOST_CHECK_EQUAL(pathitem.duration(), to_duration(K.distance_to(J), navitia::type::Mode_e::Bike).total_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), J.distance_to(K), 1);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    BOOST_CHECK_EQUAL(pathitem.duration(), to_duration(I.distance_to(J), navitia::type::Mode_e::Bike).total_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), I.distance_to(J), 1);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    BOOST_CHECK_EQUAL(pathitem.duration(), to_duration(I.distance_to(H), navitia::type::Mode_e::Bike).total_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), I.distance_to(H), 1);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    BOOST_CHECK_EQUAL(pathitem.duration(), to_duration(G.distance_to(H), navitia::type::Mode_e::Bike).total_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), G.distance_to(H), 1);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    BOOST_CHECK_EQUAL(pathitem.duration(), to_duration(distance_ag, navitia::type::Mode_e::Bike).total_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), distance_ag, 1);

    //we check the total
    int total_dur(0);
    double total_distance(0);
    for (const auto& item : path_items) {
        total_dur += item.duration();
        total_distance += item.length();
    }
    BOOST_CHECK_CLOSE(section.length(), total_distance, 1);
    BOOST_CHECK_EQUAL(section.duration(), total_dur);
}

/*
 *     A(0,0)             B(0,0.01)                       C(0,0.020)           D(0,0.03)
 *       x-------------------x                            x-----------------x
 *           SP1______ SP2                                   SP3_______SP4
 *          (0,0.005)     (0,0.007)                        (0,021)   (0,0.025)
 *
 *
 *       Un vj de SP2 à SP3
 */


/*BOOST_AUTO_TEST_CASE(map_depart_arrivee) {
    namespace ng = navitia::georef;
    int AA = 0;
    int BB = 1;
    int CC = 2;
    int DD = 3;
    ed::builder b("20120614");

    navitia::type::GeographicalCoord A(0, 0, false);
    navitia::type::GeographicalCoord B(0, 0.1, false);
    navitia::type::GeographicalCoord C(0, 0.02, false);
    navitia::type::GeographicalCoord D(0, 0.03, false);
    navitia::type::GeographicalCoord SP1(0, 0.005, false);
    navitia::type::GeographicalCoord SP2(0, 0.007, false);
    navitia::type::GeographicalCoord SP3(0, 0.021, false);
    navitia::type::GeographicalCoord SP4(0, 0.025, false);

    b.data->geo_ref->init_offset(0);
    navitia::georef::Way* way;

    way = new navitia::georef::Way();
    way->name = "rue ab"; // A->B
    way->idx = 0;
    way->way_type = "rue";
    b.data->geo_ref->ways.push_back(way);

    way = new navitia::georef::Way();
    way->name = "rue cd"; // C->D
    way->idx = 1;
    way->way_type = "rue";
    b.data->geo_ref->ways.push_back(way);


    boost::add_edge(AA, BB, navitia::georef::Edge(0,10), b.data->geo_ref->graph);
    boost::add_edge(BB, AA, navitia::georef::Edge(0,10), b.data->geo_ref->graph);
    b.data->geo_ref->ways[0]->edges.push_back(std::make_pair(AA, BB));
    b.data->geo_ref->ways[0]->edges.push_back(std::make_pair(BB, AA));

    boost::add_edge(CC, DD, navitia::georef::Edge(0,50), b.data->geo_ref->graph);
    boost::add_edge(DD, CC, navitia::georef::Edge(0,50), b.data->geo_ref->graph);
    b.data->geo_ref->ways[0]->edges.push_back(std::make_pair(AA, BB));
    b.data->geo_ref->ways[0]->edges.push_back(std::make_pair(BB, AA));

    b.sa("stop_area:stop1", SP1.lon(), SP2.lat());
    b.sa("stop_area:stop2", SP2.lon(), SP2.lat());
    b.sa("stop_area:stop3", SP3.lon(), SP2.lat());
    b.sa("stop_area:stop4", SP4.lon(), SP2.lat());

    b.connection("stop_point:stop_area:stop1", "stop_point:stop_area:stop2", 120);
    b.connection("stop_point:stop_area:stop2", "stop_point:stop_area:stop1", 120);
    b.connection("stop_point:stop_area:stop3", "stop_point:stop_area:stop4", 120);
    b.connection("stop_point:stop_area:stop4", "stop_point:stop_area:stop3", 120);
    b.vj("A")("stop_point:stop_area:stop2", 8*3600 +10*60, 8*3600 + 11 * 60)
            ("stop_point:stop_area:stop3", 8*3600 + 20 * 60 ,8*3600 + 21*60);
    b.generate_dummy_basis();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->build_proximity_list();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    navitia::georef::StreetNetwork sn_worker(b.data->geo_ref);

    RAPTOR raptor(b.data);

    std::string origin_uri = "stop_area:stop1";
    navitia::type::Type_e origin_type = b.data->get_type_of_id(origin_uri);
    navitia::type::EntryPoint origin(origin_type, origin_uri);
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.distance = 15;
    std::string destination_uri = "stop_area:stop4";
    navitia::type::Type_e destination_type = b.data->get_type_of_id(destination_uri);
    navitia::type::EntryPoint destination(destination_type, destination_uri);
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.distance = 5;
    std::vector<std::string> datetimes({"20120614T080000"});
    std::vector<std::string> forbidden;

    pbnavitia::Response resp = make_response(raptor, origin, destination, datetimes, true, 1.38, 1000, navitia::type::AccessibiliteParams(), forbidden, sn_worker);

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    BOOST_REQUIRE_EQUAL(journey.sections(0).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_REQUIRE_EQUAL(journey.sections(1).type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_REQUIRE_EQUAL(journey.sections(2).type(), pbnavitia::SectionType::STREET_NETWORK);

}*/
