#include "types.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>

using namespace navitia::streetnetwork;
using namespace boost;
BOOST_AUTO_TEST_CASE(outil_de_graph) {
    StreetNetwork sn;
    GraphBuilder builder(sn.graph);
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
