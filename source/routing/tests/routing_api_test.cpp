#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "routing/raptor_api.h"
#include "ed/build_helper.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

//to ease test we use easier speed
const navitia::flat_enum_map<nt::Mode_e, float> test_default_speed {
                                                    {{
                                                        1, //nt::Mode_e::Walking
                                                        2, //nt::Mode_e::Bike
                                                        5, //nt::Mode_e::Car
                                                        2 //nt::Mode_e::Vls
                                                    }}
                                                    };

using namespace navitia;
using namespace routing;
using namespace boost::posix_time;

void dump_response(pbnavitia::Response resp, std::string test_name) {
    bool debug_info = false;
    if (! debug_info)
        return;
    pbnavitia::Journey journey = resp.journeys(0);
    std::cout << test_name << ": " << std::endl;
    for (int idx_section = 0; idx_section < journey.sections().size(); ++idx_section) {
        auto& section = journey.sections(idx_section);
        std::cout << "section " << (int)(section.type()) << std::endl
                     << " -- coordinates :" << std::endl;
        for (int i = 0; i < section.street_network().coordinates_size(); ++i)
            std::cout << "coord: " << section.street_network().coordinates(i).lon() / type::GeographicalCoord::M_TO_DEG
                      << ", " << section.street_network().coordinates(i).lat() / type::GeographicalCoord::M_TO_DEG
                      <<std::endl;

        std::cout << "dump item : " << std::endl;
        for (int i = 0; i < section.street_network().path_items_size(); ++i)
            std::cout << "- " << section.street_network().path_items(i).name()
                      << " with " << section.street_network().path_items(i).length()
                      << " | " << section.street_network().path_items(i).duration() << "s"
                      <<std::endl;
    }


}

BOOST_AUTO_TEST_CASE(simple_journey) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("A")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
    type::Data data;
    b.generate_dummy_basis();
    b.data.pt_data.index();
    b.data.build_raptor();
    b.data.build_uri();
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    RAPTOR raptor(b.data);

    type::Type_e origin_type = b.data.get_type_of_id("stop_area:stop1");
    type::Type_e destination_type = b.data.get_type_of_id("stop_area:stop2");
    type::EntryPoint origin(origin_type, "stop_area:stop1");
    type::EntryPoint destination(destination_type, "stop_area:stop2");

    streetnetwork::StreetNetwork sn_worker(data.geo_ref);
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20120614T021000"}, true, type::AccessibiliteParams()/*false*/, forbidden, sn_worker);

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
    type::Data data;
    b.generate_dummy_basis();
    b.data.pt_data.index();
    b.data.build_raptor();
    b.data.build_uri();
    b.data.geo_ref.init();
    b.data.build_proximity_list();
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    RAPTOR raptor(b.data);

    type::Type_e origin_type = b.data.get_type_of_id("stop_area:stop1");
    type::Type_e destination_type = b.data.get_type_of_id("stop_area:stop2");
    type::EntryPoint origin(origin_type, "stop_area:stop1");
    type::EntryPoint destination(destination_type, "stop_area:stop2");

    streetnetwork::StreetNetwork sn_worker(data.geo_ref);

    // On met les horaires dans le desordre pour voir s'ils sont bien triÃ© comme attendu
    std::vector<std::string> datetimes({"20120614T080000", "20120614T090000"});
    pbnavitia::Response resp = make_response(raptor, origin, destination, datetimes, true, type::AccessibiliteParams()/*false*/, forbidden, sn_worker);

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

void add_edges(int edge_idx, georef::GeoRef& geo_ref, int idx_from, int idx_to, double dist, type::Mode_e mode) {
    boost::add_edge(idx_from + geo_ref.offsets[mode],
                    idx_to + geo_ref.offsets[mode],
                    ng::Edge(edge_idx, seconds(std::round(dist / test_default_speed[mode]))),
                    geo_ref.graph);
    boost::add_edge(idx_to + geo_ref.offsets[mode],
                    idx_from + geo_ref.offsets[mode],
                    ng::Edge(edge_idx, seconds(std::round(dist / test_default_speed[mode]))),
                    geo_ref.graph);
}
void add_edges(int edge_idx, georef::GeoRef& geo_ref, int idx_from, int idx_to, const type::GeographicalCoord& a, const type::GeographicalCoord& b, type::Mode_e mode) {
    add_edges(edge_idx, geo_ref, idx_from, idx_to, a.distance_to(b), mode);
}

