#pragma once
#include "routing/raptor_api.h"
#include "ed/build_helper.h"

namespace ng = navitia::georef;

struct test_speed_provider {
    const navitia::flat_enum_map<nt::Mode_e, float> get_default_speed() const { return test_default_speed; }

    //to ease test we use easier speed
    const navitia::flat_enum_map<nt::Mode_e, float> test_default_speed {
                                                        {{
                                                            1, //nt::Mode_e::Walking
                                                            2, //nt::Mode_e::Bike
                                                            5, //nt::Mode_e::Car
                                                            2 //nt::Mode_e::Vls
                                                        }}
                                                        };
    bt::time_duration to_duration(float dist, navitia::type::Mode_e mode) {
        return bt::seconds(std::round(dist / get_default_speed()[mode]));
    }
};

struct normal_speed_provider {
    const navitia::flat_enum_map<nt::Mode_e, float> get_default_speed() const { return navitia::georef::default_speed; }
    bt::time_duration to_duration(float dist, navitia::type::Mode_e mode) {
        return bt::milliseconds(dist / get_default_speed()[mode] * 1000);
    }
};

template <typename speed_provider_trait>
struct routing_api_data {

    routing_api_data() {
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

        boost::add_vertex(navitia::georef::Vertex(A),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(G),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(H),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(I),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(J),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(K),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(B),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(C),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(F),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(E),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(R),b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(S),b.data->geo_ref->graph);

        b.data->geo_ref->init();

        navitia::georef::Way* way;
        way = new navitia::georef::Way();
        way->name = "rue ab"; // A->B
        way->idx = 0;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ae"; // A->E
        way->idx = 1;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ef"; // E->F
        way->idx = 2;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue fc"; // F->C
        way->idx = 3;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue cb"; // C->B
        way->idx = 4;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ag"; // A->G
        way->idx = 5;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue gh"; // G->H
        way->idx = 6;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue hi"; // H->I
        way->idx = 7;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ij"; // I->J
        way->idx = 8;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue jk"; // J->K
        way->idx = 9;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue kb"; // K->B
        way->idx = 10;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ar"; // A->R
        way->idx = 11;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue bs"; // B->S
        way->idx = 12;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);

        // A->B
        add_edges(0, *b.data->geo_ref, AA, BB, 200, navitia::type::Mode_e::Walking);
        b.data->geo_ref->ways[0]->edges.push_back(std::make_pair(AA, BB));
        b.data->geo_ref->ways[0]->edges.push_back(std::make_pair(BB, AA));

        // A->E
        add_edges(1, *b.data->geo_ref, AA, EE, A, E, navitia::type::Mode_e::Walking);
        add_edges(1, *b.data->geo_ref, AA, EE, A, E, navitia::type::Mode_e::Car);
        b.data->geo_ref->ways[1]->edges.push_back(std::make_pair(AA, EE));
        b.data->geo_ref->ways[1]->edges.push_back(std::make_pair(EE, AA));

        // E->F
        add_edges(2, *b.data->geo_ref, FF, EE, F, E, navitia::type::Mode_e::Car);
        add_edges(2, *b.data->geo_ref, FF, EE, F, E, navitia::type::Mode_e::Walking);
        b.data->geo_ref->ways[2]->edges.push_back(std::make_pair(EE , FF));
        b.data->geo_ref->ways[2]->edges.push_back(std::make_pair(FF , EE));

        // F->C
        add_edges(3, *b.data->geo_ref, FF, CC, F, C, navitia::type::Mode_e::Walking);
        add_edges(3, *b.data->geo_ref, FF, CC, F, C, navitia::type::Mode_e::Car);
        b.data->geo_ref->ways[3]->edges.push_back(std::make_pair(FF , CC));
        b.data->geo_ref->ways[3]->edges.push_back(std::make_pair(CC , FF));

        // C->B
        add_edges(4, *b.data->geo_ref, BB, CC, B, C, navitia::type::Mode_e::Walking);
        add_edges(4, *b.data->geo_ref, BB, CC, 50, navitia::type::Mode_e::Car);
        b.data->geo_ref->ways[4]->edges.push_back(std::make_pair(CC , BB));
        b.data->geo_ref->ways[4]->edges.push_back(std::make_pair(BB , CC));

