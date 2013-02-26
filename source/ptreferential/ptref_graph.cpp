#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include "type/type.h"
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
        vertex_map[Type_e::eValidityPattern] = boost::add_vertex(Type_e::eValidityPattern, g);
        vertex_map[Type_e::eLine] = boost::add_vertex(Type_e::eLine, g);
        vertex_map[Type_e::eRoute] = boost::add_vertex(Type_e::eRoute, g);
        vertex_map[Type_e::eStopPoint] = boost::add_vertex(Type_e::eStopPoint, g);
        vertex_map[Type_e::eStopArea] = boost::add_vertex(Type_e::eStopArea, g);
        vertex_map[Type_e::eNetwork] = boost::add_vertex(Type_e::eNetwork, g);
        vertex_map[Type_e::ePhysicalMode] = boost::add_vertex(Type_e::ePhysicalMode, g);
        vertex_map[Type_e::eCommercialMode] = boost::add_vertex(Type_e::eCommercialMode, g);
        vertex_map[Type_e::eCity] = boost::add_vertex(Type_e::eCity, g);
        vertex_map[Type_e::eConnection] = boost::add_vertex(Type_e::eConnection, g);
        vertex_map[Type_e::eRoutePoint] = boost::add_vertex(Type_e::eRoutePoint, g);
        vertex_map[Type_e::eDistrict] = boost::add_vertex(Type_e::eDistrict, g);
        vertex_map[Type_e::eCompany] = boost::add_vertex(Type_e::eCompany, g);
        vertex_map[Type_e::eCountry] = boost::add_vertex(Type_e::eCountry, g);

        boost::add_edge(vertex_map[Type_e::eDistrict], vertex_map[Type_e::eCountry], g);
        boost::add_edge(vertex_map[Type_e::eDepartment], vertex_map[Type_e::eDistrict], g);
        
        boost::add_edge(vertex_map[Type_e::eDepartment], vertex_map[Type_e::eCity], g);

        boost::add_edge(vertex_map[Type_e::eStopArea], vertex_map[Type_e::eCity], g);
        boost::add_edge(vertex_map[Type_e::eCity], vertex_map[Type_e::eStopArea], g);


        boost::add_edge(vertex_map[Type_e::eStopPoint], vertex_map[Type_e::eStopArea], g);

        boost::add_edge(vertex_map[Type_e::eLine], vertex_map[Type_e::eNetwork], g);

        boost::add_edge(vertex_map[Type_e::eLine], vertex_map[Type_e::eCompany], g);

        boost::add_edge(vertex_map[Type_e::eLine], vertex_map[Type_e::eCommercialMode], g);
        boost::add_edge(vertex_map[Type_e::ePhysicalMode], vertex_map[Type_e::eCommercialMode], g);

        boost::add_edge(vertex_map[Type_e::eCommercialMode], vertex_map[Type_e::ePhysicalMode], g);

        boost::add_edge(vertex_map[Type_e::eCommercialMode], vertex_map[Type_e::eLine], g);
        boost::add_edge(vertex_map[Type_e::ePhysicalMode], vertex_map[Type_e::eLine], g);
        boost::add_edge(vertex_map[Type_e::eCompany], vertex_map[Type_e::eLine], g);
        boost::add_edge(vertex_map[Type_e::eLine], vertex_map[Type_e::eCompany], g);
        boost::add_edge(vertex_map[Type_e::eNetwork], vertex_map[Type_e::eLine], g);
        boost::add_edge(vertex_map[Type_e::eRoute], vertex_map[Type_e::eLine], g);

        boost::add_edge(vertex_map[Type_e::eLine], vertex_map[Type_e::eRoute], g);
        boost::add_edge(vertex_map[Type_e::eCommercialMode], vertex_map[Type_e::eRoute], g);
        boost::add_edge(vertex_map[Type_e::eRoutePoint], vertex_map[Type_e::eRoute], g);

        boost::add_edge(vertex_map[Type_e::eRoute], vertex_map[Type_e::eRoutePoint], g);
        boost::add_edge(vertex_map[Type_e::eStopPoint], vertex_map[Type_e::eRoutePoint], g);

        boost::add_edge(vertex_map[Type_e::eStopArea], vertex_map[Type_e::eStopPoint], g);
        boost::add_edge(vertex_map[Type_e::eCity], vertex_map[Type_e::eStopPoint], g);
        boost::add_edge(vertex_map[Type_e::eRoutePoint], vertex_map[Type_e::eStopPoint], g);
        boost::add_edge(vertex_map[Type_e::eConnection], vertex_map[Type_e::eStopPoint], g);

        boost::add_edge(vertex_map[Type_e::eStopPoint], vertex_map[Type_e::eConnection], g);
        
        boost::add_edge(vertex_map[Type_e::eVehicleJourney], vertex_map[Type_e::eRoute], g);
        boost::add_edge(vertex_map[Type_e::eRoute], vertex_map[Type_e::eVehicleJourney], g);
        boost::add_edge(vertex_map[Type_e::eVehicleJourney], vertex_map[Type_e::eStopTime], g);

    }

};

std::map<Type_e,Type_e> find_path(Type_e source) {
    Jointures j;
    std::vector<vertex_t> predecessors(boost::num_vertices(j.g));
    boost::dijkstra_shortest_paths(j.g, j.vertex_map[source],
                                   boost::predecessor_map(&predecessors[0]).
                                   weight_map(boost::get(&Edge::weight, j.g)));


    std::map<Type_e, Type_e> result;

    for(vertex_t u = 0; u < boost::num_vertices(j.g); ++u)
        result[j.g[u]] = j.g[predecessors[u]];
    return result;
}

} } //namespace navitia::ptref
