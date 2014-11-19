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

#include "georef/georef.h"
#include "tests/utils_test.h"
#include "type/data.h"
#include "builder.h"
#include "georef/street_network.h"
#include <boost/graph/detail/adjacency_list.hpp>

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

using namespace navitia::georef;
using namespace boost;

std::vector<navitia::type::GeographicalCoord> get_coords_from_path(const Path& path) {
    std::vector<navitia::type::GeographicalCoord> res;
    for (const auto& item : path.path_items) {
        for (const auto coord : item.coordinates) {
            res.push_back(coord);
        }
    }
    return res;
}

void print_coord(const std::vector<navitia::type::GeographicalCoord>& coord) {
    std::cout << " coord : " << std::endl;
    for (auto c : coord) {
        std::cout << " -- " << c.lon() / navitia::type::GeographicalCoord::N_M_TO_DEG
                  << ", " << c.lat() / navitia::type::GeographicalCoord::N_M_TO_DEG
                     << std::endl;
    }
}


BOOST_AUTO_TEST_CASE(init_test) {
    using namespace navitia::type;
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);
    Way w;
    w.name = "Jaures"; sn.add_way(w);
    w.name = "Hugo"; sn.add_way(w);

    b("a", 0, 0)("b", 1, 1)("c", 2, 2)("d", 3, 3)("e", 4, 4);
    b("a", "b")("b","c")("c","d")("d","e")("e","d"); //bug ? if no edge leave the vertex, the projection cannot work...
    sn.graph[b.get("a","b")].way_idx = 0;
    sn.graph[b.get("b","c")].way_idx = 0;
    sn.graph[b.get("c","d")].way_idx = 1;
    sn.graph[b.get("d","e")].way_idx = 1;
    sn.graph[b.get("e","d")].way_idx = 1;

    BOOST_CHECK_EQUAL(boost::num_vertices(b.geo_ref.graph), 5);

    sn.init();

    BOOST_CHECK_EQUAL(boost::num_vertices(b.geo_ref.graph), 15); //one graph for each transportation mode save VLS

    BOOST_CHECK_EQUAL(b.geo_ref.offsets[Mode_e::Walking], 0);
    BOOST_CHECK_EQUAL(b.geo_ref.offsets[Mode_e::Bike], 5);
    BOOST_CHECK_EQUAL(b.geo_ref.offsets[Mode_e::Car], 10);
    BOOST_CHECK_EQUAL(b.geo_ref.offsets[Mode_e::Bss], 0);
}

BOOST_AUTO_TEST_CASE(outil_de_graph) {
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder builder(sn);
    Graph & g = sn.graph;

    BOOST_CHECK_EQUAL(num_vertices(g), 0);
    BOOST_CHECK_EQUAL(num_edges(g), 0);

    // Construction de deux nœuds et d'un arc
    builder("a",0, 0)("b",1,2)("a", "b", 10_s);
    BOOST_CHECK_EQUAL(num_vertices(g), 2);
    BOOST_CHECK_EQUAL(num_edges(g), 1);

    // On vérifie que les propriétés sont bien définies
    vertex_t a = builder.vertex_map["a"];
    BOOST_CHECK_EQUAL(g[a].coord.lon(), 0);
    BOOST_CHECK_EQUAL(g[a].coord.lat(), 0);

    vertex_t b = builder.vertex_map["b"];
    navitia::type::GeographicalCoord expected;
    expected.set_xy(1,2);
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

BOOST_AUTO_TEST_CASE(nearest_segment){
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);

    /*               a           e
                     |
                  b—–o––c
                     |
                     d             */

    b("a", 0,10)("b", -10, 0)("c",10,0)("d",0,-10)("o",0,0)("e", 50,10);
    b("o", "a")("o","b")("o","c")("o","d")("b","o");

    navitia::type::GeographicalCoord c(1,2, false);
    BOOST_CHECK(sn.nearest_edge(c) == b.get("o", "a"));
    c.set_xy(2, 1);
    BOOST_CHECK(sn.nearest_edge(c) == b.get("o", "c"));
    c.set_xy(2, -1);
    BOOST_CHECK(sn.nearest_edge(c) == b.get("o", "c"));
    c.set_xy(2, -3);
    BOOST_CHECK(sn.nearest_edge(c) == b.get("o", "d"));
    c.set_xy(-10, 1);
    BOOST_CHECK(sn.nearest_edge(c) == b.get("b", "o"));
    c.set_xy(50, 10);
    BOOST_CHECK_EQUAL(sn.nearest_edge(c), b.get("o", "c"));
}

