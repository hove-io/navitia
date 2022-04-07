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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE heat_map_test

#include "type/geographical_coord.h"
#include "routing/isochrone.h"
#include "ed/build_helper.h"
#include "routing/raptor.h"
#include "routing/routing.h"
#include "tests/utils_test.h"
#include "routing/heat_map.h"
#include "utils/init.h"
#include "routing/tests/routing_api_test_data.h"
#include "utils/logger.h"

#include <boost/test/unit_test.hpp>
#include <iomanip>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <iostream>

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

using namespace navitia::routing;

BOOST_AUTO_TEST_CASE(print_map_test) {
    /*
     * lat    4     5      6
     * lon
     *  1    0min  1min  2min
     *  2    3min  4min  5min
     *  3    6min  7min  infini
     *
     */
    std::vector<SingleCoord> header;
    std::vector<std::pair<SingleCoord, std::vector<navitia::time_duration>>> body;
    int length = 3;
    for (int i = 0; i < length; i++) {
        header.push_back((SingleCoord(i + length + 1, 1)));
        std::vector<navitia::time_duration> local_duration;
        auto lon = SingleCoord(i, 1);
        for (int j = 0; j < length; j++) {
            local_duration.push_back(navitia::minutes(j + i * length));
        }
        auto local_body = std::make_pair(lon, local_duration);
        body.push_back(std::move(local_body));
    }
    auto heat_map = HeatMap(header, body);
    heat_map.body[2].second[2] = bt::pos_infin;
    const auto heat_map_string = R"({"line_headers":[{"cell_lat":{"min_lat":4,"center_lat":4.5,"max_lat":5}},)"
                                 R"({"cell_lat":{"min_lat":5,"center_lat":5.5,"max_lat":6}},)"
                                 R"({"cell_lat":{"min_lat":6,"center_lat":6.5,"max_lat":7}}],)"
                                 R"("lines":[{"cell_lon":{"min_lon":0,"center_lon":0.5,"max_lon":1},)"
                                 R"("duration":[0,60,120]},)"
                                 R"({"cell_lon":{"min_lon":1,"center_lon":1.5,"max_lon":2},)"
                                 R"("duration":[180,240,300]},)"
                                 R"({"cell_lon":{"min_lon":2,"center_lon":2.5,"max_lon":3},)"
                                 R"("duration":[360,420,null]}]})";
    BOOST_CHECK(heat_map_string == print_grid(heat_map));
}

