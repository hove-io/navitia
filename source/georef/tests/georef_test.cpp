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
#define BOOST_TEST_MODULE georef_test
#include <boost/test/unit_test.hpp>
#include <execinfo.h>
#include <iostream>
#include "utils/logger.h"

#include "georef/georef.h"
#include "tests/utils_test.h"
#include "type/data.h"
#include "builder.h"
#include "ed/build_helper.h"
#include "georef/street_network.h"
#include <boost/graph/detail/adjacency_list.hpp>

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

using namespace navitia::georef;
using namespace boost;

namespace {

std::vector<navitia::type::GeographicalCoord> get_coords_from_path(const Path& path) {
    std::vector<navitia::type::GeographicalCoord> res;
    for (const auto& item : path.path_items) {
        for (const auto& coord : item.coordinates) {
            res.push_back(coord);
        }
    }
    return res;
}

void print_coord(const std::vector<navitia::type::GeographicalCoord>& coord) {
    std::cout << " coord : " << std::endl;
    for (auto c : coord) {
        std::cout << " -- " << c.lon() / navitia::type::GeographicalCoord::N_M_TO_DEG << ", "
                  << c.lat() / navitia::type::GeographicalCoord::N_M_TO_DEG << std::endl;
    }
}

// Compute the path from the starting point to the the target geographical coord
Path compute_path(DijkstraPathFinder& finder, const navitia::type::GeographicalCoord& target_coord) {
    ProjectionData dest(target_coord, finder.geo_ref, finder.geo_ref.pl);

    auto best_pair = finder.update_path(dest);

    return finder.get_path(dest, best_pair);
}

// Compute the path from the starting point to the the target geographical coord
Path compute_path(AstarPathFinder& finder, const navitia::type::GeographicalCoord& target_coord) {
    ProjectionData dest(target_coord, finder.geo_ref, finder.geo_ref.pl);
    auto const max_dur = navitia::seconds(1000.);

    finder.start_distance_or_target_astar(max_dur, {dest[source_e], dest[target_e]});
    const auto dest_vertex = finder.find_nearest_vertex(dest, true);
    const auto res = finder.get_path(dest, dest_vertex);

    if (res.duration > max_dur) {
        return Path();
    }

    return res;
}

// Compute the path with astar and check if we have the same result
// Than a previously computed path with dijkstra
void check_has_astar_same_result_than_dijkstra(AstarPathFinder& finder,
                                         const navitia::type::GeographicalCoord& start_coord,
                                         const navitia::type::GeographicalCoord& dest_coord,
                                         navitia::type::Mode_e mode,
                                         float speed_factor,
                                         const Path& dijkstra_path) {
    finder.init(start_coord, dest_coord, mode, speed_factor);
    auto const astar_path = compute_path(finder, dest_coord);
    BOOST_REQUIRE_EQUAL(dijkstra_path.duration, astar_path.duration);

    auto const& d_path = dijkstra_path.path_items;
    auto const& a_path = astar_path.path_items;
    BOOST_CHECK_EQUAL_COLLECTIONS(d_path[0].coordinates.begin(), d_path[0].coordinates.end(),
                                  a_path[0].coordinates.begin(), a_path[0].coordinates.end());
}

}  // namespace

BOOST_AUTO_TEST_CASE(init_test) {
    using namespace navitia::type;
    GraphBuilder b;
    auto* w = new Way;
    w->name = "Jaures";
    b.geo_ref.ways.push_back(w);
    w = new Way;
    w->name = "Hugo";
    b.geo_ref.ways.push_back(w);

    b("a", 0, 0)("b", 1, 1)("c", 2, 2)("d", 3, 3)("e", 4, 4);
    b("a", "b")("b", "c")("c", "d")("d", "e")("e",
                                              "d");  // bug ? if no edge leave the vertex, the projection cannot work...
    b.geo_ref.graph[b.get("a", "b")].way_idx = 0;
    b.geo_ref.graph[b.get("b", "c")].way_idx = 0;
    b.geo_ref.graph[b.get("c", "d")].way_idx = 1;
    b.geo_ref.graph[b.get("d", "e")].way_idx = 1;
    b.geo_ref.graph[b.get("e", "d")].way_idx = 1;

    BOOST_CHECK_EQUAL(boost::num_vertices(b.geo_ref.graph), 5);

    b.geo_ref.init();

    BOOST_CHECK_EQUAL(boost::num_vertices(b.geo_ref.graph), 15);  // one graph for each transportation mode save VLS

    BOOST_CHECK_EQUAL(b.geo_ref.offsets[Mode_e::Walking], 0);
    BOOST_CHECK_EQUAL(b.geo_ref.offsets[Mode_e::Bike], 5);
    BOOST_CHECK_EQUAL(b.geo_ref.offsets[Mode_e::Car], 10);
    BOOST_CHECK_EQUAL(b.geo_ref.offsets[Mode_e::Bss], 0);
}

BOOST_AUTO_TEST_CASE(outil_de_graph) {
    GraphBuilder builder;
    Graph& g = builder.geo_ref.graph;

    BOOST_CHECK_EQUAL(num_vertices(g), 0);
    BOOST_CHECK_EQUAL(num_edges(g), 0);

    // Construction de deux nœuds et d'un arc
    builder("a", 0, 0)("b", 1, 2)("a", "b", 10_s);
    BOOST_CHECK_EQUAL(num_vertices(g), 2);
    BOOST_CHECK_EQUAL(num_edges(g), 1);

    // On vérifie que les propriétés sont bien définies
    vertex_t a = builder.vertex_map["a"];
    BOOST_CHECK_EQUAL(g[a].coord.lon(), 0);
    BOOST_CHECK_EQUAL(g[a].coord.lat(), 0);

    vertex_t b = builder.vertex_map["b"];
    navitia::type::GeographicalCoord expected;
    expected.set_xy(1, 2);
    BOOST_CHECK_EQUAL(g[b].coord, expected);

    edge_t e = edge(a, b, g).first;
    BOOST_CHECK_EQUAL(g[e].duration, navitia::seconds(10));

    // Construction implicite de nœuds
    builder("c", "d", navitia::seconds(42));
    BOOST_CHECK_EQUAL(num_vertices(g), 4);
    BOOST_CHECK_EQUAL(num_edges(g), 2);

    builder("a", "c");
    BOOST_CHECK_EQUAL(num_vertices(g), 4);
    BOOST_CHECK_EQUAL(num_edges(g), 3);
}

BOOST_AUTO_TEST_CASE(nearest_segment) {
    GraphBuilder b;

    /*               a           e
                     |
                  b—–o––c
                     |
                     d             */

    b("a", 0, 10)("b", -10, 0)("c", 10, 0)("d", 0, -10)("o", 0, 0)("e", 50, 10);
    b("o", "a")("o", "b")("o", "c")("o", "d")("b", "o");
    b.geo_ref.init();

    navitia::type::GeographicalCoord c(1, 2, false);
    BOOST_CHECK(b.geo_ref.nearest_edge(c) == b.get("o", "a"));
    c.set_xy(2, 1);
    BOOST_CHECK(b.geo_ref.nearest_edge(c) == b.get("o", "c"));
    c.set_xy(2, -1);
    BOOST_CHECK(b.geo_ref.nearest_edge(c) == b.get("o", "c"));
    c.set_xy(2, -3);
    BOOST_CHECK(b.geo_ref.nearest_edge(c) == b.get("o", "d"));
    c.set_xy(-10, 1);
    BOOST_CHECK(b.geo_ref.nearest_edge(c) == b.get("b", "o"));
    c.set_xy(50, 10);
    BOOST_CHECK_EQUAL(b.geo_ref.nearest_edge(c), b.get("o", "c"));
}

BOOST_AUTO_TEST_CASE(real_nearest_edge) {
    GraphBuilder b;

    /*        s
       a---------------b
              c-d
    */
    b("a", 0, -100)("b", 0, 100)("c", 10, 0)("d", 10, 10);
    b("a", "b")("c", "d");
    b.geo_ref.init();

    navitia::type::GeographicalCoord s(-10, 0, false);
    BOOST_CHECK(b.geo_ref.nearest_edge(s) == b.get("a", "b"));
}