BOOST_AUTO_TEST_CASE(real_nearest_edge){
    GeoRef gr;
    GraphBuilder b(gr);

    /*        s
       a---------------b
              c-d
    */
    b("a", 0, -100)("b", 0, 100)("c", 10, 0)("d", 10, 10);
    b("a", "b")("c", "d");
    navitia::type::GeographicalCoord s(-10, 0, false);
    BOOST_CHECK(gr.nearest_edge(s) == b.get("a", "b"));
}

/// Compute the path from the starting point to the the target geographical coord
Path compute_path(PathFinder& finder, const navitia::type::GeographicalCoord& target_coord) {
    ProjectionData dest(target_coord, finder.geo_ref, finder.geo_ref.pl);

    auto best_pair = finder.update_path(dest);

    return finder.get_path(dest, best_pair);
}

//not used for the moment so it is not possible anymore (but it would not be difficult to do again)
// Est-ce que le calcul de plusieurs nœuds vers plusieurs nœuds fonctionne
//BOOST_AUTO_TEST_CASE(compute_route_n_n){
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
//BOOST_AUTO_TEST_CASE(compute_zeros){
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
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);
    Way w;
    w.name = "Jaures"; sn.add_way(w);
    w.name = "Hugo"; sn.add_way(w);

    b("a", 0, 0)("b", 1, 1)("c", 2, 2)("d", 3, 3)("e", 4, 4);
    b("a", "b")("b","c")("c","d")("d","e")("e","d"); //bug ? if no edge leave the vertex, the projection cannot work...
    sn.graph[b.get("a","b")].way_idx = 0;
    sn.graph[b.get("b","c")].way_idx = 0;
    sn.graph[b.get("c","d")].way_idx = 1;
    sn.graph[b.get("d","e")].way_idx = 1;
    sn.graph[b.get("e","d")].way_idx = 1;

    sn.init();

    PathFinder path_finder(sn);
    path_finder.init({0, 0, true}, Mode_e::Walking, 1); //starting from a
    Path p = compute_path(path_finder, {4, 4, true}); //going to e
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 2);
    BOOST_CHECK_EQUAL(p.path_items[0].way_idx, 0);
    BOOST_CHECK_EQUAL(p.path_items[1].way_idx, 1);
//    BOOST_CHECK(p.path_items[0].segments[0] == b.get("a", "b"));
//    BOOST_CHECK(p.path_items[0].segments[1] == b.get("b", "c"));
//    BOOST_CHECK(p.path_items[1].segments[0] == b.get("c", "d"));
//    BOOST_CHECK(p.path_items[1].segments[1] == b.get("d", "e"));

    path_finder.init({3, 3, true}, Mode_e::Walking, 1); //starting from d
    p = compute_path(path_finder, {4, 4, true}); //going to e
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 1);
    BOOST_CHECK_EQUAL(p.path_items[0].way_idx, 1);
}