const bt::time_duration bike_sharing_pickup = seconds(30);
const bt::time_duration bike_sharing_return = seconds(45);

void add_bike_sharing_edge(int edge_idx, georef::GeoRef& geo_ref, int idx_from, int idx_to) {
    boost::add_edge(idx_from + geo_ref.offsets[type::Mode_e::Walking],
                    idx_to + geo_ref.offsets[type::Mode_e::Bike],
                    ng::Edge(edge_idx, bike_sharing_pickup),
                    geo_ref.graph);
    boost::add_edge(idx_to + geo_ref.offsets[type::Mode_e::Bike],
                    idx_from + geo_ref.offsets[type::Mode_e::Walking],
                    ng::Edge(edge_idx, bike_sharing_return),
                    geo_ref.graph);
}

namespace ng = navitia::georef;
struct streetnetworkmode_fixture {
    streetnetworkmode_fixture() {

        /*

           K  ------------------------------ J
              |                             |
              |                             |
              |                             |  I
              |                             ---------------- H
              |                                             |
              |                                             |
              |                                             |
              |                                             |
              |                                             | g
              |                                             ---------------------- A ------------R
              |                                                               /  |
              |                                                             /    |
              |                                                           /      |
              |                                                         /        |
              |                                                       /          |
              |                                                     /            |
              |                                                   /              | E
              |                                                 /                ------------------------- F
              |                                              /                                           |
              |                                          /                                               |
              |                                       /                                                  |
              |                                    /                                                     |
              |                                 /                                                        |
              |                             /                                                            |
              |                          /                                                               |
              |                      /                                                                   |
              |                  /                                                                       |
              |             /                                                                            |
              B------------------------------------------------------------------------------------------- C
              |
              |
              |
              S

                On veut aller de S vers R :
                    *) la voie cyclable est : A->G->H->I->J->K->B
                    *) la voie rÃ©servÃ©e Ã  la voiture est : A->E->F->C->B
                    *) la voie MAP est : A->B
                    *) la voie cyclable, voiture et MAP : S->B
                    *) entre A et B que le transport en commun
                    *) entre A et R que la marche a pied



                    CoordonÃ©es :
                                A(12, 8)    0
                                G(10, 8)    1
                                H(10, 10)   2
                                I(7, 10)    3
                                J(7, 12)    4
                                K(1, 12)    5
                                B(1, 3)     6
                                C(15, 3)    7
                                F(15, 5)    8
                                E(12, 5)    9
                                R(21, 8)    10
                                S(1, 1)     11



        */

        boost::add_vertex(ng::Vertex(A),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(G),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(H),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(I),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(J),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(K),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(B),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(C),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(F),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(E),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(R),b.data.geo_ref.graph);
        boost::add_vertex(ng::Vertex(S),b.data.geo_ref.graph);

        b.data.geo_ref.init();

        ng::Way* way;
        way = new ng::Way();
        way->name = "rue ab"; // A->B
        way->idx = 0;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue ae"; // A->E
        way->idx = 1;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue ef"; // E->F
        way->idx = 2;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue fc"; // F->C
        way->idx = 3;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue cb"; // C->B
        way->idx = 4;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue ag"; // A->G
        way->idx = 5;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue gh"; // G->H
        way->idx = 6;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue hi"; // H->I
        way->idx = 7;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue ij"; // I->J
        way->idx = 8;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue jk"; // J->K
        way->idx = 9;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue kb"; // K->B
        way->idx = 10;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue ar"; // A->R
        way->idx = 11;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        way = new ng::Way();
        way->name = "rue bs"; // B->S
        way->idx = 12;
        way->way_type = "rue";
        b.data.geo_ref.ways.push_back(way);

        // A->B
        add_edges(0, b.data.geo_ref, AA, BB, 200, type::Mode_e::Walking);
        b.data.geo_ref.ways[0]->edges.push_back(std::make_pair(AA, BB));
        b.data.geo_ref.ways[0]->edges.push_back(std::make_pair(BB, AA));

        // A->E
        add_edges(1, b.data.geo_ref, AA, EE, A, E, type::Mode_e::Walking);
        add_edges(1, b.data.geo_ref, AA, EE, A, E, type::Mode_e::Car);
        b.data.geo_ref.ways[1]->edges.push_back(std::make_pair(AA, EE));
        b.data.geo_ref.ways[1]->edges.push_back(std::make_pair(EE, AA));

        // E->F
        add_edges(2, b.data.geo_ref, FF, EE, F, E, type::Mode_e::Car);
        add_edges(2, b.data.geo_ref, FF, EE, F, E, type::Mode_e::Walking);
        b.data.geo_ref.ways[2]->edges.push_back(std::make_pair(EE , FF));
        b.data.geo_ref.ways[2]->edges.push_back(std::make_pair(FF , EE));

        // F->C
        add_edges(3, b.data.geo_ref, FF, CC, F, C, type::Mode_e::Walking);
        add_edges(3, b.data.geo_ref, FF, CC, F, C, type::Mode_e::Car);
        b.data.geo_ref.ways[3]->edges.push_back(std::make_pair(FF , CC));
        b.data.geo_ref.ways[3]->edges.push_back(std::make_pair(CC , FF));

        // C->B
        add_edges(4, b.data.geo_ref, BB, CC, B, C, type::Mode_e::Walking);
        add_edges(4, b.data.geo_ref, BB, CC, 50, type::Mode_e::Car);
        b.data.geo_ref.ways[4]->edges.push_back(std::make_pair(CC , BB));
        b.data.geo_ref.ways[4]->edges.push_back(std::make_pair(BB , CC));

        // A->G
        distance_ag = A.distance_to(G) - .5;//the cost of the edge is a bit less than the distance not to get a conflict with the projection
        add_edges(5, b.data.geo_ref, AA, GG, distance_ag, type::Mode_e::Walking);
        add_edges(5, b.data.geo_ref, AA, GG, distance_ag, type::Mode_e::Bike);
        b.data.geo_ref.ways[5]->edges.push_back(std::make_pair(AA , GG));
        b.data.geo_ref.ways[5]->edges.push_back(std::make_pair(GG , AA));

        // G->H
        add_edges(6, b.data.geo_ref, HH, GG, G, H, type::Mode_e::Walking);
        add_edges(6, b.data.geo_ref, HH, GG, G, H, type::Mode_e::Bike);
        b.data.geo_ref.ways[6]->edges.push_back(std::make_pair(GG , HH));
        b.data.geo_ref.ways[6]->edges.push_back(std::make_pair(HH , GG));

        // H->I
        add_edges(7, b.data.geo_ref, HH, II, H, I, type::Mode_e::Walking);
        add_edges(7, b.data.geo_ref, HH, II, H, I, type::Mode_e::Bike);
        b.data.geo_ref.ways[7]->edges.push_back(std::make_pair(HH , II));
        b.data.geo_ref.ways[7]->edges.push_back(std::make_pair(II , HH));

        // I->J
        add_edges(8, b.data.geo_ref, II, JJ, I, J, type::Mode_e::Walking);
        add_edges(8, b.data.geo_ref, II, JJ, I, J, type::Mode_e::Bike);
        b.data.geo_ref.ways[8]->edges.push_back(std::make_pair(II , JJ));
        b.data.geo_ref.ways[8]->edges.push_back(std::make_pair(JJ , II));

        // J->K
        add_edges(9, b.data.geo_ref, KK, JJ, K, J, type::Mode_e::Walking);
        add_edges(9, b.data.geo_ref, KK, JJ, K, J, type::Mode_e::Bike);
        b.data.geo_ref.ways[9]->edges.push_back(std::make_pair(JJ , KK));
        b.data.geo_ref.ways[9]->edges.push_back(std::make_pair(KK , JJ));

        // K->B
        add_edges(10, b.data.geo_ref, KK, BB, K, B, type::Mode_e::Walking);
        add_edges(10, b.data.geo_ref, KK, BB, K, B, type::Mode_e::Bike);
        b.data.geo_ref.ways[10]->edges.push_back(std::make_pair(KK , BB));
        b.data.geo_ref.ways[10]->edges.push_back(std::make_pair(BB , KK));

        // A->R
        add_edges(11, b.data.geo_ref, AA, RR, A, R, type::Mode_e::Walking);
        b.data.geo_ref.ways[11]->edges.push_back(std::make_pair(AA, RR));
        b.data.geo_ref.ways[11]->edges.push_back(std::make_pair(RR, AA));

        // B->S
        add_edges(12, b.data.geo_ref, BB, SS, B, S, type::Mode_e::Walking);
        add_edges(12, b.data.geo_ref, BB, SS, B, S, type::Mode_e::Bike);
        add_edges(12, b.data.geo_ref, BB, SS, B, S, type::Mode_e::Car);
        b.data.geo_ref.ways[12]->edges.push_back(std::make_pair(BB, SS));
        b.data.geo_ref.ways[12]->edges.push_back(std::make_pair(SS, BB));

        //add bike sharing edges
        add_bike_sharing_edge(10, b.data.geo_ref, BB, BB);
        b.data.geo_ref.ways[10]->edges.push_back(std::make_pair(BB, BB)); //on way BK
        add_bike_sharing_edge(5, b.data.geo_ref, GG, GG);
        b.data.geo_ref.ways[5]->edges.push_back(std::make_pair(GG, GG)); //on way AG

        b.sa("stopA", A.lon(), A.lat());
        b.sa("stopR", R.lon(), R.lat());
        b.vj("A")("stopA", 8*3600 +10*60, 8*3600 + 11 * 60)("stopR", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.generate_dummy_basis();
        b.data.pt_data.index();
        b.data.build_raptor();
        b.data.build_uri();
        b.data.build_proximity_list();
        b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));