BOOST_AUTO_TEST_CASE(heat_map_test) {
    /*
     *
     * A
     * ^ \
     * |  \
     * |   \<----A->C is a country road, sooooo slow
     * |    \
     * |     v
     * B----->C----------------->D
     *    ^
     *    |
     *  B->C and B->A are hyperloop roads, transport is really fast
     *
     * We want to go from near A to D, we want A->C->D even if A->B->C->D would be faster
     *
     */
    navitia::type::GeographicalCoord A = {0, 100, false};
    navitia::type::GeographicalCoord B = {0, 0, false};
    navitia::type::GeographicalCoord C = {100, 0, false};
    navitia::type::GeographicalCoord D = {200, 0, false};
    navitia::type::GeographicalCoord E = {100, 50, false};

    size_t AA = 0;
    size_t BB = 1;
    size_t CC = 2;
    size_t DD = 3;

    ed::builder b = {"20120614"};

    boost::add_vertex(ng::Vertex(A), b.data->geo_ref->graph);
    boost::add_vertex(ng::Vertex(B), b.data->geo_ref->graph);
    boost::add_vertex(ng::Vertex(C), b.data->geo_ref->graph);
    boost::add_vertex(ng::Vertex(D), b.data->geo_ref->graph);

    size_t way_idx = 0;
    for (const auto& name : {"ab", "bc", "ac", "cd"}) {
        auto* way = new ng::Way();
        way->name = "rue " + std::string(name);
        way->idx = way_idx++;
        way->way_type = "rue";
        b.data->geo_ref->ways.push_back(way);
    }

    size_t e_idx(0);
    // we add each edge as a one way street
    boost::add_edge(BB, AA, ng::Edge(e_idx++, navitia::seconds(10)), b.data->geo_ref->graph);
    // B->C is very cheap but will not be used
    boost::add_edge(BB, CC, ng::Edge(e_idx++, navitia::seconds(1)), b.data->geo_ref->graph);
    boost::add_edge(AA, CC, ng::Edge(e_idx++, navitia::seconds(1000)), b.data->geo_ref->graph);
    boost::add_edge(CC, DD, ng::Edge(e_idx++, navitia::seconds(10)), b.data->geo_ref->graph);

    b.data->geo_ref->ways[0]->edges.emplace_back(AA, BB);
    b.data->geo_ref->ways[1]->edges.emplace_back(BB, CC);
    b.data->geo_ref->ways[2]->edges.emplace_back(AA, CC);
    b.data->geo_ref->ways[3]->edges.emplace_back(CC, DD);

    b.data->geo_ref->init();
    b.data->geo_ref->build_proximity_list();

    b.data->build_proximity_list();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), 7_days);
    b.vj("A")("stop1", "08:00"_t)("stop2", "08:10"_t)("stop3", "08:20"_t);
    b.vj("B")("stop1", "08:00"_t)("stop4", "08:30"_t)("stop3", "09:00"_t);
    b.connection("stop1", "stop1", 120);
    b.connection("stop2", "stop2", 120);
    b.connection("stop3", "stop3", 120);
    b.connection("stop4", "stop4", 120);

    b.sps["stop1"]->coord = A;
    b.sps["stop2"]->coord = B;
    b.sps["stop3"]->coord = C;
    b.sps["stop4"]->coord = D;
    b.make();
    RAPTOR raptor(*b.data);
    navitia::routing::map_stop_point_duration d;
    d.emplace(navitia::routing::SpIdx(*b.sps["stop1"]), navitia::seconds(0));
    raptor.isochrone(d, navitia::DateTimeUtils::set(0, "08:00"_t), navitia::DateTimeUtils::set(0, "09:00"_t));
    const double speed = 0.8;
    const uint resolution = 100;
    auto stop_points = raptor.data.pt_data->stop_points;
    (*b.data->geo_ref).project_stop_points_and_access_points(stop_points);
    const auto max_duration = 36000;
    size_t step = 3;
    auto box = BoundBox();
    box.set_box(A, 0);
    box.set_box(D, 0);
    auto height_step = (A.lat() - D.lat()) / step;
    auto width_step = (A.lon() - D.lon()) / step;
    auto min_dist = std::max(height_step * N_DEG_TO_DISTANCE, width_step * N_DEG_TO_DISTANCE);
    min_dist = std::min(500., min_dist);
    auto mode = navitia::type::Mode_e::Walking;
    const auto bound = navitia::DateTimeUtils::set(0, "09:00"_t);
    const auto init_dt = navitia::DateTimeUtils::set(0, "07:00"_t);
    const auto isochrone = build_raster_isochrone(*b.data->geo_ref, speed, mode, init_dt, raptor, A, max_duration, true,
                                                  bound, resolution);
    BOOST_CHECK(isochrone.size() > 0);
    const auto header = R"({"line_headers":[{"cell_lat":)";
    std::size_t found_header = isochrone.find(header);
    BOOST_CHECK(found_header == 0);
    const auto body = R"(],"lines":[{"cell_lon":)";
    std::size_t found_body = isochrone.find(body);
    BOOST_CHECK(found_body != std::string::npos);
    auto distances = init_distance(*b.data->geo_ref, stop_points, init_dt, raptor, mode, E, true, bound, speed);
    auto heat_map =
        fill_heat_map(box, height_step, width_step, *b.data->geo_ref, min_dist, max_duration, speed, distances, step);
    std::vector<navitia::time_duration> result;
    for (size_t i = 0; i < step; i++) {
        for (size_t j = 0; j < step; j++) {
            result.push_back(heat_map.body[i].second[j]);
        }
    }
    BOOST_CHECK_EQUAL(result[0].total_seconds(), 244);
    BOOST_CHECK_EQUAL(result[1].total_seconds(), 207);
    BOOST_CHECK_EQUAL(result[2].total_seconds(), 178);
    for (size_t i = 3; i < result.size(); i++) {
        BOOST_CHECK(result[i].is_pos_infinity());
    }
}
