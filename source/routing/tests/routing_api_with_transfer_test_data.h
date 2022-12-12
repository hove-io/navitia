/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
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
#include "fare/fare.h"

namespace ng = navitia::georef;

using navitia::type::disruption::ChannelType;
using navitia::type::disruption::Disruption;
using navitia::type::disruption::Impact;
using navitia::type::disruption::Property;
using navitia::type::disruption::Severity;
using navitia::type::disruption::Tag;

struct test_speed_provider {
    const navitia::flat_enum_map<nt::Mode_e, float> get_default_speed() const { return test_default_speed; }

    // to ease test we use easier speed
    const navitia::flat_enum_map<nt::Mode_e, float> test_default_speed{{{
        1,  // nt::Mode_e::Walking
        2,  // nt::Mode_e::Bike
        5,  // nt::Mode_e::Car
        2,  // nt::Mode_e::Vls
        5   // nt::Mode_e::CarNoPark
    }}};
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
    routing_api_data(bool activate_pt = true) {
        /*


      A -----------------------------B---C
                                         |
                                         D---E----------------F
                                         |
                                         G

                    *) A->B has public transport
                    *) B->C->D->E are pedestrian streets
                    *) E->F has public transport

                    Coordinates:
                                A(120,  80)       0
                                B(100,  80)       1
                                C(100, 100)       2
                                D( 70, 100)       3
                                E( 70, 120)       4
                                F( 70, 120)       5
                                G( 50, 100)       6
        */

        boost::add_vertex(navitia::georef::Vertex(A), b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(B), b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(C), b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(D), b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(E), b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(F), b.data->geo_ref->graph);
        boost::add_vertex(navitia::georef::Vertex(G), b.data->geo_ref->graph);

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
        way->name = "rue ab";  // A->B
        way->idx = 0;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue bc";  // B->C
        way->idx = 1;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue cd";  // C->D
        way->idx = 2;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue de";  // D->E
        way->idx = 3;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue ef";  // E->F
        way->idx = 4;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        way = new navitia::georef::Way();
        way->name = "rue dg";  // E->F
        way->idx = 5;
        way->way_type = "rue";
        way->admin_list.push_back(admin);
        b.data->geo_ref->ways.push_back(way);

        // B->C
        add_edges(1, *b.data->geo_ref, CC, BB, C, B, navitia::type::Mode_e::Walking);
        b.data->geo_ref->ways[1]->edges.emplace_back(CC, BB);
        b.data->geo_ref->ways[1]->edges.emplace_back(BB, CC);

        // C->D
        add_edges(2, *b.data->geo_ref, CC, DD, C, D, navitia::type::Mode_e::Walking);
        b.data->geo_ref->ways[2]->edges.emplace_back(CC, DD);
        b.data->geo_ref->ways[2]->edges.emplace_back(DD, CC);

        // D->E
        add_edges(3, *b.data->geo_ref, EE, DD, E, D, navitia::type::Mode_e::Walking);
        b.data->geo_ref->ways[3]->edges.emplace_back(EE, DD);
        b.data->geo_ref->ways[3]->edges.emplace_back(DD, EE);

        // D->G
        add_edges(3, *b.data->geo_ref, DD, GG, D, G, navitia::type::Mode_e::Walking);
        b.data->geo_ref->ways[3]->edges.emplace_back(DD, GG);
        b.data->geo_ref->ways[3]->edges.emplace_back(GG, DD);

        b.generate_dummy_basis();
        b.sa("stopA", A.lon(), A.lat())("stop_point:stopA", A.lon(), A.lat());
        b.sa("stopB", B.lon(), B.lat())("stop_point:stopB", B.lon(), B.lat());
        b.sa("stopE", E.lon(), E.lat())("stop_point:stopE", E.lon(), E.lat());
        b.sa("stopF", F.lon(), F.lat())("stop_point:stopF", F.lon(), F.lat());

        if (activate_pt) {
            auto& builder_vj_FE =
                b.vj("FE")("stop_point:stopF", "10:01:01"_t)("stop_point:stopE", "10:01:02"_t).st_shape({F, E});
            b.lines["FE"]->code = "1FE";
            builder_vj_FE.vj->physical_mode = b.data->pt_data->physical_modes[4];  // Bus

            auto& builder_vj_BA =
                b.vj("BA")("stop_point:stopB", "10:10:01"_t)("stop_point:stopA", "10:11:02"_t).st_shape({B, A});
            b.lines["BA"]->code = "1BA";
            builder_vj_BA.vj->physical_mode = b.data->pt_data->physical_modes[6];  // Coach

            auto& builder_vj_BAbis =
                b.vj("BAbis")("stop_point:stopB", "10:10:00"_t)("stop_point:stopA", "10:11:03"_t).st_shape({B, A});
            b.lines["BAbis"]->code = "1BAbis";
            builder_vj_BAbis.vj->physical_mode = b.data->pt_data->physical_modes[1];  // Metro

            b.connection("stop_point:stopE", "stop_point:stopB", 120);

            auto& builder_vj_AB =
                b.vj("AB")("stop_point:stopA", "08:00:00"_t)("stop_point:stopB", "8:10:00"_t).st_shape({A, B});
            b.lines["AB"]->code = "1AB";
            builder_vj_AB.vj->physical_mode = b.data->pt_data->physical_modes[4];  // Bus

            auto& builder_vj_EF =
                b.vj("EF")("stop_point:stopE", "08:20:00"_t)("stop_point:stopF", "8:30:00"_t).st_shape({E, F});
            b.lines["EF"]->code = "1EF";
            builder_vj_EF.vj->physical_mode = b.data->pt_data->physical_modes[7];  // RER

            b.connection("stop_point:stopB", "stop_point:stopE", 120);
        }

        b.data->complete();
        b.manage_admin();
        b.make();

        // Add access points
        b.add_access_point("stop_point:stopE", "access_point:E1", true, false, 100, 100, G.lat(), G.lon());
        b.add_access_point("stop_point:stopE", "access_point:E2", true, true, 120, 120, G.lat(), G.lon());
        b.add_access_point("stop_point:stopE", "access_point:E3", false, true, 90, 90, G.lat(), G.lon());

        // Add a main stop area to our admin

        b.data->build_proximity_list();
        b.data->meta->production_date = boost::gregorian::date_period("20120614"_d, 365_days);
        b.data->compute_labels();
        b.data->fill_stop_point_address();

        std::string origin_lon = boost::lexical_cast<std::string>(A.lon()),
                    origin_lat = boost::lexical_cast<std::string>(A.lat()),
                    origin_uri = "coord:" + origin_lon + ":" + origin_lat;
        navitia::type::Type_e origin_type = b.data->get_type_of_id(origin_uri);
        origin = {origin_type, origin_uri};

        std::string destination_lon = boost::lexical_cast<std::string>(F.lon()),
                    destination_lat = boost::lexical_cast<std::string>(F.lat()),
                    destination_uri = "coord:" + destination_lon + ":" + destination_lat;
        navitia::type::Type_e destination_type = b.data->get_type_of_id(destination_uri);
        destination = {destination_type, destination_uri};

        // we give a rough shape (a square)
        std::stringstream ss;
        ss << "POLYGON((" << F.lon() - 1 << " " << F.lat() - 1 << ", " << F.lon() - 1 << " " << A.lat() + 1e-3 << ", "
           << A.lon() + 1e-3 << " " << A.lat() + 1e-3 << ", " << A.lon() + 1e-3 << " " << F.lat() - 1 << ", "
           << F.lon() - 1 << " " << F.lat() - 1 << "))";
        b.data->meta->shape = ss.str();
    }

