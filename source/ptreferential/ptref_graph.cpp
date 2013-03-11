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
#define VERTEX_MAP(type_name, collection_name) vertex_map[Type_e::type_name] = boost::add_vertex(Type_e::type_name, g);
    ITERATE_NAVITIA_PT_TYPES(VERTEX_MAP)

    // À partir d'une city, on peut avoir son departement et ses stop_points
    boost::add_edge(vertex_map.at(Type_e::Department), vertex_map.at(Type_e::City), g);
    boost::add_edge(vertex_map.at(Type_e::StopPoint), vertex_map.at(Type_e::City), g);

    // À partir d'un country, on peut avoir ses districts
    boost::add_edge(vertex_map.at(Type_e::District), vertex_map.at(Type_e::Country), g);

    // À partir d'un district, on peut avoir son country et ses départements
    boost::add_edge(vertex_map.at(Type_e::Country), vertex_map.at(Type_e::District),  g);
    boost::add_edge(vertex_map.at(Type_e::Department), vertex_map.at(Type_e::District), g);

    // À partir d'un département, on peut avoir son district et ses citys
    boost::add_edge(vertex_map.at(Type_e::District), vertex_map.at(Type_e::Department), g);
    boost::add_edge(vertex_map.at(Type_e::City), vertex_map.at(Type_e::Department), g);

    // À partir d'un stop area, on peut avoir ses stop points et sa city
    boost::add_edge(vertex_map.at(Type_e::City), vertex_map.at(Type_e::StopArea), g);
    boost::add_edge(vertex_map.at(Type_e::StopPoint), vertex_map.at(Type_e::StopArea), g);

    // À partir d'un network on peut avoir ses lignes
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::Network), g);

    // À partir d'une company, on peut avoir ses lignes
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::Company), g);

    // À partir d'un commercial mode on peut avoir ses lignes et son mode physique
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::CommercialMode), g);

    // À partir d'un physical mode on peut avoir les vehicule journeys
    boost::add_edge(vertex_map.at(Type_e::VehicleJourney), vertex_map.at(Type_e::PhysicalMode), g);

    // À partir d'une ligne on peut avoir ses modes commerciaux, compagnies, réseaux et routes
    boost::add_edge(vertex_map.at(Type_e::CommercialMode), vertex_map.at(Type_e::Line), g);
    boost::add_edge(vertex_map.at(Type_e::Company), vertex_map.at(Type_e::Line), g);
    boost::add_edge(vertex_map.at(Type_e::Network), vertex_map.at(Type_e::Line), g);
    boost::add_edge(vertex_map.at(Type_e::Route), vertex_map.at(Type_e::Line), g);

    // À partir d'une route on a sa ligne et ses journey pattern
    boost::add_edge(vertex_map.at(Type_e::JourneyPattern), vertex_map.at(Type_e::Route), g);
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::Route), g);

    // À partir d'un journey pattern on peut avoir sa route, ses points et son vehicule journey
    boost::add_edge(vertex_map.at(Type_e::Route), vertex_map.at(Type_e::JourneyPattern), g);
    boost::add_edge(vertex_map.at(Type_e::JourneyPatternPoint), vertex_map.at(Type_e::JourneyPattern), g);
    boost::add_edge(vertex_map.at(Type_e::VehicleJourney), vertex_map.at(Type_e::JourneyPattern), g);

    // À partir d'un vehicle journey, on peut avoir le journey parttern, la companie créole, le mode physique et le validity pattern
    boost::add_edge(vertex_map.at(Type_e::JourneyPattern), vertex_map.at(Type_e::VehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::Company), vertex_map.at(Type_e::VehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::PhysicalMode), vertex_map.at(Type_e::VehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::ValidityPattern), vertex_map.at(Type_e::VehicleJourney), g);

    // À partir d'un journey pattern point on obtient le journey pattern et le stop point
    boost::add_edge(vertex_map.at(Type_e::JourneyPattern), vertex_map.at(Type_e::JourneyPatternPoint), g);
    boost::add_edge(vertex_map.at(Type_e::StopPoint), vertex_map.at(Type_e::JourneyPatternPoint), g);

    // D'un stop point on obtient : le stop area, city, mode physique, journey pattern point et connection
    boost::add_edge(vertex_map.at(Type_e::StopArea), vertex_map.at(Type_e::StopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::PhysicalMode), vertex_map.at(Type_e::StopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::City), vertex_map.at(Type_e::StopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::JourneyPatternPoint), vertex_map.at(Type_e::StopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::Connection), vertex_map.at(Type_e::StopPoint), g);

    // D'une connection on a ses deux stop points
    boost::add_edge(vertex_map[Type_e::StopPoint], vertex_map[Type_e::Connection], g);
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
