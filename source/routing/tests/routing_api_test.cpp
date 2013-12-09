#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "routing/raptor_api.h"
#include "ed/build_helper.h"


using namespace navitia;
using namespace routing;
using namespace boost::posix_time;

BOOST_AUTO_TEST_CASE(simple_journey){
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
    pbnavitia::Response resp = make_response(raptor, origin, destination, {"20120614T021000"}, true, 1.38, 1000, type::AccessibiliteParams()/*false*/, forbidden, sn_worker);

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
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    RAPTOR raptor(b.data);

    type::Type_e origin_type = b.data.get_type_of_id("stop_area:stop1");
    type::Type_e destination_type = b.data.get_type_of_id("stop_area:stop2");
    type::EntryPoint origin(origin_type, "stop_area:stop1");
    type::EntryPoint destination(destination_type, "stop_area:stop2");

    streetnetwork::StreetNetwork sn_worker(data.geo_ref);

    // On met les horaires dans le desordre pour voir s'ils sont bien triÃ© comme attendu
    std::vector<std::string> datetimes({"20120614T080000", "20120614T090000"});
    pbnavitia::Response resp = make_response(raptor, origin, destination, datetimes, true, 1.38, 1000, type::AccessibiliteParams()/*false*/, forbidden, sn_worker);

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

BOOST_AUTO_TEST_CASE(journey_streetnetworkmode){

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
      |                                             ---------------------- A --------------------------------R
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
                        B(1, 2)     6
                        C(15, 2)    7
                        F(15, 5)    8
                        E(12, 5)    9
                        R(21, 8)    10
                        S(1, 1)     11



*/
    namespace ng = navitia::georef;
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
    ed::builder b("20120614");

    type::GeographicalCoord A(12, 8, false);
    boost::add_vertex(ng::Vertex(A),b.data.geo_ref.graph);

    type::GeographicalCoord G(10, 8, false);
    boost::add_vertex(ng::Vertex(G),b.data.geo_ref.graph);

    type::GeographicalCoord H(10, 10, false);
    boost::add_vertex(ng::Vertex(H),b.data.geo_ref.graph);

    type::GeographicalCoord I(7, 10, false);
    boost::add_vertex(ng::Vertex(I),b.data.geo_ref.graph);

    type::GeographicalCoord J(7, 12, false);
    boost::add_vertex(ng::Vertex(J),b.data.geo_ref.graph);

    type::GeographicalCoord K(1, 12, false);
    boost::add_vertex(ng::Vertex(K),b.data.geo_ref.graph);

    type::GeographicalCoord B(1, 2, false);
    boost::add_vertex(ng::Vertex(B),b.data.geo_ref.graph);

    type::GeographicalCoord C(15, 2, false);
    boost::add_vertex(ng::Vertex(C),b.data.geo_ref.graph);

    type::GeographicalCoord F(15, 5, false);
    boost::add_vertex(ng::Vertex(F),b.data.geo_ref.graph);

    type::GeographicalCoord E(12, 5, false);
    boost::add_vertex(ng::Vertex(E),b.data.geo_ref.graph);

    type::GeographicalCoord R(21, 8, false);
    boost::add_vertex(ng::Vertex(R),b.data.geo_ref.graph);

    type::GeographicalCoord S(1, 1, false);
    boost::add_vertex(ng::Vertex(S),b.data.geo_ref.graph);
    // Pour le vls
    type::GeographicalCoord V(0.5, 1, false);
    type::GeographicalCoord Q(18, 10, false);


    ng::vertex_t Conunt_v = boost::num_vertices(b.data.geo_ref.graph);
    b.data.geo_ref.init_offset(Conunt_v);
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
    boost::add_edge(AA, BB, ng::Edge(0,10), b.data.geo_ref.graph);
    boost::add_edge(BB, AA, ng::Edge(0,10), b.data.geo_ref.graph);
    b.data.geo_ref.ways[0]->edges.push_back(std::make_pair(AA, BB));
    b.data.geo_ref.ways[0]->edges.push_back(std::make_pair(BB, AA));

// A->E
    boost::add_edge(AA , EE, ng::Edge(1,A.distance_to(E)), b.data.geo_ref.graph);
    boost::add_edge(EE , AA, ng::Edge(1,A.distance_to(E)), b.data.geo_ref.graph);
    boost::add_edge(AA + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], EE + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(1,A.distance_to(E)), b.data.geo_ref.graph);
    boost::add_edge(EE + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], AA + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(1,A.distance_to(E)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[1]->edges.push_back(std::make_pair(AA, EE));
    b.data.geo_ref.ways[1]->edges.push_back(std::make_pair(EE, AA));

// E->F
    boost::add_edge(EE , FF , ng::Edge(2,E.distance_to(F)), b.data.geo_ref.graph);
    boost::add_edge(FF , EE , ng::Edge(2,E.distance_to(F)), b.data.geo_ref.graph);
    boost::add_edge(EE + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], FF + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(2,E.distance_to(F)), b.data.geo_ref.graph);
    boost::add_edge(FF + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], EE + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(2,E.distance_to(F)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[2]->edges.push_back(std::make_pair(EE , FF));
    b.data.geo_ref.ways[2]->edges.push_back(std::make_pair(FF , EE));

// F->C
    boost::add_edge(FF , CC , ng::Edge(3,F.distance_to(C)), b.data.geo_ref.graph);
    boost::add_edge(CC , FF , ng::Edge(3,F.distance_to(C)), b.data.geo_ref.graph);
    boost::add_edge(FF + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], CC + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(3,F.distance_to(C)), b.data.geo_ref.graph);
    boost::add_edge(CC + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], FF + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(3,F.distance_to(C)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[3]->edges.push_back(std::make_pair(FF , CC));
    b.data.geo_ref.ways[3]->edges.push_back(std::make_pair(CC , FF));

// C->B
    boost::add_edge(CC , BB , ng::Edge(4,C.distance_to(B)), b.data.geo_ref.graph);
    boost::add_edge(BB , CC , ng::Edge(4,C.distance_to(B)), b.data.geo_ref.graph);
    boost::add_edge(CC + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], BB + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(4,5), b.data.geo_ref.graph);
    boost::add_edge(BB + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], CC + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(4,5), b.data.geo_ref.graph);
    b.data.geo_ref.ways[4]->edges.push_back(std::make_pair(CC , BB));
    b.data.geo_ref.ways[4]->edges.push_back(std::make_pair(BB , CC));

