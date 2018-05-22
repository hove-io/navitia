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

#pragma once
#include "routing/raptor_api.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "type/pt_data.h"
#include "type/message.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace ng = navitia::georef;

using navitia::type::disruption::Disruption;
using navitia::type::disruption::Impact;
using navitia::type::disruption::Tag;
using navitia::type::disruption::Severity;
using navitia::type::disruption::ChannelType;
using navitia::type::disruption::Property;

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
    navitia::time_duration to_duration(float dist, navitia::type::Mode_e mode) {
        return navitia::seconds(std::round(dist / get_default_speed()[mode]));
    }
};

struct normal_speed_provider {
    const navitia::flat_enum_map<nt::Mode_e, float> get_default_speed() const { return navitia::georef::default_speed; }
    navitia::time_duration to_duration(float dist, navitia::type::Mode_e mode) {
        return navitia::milliseconds(dist / get_default_speed()[mode] * 1000);
    }
};


template <typename speed_provider_trait>
struct routing_api_data {

    routing_api_data(float distance_a_b = 200, bool activate_pt = true) : distance_ab(distance_a_b) {
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
        D --- B------------------------------------------------------------------------------------------- C
              |
              |
              |
              S

                We want to go from S to R:

                    /journeys?from=coord:8.98311981954709e-05:8.98311981954709e-05
                              &to=coord:0.0018864551621048887:0.0007186495855637672
                              &datetime=20120614080000
                              &debug=true

                    *) The bikeway is: A->G->H->I->J->K->B
                    *) The car way is: A->E->F->C->B
                    *) We can walk on A->B
                    *) A->B has public transport
                    *) S->B and B->D can be use by walk, bike or car
                    *) A->R is a pedestrian street

                    Coordinates:
                                A(120,  80)    0
                                G(100,  80)    1
                                H(100, 100)    2
                                I( 70, 100)    3
                                J( 70, 120)    4
                                K( 10, 120)    5
                                B( 10,  30)    6
                                C(220,  30)    7
                                F(220,  50)    8
                                E(120,  50)    9
                                R(210,  80)   10
                                S( 10,  10)   11
                                D(  0,  30)   12
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
        boost::add_vertex(navitia::georef::Vertex(D),b.data->geo_ref->graph);

        b.data->geo_ref->init();

        b.data->geo_ref->admins.push_back(new navitia::georef::Admin());
        auto admin = b.data->geo_ref->admins.back();
        admin->uri = "admin:74435";
        admin->name = "Condom";
        admin->insee = "03430";
        admin->level = 8;
        admin->postal_codes.push_back("03430");
        admin->coord = nt::GeographicalCoord(D.lon(), D.lat());
        admin->idx = 0;

        navitia::georef::Way* way;
        way = new navitia::georef::Way();
        way->name = "rue ab"; // A->B
        way->idx = 0;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ae"; // A->E
        way->idx = 1;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ef"; // E->F
        way->idx = 2;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        way->add_house_number(navitia::georef::HouseNumber(E.lon(), E.lat(), 2));
        way->add_house_number(navitia::georef::HouseNumber(F.lon(), F.lat(), 5));
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue fc"; // F->C
        way->idx = 3;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue cb"; // C->B
        way->idx = 4;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ag"; // A->G
        way->idx = 5;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        way->add_house_number(navitia::georef::HouseNumber(R.lon(), R.lat(), 1));
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue gh"; // G->H
        way->idx = 6;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue hi"; // H->I
        way->idx = 7;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ij"; // I->J
        way->idx = 8;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue jk"; // J->K
        way->idx = 9;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue kb"; // K->B
        way->idx = 10;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        way->add_house_number(navitia::georef::HouseNumber(HouseNmber42.lon(), HouseNmber42.lat(), 42));
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ar"; // A->R
        way->idx = 11;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue bs"; // B->S
        way->idx = 12;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue bd"; // B->D
        way->idx = 13;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        way->add_house_number(navitia::georef::HouseNumber(D.lon(), D.lat(), 1));
        b.data->geo_ref->ways.push_back(way);

        // A->B
        add_edges(0, *b.data->geo_ref, AA, BB, distance_ab, navitia::type::Mode_e::Walking);
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

        // B->D
        add_edges(13, *b.data->geo_ref, BB, DD, B, D, navitia::type::Mode_e::Walking);
        add_edges(13, *b.data->geo_ref, BB, DD, B, D, navitia::type::Mode_e::Bike);
        add_edges(13, *b.data->geo_ref, BB, DD, B, D, navitia::type::Mode_e::Car);
        b.data->geo_ref->ways[13]->edges.emplace_back(BB, DD);
        b.data->geo_ref->ways[13]->edges.emplace_back(DD, BB);

        //and we add 2 bike sharing poi, to check the placemark
        navitia::georef::POIType* poi_type = new navitia::georef::POIType();
        poi_type->uri = "poi_type:amenity:bicycle_rental";
        poi_type->name = "bicycle rental station";
        poi_type->idx = 0;
        b.data->geo_ref->poitypes.push_back(poi_type);

        navitia::georef::POIType* poi_type_parking = new navitia::georef::POIType();
        poi_type_parking->uri = "poi_type:amenity:parking";
        poi_type_parking->name = "parking";
        poi_type_parking->idx = 1;
        b.data->geo_ref->poitypes.push_back(poi_type_parking);

        navitia::georef::POI* poi_1 = new navitia::georef::POI();
        poi_1->uri = "poi:station_1";
        poi_1->name = "first station";
        poi_1->coord = B;
        poi_1->poitype_idx = 0;
        poi_1->idx = 0;
        poi_1->admin_list.push_back(admin);
        navitia::georef::POI* poi_2 = new navitia::georef::POI();
        poi_2->uri = "poi:station_2";
        poi_2->name = "second station";
        poi_2->coord = G;
        poi_2->poitype_idx = 0;
        poi_2->idx = 1;
        poi_2->admin_list.push_back(admin);
        navitia::georef::POI* poi_3 = new navitia::georef::POI();
        poi_3->uri = "poi:parking_1";
        poi_3->name = "first parking";
        poi_3->coord = D;
        poi_3->poitype_idx = 1;
        poi_3->idx = 2;
        poi_3->admin_list.push_back(admin);
        navitia::georef::POI* poi_4 = new navitia::georef::POI();
        poi_4->uri = "poi:parking_2";
        poi_4->name = "second parking";
        poi_4->coord = E;
        poi_4->poitype_idx = 1;
        poi_4->idx = 3;
        poi_4->admin_list.push_back(admin);

        b.data->geo_ref->pois.push_back(poi_1);
        b.data->geo_ref->pois.push_back(poi_2);
        b.data->geo_ref->pois.push_back(poi_3);
        b.data->geo_ref->pois.push_back(poi_4);


        b.generate_dummy_basis();
        b.sa("stopA", A.lon(), A.lat())
            ("stop_point:uselessA", A.lon(), A.lat());
        b.sa("stopB", B.lon(), B.lat());
        if (activate_pt) {
            //we add a very fast bus (2 seconds) to be faster than walking and biking
            b.vj("A", "111111", "", false, "vjA")("stop_point:stopB", "08:01"_t)("stop_point:stopA", "08:01:02"_t)
                .st_shape({B, I, A});
            b.lines["A"]->code = "1A";
            b.lines["A"]->color = "289728";
            b.lines["A"]->text_color = "FFD700";
            b.data->pt_data->headsign_handler.affect_headsign_to_stop_time(
                                b.data->pt_data->vehicle_journeys.at(0)->stop_time_list.at(0), "A00");

            b.vj("M", "111111", "", false, "vjM")("stop_point:stopB", "08:01:01"_t)("stop_point:stopA", "08:01:03"_t)
                                        .st_shape({B, I, A});
			b.lines["M"]->code = "1M";
			b.lines["M"]->color = "3ACCDC";
			b.lines["M"]->text_color = "FFFFFF";
			//b.data->pt_data->headsign_handler.affect_headsign_to_stop_time(b.data->pt_data->vehicle_journeys.at(0)->stop_time_list.at(0), "A00");

            //We need another route on the line A with a vj on it to test line sections disruptions
            b.vj("A", "000000", "", false, "vjA2").route("route2")
                ("stop_point:stopB", "22:01"_t)
                ("stop_point:stopA", "23:01:02"_t)
                .st_shape({B, I, A});

            //add another bus, much later. we'll use that one for disruptions
            b.vj("B", "111111", "", true, "vjB")("stop_point:stopB", "18:01"_t)("stop_point:stopA", "18:01:02"_t)
                .st_shape({B, I, A});
            b.lines["B"]->code = "1B";

            b.vj("C")("stop_point:stopA", "08:01"_t)("stop_point:stopB", "08:01:02"_t)
                .st_shape({A, I, B});
            b.lines["C"]->code = "1C";

            auto zone_id = std::numeric_limits<uint16_t>::max();
            b.vj("C")
                ("stop_point:stopA", "23:00:00"_t, "23:02:00"_t, zone_id, false, true, 0, 1800)
                ("stop_point:stopB", "23:05:00"_t, "23:07:00"_t, zone_id, true, false, 1800, 0);

            //we add another stop not used in the routing tests, but used for ptref tests

            b.sa("stopC", J.lon(), J.lat());
            //and we add some one vj from a to D with a metro
            auto& builder_vj = b.vj("D")("stop_point:stopA", "08:01"_t)("stop_point:stopC", "08:01:02"_t)
                .st_shape({A, K, J});
            b.lines["D"]->code = "1D";
            builder_vj.vj->physical_mode = b.data->pt_data->physical_modes[1]; // Metro
            assert (builder_vj.vj->physical_mode->name == "Metro");
            builder_vj.vj->physical_mode->vehicle_journey_list.push_back(builder_vj.vj);

            for(auto r : b.data->pt_data->routes){
                r->destination = b.sas.find("stopA")->second;
            }
        }

        b.data->complete();
        b.manage_admin();
        b.finish();
        b.data->build_uri();
        b.data->pt_data->index();
        b.data->build_raptor();

        b.data->build_proximity_list();
        b.data->meta->production_date = boost::gregorian::date_period("20120614"_d, 365_days);
        b.data->compute_labels();

        //add bike sharing edges
        b.data->geo_ref->default_time_bss_pickup = 30_s;
        b.data->geo_ref->default_time_bss_putback = 40_s;
        b.data->geo_ref->add_bss_edges(B);
        b.data->geo_ref->add_bss_edges(G);

        // add parkings
        b.data->geo_ref->default_time_parking_park = 1_s;
        b.data->geo_ref->default_time_parking_leave = 2_s;
        b.data->geo_ref->add_parking_edges(D);
        b.data->geo_ref->add_parking_edges(E);

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

        //we give a rough shape (a square)
        std::stringstream ss;
        ss << "POLYGON((" << S.lon() - 1 << " " << S.lat() - 1
                  << ", " << S.lon() - 1 << " " << R.lat() + 1e-3
                  << ", " << R.lon() + 1e-3 << " " << R.lat() + 1e-3
                  << ", " << R.lon() + 1e-3 << " " << S.lat() - 1
                  << ", " << S.lon() - 1 << " " << S.lat() - 1 << "))";
        b.data->meta->shape = ss.str();
        if (activate_pt) {
            add_disruptions();
        }
    }

    navitia::time_duration to_duration(float dist, navitia::type::Mode_e mode) {
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

    const navitia::flat_enum_map<nt::Mode_e, float> get_default_speed() const { return speed_trait.get_default_speed(); }


    void add_disruptions() {
        nt::disruption::DisruptionHolder& holder = b.data->pt_data->disruption_holder;
        auto default_date = "20120801T000000"_dt;
        using btp = boost::posix_time::time_period;
        auto default_period = btp(default_date, "20120901T120000"_dt);

        auto info_severity = boost::make_shared<Severity>();
        info_severity->uri = "info";
        info_severity->wording = "information severity";
        info_severity->color = "#FFFF00";
        info_severity->priority = 25;
        holder.severities[info_severity->uri] = info_severity;

        auto bad_severity = boost::make_shared<Severity>();
        bad_severity->uri = "disruption";
        bad_severity->wording = "bad severity";
        bad_severity->color = "#FFFFF0";
        bad_severity->priority = 0;
        holder.severities[bad_severity->uri] = bad_severity;

        auto foo_severity = boost::make_shared<Severity>();
        foo_severity->uri = "foo";
        foo_severity->wording = "foo severity";
        foo_severity->color = "#FFFFF0";
        foo_severity->priority = 50;
        holder.severities[foo_severity->uri] = foo_severity;

        std::vector<Property> properties;

        Property property1;
        property1.key = "foo";
        property1.type = "bar";
        property1.value = "42";
        properties.push_back(property1);

        Property property2;
        property2.key = "foo";
        property2.type = "bar";
        property2.value = "42";
        properties.push_back(property2);

        Property property3;
        property3.key = "fo";
        property3.type = "obar";
        property3.value = "42";
        properties.push_back(property3);

        //we create one disruption on stop A
        b.disrupt(nt::RTLevel::Adapted, "disruption_on_stop_A")
                .publication_period(default_period)
                .tag("tag")
                .properties(properties)
                .impact()
                    .uri("too_bad")
                    .application_periods(default_period)
                    .severity("info")
                    .on(nt::Type_e::StopArea, "stopA")
                    .msg("no luck", nt::disruption::ChannelType::sms)
                    .msg("try again", nt::disruption::ChannelType::sms)
                    .publish(default_period);

        //we create one disruption on line A
        b.disrupt(nt::RTLevel::Adapted, "disruption_on_line_A")
                .publication_period(default_period)
                .contributor("contrib")
                .impact()
                    .uri("too_bad_again")
                    .application_periods(default_period)
                    .severity("disruption")
                    .on(nt::Type_e::Line, "A")
                    .on(nt::Type_e::Network, "base_network")
                    .msg("sad message", nt::disruption::ChannelType::sms)
                    .msg("too sad message", nt::disruption::ChannelType::sms);

        //we create another disruption on line A, but with
        //different date to test the period filtering
        b.disrupt(nt::RTLevel::Adapted, "disruption_on_line_A_but_later")
                .publication_period(default_period)
                .impact()
                    .uri("later_impact")
                    .application_periods(btp("20121001T000000"_dt, "20121015T120000"_dt))
                    .application_periods(btp("20121201T000000"_dt, "20121215T120000"_dt))
                    .severity("info")
                    .on(nt::Type_e::Line, "A")
                    .on(nt::Type_e::Network, "base_network")
                    .msg("sad message", nt::disruption::ChannelType::sms)
                    .msg("too sad message", nt::disruption::ChannelType::sms);


        //we create another disruption on line A, but not publish at the same date as the other ones
        //this one is published from the 28th
        b.disrupt(nt::RTLevel::Adapted, "disruption_on_line_A_but_publish_later")
                .publication_period(btp("20120828T120000"_dt, "20120901T120000"_dt))
                .impact()
                    .uri("impact_published_later")
                    .application_periods(default_period)
                    .severity("info")
                    .on(nt::Type_e::Line, "A")
                    //add another pt impacted object just to test with several
                    .on(nt::Type_e::Network, "base_network")
                    .msg("sad message", nt::disruption::ChannelType::sms)
                    .msg("too sad message", nt::disruption::ChannelType::sms);

        auto dis_proper_period = boost::posix_time::time_period("20120614T060000"_dt, "20120614T120000"_dt);
        b.disrupt(nt::RTLevel::Adapted, "disruption_all_lines_at_proper_time")
                .publication_period(dis_proper_period)
                .impact()
                    .uri("too_bad_all_lines")
                    .application_periods(dis_proper_period)
                    .severity("info")
                    .on(nt::Type_e::Line, "A")
                    .on(nt::Type_e::Line, "B")
                    .on(nt::Type_e::Line, "C")
                    .msg("no luck", nt::disruption::ChannelType::sms)
                    .msg("try again", nt::disruption::ChannelType::sms);

        auto route_period = boost::posix_time::time_period("20130226T060000"_dt, "20130228T120000"_dt);
        //we create one disruption on route A:0
        b.disrupt(nt::RTLevel::Adapted, "disruption_route_A:0")
                .publication_period(route_period)
                .impact()
                    .uri("too_bad_route_A:0")
                    .application_periods(route_period)
                    .severity("info")
                    .on(nt::Type_e::Route, "A:0")
                    .msg({"no luck", "sms", "sms", "content type", default_date, default_date,
                          {ChannelType::web, ChannelType::sms}})
                    .msg({"try again", "email", "email", "content type", default_date, default_date,
                          {ChannelType::web, ChannelType::email}});

        //we create one disruption on route A:0
        auto dis_maker_period = boost::posix_time::time_period("20130426T060000"_dt, "20130430T120000"_dt);
        auto disruption_maker = b.disrupt(nt::RTLevel::Adapted, "disruption_route_A:0_and_line")
                                          .publication_period(dis_maker_period);

        disruption_maker.impact()
                .uri("too_bad_route_A:0_and_line")
                .application_periods(dis_maker_period)
                .severity("info")
                .on(nt::Type_e::Route, "A:0")
                .on(nt::Type_e::Line, "A")
                .msg("no luck", nt::disruption::ChannelType::sms)
                .msg("try again", nt::disruption::ChannelType::sms)
                .msg({"beacon in channel", "beacon", "beacon channel", "content type", default_date, default_date,
                      {ChannelType::web, ChannelType::title, ChannelType::beacon}});

        disruption_maker.impact()
                .uri("too_bad_line_B")
                .application_periods(dis_maker_period)
                .severity("disruption")
                .on(nt::Type_e::Line, "B")
                .msg("try again", nt::disruption::ChannelType::sms);

        disruption_maker.impact()
                .uri("too_bad_line_C")
                .application_periods(dis_maker_period)
                .severity("foo")
                .on(nt::Type_e::Line, "C")
                .msg("try again", nt::disruption::ChannelType::sms);
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
    int DD = 12;

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
    navitia::type::GeographicalCoord D = {0, 30, false};

    navitia::type::GeographicalCoord HouseNmber42 = {10., 100., false};

    ed::builder b = {"20120614", "routing api data"};

    navitia::type::EntryPoint origin;
    navitia::type::EntryPoint destination;
    std::vector<uint64_t> datetimes = {navitia::test::to_posix_timestamp("20120614T080000")};
    std::vector<std::string> forbidden;

    double distance_ab;
    double distance_ag;

    speed_provider_trait speed_trait;
};
