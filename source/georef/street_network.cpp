#include "street_network.h"
#include "type/data.h"
#include "georef.h"
#include <chrono>

namespace ng = navitia::georef;
namespace bp = boost::posix_time;

namespace navitia { namespace streetnetwork {

StreetNetwork::StreetNetwork(const ng::GeoRef &geo_ref) :
    geo_ref(geo_ref),
    departure_path_finder(geo_ref),
    arrival_path_finder(geo_ref)
{}

void StreetNetwork::init(const type::EntryPoint& start, boost::optional<const type::EntryPoint&> end) {
    departure_path_finder.init(start.coordinates, start.streetnetwork_params.mode);

    if (end) {
        arrival_path_finder.init((*end).coordinates, (*end).streetnetwork_params.mode);
    }
}

bool StreetNetwork::departure_launched() const {return departure_path_finder.computation_launch;}
bool StreetNetwork::arrival_launched() const {return arrival_path_finder.computation_launch;}

std::vector<std::pair<type::idx_t, double>>
StreetNetwork::find_nearest_stop_points(double radius, const proximitylist::ProximityList<type::idx_t>& pl, bool use_second) {
    // delegate to the arrival or departure pathfinder
    // results are store to build the routing path after the transportation routing computation
    return (use_second ? arrival_path_finder : departure_path_finder).find_nearest_stop_points(radius, pl);
}

double StreetNetwork::get_distance(type::idx_t target_idx, bool use_second) {
    return (use_second ? arrival_path_finder : departure_path_finder).get_distance(target_idx);
}

ng::Path StreetNetwork::get_path(type::idx_t idx, bool use_second) {
    ng::Path result;
    if (! use_second) {
        result = departure_path_finder.get_path(idx);

        if (! result.path_items.empty())
            result.path_items.front().coordinates.push_front(departure_path_finder.starting_edge.projected);
    } else {
        result = arrival_path_finder.get_path(idx);

        //we have to reverse the path
        std::reverse(result.path_items.begin(), result.path_items.end());
        for (auto& item : result.path_items) {
            std::reverse(item.coordinates.begin(), item.coordinates.end());
            //we have to reverse the directions too
            item.angle *= -1;
        }

        if (! result.path_items.empty()) {
            //no direction for the last elt
            result.path_items.back().angle = 0;
            result.path_items.back().coordinates.push_back(arrival_path_finder.starting_edge.projected);
        }
    }

    return result;
}

ng::Path StreetNetwork::get_direct_path() {
    ng::Path result;

    if(!departure_launched() || !arrival_launched())
        return result;
    //Cherche s'il y a des nœuds en commun, et retient le chemin le plus court
    size_t num_vertices = boost::num_vertices(geo_ref.graph);

    float min_dist = std::numeric_limits<float>::max();
    ng::vertex_t target = std::numeric_limits<size_t>::max();
    for(ng::vertex_t u = 0; u != num_vertices; ++u) {
        if((departure_path_finder.distances[u] != std::numeric_limits<float>::max())
                && (arrival_path_finder.distances[u] != std::numeric_limits<float>::max())
                && ((departure_path_finder.distances[u] + arrival_path_finder.distances[u]) < min_dist)) {
            target = u;
            min_dist = departure_path_finder.distances[u] + arrival_path_finder.distances[u];
        }
    }

    //Construit l'itinéraire
    if(min_dist != std::numeric_limits<float>::max()) {
        result = this->geo_ref.build_path(target, departure_path_finder.predecessors);
        auto path2 = this->geo_ref.build_path(target, arrival_path_finder.predecessors);
        for(auto p = path2.path_items.rbegin(); p != path2.path_items.rend(); ++p) {
            auto& item = *p;
            std::reverse(item.coordinates.begin(), item.coordinates.end());
            item.angle *= -1;
            result.path_items.push_back(item);
            result.length += p->length;
        }
        result.path_items.front().coordinates.push_front(departure_path_finder.starting_edge.projected);
        result.path_items.back().coordinates.push_back(arrival_path_finder.starting_edge.projected);
        result.path_items.back().angle = 0;
    }

    return result;
}

GeoRefPathFinder::GeoRefPathFinder(const ng::GeoRef& gref) : geo_ref(gref) {}

void GeoRefPathFinder::init(const type::GeographicalCoord& start_coord, nt::Mode_e mode) {
    computation_launch = false;
    // we look for the nearest edge from the start coorinate in the right transport mode (walk, bike, car, ...) (ie offset)
    this->mode = mode;
    nt::idx_t offset = this->geo_ref.offsets[mode];
    this->start_coord = start_coord;
    starting_edge = ng::ProjectionData(start_coord, this->geo_ref, offset, this->geo_ref.pl);

    //we initialize the distances to the maximum value
    size_t n = boost::num_vertices(geo_ref.graph);
    distances.assign(n, std::numeric_limits<float>::max());
    //for the predecessors no need to clean the values, the important one will be updated during search
    predecessors.resize(n);

    if (starting_edge.found) {
        //durations initializations
        distances[starting_edge.source] = starting_edge.source_distance;
        distances[starting_edge.target] = starting_edge.target_distance;
    }
}

std::vector<std::pair<type::idx_t, double>>
GeoRefPathFinder::find_nearest_stop_points(double radius, const proximitylist::ProximityList<type::idx_t>& pl) {
    if (! starting_edge.found)
        return {};

    // On trouve tous les élements à moins radius mètres en vol d'oiseau
    std::vector< std::pair<nt::idx_t, type::GeographicalCoord> > elements = pl.find_within(start_coord, radius);
    if(elements.empty())
        return {};

    computation_launch = true;
    std::vector<std::pair<type::idx_t, double>> result;

    // On lance un dijkstra depuis les deux nœuds de départ
    try {
        geo_ref.dijkstra(starting_edge.source, distances, predecessors, distance_visitor(radius, distances));
    } catch(ng::DestinationFound){}

    try {
        geo_ref.dijkstra(starting_edge.target, distances, predecessors, distance_visitor(radius, distances));
    } catch(ng::DestinationFound){}

    const auto max = std::numeric_limits<float>::max();

    for (auto element: elements) {
        ng::ProjectionData projection = this->geo_ref.projected_stop_points[element.first][mode];
        // Est-ce que le stop point a pu être raccroché au street network
        if(projection.found){
            double best_dist = max;
            if (distances[projection.source] < max) {
                best_dist = distances[projection.source] + projection.source_distance;
            }
            if (distances[projection.target] < max) {
                best_dist= std::min(best_dist, distances[projection.target] + projection.target_distance);
            }
            if (best_dist < radius) {
                result.push_back(std::make_pair(element.first, best_dist));
            }
        }
    }
    return result;
}

double GeoRefPathFinder::get_distance(type::idx_t target_idx) {
    const auto max = std::numeric_limits<float>::max();

    if (! starting_edge.found)
        return max;
    assert(boost::edge(starting_edge.source, starting_edge.target, geo_ref.graph).second);

    ng::ProjectionData target = this->geo_ref.projected_stop_points[target_idx][mode];
    if (! target.found)
        return max;
    assert(boost::edge(target.source, target.target, geo_ref.graph).second );

    computation_launch = true;

    if (distances[target.source] == max) {
        bool found = false;
        try {
            geo_ref.dijkstra(starting_edge.source, distances, predecessors,
                             ng::target_unique_visitor(target.source));
        } catch(ng::DestinationFound) { found = true; }

        //if no way has been found, we can stop the search
        if ( ! found ) {
            LOG4CPLUS_WARN(log4cplus::Logger::getInstance("Logger"), "unable to find a way from start edge ["
                           << starting_edge.source << "-" << starting_edge.target
                           << "] to [" << target.source << "-" << target.target << "]");

            return max;
        }
        try {
            geo_ref.dijkstra(starting_edge.target, distances, predecessors,
                             ng::target_unique_visitor(target.source));
        } catch(ng::DestinationFound) { found = true; }
    }

    if (distances[target.target] == max) {
        try {
            geo_ref.dijkstra(starting_edge.source, distances, predecessors,
                             ng::target_unique_visitor(target.target));
        } catch(ng::DestinationFound) {}
        try {
            geo_ref.dijkstra(starting_edge.target, distances, predecessors,
                             ng::target_unique_visitor(target.target));
         } catch(ng::DestinationFound) {}
    }

    //if we succeded in the first search, we must have found the other distances
    assert(distances[target.source] != max && distances[target.target] != max);

    auto best_dist = distances[target.source] + target.source_distance;

    const auto tmp_dist_target = distances[target.target] + target.target_distance;
    if(tmp_dist_target < best_dist) {
        best_dist = tmp_dist_target;
    }

    return best_dist;
}

ng::Path GeoRefPathFinder::get_path(type::idx_t idx) {
    ng::Path result;

    if (! computation_launch || distances[idx] == std::numeric_limits<float>::max())
        return result;

    ng::ProjectionData projection = this->geo_ref.projected_stop_points[idx][mode];

    const auto dist_source = distances[projection.source] + projection.source_distance;
    const auto dist_target = distances[projection.target] + projection.target_distance;
    if (dist_source < dist_target){
        result = this->geo_ref.build_path(projection.source, this->predecessors);
        result.length = dist_source;
    } else {
        result = this->geo_ref.build_path(projection.target, this->predecessors);
        result.length = dist_target;
    }
    return result;
}

}}