// On teste le calcul d'itinéraire de coordonnées à coordonnées
BOOST_AUTO_TEST_CASE(compute_coord){
    using namespace navitia::type;
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);
    PathFinder path_finder(sn);

    /*           a+------+b
     *            |      |
     *            |      |
     *           c+------+d
     */

    b("a",0,0)("b",10,0)("c",0,10)("d",10,10);
    b("a","b", 10_s)("b","a",10_s)("a","c",10_s)("c","a",10_s)("b","d",10_s)("d","b",10_s)("c","d",10_s)("d","c",10_s);

    Way w;
    w.name = "BobAB"; sn.add_way(w);
    w.name = "BobAC"; sn.add_way(w);
    w.name = "BobCD"; sn.add_way(w);
    w.name = "BobDB"; sn.add_way(w);
    sn.graph[b.get("a","b")].way_idx = 0;
    sn.graph[b.get("b","a")].way_idx = 0;
    sn.graph[b.get("a","c")].way_idx = 1;
    sn.graph[b.get("c","a")].way_idx = 1;
    sn.graph[b.get("c","d")].way_idx = 2;
    sn.graph[b.get("d","c")].way_idx = 2;
    sn.graph[b.get("d","b")].way_idx = 3;
    sn.graph[b.get("b","d")].way_idx = 3;

    GeographicalCoord start;
    start.set_xy(3, -1);
    GeographicalCoord destination;
    destination.set_xy(4, 11);
    sn.init();
    path_finder.init(start, Mode_e::Walking, 1);
    Path p = compute_path(path_finder, destination);
    auto coords = get_coords_from_path(p);
    print_coord(coords);
    BOOST_REQUIRE_EQUAL(coords.size(), 4);
    BOOST_REQUIRE_EQUAL(p.path_items.size(), 3);
    GeographicalCoord expected;
    expected.set_xy(3,0);
    BOOST_CHECK_EQUAL(coords[0], expected );
    expected.set_xy(0,0);
    BOOST_CHECK_EQUAL(coords[1], expected );
    expected.set_xy(0,10);
    BOOST_CHECK_EQUAL(coords[2], expected );
    expected.set_xy(4,10);
    BOOST_CHECK_EQUAL(coords[3], expected );

    // Trajet partiel : on ne parcourt pas un arc en entier, mais on passe par un nœud
    start.set_xy(7,6);
    path_finder.init(start, Mode_e::Walking, 1);
    p = compute_path(path_finder, destination);
    coords = get_coords_from_path(p);
    print_coord(coords);
    BOOST_CHECK_EQUAL(p.path_items.size(), 2);
    BOOST_REQUIRE_EQUAL(coords.size(), 3);
    BOOST_CHECK_EQUAL(coords[0], GeographicalCoord(10,6, false) );
    BOOST_CHECK_EQUAL(coords[1], GeographicalCoord(10,10, false) );
    BOOST_CHECK_EQUAL(coords[2], GeographicalCoord(4,10, false) );
}


BOOST_AUTO_TEST_CASE(compute_nearest){
    using namespace navitia::type;

    GeoRef sn;
    GraphBuilder b(sn);

    /*       1                    2
     *       +                    +
     *    o------o------o------o------o
     *    a      b      c      d      e
     */

    b("a",0,0)("b",100,0)("c",200,0)("d",300,0)("e",400,0);
    b("a","b",100_s)("b","a",100_s)("b","c",100_s)("c","b",100_s)("c","d",100_s)("d","c",100_s)("d","e",100_s)("e","d",100_s);

    GeographicalCoord c1(50,10, false);
    GeographicalCoord c2(350,20, false);
    navitia::proximitylist::ProximityList<idx_t> pl;
    pl.add(c1, 0);
    pl.add(c2, 1);
    pl.build();
    sn.init();

    StopPoint* sp1 = new StopPoint();
    sp1->coord = c1;
    StopPoint* sp2 = new StopPoint();
    sp2->coord = c2;
    std::vector<StopPoint*> stop_points;
    stop_points.push_back(sp1);
    stop_points.push_back(sp2);
    sn.project_stop_points(stop_points);

    GeographicalCoord o(0,0);

    StreetNetwork w(sn);
    EntryPoint starting_point;
    starting_point.coordinates = o;
    starting_point.streetnetwork_params.mode = Mode_e::Walking;
    starting_point.streetnetwork_params.speed_factor = 2; //to have a speed different from the default one (and greater not to have projection problems)
    w.init(starting_point);
    auto res = w.find_nearest_stop_points(10_s, pl, false);
    BOOST_CHECK_EQUAL(res.size(), 0);

    w.init(starting_point);//not mandatory, but reinit to clean the distance table to get fresh dijsktra
    res = w.find_nearest_stop_points(100_s, pl, false);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].first , 0);
    BOOST_CHECK_EQUAL(res[0].second, navitia::seconds(50 / (default_speed[Mode_e::Walking] * 2))); //the projection is done with the same mean of transport, at the same speed

    w.init(starting_point);
    res = w.find_nearest_stop_points(1000_s, pl, false);
    std::sort(res.begin(), res.end());
    BOOST_REQUIRE_EQUAL(res.size(), 2);
    BOOST_CHECK_EQUAL(res[0].first , 0);
    BOOST_CHECK_EQUAL(res[0].second, navitia::seconds(50 / (default_speed[Mode_e::Walking] * 2)));
    BOOST_CHECK_EQUAL(res[1].first , 1);
    //1 projections at the arrival, and 3 edges (100s each but at twice the speed)
    BOOST_CHECK_EQUAL(res[1].second, navitia::seconds(50 / (default_speed[Mode_e::Walking] * 2)) + 150_s);
}