/*
 * We have this graph
 *
 * Without geometries :
 *                                     x5
 *  * 30           e ---------- d
 *  |             /             |
 *  |            /              |
 *  |           /               |
 *  |          /    x1          |
 *  |         b                 |
 *  |        /                  | x2
 *  |       /                   |
 *  |      /                    |
 *  |     /                     |
 *  |    /                      |
 *  |   a ----x4----------------c
 *  |                x3
 * 0,0 ------------------------------------------ 50
 *
 *  *
 * With geometries :
 *                                     x5
 *  * 30      .--- e ---------- d -----------.
 *  |        /                                \
 *  |       /                                  \
 *  |      /                                    \
 *  |     /         x1   .-----.                 \
 *  |    '---- b         |     |                  )
 *  |          |         |     | x2              /
 *  |          |         |     |                /
 *  |          |         |     |               /
 *  |          |         |     |              /
 *  |   .------'         |     |             /
 *  |   a --.  x4        |     c -----------'
 *  |       '--------x3--'
 * 0,0 ------------------------------------------ 50
 */
BOOST_AUTO_TEST_CASE(nearest_edge_with_geometries) {
    GraphBuilder b;
    b("a", 5, 5)("b", 15, 20)("c", 30, 5)("d", 30, 30)("e", 20, 30);
    b("a", "b")("a", "c")("b", "e")("c", "d")("d", "e");

    nt::GeographicalCoord x1(23, 23, false);
    nt::GeographicalCoord x2(37, 17, false);
    nt::GeographicalCoord x3(23, 2, false);
    nt::GeographicalCoord x4(15, 5, false);
    nt::GeographicalCoord x5(40, 32, false);
    b.geo_ref.init();

    BOOST_CHECK(b.geo_ref.nearest_edge(x1) == b.get("b", "e"));
    BOOST_CHECK(b.geo_ref.nearest_edge(x2) == b.get("c", "d"));
    BOOST_CHECK(b.geo_ref.nearest_edge(x3) == b.get("a", "c"));
    BOOST_CHECK(b.geo_ref.nearest_edge(x4) == b.get("a", "c"));
    BOOST_CHECK(b.geo_ref.nearest_edge(x5) == b.get("d", "e"));

    nt::LineString geom;
    geom.push_back(nt::GeographicalCoord(5, 5, false));
    geom.push_back(nt::GeographicalCoord(5, 7, false));
    geom.push_back(nt::GeographicalCoord(15, 7, false));
    geom.push_back(nt::GeographicalCoord(15, 20, false));
    b.add_geom(b.get("a", "b"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(5, 5, false));
    geom.push_back(nt::GeographicalCoord(8, 5, false));
    geom.push_back(nt::GeographicalCoord(8, 2, false));
    geom.push_back(nt::GeographicalCoord(27, 2, false));
    geom.push_back(nt::GeographicalCoord(27, 23, false));
    geom.push_back(nt::GeographicalCoord(30, 23, false));
    geom.push_back(nt::GeographicalCoord(30, 5, false));
    b.add_geom(b.get("a", "c"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(15, 20, false));
    geom.push_back(nt::GeographicalCoord(8, 20, false));
    geom.push_back(nt::GeographicalCoord(15, 30, false));
    geom.push_back(nt::GeographicalCoord(20, 30, false));
    b.add_geom(b.get("b", "e"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(30, 5, false));
    geom.push_back(nt::GeographicalCoord(42, 5, false));
    geom.push_back(nt::GeographicalCoord(50, 20, false));
    geom.push_back(nt::GeographicalCoord(45, 30, false));
    geom.push_back(nt::GeographicalCoord(30, 30, false));
    b.add_geom(b.get("c", "d"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(30, 30, false));
    geom.push_back(nt::GeographicalCoord(20, 30, false));
    b.add_geom(b.get("d", "e"), geom);

    BOOST_CHECK(b.geo_ref.nearest_edge(x1) == b.get("a", "c"));
    BOOST_CHECK(b.geo_ref.nearest_edge(x2) == b.get("a", "c"));
    BOOST_CHECK(b.geo_ref.nearest_edge(x3) == b.get("a", "c"));
    BOOST_CHECK(b.geo_ref.nearest_edge(x4) == b.get("a", "b"));
    BOOST_CHECK(b.geo_ref.nearest_edge(x5) == b.get("c", "d"));
}

// We are using the same graph that above, with bidirectionnal edges
BOOST_AUTO_TEST_CASE(accurate_path_geometries) {
    GraphBuilder b;
    b("a", 5, 5)("b", 15, 20)("c", 30, 5)("d", 30, 30)("e", 20, 30);
    b("a", "b", 200_s, true)("a", "c", 200_s, true)("b", "e", 160_s, true)("c", "d", 100_s, true)("d", "e", 400_s,
                                                                                                  true);

    nt::GeographicalCoord x1(23, 23, false);
    nt::GeographicalCoord x2(37, 17, false);
    nt::GeographicalCoord x3(23, 2, false);
    nt::GeographicalCoord x4(15, 5, false);
    nt::GeographicalCoord x5(40, 32, false);

    nt::LineString geom;
    geom.push_back(nt::GeographicalCoord(5, 5, false));
    geom.push_back(nt::GeographicalCoord(5, 7, false));
    geom.push_back(nt::GeographicalCoord(15, 7, false));
    geom.push_back(nt::GeographicalCoord(15, 20, false));
    b.add_geom(b.get("a", "b"), geom);
    std::reverse(geom.begin(), geom.end());
    b.add_geom(b.get("b", "a"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(5, 5, false));
    geom.push_back(nt::GeographicalCoord(8, 5, false));
    geom.push_back(nt::GeographicalCoord(8, 2, false));
    geom.push_back(nt::GeographicalCoord(27, 2, false));
    geom.push_back(nt::GeographicalCoord(27, 23, false));
    geom.push_back(nt::GeographicalCoord(30, 23, false));
    geom.push_back(nt::GeographicalCoord(30, 5, false));
    b.add_geom(b.get("a", "c"), geom);
    std::reverse(geom.begin(), geom.end());
    b.add_geom(b.get("c", "a"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(15, 20, false));
    geom.push_back(nt::GeographicalCoord(8, 20, false));
    geom.push_back(nt::GeographicalCoord(15, 30, false));
    geom.push_back(nt::GeographicalCoord(20, 30, false));
    b.add_geom(b.get("b", "e"), geom);
    std::reverse(geom.begin(), geom.end());
    b.add_geom(b.get("e", "b"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(30, 5, false));
    geom.push_back(nt::GeographicalCoord(42, 5, false));
    geom.push_back(nt::GeographicalCoord(50, 20, false));
    geom.push_back(nt::GeographicalCoord(45, 30, false));
    geom.push_back(nt::GeographicalCoord(30, 30, false));
    b.add_geom(b.get("c", "d"), geom);
    std::reverse(geom.begin(), geom.end());
    b.add_geom(b.get("d", "c"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(30, 30, false));
    geom.push_back(nt::GeographicalCoord(20, 30, false));
    b.add_geom(b.get("d", "e"), geom);
    std::reverse(geom.begin(), geom.end());
    b.add_geom(b.get("e", "d"), geom);

    b.geo_ref.init();
    DijkstraPathFinder djikstra_path_finder(b.geo_ref);
    AstarPathFinder astar_path_finder(b.geo_ref);

    nt::LineString expectedGeom;
    expectedGeom.push_back(nt::GeographicalCoord(30, 17, false));
    expectedGeom.push_back(nt::GeographicalCoord(30, 23, false));
    expectedGeom.push_back(nt::GeographicalCoord(27, 23, false));
    expectedGeom.push_back(nt::GeographicalCoord(27, 2, false));
    expectedGeom.push_back(nt::GeographicalCoord(23, 2, false));
    // Computing path from x2 to x3. Both of them should be projected on the edge (a,c)
    djikstra_path_finder.init(x2, nt::Mode_e::Walking, 1);  // starting from x2
    Path p = compute_path(djikstra_path_finder, x3);        // going to x3
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[0].coordinates.begin(),
                                  p.path_items[0].coordinates.end());
    check_has_astar_same_result_than_dijkstra(astar_path_finder, x2, x3, nt::Mode_e::Walking, 1, p);

    // Same thing with a reverse geometry
    std::reverse(expectedGeom.begin(), expectedGeom.end());
    djikstra_path_finder.init(x3, nt::Mode_e::Walking, 1);  // starting from x3
    p = compute_path(djikstra_path_finder, x2);             // going to x2
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[0].coordinates.begin(),
                                  p.path_items[0].coordinates.end());
    check_has_astar_same_result_than_dijkstra(astar_path_finder, x3, x2, nt::Mode_e::Walking, 1, p);

    djikstra_path_finder.init(x4, nt::Mode_e::Walking, 1);  // starting from x4
    p = compute_path(djikstra_path_finder, x5);             // going to x5
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 3);
    check_has_astar_same_result_than_dijkstra(astar_path_finder, x4, x5, nt::Mode_e::Walking, 1, p);

    expectedGeom.clear();
    expectedGeom.push_back(nt::GeographicalCoord(15, 7, false));
    expectedGeom.push_back(nt::GeographicalCoord(5, 7, false));
    expectedGeom.push_back(nt::GeographicalCoord(5, 5, false));
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[0].coordinates.begin(),
                                  p.path_items[0].coordinates.end());

    expectedGeom.clear();
    // First coordinate is present twice since in create_path we add the first coordinate of the first edge
    // to the start of the coordinates. (Previous and following geometries are just the projections).
    expectedGeom.push_back(nt::GeographicalCoord(5, 5, false));
    expectedGeom.push_back(nt::GeographicalCoord(5, 5, false));
    expectedGeom.push_back(nt::GeographicalCoord(8, 5, false));
    expectedGeom.push_back(nt::GeographicalCoord(8, 2, false));
    expectedGeom.push_back(nt::GeographicalCoord(27, 2, false));
    expectedGeom.push_back(nt::GeographicalCoord(27, 23, false));
    expectedGeom.push_back(nt::GeographicalCoord(30, 23, false));
    expectedGeom.push_back(nt::GeographicalCoord(30, 5, false));
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[1].coordinates.begin(),
                                  p.path_items[1].coordinates.end());

    expectedGeom.clear();
    expectedGeom.push_back(nt::GeographicalCoord(30, 5, false));
    expectedGeom.push_back(nt::GeographicalCoord(42, 5, false));
    expectedGeom.push_back(nt::GeographicalCoord(50, 20, false));
    expectedGeom.push_back(nt::GeographicalCoord(45, 30, false));
    expectedGeom.push_back(nt::GeographicalCoord(40, 30, false));
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[2].coordinates.begin(),
                                  p.path_items[2].coordinates.end());
}

/*
 * We have this graph :
 *
 *                             .-----.
 *                             |     |
 *                             |     |
 *                             |     |
 *        x1                   |     |
 *   .------.                x4|     |x5
 * x3|      |                  |     |
 *   |      a------------------b-----c---------d
 *   |      |
 *   '------'
 *        x2
 */
BOOST_AUTO_TEST_CASE(parallel_and_same_vertex_edges) {
    GraphBuilder b;
    b("a", 50, 50)("b", 200, 50)("c", 250, 50)("d", 350, 50);
    b("a", "a", 200_s, true)("a", "b", 200_s, true)("b", "c", 300_s, true)("b", "c", 50_s, true)("c", "d", 120_s, true);

    nt::GeographicalCoord x1(40, 100, false);
    nt::GeographicalCoord x2(40, 0, false);
    nt::GeographicalCoord x3(0, 70, false);
    nt::GeographicalCoord x4(200, 70, false);
    nt::GeographicalCoord x5(250, 70, false);

    nt::LineString geom;
    geom.push_back(nt::GeographicalCoord(50, 50, false));
    geom.push_back(nt::GeographicalCoord(200, 50, false));
    b.add_geom(b.get("a", "b"), geom);
    std::reverse(geom.begin(), geom.end());
    b.add_geom(b.get("b", "a"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(250, 50, false));
    geom.push_back(nt::GeographicalCoord(350, 50, false));
    b.add_geom(b.get("c", "d"), geom);
    std::reverse(geom.begin(), geom.end());
    b.add_geom(b.get("d", "c"), geom);

    geom.clear();
    geom.push_back(nt::GeographicalCoord(50, 50, false));
    geom.push_back(nt::GeographicalCoord(50, 100, false));
    geom.push_back(nt::GeographicalCoord(0, 100, false));
    geom.push_back(nt::GeographicalCoord(0, 0, false));
    geom.push_back(nt::GeographicalCoord(50, 0, false));
    geom.push_back(nt::GeographicalCoord(50, 50, false));
    BOOST_FOREACH (const auto out_edge, boost::out_edges(b.get("a"), b.geo_ref.graph)) {
        if (boost::target(out_edge, b.geo_ref.graph) == b.get("a")) {
            b.add_geom(out_edge, geom);
            std::reverse(geom.begin(), geom.end());
        }
    }

    nt::LineString long_geom_bc, short_geom_bc;
    long_geom_bc.push_back(nt::GeographicalCoord(200, 50, false));
    long_geom_bc.push_back(nt::GeographicalCoord(200, 200, false));
    long_geom_bc.push_back(nt::GeographicalCoord(250, 200, false));
    long_geom_bc.push_back(nt::GeographicalCoord(250, 50, false));
    short_geom_bc.push_back(nt::GeographicalCoord(200, 50, false));
    short_geom_bc.push_back(nt::GeographicalCoord(250, 50, false));

    BOOST_FOREACH (const auto out_edge, boost::out_edges(b.get("b"), b.geo_ref.graph)) {
        if (boost::target(out_edge, b.geo_ref.graph) == b.get("c")) {
            b.add_geom(out_edge,
                       b.geo_ref.graph[out_edge].duration.total_seconds() == 50 ? short_geom_bc : long_geom_bc);
        }
    }
    std::reverse(long_geom_bc.begin(), long_geom_bc.end());
    std::reverse(short_geom_bc.begin(), short_geom_bc.end());
    BOOST_FOREACH (const auto out_edge, boost::out_edges(b.get("c"), b.geo_ref.graph)) {
        if (boost::target(out_edge, b.geo_ref.graph) == b.get("b")) {
            b.add_geom(out_edge,
                       b.geo_ref.graph[out_edge].duration.total_seconds() == 50 ? short_geom_bc : long_geom_bc);
        }
    }

    b.geo_ref.init();

    auto edge_x1 = b.geo_ref.nearest_edge(x1);
    BOOST_REQUIRE_EQUAL(boost::source(edge_x1, b.geo_ref.graph), b.get("a"));
    BOOST_REQUIRE_EQUAL(boost::target(edge_x1, b.geo_ref.graph), b.get("a"));
    auto edge_x4 = b.geo_ref.nearest_edge(x4);
    BOOST_REQUIRE_EQUAL(boost::source(edge_x4, b.geo_ref.graph), b.get("b"));
    BOOST_REQUIRE_EQUAL(boost::target(edge_x4, b.geo_ref.graph), b.get("c"));
    BOOST_REQUIRE_EQUAL(b.geo_ref.graph[edge_x4].duration.total_seconds(), 300);

    StreetNetwork worker(b.geo_ref);
    auto origin = nt::EntryPoint();
    ;
    auto destination = nt::EntryPoint();
    origin.streetnetwork_params.max_duration = 3600_s;
    destination.streetnetwork_params.max_duration = 3600_s;

    // x1 to x2, we should not use the direct path on the edge
    origin.coordinates = x1;
    destination.coordinates = x2;
    worker.init(origin, destination);
    Path p = worker.get_direct_path(origin, destination);
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 1);
    nt::LineString expectedGeom;
    expectedGeom.push_back(nt::GeographicalCoord(40, 100, false));
    expectedGeom.push_back(nt::GeographicalCoord(50, 100, false));
    expectedGeom.push_back(nt::GeographicalCoord(50, 50, false));  // Last point of the start projection
    expectedGeom.push_back(
        nt::GeographicalCoord(50, 50, false));  // Returned by build_path, only one coordinate, the vertex
    expectedGeom.push_back(nt::GeographicalCoord(50, 50, false));  // First part of the end projection
    expectedGeom.push_back(nt::GeographicalCoord(50, 0, false));
    expectedGeom.push_back(nt::GeographicalCoord(40, 0, false));
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[0].coordinates.begin(),
                                  p.path_items[0].coordinates.end());

    // x1 to x3, we should use the direct path on the edge
    destination.coordinates = x3;
    worker.init(origin, destination);
    p = worker.get_direct_path(origin, destination);
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 1);
    expectedGeom.clear();
    expectedGeom.push_back(nt::GeographicalCoord(40, 100, false));
    expectedGeom.push_back(nt::GeographicalCoord(0, 100, false));
    expectedGeom.push_back(nt::GeographicalCoord(0, 70, false));
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[0].coordinates.begin(),
                                  p.path_items[0].coordinates.end());

    // x4 to x5, we should use the shorter parallel edge
    origin.coordinates = x4;
    destination.coordinates = x5;
    worker.init(origin, destination);
    p = worker.get_direct_path(origin, destination);
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 3);

    expectedGeom.clear();
    expectedGeom.push_back(nt::GeographicalCoord(200, 70, false));
    expectedGeom.push_back(nt::GeographicalCoord(200, 50, false));
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[0].coordinates.begin(),
                                  p.path_items[0].coordinates.end());
    expectedGeom.clear();
    expectedGeom.push_back(nt::GeographicalCoord(200, 50, false));
    expectedGeom.push_back(nt::GeographicalCoord(200, 50, false));
    expectedGeom.push_back(nt::GeographicalCoord(250, 50, false));
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[1].coordinates.begin(),
                                  p.path_items[1].coordinates.end());
    expectedGeom.clear();
    expectedGeom.push_back(nt::GeographicalCoord(250, 50, false));
    expectedGeom.push_back(nt::GeographicalCoord(250, 70, false));
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedGeom.begin(), expectedGeom.end(), p.path_items[2].coordinates.begin(),
                                  p.path_items[2].coordinates.end());
}