        std::string origin_lon = boost::lexical_cast<std::string>(S.lon()),
                origin_lat = boost::lexical_cast<std::string>(S.lat()),
                origin_uri = "coord:"+origin_lon+":"+origin_lat;
        type::Type_e origin_type = b.data.get_type_of_id(origin_uri);
        origin = {origin_type, origin_uri};

        std::string destination_lon = boost::lexical_cast<std::string>(R.lon()),
                destination_lat = boost::lexical_cast<std::string>(R.lat()),
                destination_uri = "coord:"+destination_lon+":"+destination_lat;
        type::Type_e destination_type = b.data.get_type_of_id(destination_uri);
        destination = {destination_type, destination_uri};
        // On met les horaires dans le desordre pour voir s'ils sont bien triÃ© comme attendu
    }

    pbnavitia::Response make_response() {
        streetnetwork::StreetNetwork sn_worker(b.data.geo_ref);
        RAPTOR raptor(b.data);
        return ::make_response(raptor, origin, destination, datetimes, true, type::AccessibiliteParams(), forbidden, sn_worker);
    }
    int AA = 0;
    int GG = 1;
    int HH = 2;
    int II = 3;
    int JJ = 4;
    int KK = 5;
    int BB = 6;
    int CC = 7;
    int FF = 8;
    int EE = 9;
    int RR = 10;
    int SS = 11;

    type::GeographicalCoord A = {120, 80, false};
    type::GeographicalCoord G = {100, 80, false};
    type::GeographicalCoord H = {100, 100, false};
    type::GeographicalCoord I = {70, 100, false};
    type::GeographicalCoord J = {70, 120, false};
    type::GeographicalCoord K = {10, 120, false};
    type::GeographicalCoord B = {10, 30, false};
    type::GeographicalCoord C = {220, 30, false};
    type::GeographicalCoord F = {220, 50, false};
    type::GeographicalCoord E = {120, 50, false};
    type::GeographicalCoord R = {210, 80, false};
    type::GeographicalCoord S = {10, 10, false};

    ed::builder b = {"20120614"};
    type::EntryPoint origin;
    type::EntryPoint destination;
    std::vector<std::string> datetimes = {"20120614T080000", "20120614T090000"};
    std::vector<std::string> forbidden;

    double distance_ag;
};

