#pragma once
#include "type/data.h"
#include "boost/functional/hash.hpp"
namespace navitia { namespace timetables {


struct Node {
    type::idx_t sp_idx;
    uint32_t nb;

    Node() : sp_idx(type::invalid_idx), nb(0) {}
    Node(type::idx_t idx) : sp_idx(idx), nb(0) {}

    bool operator==(const Node & other) const {
        return (this->sp_idx == other.sp_idx) && (this->nb == other.nb);
    }

};

std::size_t hash_value(Node const& n);


typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, Node, boost::no_property> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;
typedef boost::hash<Node> hashnode;
typedef std::unordered_map<Node, vertex_t, hashnode> map_nodevertex;

struct Thermometer {
    vertex_t null_vertex = boost::graph_traits<Graph>::null_vertex();


    type::Data& d;

    Thermometer(type::Data &d) : d(d), line_idx(type::invalid_idx) {}

    std::vector<type::idx_t> get_thermometer(type::idx_t line_idx);

    std::vector<uint32_t> match_route(const type::Route & route);

    struct cant_match {
        type::idx_t rp_idx;
        cant_match() : rp_idx(type::invalid_idx) {}
        cant_match(type::idx_t rp_idx) : rp_idx(rp_idx) {}
    };

private :
    std::vector<type::idx_t> thermometer;
    type::idx_t line_idx;
    void generate_thermometer();
    Graph route2graph(const type::Route &route);

    Graph unify_graphes(const std::vector<Graph> graphes);

    void break_cycles(Graph & graph, map_nodevertex &node_vertices);

    map_nodevertex generate_map(const Graph &graph);
};




}}
