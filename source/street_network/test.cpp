#include "types.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>

//using namespace navitia::streetnetwork;
using namespace navitia::georef;
using namespace boost;
BOOST_AUTO_TEST_CASE(outil_de_graph) {
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder builder(sn);
    Graph & g = sn.graph;

    BOOST_CHECK_EQUAL(num_vertices(g), 0);
    BOOST_CHECK_EQUAL(num_edges(g), 0);

    // Construction de deux nœuds et d'un arc
    builder("a",0, 0)("b",1,2)("a", "b", 10);
    BOOST_CHECK_EQUAL(num_vertices(g), 2);
    BOOST_CHECK_EQUAL(num_edges(g), 1);

    // On vérifie que les propriétés sont bien définies
    vertex_t a = builder.vertex_map["a"];
    BOOST_CHECK_EQUAL(g[a].coord.x, 0);
    BOOST_CHECK_EQUAL(g[a].coord.y, 0);

    vertex_t b = builder.vertex_map["b"];
    BOOST_CHECK_EQUAL(g[b].coord.x, 1);
    BOOST_CHECK_EQUAL(g[b].coord.y, 2);

    edge_t e = edge(a, b, g).first;
    BOOST_CHECK_EQUAL(g[e].length, 10);

    // Construction implicite de nœuds
    builder("c", "d", 42);
    BOOST_CHECK_EQUAL(num_vertices(g), 4);
    BOOST_CHECK_EQUAL(num_edges(g), 2);

    builder("a", "c");
    BOOST_CHECK_EQUAL(num_vertices(g), 4);
    BOOST_CHECK_EQUAL(num_edges(g), 3);
}

BOOST_AUTO_TEST_CASE(projection) {
    using namespace navitia::type;
    using boost::tie;

    GeographicalCoord a(0,0, false); // Début de segment
    GeographicalCoord b(10, 0, false); // Fin de segment
    GeographicalCoord p(5, 5, false); // Point à projeter
    GeographicalCoord pp; //Point projeté
    float d; // Distance du projeté
    tie(pp,d) = project(p, a, b);

    BOOST_CHECK_EQUAL(pp.x, 5);
    BOOST_CHECK_EQUAL(pp.y, 0);
    BOOST_CHECK_EQUAL(d, 5);

    // Si on est à l'extérieur à gauche
    p.x = -10; p.y = 0;
    tie(pp,d) = project(p, a, b);
    BOOST_CHECK_EQUAL(pp.x, a.x);
    BOOST_CHECK_EQUAL(pp.y, a.y);
    BOOST_CHECK_EQUAL(d, 10);

    // Si on est à l'extérieur à droite
    p.x = 20; p.y = 0;
    tie(pp,d) = project(p, a, b);
    BOOST_CHECK_EQUAL(pp.x, b.x);
    BOOST_CHECK_EQUAL(pp.y, b.y);
    BOOST_CHECK_EQUAL(d, 10);

    // On refait la même en permutant A et B

    // Si on est à l'extérieur à gauche
    p.x = -10; p.y = 0;
    tie(pp,d) = project(p, b, a);
    BOOST_CHECK_EQUAL(pp.x, a.x);
    BOOST_CHECK_EQUAL(pp.y, a.y);
    BOOST_CHECK_EQUAL(d, 10);

    // Si on est à l'extérieur à droite
    p.x = 20; p.y = 0;
    tie(pp,d) = project(p, b, a);
    BOOST_CHECK_EQUAL(pp.x, b.x);
    BOOST_CHECK_EQUAL(pp.y, b.y);
    BOOST_CHECK_EQUAL(d, 10);

    // On essaye avec des trucs plus pentus ;)
    a.x = -3; a.y = -3;
    b.x = 5; b.y = 5;
    p.x = -2;  p.y = 2;
    tie(pp,d) = project(p, a, b);
    BOOST_CHECK_SMALL(pp.x, 1e-3);
    BOOST_CHECK_SMALL(pp.y, 1e-3);
    BOOST_CHECK_CLOSE(d, ::sqrt(2)*2, 1e-3);
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

    navitia::type::GeographicalCoord c(1,2,false);
    BOOST_CHECK(sn.nearest_edge(c) == b.get("o", "a"));
    c.x = 2; c.y = 1;
    BOOST_CHECK(sn.nearest_edge(c) == b.get("o", "c"));
    c.x = 2; c.y = -1;
    BOOST_CHECK(sn.nearest_edge(c) == b.get("o", "c"));
    c.x = 2; c.y = -3;
    BOOST_CHECK(sn.nearest_edge(c) == b.get("o", "d"));
    c.x = -10; c.y = 1;
    BOOST_CHECK(sn.nearest_edge(c) == b.get("b", "o"));
    c.x = 50; c.y = 10;
    BOOST_CHECK_THROW(sn.nearest_edge(c), navitia::proximitylist::NotFound);
}

