#pragma once

#include "type/type.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

using namespace navitia::type;
namespace navitia { namespace ptref {

struct Edge {
    float weight;
    Edge() : weight(1){}
};

struct Node {
    Type_e type;
    Node(Type_e type) : type(type) {}
};

typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, Type_e, Edge > Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

struct Jointures {
    std::map<Type_e, vertex_t> vertex_map;
    Graph g;
    Jointures();
};

}}