// Walking
BOOST_FIXTURE_TEST_CASE(walking_test, streetnetworkmode_fixture) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.max_duration = seconds(200 / test_default_speed[type::Mode_e::Walking]);
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.max_duration = seconds(50 / test_default_speed[type::Mode_e::Walking]);

    pbnavitia::Response resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);
    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);

    dump_response(resp, "walking");

    BOOST_CHECK_EQUAL(section.street_network().coordinates_size(), 4);
    BOOST_CHECK_EQUAL(section.street_network().duration(), 200);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 3);
    BOOST_CHECK_EQUAL(section.street_network().path_items(0).name(), "rue bs");
    BOOST_CHECK_EQUAL(section.street_network().path_items(1).name(), "rue ab");
    BOOST_CHECK_EQUAL(section.street_network().path_items(1).duration(), 200);
    BOOST_CHECK_EQUAL(section.street_network().path_items(2).name(), "rue ar");
}

//biking
BOOST_FIXTURE_TEST_CASE(biking, streetnetworkmode_fixture) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Bike];
    double total_distance = S.distance_to(B) + B.distance_to(K) + K.distance_to(J) + J.distance_to(I)
            + I.distance_to(H) + H.distance_to(G) + G.distance_to(A) + A.distance_to(R) + 1;
    origin.streetnetwork_params.max_duration = seconds(total_distance / test_default_speed[type::Mode_e::Bike]);
    origin.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    destination.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Bike];
    destination.streetnetwork_params.max_duration = seconds(total_distance / test_default_speed[type::Mode_e::Bike]);
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);
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
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    BOOST_CHECK_EQUAL(pathitem.direction(), 90); //one random direction check to ensure it has been computed
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
}