// Récupérer les cordonnées d'un numéro impair :
BOOST_AUTO_TEST_CASE(numero_impair){
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

    way.name="AA";
    way.way_type="rue";
    nt::GeographicalCoord upper(1,53);
    nt::GeographicalCoord lower(1,3);

    hn.coord=lower;
    hn.number = 3;
    way.house_number_left.push_back(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v,graph);

    hn.coord.set_lon(1.0);
    hn.coord.set_lat(7.0);
    hn.number = 7;
    way.house_number_left.push_back(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v,graph);

    boost::add_edge(debut, fin,e1, graph);
    boost::add_edge(fin, debut,e1, graph);
    way.edges.push_back(std::make_pair(debut, fin));
    way.edges.push_back(std::make_pair(fin,debut));


    hn.coord.set_lon(1.0);
    hn.coord.set_lat(17.0);
    hn.number = 17;
    way.add_house_number(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v,graph);

    boost::add_edge(fin, debut,e1, graph);
    boost::add_edge(debut, fin,e1, graph);
    way.edges.push_back(std::make_pair(fin,debut));
    way.edges.push_back(std::make_pair(debut, fin));


    hn.coord= upper;
    hn.number = 53;
    way.add_house_number(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v,graph);

    boost::add_edge(debut, fin,e1, graph);
    boost::add_edge(fin, debut,e1, graph);
    way.edges.push_back(std::make_pair(debut, fin));
    way.edges.push_back(std::make_pair(fin,debut));

// Numéro recherché est > au plus grand numéro dans la rue
    nt::GeographicalCoord result = way.nearest_coord(55, graph);
    BOOST_CHECK_EQUAL(result, upper);

// Numéro recherché est < au plus petit numéro dans la rue
    result = way.nearest_coord(1, graph);
    BOOST_CHECK_EQUAL(result, lower);

// Numéro recherché est = à numéro dans la rue
    result = way.nearest_coord(17, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(1.0,17.0));

// Numéro recherché est n'existe pas mais il est inclus entre le plus petit et grand numéro de la rue
    // ==>  Extrapolation des coordonnées
   result = way.nearest_coord(43, graph);
   BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(1.0,43.0));



// liste des numéros pair est vide ==> Calcul du barycentre de la rue
   result = way.nearest_coord(40, graph);
   BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(1,28)); // 3 + 25
// les deux listes des numéros pair et impair sont vides ==> Calcul du barycentre de la rue
   way.house_number_left.clear();
   result = way.nearest_coord(9, graph);
   BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(1,28));

}

// Récupérer les cordonnées d'un numéro pair :
BOOST_AUTO_TEST_CASE(numero_pair){
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
    way.name="AA";
    way.way_type="rue";
    nt::GeographicalCoord upper(2.0,54.0);
    nt::GeographicalCoord lower(2.0,4.0);

    hn.coord=lower;
    hn.number = 4;
    way.add_house_number(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v,graph);

    hn.coord.set_lon(2.0);
    hn.coord.set_lat(8.0);
    hn.number = 8;
    way.add_house_number(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v,graph);

    boost::add_edge(debut, fin,e1, graph);
    boost::add_edge(fin, debut,e1, graph);
    way.edges.push_back(std::make_pair(debut, fin));
    way.edges.push_back(std::make_pair(fin,debut));


    hn.coord.set_lon(2.0);
    hn.coord.set_lat(18.0);
    hn.number = 18;
    way.add_house_number(hn);
    v.coord = hn.coord;
    debut = boost::add_vertex(v,graph);

    boost::add_edge(fin, debut,e1, graph);
    boost::add_edge(debut, fin,e1, graph);
    way.edges.push_back(std::make_pair(fin,debut));
    way.edges.push_back(std::make_pair(debut, fin));


    hn.coord= upper;
    hn.number = 54;
    way.add_house_number(hn);
    v.coord = hn.coord;
    fin = boost::add_vertex(v,graph);

    boost::add_edge(debut, fin,e1, graph);
    boost::add_edge(fin, debut,e1, graph);
    way.edges.push_back(std::make_pair(debut, fin));
    way.edges.push_back(std::make_pair(fin,debut));


// Numéro recherché est > au plus grand numéro dans la rue
    nt::GeographicalCoord result = way.nearest_coord(56, graph);
    BOOST_CHECK_EQUAL(result, upper);

// Numéro recherché est < au plus petit numéro dans la rue
    result = way.nearest_coord(2, graph);
    BOOST_CHECK_EQUAL(result, lower);

// Numéro recherché est = à numéro dans la rue
    result = way.nearest_coord(18, graph);
    BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(2.0,18.0));