        // A->G
        distance_ag = A.distance_to(G) - .5;//the cost of the edge is a bit less than the distance not to get a conflict with the projection
        add_edges(5, *b.data->geo_ref, AA, GG, distance_ag, navitia::type::Mode_e::Walking);
        add_edges(5, *b.data->geo_ref, AA, GG, distance_ag, navitia::type::Mode_e::Bike);
        b.data->geo_ref->ways[5]->edges.push_back(std::make_pair(AA , GG));
        b.data->geo_ref->ways[5]->edges.push_back(std::make_pair(GG , AA));

        // G->H
        add_edges(6, *b.data->geo_ref, HH, GG, G, H, navitia::type::Mode_e::Walking);
        add_edges(6, *b.data->geo_ref, HH, GG, G, H, navitia::type::Mode_e::Bike);
        b.data->geo_ref->ways[6]->edges.push_back(std::make_pair(GG , HH));
        b.data->geo_ref->ways[6]->edges.push_back(std::make_pair(HH , GG));

        // H->I
        add_edges(7, *b.data->geo_ref, HH, II, H, I, navitia::type::Mode_e::Walking);
        add_edges(7, *b.data->geo_ref, HH, II, H, I, navitia::type::Mode_e::Bike);
        b.data->geo_ref->ways[7]->edges.push_back(std::make_pair(HH , II));
        b.data->geo_ref->ways[7]->edges.push_back(std::make_pair(II , HH));

        // I->J
        add_edges(8, *b.data->geo_ref, II, JJ, I, J, navitia::type::Mode_e::Walking);
        add_edges(8, *b.data->geo_ref, II, JJ, I, J, navitia::type::Mode_e::Bike);
        b.data->geo_ref->ways[8]->edges.push_back(std::make_pair(II , JJ));
        b.data->geo_ref->ways[8]->edges.push_back(std::make_pair(JJ , II));

        // J->K
        add_edges(9, *b.data->geo_ref, KK, JJ, K, J, navitia::type::Mode_e::Walking);
        add_edges(9, *b.data->geo_ref, KK, JJ, K, J, navitia::type::Mode_e::Bike);
        b.data->geo_ref->ways[9]->edges.push_back(std::make_pair(JJ , KK));
        b.data->geo_ref->ways[9]->edges.push_back(std::make_pair(KK , JJ));

        // K->B
        add_edges(10, *b.data->geo_ref, KK, BB, K, B, navitia::type::Mode_e::Walking);
        add_edges(10, *b.data->geo_ref, KK, BB, K, B, navitia::type::Mode_e::Bike);
        b.data->geo_ref->ways[10]->edges.push_back(std::make_pair(KK , BB));
        b.data->geo_ref->ways[10]->edges.push_back(std::make_pair(BB , KK));

        // A->R
        add_edges(11, *b.data->geo_ref, AA, RR, A, R, navitia::type::Mode_e::Walking);
        b.data->geo_ref->ways[11]->edges.push_back(std::make_pair(AA, RR));
        b.data->geo_ref->ways[11]->edges.push_back(std::make_pair(RR, AA));

        // B->S
        add_edges(12, *b.data->geo_ref, BB, SS, B, S, navitia::type::Mode_e::Walking);
        add_edges(12, *b.data->geo_ref, BB, SS, B, S, navitia::type::Mode_e::Bike);
        add_edges(12, *b.data->geo_ref, BB, SS, B, S, navitia::type::Mode_e::Car);
        b.data->geo_ref->ways[12]->edges.push_back(std::make_pair(BB, SS));
        b.data->geo_ref->ways[12]->edges.push_back(std::make_pair(SS, BB));

        //add bike sharing edges
        add_bike_sharing_edge(10, *b.data->geo_ref, BB, BB);
        b.data->geo_ref->ways[10]->edges.push_back(std::make_pair(BB, BB)); //on way BK
        add_bike_sharing_edge(5, *b.data->geo_ref, GG, GG);
        b.data->geo_ref->ways[5]->edges.push_back(std::make_pair(GG, GG)); //on way AG