// Biking with a different speed
BOOST_FIXTURE_TEST_CASE(biking_with_different_speed, streetnetworkmode_fixture) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Bike];
    origin.streetnetwork_params.max_duration = bt::pos_infin;
    origin.streetnetwork_params.speed_factor = .5;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    destination.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Bike];
    destination.streetnetwork_params.max_duration = bt::pos_infin;
    destination.streetnetwork_params.speed_factor = .5;

    auto resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);
    auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    auto section = journey.sections(0);

    dump_response(resp, "biking with different speed");

    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ag");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 8);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_CHECK_EQUAL(section.street_network().duration(), 130*2 - 20 - 20); //it's the biking distance / biking speed
//    BOOST_CHECK_EQUAL(section.street_network().duration(), 130*2);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 7);

    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(pathitem.length(), 0); //since the biker si slower than usual, the projection of the starting point is done on B
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
    BOOST_CHECK_EQUAL(pathitem.length(), 0); //same here the projection is done on G
}

// Car
BOOST_FIXTURE_TEST_CASE(car, streetnetworkmode_fixture) {
    auto total_distance = S.distance_to(B) + B.distance_to(C) + C.distance_to(F) + F.distance_to(E) + E.distance_to(A) + 1;

    origin.streetnetwork_params.mode = navitia::type::Mode_e::Car;
    origin.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Car];
    origin.streetnetwork_params.max_duration = seconds(total_distance / test_default_speed[type::Mode_e::Car]);
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = navitia::type::Mode_e::Car;
    destination.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Car];
    destination.streetnetwork_params.max_duration = seconds(total_distance / test_default_speed[type::Mode_e::Car]);
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);
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
BOOST_FIXTURE_TEST_CASE(bss_test, streetnetworkmode_fixture) {

    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bss;
    origin.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Bss];
    origin.streetnetwork_params.max_duration = bt::pos_infin;
    origin.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bss;
    destination.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Bss];
    destination.streetnetwork_params.max_duration = bt::pos_infin;
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);
    auto journey = resp.journeys(0);
    dump_response(resp, "bss");

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 5);
    //we should have 5 sections
    //1 walk, 1 boarding, 1 bike, 1 landing, and 1 final walking section
    auto section = journey.sections(0);

    //walk
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);

    //boarding
    section = journey.sections(1);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::boarding);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    BOOST_CHECK_EQUAL(pathitem.duration(), bike_sharing_pickup.total_seconds());
    BOOST_CHECK_EQUAL(pathitem.length(), 0);

    //bike
    section = journey.sections(2);
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

    //landing
    section = journey.sections(3);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::landing);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.duration(), bike_sharing_return.total_seconds());
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    BOOST_CHECK_EQUAL(pathitem.length(), 0);

    //walking
    section = journey.sections(4);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue ag");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ar");
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);

    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 2);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ar");
    BOOST_CHECK_EQUAL(pathitem.duration(), 0); //projection
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

    type::GeographicalCoord A(0, 0, false);
    type::GeographicalCoord B(0, 0.1, false);
    type::GeographicalCoord C(0, 0.02, false);
    type::GeographicalCoord D(0, 0.03, false);
    type::GeographicalCoord SP1(0, 0.005, false);
    type::GeographicalCoord SP2(0, 0.007, false);
    type::GeographicalCoord SP3(0, 0.021, false);
    type::GeographicalCoord SP4(0, 0.025, false);

    b.data.geo_ref.init_offset(0);
    ng::Way* way;

    way = new ng::Way();
    way->name = "rue ab"; // A->B
    way->idx = 0;
    way->way_type = "rue";
    b.data.geo_ref.ways.push_back(way);

    way = new ng::Way();
    way->name = "rue cd"; // C->D
    way->idx = 1;
    way->way_type = "rue";
    b.data.geo_ref.ways.push_back(way);


    boost::add_edge(AA, BB, ng::Edge(0,10), b.data.geo_ref.graph);
    boost::add_edge(BB, AA, ng::Edge(0,10), b.data.geo_ref.graph);
    b.data.geo_ref.ways[0]->edges.push_back(std::make_pair(AA, BB));
    b.data.geo_ref.ways[0]->edges.push_back(std::make_pair(BB, AA));

    boost::add_edge(CC, DD, ng::Edge(0,50), b.data.geo_ref.graph);
    boost::add_edge(DD, CC, ng::Edge(0,50), b.data.geo_ref.graph);
    b.data.geo_ref.ways[0]->edges.push_back(std::make_pair(AA, BB));
    b.data.geo_ref.ways[0]->edges.push_back(std::make_pair(BB, AA));

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
    b.data.pt_data.index();
    b.data.build_raptor();
    b.data.build_uri();
    b.data.build_proximity_list();
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    streetnetwork::StreetNetwork sn_worker(b.data.geo_ref);

    RAPTOR raptor(b.data);

    std::string origin_uri = "stop_area:stop1";
    type::Type_e origin_type = b.data.get_type_of_id(origin_uri);
    type::EntryPoint origin(origin_type, origin_uri);
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.distance = 15;
    std::string destination_uri = "stop_area:stop4";
    type::Type_e destination_type = b.data.get_type_of_id(destination_uri);
    type::EntryPoint destination(destination_type, destination_uri);
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.distance = 5;
    std::vector<std::string> datetimes({"20120614T080000"});
    std::vector<std::string> forbidden;

    pbnavitia::Response resp = make_response(raptor, origin, destination, datetimes, true, 1.38, 1000, type::AccessibiliteParams(), forbidden, sn_worker);

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    BOOST_REQUIRE_EQUAL(journey.sections(0).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_REQUIRE_EQUAL(journey.sections(1).type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_REQUIRE_EQUAL(journey.sections(2).type(), pbnavitia::SectionType::STREET_NETWORK);

}*/