// Numéro recherché est n'existe pas mais il est inclus entre le plus petit et grand numéro de la rue
    // ==>  Extrapolation des coordonnées
   result = way.nearest_coord(44, graph);
   BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(2.0,44.0));

// liste des numéros impair est vide ==> Calcul du barycentre de la rue
   result = way.nearest_coord(41, graph);
   BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(2,29)); // 4+25

// les deux listes des numéros pair et impair sont vides ==> Calcul du barycentre de la rue
   way.house_number_right.clear();
   result = way.nearest_coord(10, graph);
   BOOST_CHECK_EQUAL(result, nt::GeographicalCoord(2,29));
}

// Recherche d'un numéro à partir des coordonnées
BOOST_AUTO_TEST_CASE(coord){
    navitia::georef::Way way;
    navitia::georef::HouseNumber hn;

 /*
(2,4)       (2,8)       (2,18)       (2,54)
  4           8           18           54
*/

    way.name="AA";
    way.way_type="rue";
    nt::GeographicalCoord upper(2.0,54.0);
    nt::GeographicalCoord lower(2.0,4.0);

    hn.coord=lower;
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

    hn.coord= upper;
    hn.number = 54;
    way.add_house_number(hn);



// les coordonnées sont à l'extérieur de la rue coté supérieur
    int result = way.nearest_number(nt::GeographicalCoord(1.0,55.0));
    BOOST_CHECK_EQUAL(result, 54);

// les coordonnées sont à l'extérieur de la rue coté inférieur
    result = way.nearest_number(nt::GeographicalCoord(2.0,3.0));
    BOOST_CHECK_EQUAL(result, 4);

// coordonnées recherchées est = à coordonnées dans la rue
    result = way.nearest_number(nt::GeographicalCoord(2.0,8.0));
    BOOST_CHECK_EQUAL(result, 8);

// les deux listes des numéros sont vides
    way.house_number_right.clear();
    result = way.nearest_number(nt::GeographicalCoord(2.0,8.0));
    BOOST_CHECK_EQUAL(result, -1);

}
// Test de autocomplete
BOOST_AUTO_TEST_CASE(build_autocomplete_test){

        navitia::georef::GeoRef geo_ref;
        navitia::georef::HouseNumber hn;
        navitia::georef::Graph graph;
        vertex_t debut, fin;
        Vertex v;
        navitia::georef::Edge e1;
        std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> result;
        int nbmax = 10;

        navitia::georef::Way  way;
        way.name = "jeanne d'arc";
        way.way_type = "rue";
        geo_ref.add_way(way);


        way.name = "jean jaures";
        way.way_type = "place";
        geo_ref.add_way(way);


        way.name = "jean paul gaultier paris";
        way.way_type = "rue";
        geo_ref.add_way(way);


        way.name = "jean jaures";
        way.way_type = "avenue";
        geo_ref.add_way(way);


        way.name = "poniatowski";
        way.way_type = "boulevard";
        geo_ref.add_way(way);


        way.name = "pente de Bray";
        way.way_type = "";
        geo_ref.add_way(way);

        /*
       (2,4)       (2,4)       (2,18)       (2,54)
         4           4           18           54
       */

        way.name = "jean jaures";
        way.way_type = "rue";
        // Ajout des numéros et les noeuds
        nt::GeographicalCoord upper(2.0,54.0);
        nt::GeographicalCoord lower(2.0,4.0);

        hn.coord=lower;
        hn.number = 4;
        way.add_house_number(hn);
        v.coord = hn.coord;
        debut = boost::add_vertex(v,graph);

        hn.coord.set_lon(2.0);
        hn.coord.set_lat(8.0);
        hn.number = 8;
        way.add_house_number(hn);
        v.coord = hn.coord;
        fin = boost::add_vertex(v,graph);

        boost::add_edge(debut, fin,e1, graph);
        boost::add_edge(fin, debut,e1, graph);
        way.edges.push_back(std::make_pair(debut, fin));
        way.edges.push_back(std::make_pair(fin,debut));


        hn.coord.set_lon(2.0);
        hn.coord.set_lat(18.0);
        hn.number = 18;
        way.add_house_number(hn);
        v.coord = hn.coord;
        debut = boost::add_vertex(v,graph);

        boost::add_edge(fin, debut,e1, graph);
        boost::add_edge(debut, fin,e1, graph);
        way.edges.push_back(std::make_pair(fin,debut));
        way.edges.push_back(std::make_pair(debut, fin));


        hn.coord= upper;
        hn.number = 54;
        way.add_house_number(hn);
        v.coord = hn.coord;
        fin = boost::add_vertex(v,graph);

        boost::add_edge(debut, fin,e1, graph);
        boost::add_edge(fin, debut,e1, graph);
        way.edges.push_back(std::make_pair(debut, fin));
        way.edges.push_back(std::make_pair(fin,debut));

        geo_ref.add_way(way);

        /*way.name = "jean zay";
        way.way_type = "rue";
        geo_ref.add_way(way);

        way.name = "jean paul gaultier";
        way.way_type = "place";
        geo_ref.add_way(way);*/

        geo_ref.build_autocomplete_list();

        result = geo_ref.find_ways("10 rue jean jaures", nbmax, false, [](int){return true;});
        if (result.empty())
            result.clear();

    }

