#include "ptreferential.h"

#include <boost/graph/dijkstra_shortest_paths.hpp>
// Ce fichier existe pour
// 1. Mieux séparer fonctionnellement les choses
// 2. Éviter trop de recompilation
// 3. Parce ces $"«(»)# de chez boost on un clash de nom entre boost::variant et property_map...

namespace navitia { namespace ptref {

Jointures::Jointures() {
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

    boost::add_edge(vertex_map[eCountry], vertex_map[eDistrict], g);

    boost::add_edge(vertex_map[eCity], vertex_map[eStopArea], g);
    boost::add_edge(vertex_map[eCity], vertex_map[eDepartment], g);

    boost::add_edge(vertex_map[eStopArea], vertex_map[eStopPoint], g);
    boost::add_edge(vertex_map[eStopArea], vertex_map[eCity], g);

    boost::add_edge(vertex_map[eNetwork], vertex_map[eLine], g);

    boost::add_edge(vertex_map[eCompany], vertex_map[eLine], g);

    boost::add_edge(vertex_map[eModeType], vertex_map[eLine], g);
    boost::add_edge(vertex_map[eModeType], vertex_map[eMode], g);

    boost::add_edge(vertex_map[eMode], vertex_map[eModeType], g);

    boost::add_edge(vertex_map[eLine], vertex_map[eModeType], g);
    boost::add_edge(vertex_map[eLine], vertex_map[eMode], g);
    boost::add_edge(vertex_map[eLine], vertex_map[eCompany], g);
    boost::add_edge(vertex_map[eLine], vertex_map[eNetwork], g);
    boost::add_edge(vertex_map[eLine], vertex_map[eRoute], g);

    boost::add_edge(vertex_map[eRoute], vertex_map[eLine], g);
    boost::add_edge(vertex_map[eRoute], vertex_map[eModeType], g);
    boost::add_edge(vertex_map[eRoute], vertex_map[eRoutePoint], g);
    boost::add_edge(vertex_map[eRoute], vertex_map[eVehicleJourney], g);

    boost::add_edge(vertex_map[eVehicleJourney], vertex_map[eRoute], g);
    boost::add_edge(vertex_map[eVehicleJourney], vertex_map[eCompany], g);
    boost::add_edge(vertex_map[eVehicleJourney], vertex_map[eMode], g);
    boost::add_edge(vertex_map[eVehicleJourney], vertex_map[eVehicle], g);
    boost::add_edge(vertex_map[eVehicleJourney], vertex_map[eValidityPattern], g);

    boost::add_edge(vertex_map[eRoutePoint], vertex_map[eRoute], g);
    boost::add_edge(vertex_map[eRoutePoint], vertex_map[eStopPoint], g);

    boost::add_edge(vertex_map[eStopPoint], vertex_map[eStopArea], g);
    boost::add_edge(vertex_map[eStopPoint], vertex_map[eCity], g);
    boost::add_edge(vertex_map[eStopPoint], vertex_map[eMode], g);
    boost::add_edge(vertex_map[eStopPoint], vertex_map[eNetwork], g);

    boost::add_edge(vertex_map[eStopTime], vertex_map[eVehicle], g);
    boost::add_edge(vertex_map[eStopTime], vertex_map[eStopPoint], g);
}

std::vector<Type_e> Jointures::find_path(Type_e source) {
    std::vector<vertex_t> predecessors(boost::num_vertices(g));
    boost::dijkstra_shortest_paths(g, source,
                                   boost::predecessor_map(&predecessors[0]).
                                   weight_map(boost::get(&Edge::weight, g)));


    std::vector<Type_e> result;
    BOOST_FOREACH(vertex_t vertex, predecessors)
            result.push_back(g[vertex]);
    return result;
}

} } //namespace navitia::ptref