// A->G
    double distance_ag = A.distance_to(G) - .5;//the cost of the edge is a bit less than the distance not to get a conflict with the projection
    boost::add_edge(AA , GG , ng::Edge(5,distance_ag), b.data.geo_ref.graph);
    boost::add_edge(GG , AA , ng::Edge(5,distance_ag), b.data.geo_ref.graph);
    boost::add_edge(AA + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], GG + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(5,distance_ag), b.data.geo_ref.graph);
    boost::add_edge(GG + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], AA + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(5,distance_ag), b.data.geo_ref.graph);
    b.data.geo_ref.ways[5]->edges.push_back(std::make_pair(AA , GG));
    b.data.geo_ref.ways[5]->edges.push_back(std::make_pair(GG , AA));

// G->H
    boost::add_edge(GG , HH , ng::Edge(6,G.distance_to(H)), b.data.geo_ref.graph);
    boost::add_edge(HH , GG , ng::Edge(6,G.distance_to(H)), b.data.geo_ref.graph);
    boost::add_edge(GG + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], HH + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(6,G.distance_to(H)), b.data.geo_ref.graph);
    boost::add_edge(HH + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], GG + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(6,G.distance_to(H)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[6]->edges.push_back(std::make_pair(GG , HH));
    b.data.geo_ref.ways[6]->edges.push_back(std::make_pair(HH , GG));

// H->I
    boost::add_edge(HH , II , ng::Edge(7,H.distance_to(I)), b.data.geo_ref.graph);
    boost::add_edge(II , HH , ng::Edge(7,H.distance_to(I)), b.data.geo_ref.graph);
    boost::add_edge(HH + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], II + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(7,H.distance_to(I)), b.data.geo_ref.graph);
    boost::add_edge(II + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], HH + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(7,H.distance_to(I)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[7]->edges.push_back(std::make_pair(HH , II));
    b.data.geo_ref.ways[7]->edges.push_back(std::make_pair(II , HH));

// I->J
    boost::add_edge(II , JJ , ng::Edge(8,I.distance_to(J)), b.data.geo_ref.graph);
    boost::add_edge(JJ , II , ng::Edge(8,I.distance_to(J)), b.data.geo_ref.graph);
    boost::add_edge(II + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], JJ + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(8,I.distance_to(J)), b.data.geo_ref.graph);
    boost::add_edge(JJ + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], II + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(8,I.distance_to(J)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[8]->edges.push_back(std::make_pair(II , JJ));
    b.data.geo_ref.ways[8]->edges.push_back(std::make_pair(JJ , II));

// J->K
    boost::add_edge(JJ , KK , ng::Edge(9,J.distance_to(K)), b.data.geo_ref.graph);
    boost::add_edge(KK , JJ , ng::Edge(9,J.distance_to(K)), b.data.geo_ref.graph);
    boost::add_edge(JJ + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], KK + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(9,J.distance_to(K)), b.data.geo_ref.graph);
    boost::add_edge(KK + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], JJ + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(9,J.distance_to(K)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[9]->edges.push_back(std::make_pair(JJ , KK));
    b.data.geo_ref.ways[9]->edges.push_back(std::make_pair(KK , JJ));

// K->B
    boost::add_edge(KK , BB , ng::Edge(10,K.distance_to(B)), b.data.geo_ref.graph);
    boost::add_edge(BB , KK , ng::Edge(10,K.distance_to(B)), b.data.geo_ref.graph);
    boost::add_edge(KK + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], BB + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(10,K.distance_to(B)), b.data.geo_ref.graph);
    boost::add_edge(BB + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], KK + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(10,K.distance_to(B)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[10]->edges.push_back(std::make_pair(KK , BB));
    b.data.geo_ref.ways[10]->edges.push_back(std::make_pair(BB , KK));

// A->R
    boost::add_edge(AA, RR, ng::Edge(11,A.distance_to(R)), b.data.geo_ref.graph);
    boost::add_edge(RR, AA, ng::Edge(11,A.distance_to(R)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[11]->edges.push_back(std::make_pair(AA, RR));
    b.data.geo_ref.ways[11]->edges.push_back(std::make_pair(RR, AA));

// B->S
    boost::add_edge(BB, SS, ng::Edge(12,B.distance_to(S)), b.data.geo_ref.graph);
    boost::add_edge(SS, BB, ng::Edge(12,B.distance_to(S)), b.data.geo_ref.graph);
    b.data.geo_ref.ways[12]->edges.push_back(std::make_pair(BB, SS));
    b.data.geo_ref.ways[12]->edges.push_back(std::make_pair(SS, BB));
    boost::add_edge(BB + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], SS + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(12,B.distance_to(S)), b.data.geo_ref.graph);
    boost::add_edge(SS + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], BB + b.data.geo_ref.offsets[navitia::type::Mode_e::Bike], ng::Edge(12,B.distance_to(S)), b.data.geo_ref.graph);
    boost::add_edge(BB + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], SS + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(12,B.distance_to(S)), b.data.geo_ref.graph);
    boost::add_edge(SS + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], BB + b.data.geo_ref.offsets[navitia::type::Mode_e::Car], ng::Edge(12,B.distance_to(S)), b.data.geo_ref.graph);

    b.sa("stopA", A.lon(), A.lat());
    b.sa("stopR", R.lon(), R.lat());
    b.vj("A")("stopA", 8*3600 +10*60, 8*3600 + 11 * 60)("stopR", 8*3600 + 20 * 60 ,8*3600 + 21*60);
    b.generate_dummy_basis();
    b.data.pt_data.index();
    b.data.build_raptor();
    b.data.build_uri();
    b.data.build_proximity_list();
    b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
    streetnetwork::StreetNetwork sn_worker(b.data.geo_ref);

    RAPTOR raptor(b.data);

    std::string origin_lon = boost::lexical_cast<std::string>(S.lon()),
                origin_lat = boost::lexical_cast<std::string>(S.lat()),
                origin_uri = "coord:"+origin_lon+":"+origin_lat;
    type::Type_e origin_type = b.data.get_type_of_id(origin_uri);
    type::EntryPoint origin(origin_type, origin_uri);
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.distance = 15;
    std::string destination_lon = boost::lexical_cast<std::string>(R.lon()),
                destination_lat = boost::lexical_cast<std::string>(R.lat()),
                destination_uri = "coord:"+destination_lon+":"+destination_lat;
    type::Type_e destination_type = b.data.get_type_of_id(destination_uri);
    type::EntryPoint destination(destination_type, destination_uri);
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.distance = 5;
    // On met les horaires dans le desordre pour voir s'ils sont bien triÃ© comme attendu
    std::vector<std::string> datetimes({"20120614T080000", "20120614T090000"});
    std::vector<std::string> forbidden;

    pbnavitia::Response resp = make_response(raptor, origin, destination, datetimes, true, 1.38, 1000, type::AccessibiliteParams()/*false*/, forbidden, sn_worker);