BOOST_AUTO_TEST_CASE(two_scc) {
    using namespace navitia::type;

    GeoRef sn;
    GraphBuilder b(sn);

    /*       1             2
     *       +             +
     *    o------o   o---o------o
     *    a      b   c   d      e
     */

    b("a",0,0)("b",100,0)("c",200,0)("d",300,0)("e",400,0);
    b("a","b",navitia::seconds(100))("b","a",navitia::seconds(100))("c","d",navitia::seconds(100))("d","c",navitia::seconds(100))("d","e",navitia::seconds(100))("e","d",navitia::seconds(100));

    GeographicalCoord c1(50,10, false);
    GeographicalCoord c2(350,20, false);
    navitia::proximitylist::ProximityList<idx_t> pl;
    pl.add(c1, 0);
    pl.add(c2, 1);
    pl.build();
    sn.init();

    StopPoint* sp1 = new StopPoint();
    sp1->coord = c1;
    StopPoint* sp2 = new StopPoint();
    sp2->coord = c2;
    std::vector<StopPoint*> stop_points;
    stop_points.push_back(sp1);
    stop_points.push_back(sp2);
    sn.project_stop_points(stop_points);

    StreetNetwork w(sn);

    EntryPoint starting_point;
    starting_point.coordinates = c1;
    starting_point.streetnetwork_params.mode = Mode_e::Walking;
    w.init(starting_point);

    auto max = w.get_distance(1, false);
    BOOST_CHECK_EQUAL(max, bt::pos_infin);
}

BOOST_AUTO_TEST_CASE(angle_computation) {
    //simple case

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

    nt::GeographicalCoord a {48.849143, 2.391776};
    nt::GeographicalCoord b {48.850456, 2.390596};
    nt::GeographicalCoord c {48.850428, 2.387356};
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

//small test to make sure the time manipulation works in the SpeedDistanceCombiner
BOOST_AUTO_TEST_CASE(SpeedDistanceCombiner_test) {
    navitia::time_duration dur = 10_s;

    SpeedDistanceCombiner comb(2);

    BOOST_CHECK_EQUAL(dur / 2, 5_s);

    navitia::time_duration dur2 = 60_s;
    BOOST_CHECK_EQUAL(comb(dur, dur2), navitia::seconds(10+60/2));
}

BOOST_AUTO_TEST_CASE(SpeedDistanceCombiner_test2) {
    navitia::time_duration dur = 10_s;

    SpeedDistanceCombiner comb(0.5);

    BOOST_CHECK_EQUAL(dur / 0.5, 20_s);

    navitia::time_duration dur2 = 60_s;
    BOOST_CHECK_EQUAL(comb(dur, dur2), 130_s);
}

//test allowed mode creation
BOOST_AUTO_TEST_CASE(transportation_mode_creation) {

    const auto allowed_transportation_mode = create_from_allowedlist({{{
                                                                    {nt::Mode_e::Walking},
                                                                    {},
                                                                    {nt::Mode_e::Walking, nt::Mode_e::Car},
                                                                    {nt::Mode_e::Walking, nt::Mode_e::Bss}
                                                              }}});

    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Walking][nt::Mode_e::Walking], true);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Walking][nt::Mode_e::Car], false);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Walking][nt::Mode_e::Bss], false);

    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Bss][nt::Mode_e::Walking], true);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Bss][nt::Mode_e::Bss], true);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Bss][nt::Mode_e::Car], false);
    BOOST_CHECK_EQUAL(allowed_transportation_mode[nt::Mode_e::Bss][nt::Mode_e::Bike], false);
}