        b.sa("stopA", A.lon(), A.lat());
        b.sa("stopR", R.lon(), R.lat());
        b.vj("A")("stopA", 8*3600 +10*60, 8*3600 + 11 * 60)("stopR", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.generate_dummy_basis();
        b.data->pt_data->index();
        b.data->build_raptor();
        b.data->build_uri();
        b.data->build_proximity_list();
        b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));

        std::string origin_lon = boost::lexical_cast<std::string>(S.lon()),
                origin_lat = boost::lexical_cast<std::string>(S.lat()),
                origin_uri = "coord:"+origin_lon+":"+origin_lat;
        navitia::type::Type_e origin_type = b.data->get_type_of_id(origin_uri);
        origin = {origin_type, origin_uri};

        std::string destination_lon = boost::lexical_cast<std::string>(R.lon()),
                destination_lat = boost::lexical_cast<std::string>(R.lat()),
                destination_uri = "coord:"+destination_lon+":"+destination_lat;
        navitia::type::Type_e destination_type = b.data->get_type_of_id(destination_uri);
        destination = {destination_type, destination_uri};
        // On met les horaires dans le desordre pour voir s'ils sont bien triÃ© comme attendu
    }

    bt::time_duration to_duration(double dist, navitia::type::Mode_e mode) {
        return speed_trait.to_duration(dist, mode);
    }

    void add_edges(int edge_idx, navitia::georef::GeoRef& geo_ref, int idx_from, int idx_to, float dist, navitia::type::Mode_e mode) {
        boost::add_edge(idx_from + geo_ref.offsets[mode],
                        idx_to + geo_ref.offsets[mode],
                        navitia::georef::Edge(edge_idx, to_duration(dist, mode)),
                        geo_ref.graph);
        boost::add_edge(idx_to + geo_ref.offsets[mode],
                        idx_from + geo_ref.offsets[mode],
                        navitia::georef::Edge(edge_idx, to_duration(dist, mode)),
                        geo_ref.graph);
    }
    void add_edges(int edge_idx, navitia::georef::GeoRef& geo_ref, int idx_from, int idx_to,
                   const navitia::type::GeographicalCoord& a, const navitia::type::GeographicalCoord& b, navitia::type::Mode_e mode) {
        add_edges(edge_idx, geo_ref, idx_from, idx_to, a.distance_to(b), mode);
    }

    const bt::time_duration bike_sharing_pickup = bt::seconds(30);
    const bt::time_duration bike_sharing_return = bt::seconds(45);

    void add_bike_sharing_edge(int edge_idx, navitia::georef::GeoRef& geo_ref, int idx_from, int idx_to) {
        boost::add_edge(idx_from + geo_ref.offsets[navitia::type::Mode_e::Walking],
                        idx_to + geo_ref.offsets[navitia::type::Mode_e::Bike],
                        navitia::georef::Edge(edge_idx, bike_sharing_pickup),
                        geo_ref.graph);
        boost::add_edge(idx_to + geo_ref.offsets[navitia::type::Mode_e::Bike],
                        idx_from + geo_ref.offsets[navitia::type::Mode_e::Walking],
                        navitia::georef::Edge(edge_idx, bike_sharing_return),
                        geo_ref.graph);
    }

    const navitia::flat_enum_map<nt::Mode_e, float> get_default_speed() const { return speed_trait.get_default_speed(); }


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

    navitia::type::GeographicalCoord A = {120, 80, false};
    navitia::type::GeographicalCoord G = {100, 80, false};
    navitia::type::GeographicalCoord H = {100, 100, false};
    navitia::type::GeographicalCoord I = {70, 100, false};
    navitia::type::GeographicalCoord J = {70, 120, false};
    navitia::type::GeographicalCoord K = {10, 120, false};
    navitia::type::GeographicalCoord B = {10, 30, false};
    navitia::type::GeographicalCoord C = {220, 30, false};
    navitia::type::GeographicalCoord F = {220, 50, false};
    navitia::type::GeographicalCoord E = {120, 50, false};
    navitia::type::GeographicalCoord R = {210, 80, false};
    navitia::type::GeographicalCoord S = {10, 10, false};

    ed::builder b = {"20120614"};
    navitia::type::EntryPoint origin;
    navitia::type::EntryPoint destination;
    std::vector<std::string> datetimes = {"20120614T080000", "20120614T090000"};
    std::vector<std::string> forbidden;

    double distance_ag;

    speed_provider_trait speed_trait;
};