// Est-ce que le calcul de plusieurs nœuds vers plusieurs nœuds fonctionne
BOOST_AUTO_TEST_CASE(compute_route_n_n){
    using namespace navitia::type;
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);

    /*               a           e
                     |
                  b—–o––c
                     |
                     d             */
    b("e", 0,0)("a",0,1)("b",0,2)("c",0,3)("d",0,4)("o",0,5);
    b("a", "o", 1)("b","o",2)("o","c", 3)("o","d", 4);

    std::vector<vertex_t> starts = {b.get("a"), b.get("b")};
    std::vector<vertex_t> dests = {b.get("c"), b.get("d")};
    Path p = sn.compute(starts, dests);

    BOOST_CHECK_EQUAL(p.coordinates.size(), 3);
    BOOST_CHECK_EQUAL(p.coordinates[0], GeographicalCoord(0,1,false)); // a
    BOOST_CHECK_EQUAL(p.coordinates[1], GeographicalCoord(0,5,false)); // o
    BOOST_CHECK_EQUAL(p.coordinates[2], GeographicalCoord(0,3,false)); // c

    // On lève une exception s'il n'y a pas d'itinéraire
    starts = {b.get("e")};
    dests = {b.get("a")};
    BOOST_CHECK_THROW(sn.compute(starts, dests), navitia::proximitylist::NotFound);

    // Cas où le nœud de départ et d'arrivée sont confondus
    starts = {b.get("a")};
    dests = {b.get("a")};
    p = sn.compute(starts, dests);
    BOOST_CHECK_EQUAL(p.coordinates.size(), 1);
    BOOST_CHECK_EQUAL(p.path_items.size(), 0);
    BOOST_CHECK(p.coordinates[0] == GeographicalCoord(0,1,false)); // a
}

// On teste la prise en compte de la distance initiale au nœud
BOOST_AUTO_TEST_CASE(compute_zeros){
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);
    b("a", "o", 1)("b", "o",2);
    std::vector<vertex_t> starts = {b.get("a"), b.get("b")};
    std::vector<vertex_t> dests = {b.get("o")};

    Path p = sn.compute(starts, dests);
    BOOST_CHECK_EQUAL(p.path_items.size(), 1);
    BOOST_CHECK(p.path_items[0].segments[0] == b.get("a","o"));

    p = sn.compute(starts, dests, {3,1});
    BOOST_CHECK(p.path_items[0].segments[0] == b.get("b","o"));

    p = sn.compute(starts, dests, {2,2});
    BOOST_CHECK(p.path_items[0].segments[0] == b.get("a","o"));
}

// Est-ce que les indications retournées sont bonnes
BOOST_AUTO_TEST_CASE(compute_directions){
    using namespace navitia::type;
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);
    Way w;
    w.name = "Jaures"; sn.ways.push_back(w);
    w.name = "Hugo"; sn.ways.push_back(w);

    b("a", "b")("b","c")("c","d")("d","e");
    sn.graph[b.get("a","b")].way_idx = 0;
    sn.graph[b.get("b","c")].way_idx = 0;
    sn.graph[b.get("c","d")].way_idx = 1;
    sn.graph[b.get("d","e")].way_idx = 1;

    std::vector<vertex_t> starts = {b.get("a")};
    std::vector<vertex_t> dests = {b.get("e")};
    Path p = sn.compute(starts, dests);
    BOOST_CHECK_EQUAL(p.path_items.size(), 2);
    BOOST_CHECK_EQUAL(p.path_items[0].way_idx, 0);
    BOOST_CHECK_EQUAL(p.path_items[1].way_idx, 1);
    BOOST_CHECK(p.path_items[0].segments[0] == b.get("a", "b"));
    BOOST_CHECK(p.path_items[0].segments[1] == b.get("b", "c"));
    BOOST_CHECK(p.path_items[1].segments[0] == b.get("c", "d"));
    BOOST_CHECK(p.path_items[1].segments[1] == b.get("d", "e"));

    starts = {b.get("d")};
    dests = {b.get("e")};
    p = sn.compute(starts, dests);
    BOOST_CHECK_EQUAL(p.path_items.size(), 1);
    BOOST_CHECK_EQUAL(p.path_items[0].way_idx, 1);
}

