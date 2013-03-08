#include "ptref_graph.h"
#include "ptreferential.h"

namespace navitia { namespace ptref {


/** Contient le graph des transitions
 *
 * Un arc (u,v) entre les entités u et v, veut dire que l'on peut
 * obtenir U à partir de V
 *
 * Cela peut sembler contre-intuitif, mais cela permet de ne faire qu'un
 * seul appel à Dijkstra pour avoir tout ce qui nous interesse
 */

Jointures::Jointures() {
    vertex_map[Type_e::eValidityPattern] = boost::add_vertex(Type_e::eValidityPattern, g);
    vertex_map[Type_e::eLine] = boost::add_vertex(Type_e::eLine, g);
    vertex_map[Type_e::eJourneyPattern] = boost::add_vertex(Type_e::eJourneyPattern, g);
    vertex_map[Type_e::eStopPoint] = boost::add_vertex(Type_e::eStopPoint, g);
    vertex_map[Type_e::eStopArea] = boost::add_vertex(Type_e::eStopArea, g);
    vertex_map[Type_e::eNetwork] = boost::add_vertex(Type_e::eNetwork, g);
    vertex_map[Type_e::ePhysicalMode] = boost::add_vertex(Type_e::ePhysicalMode, g);
    vertex_map[Type_e::eCommercialMode] = boost::add_vertex(Type_e::eCommercialMode, g);
    vertex_map[Type_e::eCity] = boost::add_vertex(Type_e::eCity, g);
    vertex_map[Type_e::eConnection] = boost::add_vertex(Type_e::eConnection, g);
    vertex_map[Type_e::eJourneyPatternPoint] = boost::add_vertex(Type_e::eJourneyPatternPoint, g);
    vertex_map[Type_e::eDistrict] = boost::add_vertex(Type_e::eDistrict, g);
    vertex_map[Type_e::eCompany] = boost::add_vertex(Type_e::eCompany, g);
    vertex_map[Type_e::eCountry] = boost::add_vertex(Type_e::eCountry, g);
    vertex_map[Type_e::eRoute] = boost::add_vertex(Type_e::eRoute, g);
    vertex_map[Type_e::eDepartment] = boost::add_vertex(Type_e::eDepartment, g);
    vertex_map[Type_e::eVehicleJourney] = boost::add_vertex(Type_e::eVehicleJourney, g);

    // À partir d'une city, on peut avoir son departement et ses stop_points
    boost::add_edge(vertex_map.at(Type_e::eDepartment), vertex_map.at(Type_e::eCity), g);
    boost::add_edge(vertex_map.at(Type_e::eStopPoint), vertex_map.at(Type_e::eCity), g);

    // À partir d'un country, on peut avoir ses districts
    boost::add_edge(vertex_map.at(Type_e::eDistrict), vertex_map.at(Type_e::eCountry), g);

    // À partir d'un district, on peut avoir son country et ses départements
    boost::add_edge(vertex_map.at(Type_e::eCountry), vertex_map.at(Type_e::eDistrict),  g);
    boost::add_edge(vertex_map.at(Type_e::eDepartment), vertex_map.at(Type_e::eDistrict), g);

    // À partir d'un département, on peut avoir son district et ses citys
    boost::add_edge(vertex_map.at(Type_e::eDistrict), vertex_map.at(Type_e::eDepartment), g);
    boost::add_edge(vertex_map.at(Type_e::eCity), vertex_map.at(Type_e::eDepartment), g);

    // À partir d'un stop area, on peut avoir ses stop points et sa city
    boost::add_edge(vertex_map.at(Type_e::eCity), vertex_map.at(Type_e::eStopArea), g);
    boost::add_edge(vertex_map.at(Type_e::eStopPoint), vertex_map.at(Type_e::eStopArea), g);

    // À partir d'un network on peut avoir ses lignes
    boost::add_edge(vertex_map.at(Type_e::eLine), vertex_map.at(Type_e::eNetwork), g);

    // À partir d'une company, on peut avoir ses lignes
    boost::add_edge(vertex_map.at(Type_e::eLine), vertex_map.at(Type_e::eCompany), g);

    // À partir d'un commercial mode on peut avoir ses lignes et son mode physique
    boost::add_edge(vertex_map.at(Type_e::eLine), vertex_map.at(Type_e::eCommercialMode), g);

    // À partir d'un physical mode on peut avoir les vehicule journeys
    boost::add_edge(vertex_map.at(Type_e::eVehicleJourney), vertex_map.at(Type_e::ePhysicalMode), g);

    // À partir d'une ligne on peut avoir ses modes commerciaux, compagnies, réseaux et routes
    boost::add_edge(vertex_map.at(Type_e::eCommercialMode), vertex_map.at(Type_e::eLine), g);
    boost::add_edge(vertex_map.at(Type_e::eCompany), vertex_map.at(Type_e::eLine), g);
    boost::add_edge(vertex_map.at(Type_e::eNetwork), vertex_map.at(Type_e::eLine), g);
    boost::add_edge(vertex_map.at(Type_e::eRoute), vertex_map.at(Type_e::eLine), g);

    // À partir d'une route on a sa ligne et ses journey pattern
    boost::add_edge(vertex_map.at(Type_e::eJourneyPattern), vertex_map.at(Type_e::eRoute), g);
    boost::add_edge(vertex_map.at(Type_e::eLine), vertex_map.at(Type_e::eRoute), g);

    // À partir d'un journey pattern on peut avoir sa route, ses points et son vehicule journey
    boost::add_edge(vertex_map.at(Type_e::eRoute), vertex_map.at(Type_e::eJourneyPattern), g);
    boost::add_edge(vertex_map.at(Type_e::eJourneyPatternPoint), vertex_map.at(Type_e::eJourneyPattern), g);
    boost::add_edge(vertex_map.at(Type_e::eVehicleJourney), vertex_map.at(Type_e::eJourneyPattern), g);

    // À partir d'un vehicle journey, on peut avoir le journey parttern, la companie créole, le mode physique et le validity pattern
    boost::add_edge(vertex_map.at(Type_e::eJourneyPattern), vertex_map.at(Type_e::eVehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::eCompany), vertex_map.at(Type_e::eVehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::ePhysicalMode), vertex_map.at(Type_e::eVehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::eValidityPattern), vertex_map.at(Type_e::eVehicleJourney), g);

    // À partir d'un journey pattern point on obtient le journey pattern et le stop point
    boost::add_edge(vertex_map.at(Type_e::eJourneyPattern), vertex_map.at(Type_e::eJourneyPatternPoint), g);
    boost::add_edge(vertex_map.at(Type_e::eStopPoint), vertex_map.at(Type_e::eJourneyPatternPoint), g);

    // D'un stop point on obtient : le stop area, city, mode physique, journey pattern point et connection
    boost::add_edge(vertex_map.at(Type_e::eStopArea), vertex_map.at(Type_e::eStopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::ePhysicalMode), vertex_map.at(Type_e::eStopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::eCity), vertex_map.at(Type_e::eStopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::eJourneyPatternPoint), vertex_map.at(Type_e::eStopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::eConnection), vertex_map.at(Type_e::eStopPoint), g);

    // D'une connection on a ses deux stop points
    boost::add_edge(vertex_map[Type_e::eStopPoint], vertex_map[Type_e::eConnection], g);
}

// Retourne un map qui indique pour chaque type par quel type on peut l'atteindre
// Si le prédécesseur est égal au type, c'est qu'il n'y a pas de chemin
std::map<Type_e,Type_e> find_path(Type_e source) {
    Jointures j;
    if(j.vertex_map.find(source) == j.vertex_map.end()){
        throw ptref_error("Type doesnot exist as a vertex");
    }

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