// not used for the moment so it is not possible anymore (but it would not be difficult to do again)
// Est-ce que le calcul de plusieurs nœuds vers plusieurs nœuds fonctionne
// BOOST_AUTO_TEST_CASE(compute_route_n_n){
//    using namespace navitia::type;
//    //StreetNetwork sn;
//    GeoRef sn;
//    GraphBuilder b(sn);

//    /*               a           e
//                     |
//                  b—–o––c
//                     |
//                     d             */
//    b("e", 0,0)("a",0,1)("b",0,2)("c",0,3)("d",0,4)("o",0,5);
//    b("a", "o", 1)("b","o",2)("o","c", 3)("o","d", 4);

//    std::vector<vertex_t> starts = {b.get("a"), b.get("b")};
//    std::vector<vertex_t> dests = {b.get("c"), b.get("d")};

//    GeoRefPathFinder path_finder(sn);
//    path_finder.init(starts, Mode_e::Walking);
//    Path p = path_finder.compute_path(dests);

//    auto coords = get_coords_from_path(p);
//    BOOST_CHECK_EQUAL(coords.size(), 3);
//    GeographicalCoord expected;
//    expected.set_xy(0,1);
//    BOOST_CHECK_EQUAL(coords[0], expected); // a
//    expected.set_xy(0,5);
//    BOOST_CHECK_EQUAL(coords[1], expected); // o
//    expected.set_xy(0,3);
//    BOOST_CHECK_EQUAL(coords[2], expected); // c