// On teste le calcul d'itinéraire de coordonnées à coordonnées
BOOST_AUTO_TEST_CASE(compute_coord){
    using namespace navitia::type;
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);

    /*           a+------+b
     *            |      |
     *            |      |
     *           c+------+d
     */

    b("a",0,0)("b",10,0)("c",0,10)("d",10,10);
    b("a","b", 10)("b","a",10)("a","c",10)("b","d",10)("c","d",10)("d","c",10);

    GeographicalCoord start(3, -1, false);
    GeographicalCoord destination(4, 11, false);
    Path p = sn.compute(start, destination);
    BOOST_CHECK_EQUAL(p.coordinates.size(), 4);
    BOOST_CHECK_EQUAL(p.path_items.size(), 1);
    BOOST_CHECK_EQUAL(p.coordinates[0], GeographicalCoord(3,0,false) );
    BOOST_CHECK_EQUAL(p.coordinates[1], GeographicalCoord(0,0,false) );
    BOOST_CHECK_EQUAL(p.coordinates[2], GeographicalCoord(0,10,false) );
    BOOST_CHECK_EQUAL(p.coordinates[3], GeographicalCoord(4,10,false) );

    start.x = 6; destination.x = 7;
    p = sn.compute(start, destination);
    BOOST_CHECK_EQUAL(p.coordinates.size(), 4);
    BOOST_CHECK_EQUAL(p.path_items.size(), 1);
    BOOST_CHECK_EQUAL(p.coordinates[0], GeographicalCoord(6,0,false) );
    BOOST_CHECK_EQUAL(p.coordinates[1], GeographicalCoord(10,0,false) );
    BOOST_CHECK_EQUAL(p.coordinates[2], GeographicalCoord(10,10,false) );
    BOOST_CHECK_EQUAL(p.coordinates[3], GeographicalCoord(7,10,false) );
}

BOOST_AUTO_TEST_CASE(compute_nearest){
    using namespace navitia::type;
    //StreetNetwork sn;
    GeoRef sn;
    GraphBuilder b(sn);

    /*       1             2
     *       +             +
     *    o------o---o---o------o
     *    a      b   c   d      e
     */

    b("a",0,0)("b",100,0)("c",200,0)("d",300,0)("e",400,0);
    b("a","b",100)("b","a",100)("b","c",100)("c","b",100)("c","d",100)("d","c",100)("d","e",100)("e","d",100);

    GeographicalCoord c1(50,10,false);
    GeographicalCoord c2(350,20,false);
    navitia::proximitylist::ProximityList<idx_t> pl;
    pl.add(c1, 1);
    pl.add(c2, 2);
    pl.build();

    GeographicalCoord o(0,0,false);

    StreetNetworkWorker w(sn);

    auto res = w.find_nearest(o, pl, 10);
    BOOST_CHECK_EQUAL(res.size(), 0);

    res = w.find_nearest(o, pl, 100);
    BOOST_CHECK_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].first , 1);
    BOOST_CHECK_CLOSE(res[0].second, 50, 1);

    res = w.find_nearest(o, pl, 1000);
    std::sort(res.begin(), res.end());
    BOOST_CHECK_EQUAL(res.size(), 2);
    BOOST_CHECK_EQUAL(res[0].first , 1);
    BOOST_CHECK_CLOSE(res[0].second, 50, 1);
    BOOST_CHECK_EQUAL(res[1].first , 2);
    BOOST_CHECK_CLOSE(res[1].second, 350, 1);
}

// Récupérer les cordonnées d'un numéro :

BOOST_AUTO_TEST_CASE(coord_housenumber){
    navitia::georef::Way way;
    navitia::georef::HouseNumber hn;

    /*
rue canoville
(1,1)       (1,7)       (1,10)       (1,18)
2           8           16           22
      */
    way.name="canoville";
    way.way_type="rue";
    hn.coord.x=1.0;
    hn.coord.y=1.0;
    hn.number = 2;
    way.house_number_right.push_back(hn);

    hn.coord.x=1.0;
    hn.coord.y=7.0;
    hn.number = 8;
    way.house_number_right.push_back(hn);

    hn.coord.x=1.0;
    hn.coord.y=10.0;
    hn.number = 16;
    way.house_number_right.push_back(hn);

    hn.coord.x=1.0;
    hn.coord.y=18.0;
    hn.number = 22;
    way.house_number_right.push_back(hn);

    way.sort_house_number();
    nt::GeographicalCoord to_return = way.get_geographicalCoord_by_number(10);
    if (to_return.x ==0)
        to_return.x =0;

}
