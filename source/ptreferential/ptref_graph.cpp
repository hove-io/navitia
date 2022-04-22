/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "ptref_graph.h"

#include "ptreferential.h"

#include <boost/graph/dijkstra_shortest_paths.hpp>

namespace navitia {
namespace ptref {

using navitia::type::Type_e;

/** Contient le graph des transitions
 *
 * Un arc (u,v) entre les entités u et v, veut dire que l'on peut
 * obtenir U à partir de V
 *
 * Cela peut sembler contre-intuitif, mais cela permet de ne faire qu'un
 * seul appel à Dijkstra pour avoir tout ce qui nous interesse
 */

Jointures::Jointures() {
    for (auto p : vertex_map) {
        p.second = boost::graph_traits<Graph>::null_vertex();
    }
#define VERTEX_MAP(type_name, collection_name) vertex_map[Type_e::type_name] = boost::add_vertex(Type_e::type_name, g);
    ITERATE_NAVITIA_PT_TYPES(VERTEX_MAP)
    vertex_map[Type_e::JourneyPattern] = boost::add_vertex(Type_e::JourneyPattern, g);
    vertex_map[Type_e::JourneyPatternPoint] = boost::add_vertex(Type_e::JourneyPatternPoint, g);
    vertex_map[Type_e::POI] = boost::add_vertex(Type_e::POI, g);
    vertex_map[Type_e::POIType] = boost::add_vertex(Type_e::POIType, g);
    vertex_map[Type_e::Connection] = boost::add_vertex(Type_e::Connection, g);
    vertex_map[Type_e::Impact] = boost::add_vertex(Type_e::Impact, g);
    vertex_map[Type_e::MetaVehicleJourney] = boost::add_vertex(Type_e::MetaVehicleJourney, g);

    // From a StopArea, we can have its StopPoints.
    boost::add_edge(vertex_map.at(Type_e::StopPoint), vertex_map.at(Type_e::StopArea), g);

    // From a Network, we can have its Lines.
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::Network), g);

