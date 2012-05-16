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

    // Construction implicite d'arcs
    builder("c", "d", 42);
    BOOST_CHECK_EQUAL(num_vertices(g), 4);
    BOOST_CHECK_EQUAL(num_edges(g), 2);

    builder("a", "c");
    BOOST_CHECK_EQUAL(num_vertices(g), 4);
    BOOST_CHECK_EQUAL(num_edges(g), 3);
}