//    starts = {b.get("e")};
//    dests = {b.get("a")};
//    path_finder.init(starts, Mode_e::Walking);
//    p = path_finder.compute_path(dests);
//    // no throw in no itineraryn but the path should be empty
//    BOOST_CHECK(p.path_items.empty());

//    //we add a way to a, otherwise 2 path item will be created
//    Way w;
//    w.name = "Bob"; sn.add_way(w);
//    sn.graph[b.get("a","o")].way_idx = 0;

//    // If the departure and arrival nodes are the same one
//    GeographicalCoord projected_start(0,1,true);
//    p = sn.compute(projected_start, projected_start);
//    coords = get_coords_from_path(p);
//    BOOST_REQUIRE_EQUAL(coords.size(), 1); //only one coord
//    BOOST_CHECK_EQUAL(p.path_items.size(), 1); //there is 2 default item
//    BOOST_CHECK_EQUAL(coords[0], GeographicalCoord(0,1, true)); // a
//}

// On teste la prise en compte de la distance initiale au nœud
// BOOST_AUTO_TEST_CASE(compute_zeros){
//    //StreetNetwork sn;
//    GeoRef sn;
//    GraphBuilder b(sn);
//    b("a", "o", 1)("b", "o",2);
//    std::vector<vertex_t> starts = {b.get("a"), b.get("b")};
//    std::vector<vertex_t> dests = {b.get("o")};

//    Path p = sn.compute(starts, dests);
//    BOOST_CHECK_EQUAL(p.path_items.size(), 1);
////    BOOST_CHECK(p.path_items[0].segments[0] == b.get("a","o"));

//    p = sn.compute(starts, dests, {3,1});
////    BOOST_CHECK(p.path_items[0].segments[0] == b.get("b","o"));

//    p = sn.compute(starts, dests, {2,2});
////    BOOST_CHECK(p.path_items[0].segments[0] == b.get("a","o"));
//}

// Est-ce que les indications retournées sont bonnes
BOOST_AUTO_TEST_CASE(compute_directions_test) {
    using namespace navitia::type;
    GraphBuilder b;
    auto* w = new Way;
    w->name = "Jaures";
    b.geo_ref.ways.push_back(w);
    w = new Way;
    w->name = "Hugo";
    b.geo_ref.ways.push_back(w);

    b("a", 0, 0)("b", 1, 1)("c", 2, 2)("d", 3, 3)("e", 4, 4);
    b("a", "b")("b", "c")("c", "d")("d", "e")("e",
                                              "d");  // bug ? if no edge leave the vertex, the projection cannot work...
    b.geo_ref.graph[b.get("a", "b")].way_idx = 0;
    b.geo_ref.graph[b.get("b", "c")].way_idx = 0;
    b.geo_ref.graph[b.get("c", "d")].way_idx = 1;
    b.geo_ref.graph[b.get("d", "e")].way_idx = 1;
    b.geo_ref.graph[b.get("e", "d")].way_idx = 1;

    b.geo_ref.init();

    DijkstraPathFinder djikstra_path_finder(b.geo_ref);
    AstarPathFinder astar_path_finder(b.geo_ref);

    auto start = nt::GeographicalCoord{0, 0, true};
    auto dest = nt::GeographicalCoord{4, 4, true};

    djikstra_path_finder.init(start, Mode_e::Walking, 1);  // starting from a
    Path p = compute_path(djikstra_path_finder, dest);    // going to e
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 2);
    BOOST_CHECK_EQUAL(p.path_items[0].way_idx, 0);
    BOOST_CHECK_EQUAL(p.path_items[1].way_idx, 1);
    check_has_astar_same_result_than_dijkstra(astar_path_finder, start, dest, Mode_e::Walking, 1, p);
    //    BOOST_CHECK(p.path_items[0].segments[0] == b.get("a", "b"));
    //    BOOST_CHECK(p.path_items[0].segments[1] == b.get("b", "c"));
    //    BOOST_CHECK(p.path_items[1].segments[0] == b.get("c", "d"));
    //    BOOST_CHECK(p.path_items[1].segments[1] == b.get("d", "e"));

    start = nt::GeographicalCoord{3, 3, true};
    dest = nt::GeographicalCoord{4, 4, true};
    djikstra_path_finder.init(start, Mode_e::Walking, 1);  // starting from d
    p = compute_path(djikstra_path_finder, dest);         // going to e
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 1);
    BOOST_CHECK_EQUAL(p.path_items[0].way_idx, 1);
    check_has_astar_same_result_than_dijkstra(astar_path_finder, start, dest, Mode_e::Walking, 1, p);
}

