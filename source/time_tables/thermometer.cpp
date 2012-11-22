#include "thermometer.h"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/topological_sort.hpp"

#include <boost/graph/graphviz.hpp> //DEBUG


namespace navitia { namespace timetables {

std::size_t hash_value(Node const& n)
{
        boost::hash<std::pair<type::idx_t, uint32_t>> hasher;
        return hasher(std::make_pair(n.sp_idx, n.nb));
}

struct vis_construct_graph : public boost::default_dfs_visitor
{
    template <class Edge, class Graph>
    void back_edge(Edge e , Graph& ) {
        throw e;
    }
};


map_nodevertex Thermometer::generate_map(const Graph &graph) {
    map_nodevertex result;
    boost::graph_traits<Graph>::vertex_iterator vi, vi_end, next;

    tie(vi, vi_end) = vertices(graph);
    for (next = vi; vi != vi_end; vi = next) {
        ++next;
        result.insert(std::make_pair(graph[*vi], *vi));
    }
    return result;
}




void Thermometer::break_cycles(Graph & graph, map_nodevertex &node_vertices) {
    vis_construct_graph v;

    try {
        boost::depth_first_search(graph, boost::visitor(v));
    } catch(edge_t e) {
        Node n = graph[boost::target(e, graph)];
        ++n.nb;
        node_vertices[n] = boost::add_vertex(n, graph);
        vertex_t source = boost::source(e, graph), target = node_vertices[n];
        boost::remove_edge(e, graph);
        boost::add_edge(source, target, graph);

        break_cycles(graph, node_vertices);
    }
}



Graph Thermometer::route2graph(const type::Route &route) {
    Graph result;
    vertex_t prev_vertex = null_vertex;
    std::/*unordered_*/map<type::idx_t, uint32_t> idx_nb;


    for(type::idx_t route_point_idx : route.route_point_list) {
        Node n;
        n.sp_idx = d.pt_data.route_points[route_point_idx].stop_point_idx;
        if(idx_nb.find(d.pt_data.route_points[route_point_idx].stop_point_idx) == idx_nb.end()) {
            n.nb = 0;
            idx_nb[d.pt_data.route_points[route_point_idx].stop_point_idx] = 0;
        } else {
            ++idx_nb[d.pt_data.route_points[route_point_idx].stop_point_idx];
            n.nb = idx_nb[d.pt_data.route_points[route_point_idx].stop_point_idx];
        }
        auto v = boost::add_vertex(n, result);

        if(prev_vertex != null_vertex) {
            boost::add_edge(prev_vertex, v, result);
        }
        prev_vertex = v;
    }

    return result;
}

Graph Thermometer::unify_graphes(const std::vector<Graph> graphes) {
    Graph result;
    map_nodevertex node_vertex;

    for(auto g : graphes) {
        auto iter_edges = boost::edges(g);
        auto first = iter_edges.first;
        auto last = iter_edges.second;

        while(first != last) {
            auto it_source = node_vertex.find(g[boost::source(*first, g)]);
            if(it_source == node_vertex.end()) {
                node_vertex.insert(std::make_pair(g[boost::source(*first, g)], boost::add_vertex(g[boost::source(*first, g)], result)));
            }
            it_source = node_vertex.find(g[boost::target(*first, g)]);
            if(it_source == node_vertex.end()) {
                node_vertex.insert(std::make_pair(g[boost::target(*first, g)], boost::add_vertex(g[boost::target(*first, g)], result)));
            }
            boost::add_edge(node_vertex[g[boost::source(*first, g)]], node_vertex[g[boost::target(*first, g)]], result);
            ++first;
        }
    }

    return result;
}



void Thermometer::generate_thermometer() {
    std::vector<Graph> graphes;
    for(type::Route route : d.pt_data.routes) {
        if(route.line_idx == line_idx) {
            graphes.push_back(route2graph(route));
        }
    }
    Graph unified_graph = this->unify_graphes(graphes);

    auto tmp = generate_map(unified_graph);
    this->break_cycles(unified_graph, tmp);

    std::vector<vertex_t> temp;
    boost::topological_sort(unified_graph, std::back_inserter(temp));
    std::reverse(temp.begin(), temp.end());

    for(vertex_t n : temp)
        thermometer.push_back(unified_graph[n].sp_idx);
}




std::vector<type::idx_t> Thermometer::get_thermometer(type::idx_t line_idx_) {
    if(line_idx_ != line_idx) {
        line_idx = line_idx_;
        generate_thermometer();
    }
    return thermometer;
}






std::vector<uint32_t> Thermometer::match_route(const type::Route & route) {
    std::vector<uint32_t> result;
    uint32_t last_pos = 0;
    auto it = thermometer.begin();
    for(type::idx_t rpidx : route.route_point_list) {
        it = std::find(it, thermometer.end(), d.pt_data.route_points[rpidx].stop_point_idx);
        if(it==result.end())
            throw cant_match(rpidx);
        else {
            result.push_back( distance(thermometer.begin(), it));
        }
        ++it;
    }

    return result;
}

}}