    navitia::time_duration to_duration(float dist, navitia::type::Mode_e mode) {
        return speed_trait.to_duration(dist, mode);
    }

    void add_edges(int edge_idx,
                   navitia::georef::GeoRef& geo_ref,
                   int idx_from,
                   int idx_to,
                   float dist,
                   navitia::type::Mode_e mode) {
        boost::add_edge(idx_from + geo_ref.offsets[mode], idx_to + geo_ref.offsets[mode],
                        navitia::georef::Edge(edge_idx, to_duration(dist, mode)), geo_ref.graph);
        boost::add_edge(idx_to + geo_ref.offsets[mode], idx_from + geo_ref.offsets[mode],
                        navitia::georef::Edge(edge_idx, to_duration(dist, mode)), geo_ref.graph);
    }
    void add_edges(int edge_idx,
                   navitia::georef::GeoRef& geo_ref,
                   int idx_from,
                   int idx_to,
                   const navitia::type::GeographicalCoord& a,
                   const navitia::type::GeographicalCoord& b,
                   navitia::type::Mode_e mode) {
        add_edges(edge_idx, geo_ref, idx_from, idx_to, a.distance_to(b), mode);
    }

    const navitia::flat_enum_map<nt::Mode_e, float> get_default_speed() const {
        return speed_trait.get_default_speed();
    }

    int AA = 0;
    int BB = 1;
    int CC = 2;
    int DD = 3;
    int EE = 4;
    int FF = 5;
    int GG = 6;

    navitia::type::GeographicalCoord A = {120, 2000, false};
    navitia::type::GeographicalCoord B = {1990, 2000, false};
    navitia::type::GeographicalCoord C = {2000, 2000, false};
    navitia::type::GeographicalCoord D = {2000, 1900, false};
    navitia::type::GeographicalCoord E = {2010, 1900, false};
    navitia::type::GeographicalCoord F = {3000, 1900, false};
    navitia::type::GeographicalCoord G = {2000, 1800, false};

    ed::builder b = {"20120614", ed::builder::make_builder, true, "routing api data"};

    navitia::type::EntryPoint origin;
    navitia::type::EntryPoint destination;
    std::vector<uint64_t> datetimes = {navitia::test::to_posix_timestamp("20120614T080000")};
    std::vector<std::string> forbidden;

    speed_provider_trait speed_trait;
};