// On teste le calcul d'itinéraire de coordonnées à coordonnées
BOOST_AUTO_TEST_CASE(compute_coord) {
    using namespace navitia::type;
    GraphBuilder b;
    DijkstraPathFinder djikstra_path_finder(b.geo_ref);
    AstarPathFinder astar_path_finder(b.geo_ref);

    /*           a+------+b
     *            |      |
     *            |      |
     *           c+------+d
     */

    b("a", 0, 0)("b", 10, 0)("c", 0, 10)("d", 10, 10);
    b("a", "b", 10_s)("b", "a", 10_s)("b", "d", 10_s)("d", "b", 10_s)("c", "d", 10_s)("d", "c", 10_s);

    // put lots of edges between a and c to check if we manage that
    // correctly
    b("a", "c", 20_s)("a", "c", 10_s)("a", "c", 30_s);
    b("c", "a", 20_s)("c", "a", 10_s)("c", "a", 30_s);

    auto* w = new Way;
    w->name = "BobAB";
    b.geo_ref.ways.push_back(w);
    w = new Way;
    w->name = "BobAC";
    b.geo_ref.ways.push_back(w);
    w = new Way;
    w->name = "BobCD";
    b.geo_ref.ways.push_back(w);
    w = new Way;
    w->name = "BobDB";
    b.geo_ref.ways.push_back(w);
    b.geo_ref.graph[b.get("a", "b")].way_idx = 0;
    b.geo_ref.graph[b.get("b", "a")].way_idx = 0;

    auto vertex_a = b.get("a"), vertex_c = b.get("c");
    for (auto range = out_edges(vertex_a, b.geo_ref.graph); range.first != range.second; ++range.first) {
        if (target(*range.first, b.geo_ref.graph) != vertex_c) {
            continue;
        }
        b.geo_ref.graph[*range.first].way_idx = 1;
    }
    for (auto range = out_edges(vertex_c, b.geo_ref.graph); range.first != range.second; ++range.first) {
        if (target(*range.first, b.geo_ref.graph) != vertex_a) {
            continue;
        }
        b.geo_ref.graph[*range.first].way_idx = 1;
    }

    b.geo_ref.graph[b.get("c", "d")].way_idx = 2;
    b.geo_ref.graph[b.get("d", "c")].way_idx = 2;
    b.geo_ref.graph[b.get("d", "b")].way_idx = 3;
    b.geo_ref.graph[b.get("b", "d")].way_idx = 3;

    GeographicalCoord start;
    start.set_xy(3, -1);
    GeographicalCoord destination;
    destination.set_xy(4, 11);
    b.geo_ref.init();
    djikstra_path_finder.init(start, Mode_e::Walking, 1);
    Path p = compute_path(djikstra_path_finder, destination);
    auto coords = get_coords_from_path(p);
    print_coord(coords);
    BOOST_REQUIRE_EQUAL(coords.size(), 4);
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 3);
    check_has_astar_same_result_than_dijkstra(astar_path_finder, start, destination, Mode_e::Walking, 1, p);

    GeographicalCoord expected;
    expected.set_xy(3, 0);
    BOOST_CHECK_EQUAL(coords[0], expected);
    expected.set_xy(0, 0);
    BOOST_CHECK_EQUAL(coords[1], expected);
    expected.set_xy(0, 10);
    BOOST_CHECK_EQUAL(coords[2], expected);
    expected.set_xy(4, 10);
    BOOST_CHECK_EQUAL(coords[3], expected);
    BOOST_CHECK_EQUAL(p.path_items[1].duration, 10_s);  // check that the shortest edge is used

    // Trajet partiel : on ne parcourt pas un arc en entier, mais on passe par un nœud
    start.set_xy(7, 6);
    djikstra_path_finder.init(start, Mode_e::Walking, 1);
    p = compute_path(djikstra_path_finder, destination);
    coords = get_coords_from_path(p);
    print_coord(coords);
    BOOST_CHECK_EQUAL(p.path_items.size(), 2);
    BOOST_REQUIRE_EQUAL(coords.size(), 3);
    BOOST_CHECK_EQUAL(coords[0], GeographicalCoord(10, 6, false));
    BOOST_CHECK_EQUAL(coords[1], GeographicalCoord(10, 10, false));
    BOOST_CHECK_EQUAL(coords[2], GeographicalCoord(4, 10, false));
    check_has_astar_same_result_than_dijkstra(astar_path_finder, start, destination, Mode_e::Walking, 1, p);

}

BOOST_AUTO_TEST_CASE(compute_nearest) {
    using namespace navitia::type;

    GraphBuilder b;

    /*       1                    2
     *       +                    +
     *    o------o------o------o------o
     *    a      b      c      d      e
     */

    b("a", 0, 0)("b", 100, 0)("c", 200, 0)("d", 300, 0)("e", 400, 0);
    b("a", "b", 100_s)("b", "a", 100_s)("b", "c", 100_s)("c", "b", 100_s)("c", "d", 100_s)("d", "c", 100_s)(
        "d", "e", 100_s)("e", "d", 100_s);

    GeographicalCoord c1(50, 10, false);
    GeographicalCoord c2(350, 20, false);
    navitia::proximitylist::ProximityList<idx_t> pl;
    pl.add(c1, 0);
    pl.add(c2, 1);
    pl.build();
    b.geo_ref.init();

    StopPoint* sp1 = new StopPoint();
    sp1->idx = 0;
    sp1->coord = c1;
    StopPoint* sp2 = new StopPoint();
    sp2->idx = 1;
    sp2->coord = c2;
    std::vector<StopPoint*> stop_points;
    stop_points.push_back(sp1);
    stop_points.push_back(sp2);
    b.geo_ref.project_stop_points(stop_points);

    GeographicalCoord o(0, 0);

    StreetNetwork w(b.geo_ref);
    EntryPoint starting_point;
    starting_point.coordinates = o;
    starting_point.streetnetwork_params.mode = Mode_e::Walking;
    starting_point.streetnetwork_params.speed_factor =
        2;  // to have a speed different from the default one (and greater not to have projection problems)
    w.init(starting_point);
    auto res = w.find_nearest_stop_points(10_s, pl, false);
    BOOST_CHECK_EQUAL(res.size(), 0);

    w.init(starting_point);  // not mandatory, but reinit to clean the distance table to get fresh dijsktra
    res = w.find_nearest_stop_points(100_s, pl, false);
    // the projection is done with the same mean of transport, at the same speed
    navitia::routing::map_stop_point_duration tested_map;
    float_t walk_speed = default_speed[Mode_e::Walking] * 2;
    tested_map[navitia::routing::SpIdx(*sp1)] = navitia::seconds(60 / walk_speed);
    BOOST_CHECK_EQUAL_COLLECTIONS(res.cbegin(), res.cend(), tested_map.cbegin(), tested_map.cend());

    w.init(starting_point);
    res = w.find_nearest_stop_points(1000_s, pl, false);
    // 1 projections at the arrival, and 3 edges (100s each but at twice the speed)
    tested_map.clear();
    tested_map[navitia::routing::SpIdx(*sp1)] = navitia::seconds(60 / walk_speed);
    tested_map[navitia::routing::SpIdx(*sp2)] = navitia::seconds(50 / walk_speed) + 150_s;
    BOOST_CHECK_EQUAL_COLLECTIONS(res.cbegin(), res.cend(), tested_map.cbegin(), tested_map.cend());
}

