#include "ptreferential.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/foreach.hpp>
// Ce fichier existe pour
// 1. Mieux séparer fonctionnellement les choses
// 2. Éviter trop de recompilation
// 3. Parce ces $"«(»)# de chez boost on un clash de nom entre boost::variant et property_map...

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

/** Contient le graph des transitions
 *
 * Un arc (u,v) entre les entités u et v, veut dire que l'on peut
 * obtenir U à partir de V
 *
 * Cela peut sembler contre-intuitif, mais cela permet de ne faire qu'un
 * seul appel à Dijkstra pour avoir tout ce qui nous interesse
 */
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, Type_e, Edge > Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

struct Jointures {
    std::map<Type_e, vertex_t> vertex_map;
    Graph g;

    Jointures() {
        vertex_map[eValidityPattern] = boost::add_vertex(eValidityPattern, g);
        vertex_map[eLine] = boost::add_vertex(eLine, g);
        vertex_map[eRoute] = boost::add_vertex(eRoute, g);
        vertex_map[eVehicleJourney] = boost::add_vertex(eVehicleJourney, g);
        vertex_map[eStopPoint] = boost::add_vertex(eStopPoint, g);
        vertex_map[eStopArea] = boost::add_vertex(eStopArea, g);
        vertex_map[eStopTime] = boost::add_vertex(eStopTime, g);
        vertex_map[eNetwork] = boost::add_vertex(eNetwork, g);
        vertex_map[eMode] = boost::add_vertex(eMode, g);
        vertex_map[eModeType] = boost::add_vertex(eModeType, g);
        vertex_map[eCity] = boost::add_vertex(eCity, g);
        vertex_map[eConnection] = boost::add_vertex(eConnection, g);
        vertex_map[eRoutePoint] = boost::add_vertex(eRoutePoint, g);
        vertex_map[eDistrict] = boost::add_vertex(eDistrict, g);
        vertex_map[eCompany] = boost::add_vertex(eCompany, g);
        vertex_map[eVehicle] = boost::add_vertex(eVehicle, g);
        vertex_map[eCountry] = boost::add_vertex(eCountry, g);

        boost::add_edge(vertex_map[eDistrict], vertex_map[eCountry], g);

        boost::add_edge(vertex_map[eStopArea], vertex_map[eCity], g);
        boost::add_edge(vertex_map[eDepartment], vertex_map[eCity], g);

        boost::add_edge(vertex_map[eStopPoint], vertex_map[eStopArea], g);
        boost::add_edge(vertex_map[eCity], vertex_map[eStopArea], g);

        boost::add_edge(vertex_map[eLine], vertex_map[eNetwork], g);

        boost::add_edge(vertex_map[eLine], vertex_map[eCompany], g);

        boost::add_edge(vertex_map[eLine], vertex_map[eModeType], g);
        boost::add_edge(vertex_map[eMode], vertex_map[eModeType], g);

        boost::add_edge(vertex_map[eModeType], vertex_map[eMode], g);

        boost::add_edge(vertex_map[eModeType], vertex_map[eLine], g);
        boost::add_edge(vertex_map[eMode], vertex_map[eLine], g);
        boost::add_edge(vertex_map[eCompany], vertex_map[eLine], g);
        boost::add_edge(vertex_map[eNetwork], vertex_map[eLine], g);
        boost::add_edge(vertex_map[eRoute], vertex_map[eLine], g);

        boost::add_edge(vertex_map[eLine], vertex_map[eRoute], g);
        boost::add_edge(vertex_map[eModeType], vertex_map[eRoute], g);
        boost::add_edge(vertex_map[eRoutePoint], vertex_map[eRoute], g);
        boost::add_edge(vertex_map[eVehicleJourney], vertex_map[eRoute], g);

        boost::add_edge(vertex_map[eRoute], vertex_map[eVehicleJourney], g);
        boost::add_edge(vertex_map[eCompany], vertex_map[eVehicleJourney], g);
        boost::add_edge(vertex_map[eMode], vertex_map[eVehicleJourney], g);
        boost::add_edge(vertex_map[eVehicle], vertex_map[eVehicleJourney], g);
        boost::add_edge(vertex_map[eValidityPattern], vertex_map[eVehicleJourney], g);

        boost::add_edge(vertex_map[eRoute], vertex_map[eRoutePoint], g);
        boost::add_edge(vertex_map[eStopPoint], vertex_map[eRoutePoint], g);

        boost::add_edge(vertex_map[eStopArea], vertex_map[eStopPoint], g);
        boost::add_edge(vertex_map[eCity], vertex_map[eStopPoint], g);
        boost::add_edge(vertex_map[eMode], vertex_map[eStopPoint], g);
        boost::add_edge(vertex_map[eNetwork], vertex_map[eStopPoint], g);

        boost::add_edge(vertex_map[eVehicle], vertex_map[eStopTime], g);
        boost::add_edge(vertex_map[eStopPoint], vertex_map[eStopTime], g);
    }
};

std::vector<Type_e> find_path(Type_e source) {
    Jointures j;
    std::vector<vertex_t> predecessors(boost::num_vertices(j.g));
    boost::dijkstra_shortest_paths(j.g, source,
                                   boost::predecessor_map(&predecessors[0]).
                                   weight_map(boost::get(&Edge::weight, j.g)));


    std::vector<Type_e> result;
    BOOST_FOREACH(vertex_t vertex, predecessors)
            result.push_back(j.g[vertex]);
    return result;
}

} } //namespace navitia::ptref
