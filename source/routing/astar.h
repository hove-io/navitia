#pragma once
#include "routing.h"
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_traits.hpp"

namespace navitia { namespace routing { namespace astar {

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, boost::property<boost::edge_weight_t, uint> > GraphAStar;
/// Représentation d'un nœud dans le graphe
typedef boost::graph_traits<GraphAStar>::vertex_descriptor vertex_t;
typedef boost::graph_traits<GraphAStar>::edge_descriptor as_edge_t;


class Astar
{
public:
    const type::PT_Data & data;
    GraphAStar a_graph;
    std::vector<uint> min_time;

    size_t stop_point_offset,
           route_point_offset;

    Astar(const type::PT_Data &data);

    Path compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day);

    /// Génère le graphe sur le quel sera fait le calcul
    void build_graph();

    /** Calcule le temps minimal pour atteindre un nœud
     *
     *  Sert pour la heuristique A*
     */
    void build_heuristic(vertex_t destination);
};

}}}