// Récupérer les cordonnées d'un numéro impair :
BOOST_AUTO_TEST_CASE(numero_impair) {
    navitia::georef::Way way;
    navitia::georef::HouseNumber hn;
    navitia::georef::Graph graph;
    vertex_t debut, fin;
    Vertex v;
    navitia::georef::Edge e1;

    /*
   (1,3)       (1,7)       (1,17)       (1,53)
     3           7           17           53
   */

    way.name = "AA";
    way.way_type = "rue";
    nt::GeographicalCoord upper(1, 53);
    nt::GeographicalCoord lower(1, 3);

    hn.coord = lower;
    hn.number = 3;
    way.house_number_left.push_back(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v, graph);

    hn.coord.set_lon(1.0);
    hn.coord.set_lat(7.0);
    hn.number = 7;
    way.house_number_left.push_back(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v, graph);

    boost::add_edge(debut, fin, e1, graph);
    boost::add_edge(fin, debut, e1, graph);
    way.edges.push_back(std::make_pair(debut, fin));
    way.edges.push_back(std::make_pair(fin, debut));

    hn.coord.set_lon(1.0);
    hn.coord.set_lat(17.0);
    hn.number = 17;
    way.add_house_number(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v, graph);

    boost::add_edge(fin, debut, e1, graph);
    boost::add_edge(debut, fin, e1, graph);
    way.edges.push_back(std::make_pair(fin, debut));
    way.edges.push_back(std::make_pair(debut, fin));

    hn.coord = upper;
    hn.number = 53;
    way.add_house_number(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v, graph);

    boost::add_edge(debut, fin, e1, graph);
    boost::add_edge(fin, debut, e1, graph);
    way.edges.push_back(std::make_pair(debut, fin));
    way.edges.push_back(std::make_pair(fin, debut));

    // Numéro recherché est > au plus grand numéro dans la rue
    nt::GeographicalCoord result = way.nearest_coord(55, graph);
    BOOST_CHECK_EQUAL(result, upper);

    // Numéro recherché est < au plus petit numéro dans la rue
    result = way.nearest_coord(1, graph);
    BOOST_CHECK_EQUAL(result, lower);

    // Numéro recherché est = à numéro dans la rue
    result = way.nearest_coord(17, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(1.0, 17.0));

    // Numéro recherché est n'existe pas mais il est inclus entre le plus petit et grand numéro de la rue
    // ==>  Extrapolation des coordonnées
    result = way.nearest_coord(43, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(1.0, 43.0));

    // liste des numéros pair est vide ==> Calcul du barycentre de la rue
    result = way.nearest_coord(40, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(1, 28));  // 3 + 25
    // les deux listes des numéros pair et impair sont vides ==> Calcul du barycentre de la rue
    way.house_number_left.clear();
    result = way.nearest_coord(9, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(1, 28));
}

// Récupérer les cordonnées d'un numéro pair :
BOOST_AUTO_TEST_CASE(numero_pair) {
    navitia::georef::Way way;
    navitia::georef::HouseNumber hn;
    navitia::georef::Graph graph;
    vertex_t debut, fin;
    Vertex v;
    navitia::georef::Edge e1;

    /*
   (2,4)       (2,8)       (2,18)       (2,54)
     4           8           18           54
   */
    way.name = "AA";
    way.way_type = "rue";
    nt::GeographicalCoord upper(2.0, 54.0);
    nt::GeographicalCoord lower(2.0, 4.0);

    hn.coord = lower;
    hn.number = 4;
    way.add_house_number(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v, graph);

    hn.coord.set_lon(2.0);
    hn.coord.set_lat(8.0);
    hn.number = 8;
    way.add_house_number(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v, graph);

    boost::add_edge(debut, fin, e1, graph);
    boost::add_edge(fin, debut, e1, graph);
    way.edges.push_back(std::make_pair(debut, fin));
    way.edges.push_back(std::make_pair(fin, debut));

    hn.coord.set_lon(2.0);
    hn.coord.set_lat(18.0);
    hn.number = 18;
    way.add_house_number(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v, graph);

    boost::add_edge(fin, debut, e1, graph);
    boost::add_edge(debut, fin, e1, graph);
    way.edges.push_back(std::make_pair(fin, debut));
    way.edges.push_back(std::make_pair(debut, fin));

    hn.coord = upper;
    hn.number = 54;
    way.add_house_number(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v, graph);

    boost::add_edge(debut, fin, e1, graph);
    boost::add_edge(fin, debut, e1, graph);
    way.edges.push_back(std::make_pair(debut, fin));
    way.edges.push_back(std::make_pair(fin, debut));

    // Numéro recherché est > au plus grand numéro dans la rue
    nt::GeographicalCoord result = way.nearest_coord(56, graph);
    BOOST_CHECK_EQUAL(result, upper);

    // Numéro recherché est < au plus petit numéro dans la rue
    result = way.nearest_coord(2, graph);
    BOOST_CHECK_EQUAL(result, lower);

    // Numéro recherché est = à numéro dans la rue
    result = way.nearest_coord(18, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(2.0, 18.0));

    // Numéro recherché est n'existe pas mais il est inclus entre le plus petit et grand numéro de la rue
    // ==>  Extrapolation des coordonnées
    result = way.nearest_coord(44, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(2.0, 44.0));

    // liste des numéros impair est vide ==> Calcul du barycentre de la rue
    result = way.nearest_coord(41, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(2, 29));  // 4+25

    // les deux listes des numéros pair et impair sont vides ==> Calcul du barycentre de la rue
    way.house_number_right.clear();
    result = way.nearest_coord(10, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(2, 29));
}

// Recherche d'un numéro à partir des coordonnées
BOOST_AUTO_TEST_CASE(coord) {
    navitia::georef::Way way;
    navitia::georef::HouseNumber hn;

    /*
   (2,4)       (2,8)       (2,18)       (2,54)
     4           8           18           54
   */

    way.name = "AA";
    way.way_type = "rue";
    nt::GeographicalCoord upper(2.0, 54.0);
    nt::GeographicalCoord lower(2.0, 4.0);

    hn.coord = lower;
    hn.number = 4;
    way.add_house_number(hn);

    hn.coord.set_lon(2.0);
    hn.coord.set_lat(8.0);
    hn.number = 8;
    way.add_house_number(hn);

    hn.coord.set_lon(2.0);
    hn.coord.set_lat(18.0);
    hn.number = 18;
    way.add_house_number(hn);

    hn.coord = upper;
    hn.number = 54;
    way.add_house_number(hn);

    // les coordonnées sont à l'extérieur de la rue coté supérieur
    int result = way.nearest_number(nt::GeographicalCoord(1.0, 55.0)).first;
    BOOST_CHECK_EQUAL(result, 54);

    // les coordonnées sont à l'extérieur de la rue coté inférieur
    result = way.nearest_number(nt::GeographicalCoord(2.0, 3.0)).first;
    BOOST_CHECK_EQUAL(result, 4);

    // coordonnées recherchées est = à coordonnées dans la rue
    result = way.nearest_number(nt::GeographicalCoord(2.0, 8.0)).first;
    BOOST_CHECK_EQUAL(result, 8);

    // les deux listes des numéros sont vides
    way.house_number_right.clear();
    result = way.nearest_number(nt::GeographicalCoord(2.0, 8.0)).first;
    BOOST_CHECK_EQUAL(result, -1);
}