    // From a Company, we can have its Lines.
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::Company), g);

    // From a CommercialMode, we can have its Lines.
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::CommercialMode), g);

    // PhysicalMode => {VehicleJourney, JourneyPattern}
    boost::add_edge(vertex_map.at(Type_e::VehicleJourney), vertex_map.at(Type_e::PhysicalMode), g);
    boost::add_edge(vertex_map.at(Type_e::JourneyPattern), vertex_map.at(Type_e::PhysicalMode), Edge(0.9), g);
    boost::add_edge(vertex_map.at(Type_e::JourneyPatternPoint), vertex_map.at(Type_e::PhysicalMode), Edge(1.8), g);

    // From a line, get its commercial_modes, companies, networks and routes
    boost::add_edge(vertex_map.at(Type_e::CommercialMode), vertex_map.at(Type_e::Line), g);
    boost::add_edge(vertex_map.at(Type_e::Company), vertex_map.at(Type_e::Line), g);
    boost::add_edge(vertex_map.at(Type_e::Network), vertex_map.at(Type_e::Line), g);
    boost::add_edge(vertex_map.at(Type_e::Route), vertex_map.at(Type_e::Line), g);

    // From a route, get its line, vehicle_journeys and journey_patterns
    boost::add_edge(vertex_map.at(Type_e::JourneyPattern), vertex_map.at(Type_e::Route), g);
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::Route), g);
    boost::add_edge(vertex_map.at(Type_e::VehicleJourney), vertex_map.at(Type_e::Route), g);

    // Avoid JourneyPattern-JourneyPatternPoint on Route <-> StopPoint and Route <-> StopArea for performance issue
    boost::add_edge(vertex_map.at(Type_e::StopPoint), vertex_map.at(Type_e::Route), Edge(2.5), g);
    boost::add_edge(vertex_map.at(Type_e::Route), vertex_map.at(Type_e::StopPoint), Edge(2.5), g);
    boost::add_edge(vertex_map.at(Type_e::StopArea), vertex_map.at(Type_e::Route), Edge(3.3), g);
    boost::add_edge(vertex_map.at(Type_e::Route), vertex_map.at(Type_e::StopArea), Edge(3.3), g);

    // JourneyPattern => {Route, JourneyPatternPoint, VehicleJourney, PhysicalMode}
    boost::add_edge(vertex_map.at(Type_e::Route), vertex_map.at(Type_e::JourneyPattern), g);
    boost::add_edge(vertex_map.at(Type_e::JourneyPatternPoint), vertex_map.at(Type_e::JourneyPattern), g);
    boost::add_edge(vertex_map.at(Type_e::VehicleJourney), vertex_map.at(Type_e::JourneyPattern), g);
    boost::add_edge(vertex_map.at(Type_e::PhysicalMode), vertex_map.at(Type_e::JourneyPattern), Edge(0.9), g);

    // from a VehicleJourney, we can have the Route, the
    // JourneyPattern, the Company, the PhysicalMode, the
    // ValidityPattern, the MetaVehicleJourney, the Dataset
    boost::add_edge(vertex_map.at(Type_e::Route), vertex_map.at(Type_e::VehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::JourneyPattern), vertex_map.at(Type_e::VehicleJourney), g);
    // Higher weight on the Company to get Route->Line better than Company->Line.
    boost::add_edge(vertex_map.at(Type_e::Company), vertex_map.at(Type_e::VehicleJourney), Edge(2.5), g);
    boost::add_edge(vertex_map.at(Type_e::PhysicalMode), vertex_map.at(Type_e::VehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::ValidityPattern), vertex_map.at(Type_e::VehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::MetaVehicleJourney), vertex_map.at(Type_e::VehicleJourney), g);
    boost::add_edge(vertex_map.at(Type_e::Dataset), vertex_map.at(Type_e::VehicleJourney), g);

    // From a JourneyPatternPoint, we can have the JourneyPattern and
    // the StopPoints.
    boost::add_edge(vertex_map.at(Type_e::JourneyPattern), vertex_map.at(Type_e::JourneyPatternPoint), g);
    boost::add_edge(vertex_map.at(Type_e::StopPoint), vertex_map.at(Type_e::JourneyPatternPoint), g);

    // From a StopPoint, we can have its StopArea, Its
    // JourneyPatternPoints and its Connection.
    boost::add_edge(vertex_map.at(Type_e::StopArea), vertex_map.at(Type_e::StopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::JourneyPatternPoint), vertex_map.at(Type_e::StopPoint), g);
    boost::add_edge(vertex_map.at(Type_e::Connection), vertex_map.at(Type_e::StopPoint), g);

    // D'une connection on a ses deux stop points
    boost::add_edge(vertex_map.at(Type_e::StopPoint), vertex_map.at(Type_e::Connection), g);

    // De poi vers poi type et vice et versa
    boost::add_edge(vertex_map.at(Type_e::POI), vertex_map.at(Type_e::POIType), g);
    boost::add_edge(vertex_map.at(Type_e::POIType), vertex_map.at(Type_e::POI), g);

    // from line to calendar
    boost::add_edge(vertex_map.at(Type_e::Calendar), vertex_map.at(Type_e::Line), g);
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::Calendar), g);

    // from line_group to lines
    boost::add_edge(vertex_map.at(Type_e::LineGroup), vertex_map.at(Type_e::Line), g);
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::LineGroup), g);

    // From a MetaVehicleJourney, we can have its VehicleJourneys.
    boost::add_edge(vertex_map.at(Type_e::VehicleJourney), vertex_map.at(Type_e::MetaVehicleJourney), g);

    // From a dataset we can have a contributor and vehiclejourneys
    boost::add_edge(vertex_map.at(Type_e::Contributor), vertex_map.at(Type_e::Dataset), g);
    boost::add_edge(vertex_map.at(Type_e::VehicleJourney), vertex_map.at(Type_e::Dataset), g);

    // From a contributor we can have datasets
    boost::add_edge(vertex_map.at(Type_e::Dataset), vertex_map.at(Type_e::Contributor), g);

    // From a Route we can have datasets
    // The weight is used to force the use of a path among two paths possible
    // Example: Search physical modes of stop_point with weight=3
    // force use path stop_point >> jpp >> jp >> vehiclejourney >> physical_mode
    // instead path stop_point >> frame >> vehiclejourney >> physical_mode
    boost::add_edge(vertex_map.at(Type_e::Dataset), vertex_map.at(Type_e::Route), Edge(2), g);
    // From a StopPoint we can have datasets
    boost::add_edge(vertex_map.at(Type_e::Dataset), vertex_map.at(Type_e::StopPoint), Edge(3), g);
    // We use a weight to avoid the usage of this path: we want it to be used only when we look for the dataset
    // or the contributor of a network. If we want the vehicle_journeys of a network we have to use the path:
    // Network -> Line -> Route -> VJ with a cost a 3 and not the path using the dataset:
    // Network -> Dataset -> VJ that has a cost of 4
    boost::add_edge(vertex_map.at(Type_e::Dataset), vertex_map.at(Type_e::Network), Edge(3), g);

    // edges for the impacts. for the moment we only need unilateral links,
    // we don't need from an impact all the impacted objects
    const auto objects_having_impacts = {Type_e::StopPoint,         Type_e::Line,    Type_e::Route,
                                         Type_e::StopArea,          Type_e::Network, Type_e::VehicleJourney,
                                         Type_e::MetaVehicleJourney};
    for (auto object : objects_having_impacts) {
        boost::add_edge(vertex_map.at(Type_e::Impact), vertex_map.at(object), g);
    }

    // Retrieve a PT_object from an Impact. The edges have a super heavy weight to make sure we don't go
    // through the impact object to resolve other types. Because objects might not have impact attached,
    // we would not convert object properly otherwise.
    boost::add_edge(vertex_map.at(Type_e::Network), vertex_map.at(Type_e::Impact), Edge(100), g);
    boost::add_edge(vertex_map.at(Type_e::Line), vertex_map.at(Type_e::Impact), Edge(100), g);
    boost::add_edge(vertex_map.at(Type_e::Route), vertex_map.at(Type_e::Impact), Edge(100), g);
    boost::add_edge(vertex_map.at(Type_e::StopArea), vertex_map.at(Type_e::Impact), Edge(100), g);
    boost::add_edge(vertex_map.at(Type_e::StopPoint), vertex_map.at(Type_e::Impact), Edge(100), g);
    boost::add_edge(vertex_map.at(Type_e::MetaVehicleJourney), vertex_map.at(Type_e::Impact), Edge(100), g);
}

// Retourne un map qui indique pour chaque type par quel type on peut l'atteindre
// Si le prédécesseur est égal au type, c'est qu'il n'y a pas de chemin
std::map<Type_e, Type_e> find_path(Type_e source) {
    // the ptref graph is a graph on types, it does not depend of the data, thus it is a static variable
    static const Jointures j;

    if (j.vertex_map[source] == boost::graph_traits<Jointures::Graph>::null_vertex()) {
        throw ptref_error("Type does not exist as a vertex");
    }

    std::vector<Jointures::vertex_t> predecessors(boost::num_vertices(j.g));
    boost::dijkstra_shortest_paths(j.g, j.vertex_map[source],
                                   boost::predecessor_map(&predecessors[0]).weight_map(boost::get(&Edge::weight, j.g)));

    std::map<Type_e, Type_e> result;

    for (Jointures::vertex_t u = 0; u < boost::num_vertices(j.g); ++u) {
        result[j.g[u]] = j.g[predecessors[u]];
    }
    return result;
}

}  // namespace ptref
}  // namespace navitia