// Marche Ã  pied
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);
    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 5);
    BOOST_REQUIRE_EQUAL(section.street_network().length(), 10);
    BOOST_REQUIRE_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    pbnavitia::PathItem pathitem = section.street_network().path_items(0);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue ab");
    BOOST_REQUIRE_EQUAL(pathitem.length(), 10);
// vÃ©lo
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Bike];
    origin.streetnetwork_params.speed = 13;
    double total_distance = S.distance_to(B) + B.distance_to(K) + K.distance_to(J) + J.distance_to(I) + I.distance_to(H) + H.distance_to(G) + G.distance_to(A) + A.distance_to(R) + 1;
    origin.streetnetwork_params.distance = total_distance;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    destination.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Bike];
    destination.streetnetwork_params.distance = total_distance;
    resp = make_response(raptor, origin, destination, datetimes, true, 1.38, 1000, type::AccessibiliteParams()/*false*/, forbidden, sn_worker);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);
    journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_REQUIRE_EQUAL(section.origin().address().name(), "rue kb");
    BOOST_REQUIRE_EQUAL(section.destination().address().name(), "rue ag");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 10);
    BOOST_REQUIRE_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_REQUIRE_EQUAL(section.street_network().length(), 19);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 6);

    pathitem = section.street_network().path_items(0);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue kb");
    pathitem = section.street_network().path_items(1);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue jk");
    pathitem = section.street_network().path_items(2);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue ij");
    pathitem = section.street_network().path_items(3);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue hi");
    pathitem = section.street_network().path_items(4);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue gh");
    pathitem = section.street_network().path_items(5);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue ag");

    // Car
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Car;
    origin.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Car];
    origin.streetnetwork_params.speed = 13;
    origin.streetnetwork_params.distance = S.distance_to(B) + B.distance_to(C) + C.distance_to(F) + F.distance_to(E) + E.distance_to(A) + 1;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Car;
    destination.streetnetwork_params.offset = b.data.geo_ref.offsets[navitia::type::Mode_e::Car];
    destination.streetnetwork_params.distance = 5;
    resp = make_response(raptor, origin, destination, datetimes, true, 1.38, 1000, type::AccessibiliteParams()/*false*/, forbidden, sn_worker);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);
    journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_REQUIRE_EQUAL(section.origin().address().name(), "rue cb");
    BOOST_REQUIRE_EQUAL(section.destination().address().name(), "rue fc");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 6);
    BOOST_REQUIRE_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Car);
    BOOST_REQUIRE_EQUAL(section.street_network().length(), 7);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 2);
    //since R is not accessible by car, we project R in the closest edge in the car graph
    //this edge is F-C, so this is the end of the journey (the rest of it is as the crow flies)
    pathitem = section.street_network().path_items(0);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue cb");
    pathitem = section.street_network().path_items(1);
    BOOST_REQUIRE_EQUAL(pathitem.name(), "rue fc");
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