// Test de autocomplete
BOOST_AUTO_TEST_CASE(build_autocomplete_test) {
    ed::builder b = {"20140614"};
    Vertex v;
    std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> result;
    int nbmax = 10;
    std::set<std::string> ghostwords;

    b.add_way("jeanne d'arc", "rue");
    b.add_way("jean jaures", "place");
    b.add_way("jean paul gaultier paris", "rue");
    b.add_way("jean jaures", "avenue");
    b.add_way("poniatowski", "boulevard");
    b.add_way("pente de Bray", "");

    /*
   (2,4)       (2,4)       (2,18)       (2,54)
     4           4           18           54
   */

    auto way = new navitia::georef::Way;
    way->idx = 6;
    way->name = "jean jaures";
    way->way_type = "rue";
    // Ajout des numéros et les noeuds
    nt::GeographicalCoord upper(2.0, 54.0);
    nt::GeographicalCoord lower(2.0, 4.0);

    navitia::georef::HouseNumber hn;
    hn.coord = lower;
    hn.number = 4;
    way->add_house_number(hn);
    v.coord = hn.coord;
    vertex_t debut = boost::add_vertex(v, b.data->geo_ref->graph);

    hn.coord.set_lon(2.0);
    hn.coord.set_lat(8.0);
    hn.number = 8;
    way->add_house_number(hn);
    v.coord = hn.coord;
    vertex_t fin = boost::add_vertex(v, b.data->geo_ref->graph);

    boost::add_edge(debut, fin, navitia::georef::Edge{way->idx, 1_s}, b.data->geo_ref->graph);
    boost::add_edge(fin, debut, navitia::georef::Edge{way->idx, 1_s}, b.data->geo_ref->graph);
    way->edges.push_back(std::make_pair(debut, fin));
    way->edges.push_back(std::make_pair(fin, debut));

    hn.coord.set_lon(2.0);
    hn.coord.set_lat(18.0);
    hn.number = 18;
    way->add_house_number(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v, b.data->geo_ref->graph);

    boost::add_edge(fin, debut, navitia::georef::Edge{way->idx, 1_s}, b.data->geo_ref->graph);
    boost::add_edge(debut, fin, navitia::georef::Edge{way->idx, 1_s}, b.data->geo_ref->graph);
    way->edges.push_back(std::make_pair(fin, debut));
    way->edges.push_back(std::make_pair(debut, fin));

    hn.coord = upper;
    hn.number = 54;
    way->add_house_number(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v, b.data->geo_ref->graph);

    boost::add_edge(debut, fin, navitia::georef::Edge{way->idx, 1_s}, b.data->geo_ref->graph);
    boost::add_edge(fin, debut, navitia::georef::Edge{way->idx, 1_s}, b.data->geo_ref->graph);
    way->edges.push_back(std::make_pair(debut, fin));
    way->edges.push_back(std::make_pair(fin, debut));
    b.data->geo_ref->ways.push_back(way);

    Admin* ad = new Admin;
    ad->name = "Quimper";
    ad->uri = "Quimper";
    ad->level = 8;
    ad->postal_codes.push_back("29000");
    ad->idx = 0;
    ad->insee = "92232";
    b.data->geo_ref->admins.push_back(ad);

    b.manage_admin();
    b.build_autocomplete();

    result = b.data->geo_ref->find_ways("10 rue jean jaures", nbmax, false, [](int) { return true; }, ghostwords);

    // we should have found the 10 of the rue jean jaures
    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_CHECK_EQUAL(result[0].house_number, 10);
    BOOST_REQUIRE(b.data->geo_ref->ways[result[0].idx]);
    BOOST_CHECK_EQUAL(b.data->geo_ref->ways[result[0].idx]->get_label(), "jean jaures (Quimper)");

    // when we search only for jean jaures we should have the avenue, the place and the street
    result = b.data->geo_ref->find_ways("jean jaures", nbmax, false, [](int) { return true; }, ghostwords);
    BOOST_REQUIRE_EQUAL(result.size(), 3);
    // and none of the result should have a house number
    for (const auto& r : result) {
        BOOST_CHECK_EQUAL(r.house_number, -1);
    }
}

BOOST_AUTO_TEST_CASE(two_scc) {
    using namespace navitia::type;

    GraphBuilder b;

    /*       1             2
     *       +             +
     *    o------o   o---o------o
     *    a      b   c   d      e
     */

    b("a", 0, 0)("b", 100, 0)("c", 200, 0)("d", 300, 0)("e", 400, 0);
    b("a", "b", navitia::seconds(100))("b", "a", navitia::seconds(100))("c", "d", navitia::seconds(100))(
        "d", "c", navitia::seconds(100))("d", "e", navitia::seconds(100))("e", "d", navitia::seconds(100));

    GeographicalCoord c1(50, 10, false);
    GeographicalCoord c2(350, 20, false);
    navitia::proximitylist::ProximityList<idx_t> pl;
    pl.add(c1, 0);
    pl.add(c2, 1);
    pl.build();
    b.geo_ref.init();

    StopPoint* sp1 = new StopPoint();
    sp1->coord = c1;
    StopPoint* sp2 = new StopPoint();
    sp2->coord = c2;
    std::vector<StopPoint*> stop_points;
    stop_points.push_back(sp1);
    stop_points.push_back(sp2);
    b.geo_ref.project_stop_points(stop_points);

    StreetNetwork w(b.geo_ref);

    EntryPoint starting_point;
    starting_point.coordinates = c1;
    starting_point.streetnetwork_params.mode = Mode_e::Walking;
    w.init(starting_point);

    auto max = w.get_distance(1, false);
    BOOST_CHECK_EQUAL(max, bt::pos_infin);
}

BOOST_AUTO_TEST_CASE(angle_computation) {
    // simple case

    /*
     * ----------------
     * |
     * |
     * |
     *
     *=> 90
     */
    Path p;
    p.path_items.push_back(PathItem());
    p.path_items.back().coordinates.push_back({1, 1});
    p.path_items.back().coordinates.push_back({1, 2});

    int angle = compute_directions(p, {2, 2});

    BOOST_CHECK_CLOSE(1.0 * angle, 90, 1);
}

BOOST_AUTO_TEST_CASE(angle_computation_2) {
    Path p;
    p.path_items.push_back(PathItem());
    p.path_items.back().coordinates.push_back({2, -3});
    p.path_items.back().coordinates.push_back({3, 1});

    int angle = compute_directions(p, {-1, 4});

    BOOST_CHECK_EQUAL(1.0 * angle, 113 - 180);
}

BOOST_AUTO_TEST_CASE(angle_computation_lon_lat) {
    Path p;
    p.path_items.push_back(PathItem());

    nt::GeographicalCoord a{48.849143, 2.391776};
    nt::GeographicalCoord b{48.850456, 2.390596};
    nt::GeographicalCoord c{48.850428, 2.387356};
    p.path_items.back().coordinates.push_back(a);
    p.path_items.back().coordinates.push_back(b);

    int angle = compute_directions(p, c);

    int val = 49;
    BOOST_CHECK_CLOSE(1.0 * angle, val, 1);

    Path p2;
    p2.path_items.push_back(PathItem());

    p2.path_items.back().coordinates.push_back(c);
    p2.path_items.back().coordinates.push_back(b);

    angle = compute_directions(p2, a);

    BOOST_CHECK_CLOSE(1.0 * angle, -1 * val, 1.0);
}

// small test to make sure the time manipulation works in the SpeedDistanceCombiner
BOOST_AUTO_TEST_CASE(SpeedDistanceCombiner_test) {
    navitia::time_duration dur = 10_s;

    SpeedDistanceCombiner comb(2);

    BOOST_CHECK_EQUAL(dur / 2, 5_s);

    navitia::time_duration dur2 = 60_s;
    BOOST_CHECK_EQUAL(comb(dur, dur2), navitia::seconds(10 + 60 / 2));
}

BOOST_AUTO_TEST_CASE(SpeedDistanceCombiner_test2) {
    navitia::time_duration dur = 10_s;

    SpeedDistanceCombiner comb(0.5);

    BOOST_CHECK_EQUAL(dur / 0.5, 20_s);

    navitia::time_duration dur2 = 60_s;
    BOOST_CHECK_EQUAL(comb(dur, dur2), 130_s);
}

// test allowed mode creation
BOOST_AUTO_TEST_CASE(transportation_mode_creation) {
    const auto allowed_transportation_mode = create_from_allowedlist({{{{nt::Mode_e::Walking},
                                                                        {},
                                                                        {nt::Mode_e::Walking, nt::Mode_e::Car},
                                                                        {nt::Mode_e::Walking, nt::Mode_e::Bss}}}});

    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Walking][nt::Mode_e::Walking], true);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Walking][nt::Mode_e::Car], false);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Walking][nt::Mode_e::Bss], false);

    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Bss][nt::Mode_e::Walking], true);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Bss][nt::Mode_e::Bss], true);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Bss][nt::Mode_e::Car], false);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Bss][nt::Mode_e::Bike], false);
}

BOOST_AUTO_TEST_CASE(geolocalization) {
    //     (0,100)    rue AB   (100,100)
    //      A +--------------------+ B
    //        |
    //        | + D (10,80): 2 rue AB
    //        | + E (10,70): 3 rue AC
    // rue AC |
    //        |
    //        |
    //        |
    //      C + (0,0)
    using navitia::georef::Edge;
    using navitia::georef::HouseNumber;
    using nt::GeographicalCoord;

    const int AA = 0;
    const GeographicalCoord A = {0, 100, false};
    const int BB = 1;
    const GeographicalCoord B = {100, 100, false};
    const int CC = 2;
    const GeographicalCoord C = {0, 0, false};
    const GeographicalCoord D = {10, 80, false};
    const GeographicalCoord E = {10, 70, false};

    ed::builder b = {"20140828"};

    boost::add_vertex(navitia::georef::Vertex(A), b.data->geo_ref->graph);
    boost::add_vertex(navitia::georef::Vertex(B), b.data->geo_ref->graph);
    boost::add_vertex(navitia::georef::Vertex(C), b.data->geo_ref->graph);
    boost::add_vertex(navitia::georef::Vertex(D), b.data->geo_ref->graph);
    boost::add_vertex(navitia::georef::Vertex(E), b.data->geo_ref->graph);
    b.data->geo_ref->init();

    b.data->geo_ref->admins.push_back(new navitia::georef::Admin());
    auto admin = b.data->geo_ref->admins.back();
    admin->uri = "admin:74435";
    admin->name = "Condom";
    admin->insee = "32107";
    admin->level = 8;
    admin->postal_codes.push_back("32100");

    navitia::georef::Way* ab = new navitia::georef::Way();
    ab->name = "rue AB";
    ab->idx = 0;
    ab->way_type = "rue";
    ab->house_number_right.push_back(HouseNumber(D.lon(), D.lat(), 2));
    ab->admin_list.push_back(admin);
    b.data->geo_ref->ways.push_back(ab);

    navitia::georef::Way* ac = new navitia::georef::Way();
    ac->name = "rue AC";
    ac->idx = 1;
    ac->way_type = "rue";
    ac->house_number_left.push_back(HouseNumber(E.lon(), E.lat(), 3));
    ac->admin_list.push_back(admin);
    b.data->geo_ref->ways.push_back(ac);

    // A->B
    add_edge(AA, BB, Edge(0, 42_s), b.data->geo_ref->graph);
    add_edge(BB, AA, Edge(0, 42_s), b.data->geo_ref->graph);
    ab->edges.push_back(std::make_pair(AA, BB));
    ab->edges.push_back(std::make_pair(BB, AA));

    // A->C
    add_edge(AA, CC, Edge(1, 42_s), b.data->geo_ref->graph);
    add_edge(CC, AA, Edge(1, 42_s), b.data->geo_ref->graph);
    ac->edges.push_back(std::make_pair(AA, CC));
    ac->edges.push_back(std::make_pair(CC, AA));

    b.data->build_uri();
    b.data->build_proximity_list();

    const Way *const_ab = ab, *const_ac = ac;
    BOOST_CHECK_EQUAL(b.data->geo_ref->nearest_addr(D), std::make_pair(2, const_ab));
    BOOST_CHECK_EQUAL(b.data->geo_ref->nearest_addr(E), std::make_pair(3, const_ac));
}

BOOST_AUTO_TEST_CASE(range_postal_codes) {
    navitia::georef::Admin* admin = new navitia::georef::Admin();
    admin->postal_codes = {"44000", "44100", "44200", "44300"};
    BOOST_CHECK_EQUAL(admin->get_range_postal_codes(), "44000-44300");
}

BOOST_AUTO_TEST_CASE(empty_postal_codes) {
    navitia::georef::Admin* admin = new navitia::georef::Admin();
    admin->postal_codes = {};
    BOOST_CHECK_EQUAL(admin->get_range_postal_codes(), "");
}

BOOST_AUTO_TEST_CASE(one_postal_code) {
    navitia::georef::Admin* admin = new navitia::georef::Admin();
    admin->postal_codes = {"03430"};
    BOOST_CHECK_EQUAL(admin->get_range_postal_codes(), "03430");
}

BOOST_AUTO_TEST_CASE(first_number_0) {
    navitia::georef::Admin* admin = new navitia::georef::Admin();
    admin->postal_codes = {"04400", "04410", "04420", "04430"};
    BOOST_CHECK_EQUAL(admin->get_range_postal_codes(), "04400-04430");
}

BOOST_AUTO_TEST_CASE(string_and_int_post_codes) {
    navitia::georef::Admin* admin = new navitia::georef::Admin();
    admin->postal_codes = {"44000", "44100", "44200", "44300", "abcd"};
    BOOST_CHECK_EQUAL(admin->get_range_postal_codes(), "44000;44100;44200;44300;abcd");
}

BOOST_AUTO_TEST_CASE(list_postal_codes) {
    navitia::georef::Admin* admin = new navitia::georef::Admin();
    admin->postal_codes = {"44000", "44100", "44200", "44300"};
    BOOST_CHECK_EQUAL(admin->postal_codes_to_string(), "44000;44100;44200;44300");
}

BOOST_AUTO_TEST_CASE(find_nearest_on_same_edge) {
    using namespace navitia::type;

    GraphBuilder b;

    /*                      0          1
     *                      +          +
     *    o-----------o---------------------o----------------o
     *    a           b                     c                d
     *                     +             +
     *                     2             3
     */

    b("a", 0, 0)("b", 100, 0)("c", 300, 0)("d", 400, 0);
    b("a", "b", 100_s, true)("b", "c", 200_s, true)("c", "d", 100_s, true);

    GeographicalCoord c0(120, 10, false);
    GeographicalCoord c1(250, 20, false);
    GeographicalCoord c2(110, -10, false);
    GeographicalCoord c3(280, -30, false);
    navitia::proximitylist::ProximityList<idx_t> pl;
    pl.add(c0, 0);
    pl.add(c1, 1);
    pl.add(c2, 2);
    pl.add(c3, 3);
    pl.build();
    b.geo_ref.init();

    StopPoint* sp0 = new StopPoint();
    sp0->coord = c0;
    sp0->idx = 0;
    StopPoint* sp1 = new StopPoint();
    sp1->coord = c1;
    sp1->idx = 1;
    StopPoint* sp2 = new StopPoint();
    sp2->coord = c2;
    sp2->idx = 2;
    StopPoint* sp3 = new StopPoint();
    sp3->coord = c3;
    std::vector<StopPoint*> stop_points;
    stop_points.push_back(sp0);
    stop_points.push_back(sp1);
    stop_points.push_back(sp2);
    stop_points.push_back(sp3);
    b.geo_ref.project_stop_points(stop_points);

    StreetNetwork w(b.geo_ref);
    EntryPoint starting_point;
    starting_point.coordinates = c3;
    starting_point.streetnetwork_params.mode = Mode_e::Walking;
    starting_point.streetnetwork_params.speed_factor = 1;
    w.init(starting_point);
    auto res = w.find_nearest_stop_points(10_s, pl, false);
    BOOST_CHECK_EQUAL(res.size(), 0);

    w.init(starting_point);  // not mandatory, but reinit to clean the distance table to get fresh dijsktra
    res = w.find_nearest_stop_points(180_s, pl, false);

    // if you give the coord of the stop_point, you have to go to the street then go back to the stop_point, too bad!
    navitia::routing::map_stop_point_duration tested_map;
    tested_map[navitia::routing::SpIdx(3)] = navitia::seconds(60 / default_speed[Mode_e::Walking]);
    tested_map[navitia::routing::SpIdx(1)] = navitia::seconds(80 / default_speed[Mode_e::Walking]);
    tested_map[navitia::routing::SpIdx(0)] = navitia::seconds(200 / default_speed[Mode_e::Walking]);
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), tested_map.begin(), tested_map.end());

    for (auto& elt : res) {
        BOOST_CHECK_EQUAL(elt.second, w.get_path(elt.first.val, false).duration);
    }
}
