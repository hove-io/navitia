#include "street_network.h"
#include "type/data.h"
#include "georef.h"
#include <chrono>

namespace ng = navitia::georef;

namespace navitia { namespace streetnetwork {

StreetNetwork::StreetNetwork(const ng::GeoRef &geo_ref) : geo_ref(geo_ref), departure_launch(false), arrival_launch(false){}

void StreetNetwork::init() {
    departure_launch = false;
    arrival_launch = false;
}

bool StreetNetwork::departure_launched() {return departure_launch;}
bool StreetNetwork::arrival_launched() {return arrival_launch;}

type::idx_t StreetNetwork::get_offset(const type::Mode_e & mode){
    switch(mode){
    case type::Mode_e::Bike:
        return geo_ref.bike_offset;
    case type::Mode_e::Car:
        return geo_ref.car_offset;
    default: return 0;
    }
}


std::vector<std::pair<type::idx_t, double>>
StreetNetwork::find_nearest_stop_points(const type::GeographicalCoord& start_coord,
                                        const proximitylist::ProximityList<type::idx_t>& pl,
                                        double radius, bool use_second,nt::idx_t offset){
    // we look for the nearest edge from the start coorinate in the right transport mode (walk, bike, car, ...) (ie offset)
    ng::ProjectionData nearest_edge = ng::ProjectionData(start_coord, this->geo_ref, offset, this->geo_ref.pl);

    if(!nearest_edge.found)
        return std::vector< std::pair<nt::idx_t, double> >();

    // On trouve tous les élements à moins radius mètres en vol d'oiseau
    std::vector< std::pair<nt::idx_t, type::GeographicalCoord> > elements = pl.find_within(start_coord, radius);
    if(elements.empty())
        return std::vector< std::pair<nt::idx_t, double> >();

    // Est-ce qu'on calcule au départ ou à l'arrivée
    // Les résultats sont gardés pour reconstruire l'itinéraire routier après avoir calculé l'itinéraire TC
    if(!use_second) {
        departure_launch = true;
        this->departure = nearest_edge;
        return find_nearest_stop_points(nearest_edge, radius, elements, distances, predecessors, idx_projection, offset);
    }
    else{
        arrival_launch = true;
        this->destination = nearest_edge;
        return find_nearest_stop_points(nearest_edge, radius, elements, distances2, predecessors2, idx_projection2, offset);
    }
}

std::vector<std::pair<type::idx_t, double>>
StreetNetwork::find_nearest_stop_points(const ng::ProjectionData& start, double radius,
                                        const std::vector<std::pair<type::idx_t, type::GeographicalCoord>>& elements,
                                        std::vector<float>& dist,
                                        std::vector<ng::vertex_t>& preds,
                                        std::map<type::idx_t, ng::ProjectionData>& idx_proj,nt::idx_t offset){
    std::vector<std::pair<type::idx_t, double>> result;
    geo_ref.init(dist, preds);
    idx_proj.clear();

    // On lance un dijkstra depuis les deux nœuds de départ
    dist[start.source] = start.source_distance;
    try{
        geo_ref.dijkstra(start.source, dist, preds, distance_visitor(radius, dist));
    }catch(ng::DestinationFound){}

    dist[start.target] = start.target_distance;
    try{
        geo_ref.dijkstra(start.target, dist, preds, distance_visitor(radius, dist));
    }catch(ng::DestinationFound){}

    const double max = std::numeric_limits<float>::max();

    for(auto element: elements){
        ng::ProjectionData projection = ng::ProjectionData(element.second, this->geo_ref, offset, this->geo_ref.pl);//TODO save stop point projection on all network
        // Est-ce que le stop point a pu être raccroché au street network
        if(projection.found){
            double best_dist = max;
            if(dist[projection.source] < max){
                best_dist = dist[projection.source] + projection.source_distance;
            }
            if(dist[projection.target] < max){
                best_dist= std::min(best_dist, dist[projection.target] + projection.target_distance);
            }
            if(best_dist < radius){
                result.push_back(std::make_pair(element.first, best_dist));
                idx_proj[element.first] = projection;
            }
        }
    }
    return result;
}

double StreetNetwork::get_distance(const type::GeographicalCoord& start_coord, const type::GeographicalCoord& target_coord,
                                   const type::idx_t& target_idx,
                                   bool use_second, nt::idx_t offset,
                                   bool init) {
    const double max = std::numeric_limits<float>::max();
    ng::ProjectionData start_edge = ng::ProjectionData(start_coord, this->geo_ref, offset, this->geo_ref.pl);
    if(!start_edge.found)
        return max;
    assert(boost::edge(start_edge.source, start_edge.target, geo_ref.graph).second);

    ng::ProjectionData projection = ng::ProjectionData(target_coord, this->geo_ref, offset, this->geo_ref.pl); //TODO save stop point projection on all network
    if(!projection.found)
        return max;
    assert(boost::edge(projection.source, projection.target, geo_ref.graph).second );

    if(!use_second) {
        departure_launch = true;
        this->departure = start_edge;
        return get_distance(start_edge, projection, target_idx, distances, predecessors, idx_projection, init);
    } else {
        arrival_launch = true;
        this->destination = start_edge;
        return get_distance(start_edge, projection, target_idx, distances2, predecessors2, idx_projection2, init);
    }
    return max;
}


double StreetNetwork::get_distance(const ng::ProjectionData& start,
                                   const ng::ProjectionData& target,
                                   const type::idx_t target_idx,
                                   std::vector<float>& dist,
                                   std::vector<ng::vertex_t>& preds,
                                   std::map<type::idx_t, ng::ProjectionData>& idx_proj,
                                   bool init) {
    const float max = std::numeric_limits<float>::max();
    double best_dist = max;
    if(!init) {
        geo_ref.init(dist, preds);
        dist[start.source] = start.source_distance;
        dist[start.target] = start.target_distance;
        idx_proj.clear();
    }
    auto logger = log4cplus::Logger::getInstance("Logger");

    if(dist[target.source] == max) {
        bool found = false;
        try {
            geo_ref.dijkstra(start.source, dist, preds,
                             ng::target_unique_visitor(target.source));
        } catch(ng::DestinationFound) { found = true; }

        //if no way has been found, we can stop the search
        if ( ! found ) {
            LOG4CPLUS_WARN(logger, "unable to find a way from start edge [" << start.source << "-" << start.target
                           << "] to [" << target.source << "-" << target.target << "]");

            return max;
        }
        try {
            geo_ref.dijkstra(start.target, dist, preds,
                             ng::target_unique_visitor(target.source));
        } catch(ng::DestinationFound) { found = true; }
    }

    if(dist[target.target] == max) {
        bool found = false;
        try {
            geo_ref.dijkstra(start.source, dist, preds,
                             ng::target_unique_visitor(target.target));
        } catch(ng::DestinationFound) {found = true;}
        try {
            geo_ref.dijkstra(start.target, dist, preds,
                             ng::target_unique_visitor(target.target));
         } catch(ng::DestinationFound) { found = true; }
    }

    assert(dist[target.source] != max && dist[target.target] != max); //if we succeded in the first search, we must have found the other distances

    best_dist = dist[target.source] + target.source_distance;
    idx_proj[target_idx] = target;

    const auto tmp_dist_target = dist[target.target] + target.target_distance;
    if(tmp_dist_target < best_dist) {
        best_dist = tmp_dist_target;
    }

    return best_dist;
}


ng::Path StreetNetwork::get_path(type::idx_t idx, bool use_second){
    ng::Path result;
    if(!use_second){
        if(!departure_launched()
            || (distances[idx] == std::numeric_limits<float>::max()
                && this->idx_projection.find(idx) == idx_projection.end()))
            return result;

        ng::ProjectionData projection = idx_projection[idx];

        const auto dist_source = distances[projection.source] + projection.source_distance;
        const auto dist_target = distances[projection.target] + projection.target_distance;
        if(dist_source < dist_target){
            result = this->geo_ref.build_path(projection.source, this->predecessors);
            result.length = dist_source;
        } else {
            result = this->geo_ref.build_path(projection.target, this->predecessors);
            result.length = dist_target;
        }
        result.coordinates.push_front(departure.projected);
    } else {
        if(!arrival_launched()
            || (distances2[idx] == std::numeric_limits<float>::max() &&
                this->idx_projection2.find(idx) == idx_projection2.end()))
            return result;

        ng::ProjectionData projection = idx_projection2[idx];

        const auto dist_source = distances2[projection.source] + projection.source_distance;
        const auto dist_target = distances2[projection.target] + projection.target_distance;
        if(dist_source < dist_target){
            result = this->geo_ref.build_path(projection.source, this->predecessors2);
            result.length = dist_source;
        } else {
            result = this->geo_ref.build_path(projection.target, this->predecessors2);
            result.length = dist_target;
        }
        std::reverse(result.path_items.begin(), result.path_items.end());
        std::reverse(result.coordinates.begin(), result.coordinates.end());
        result.coordinates.push_back(destination.projected);
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
    for(ng::vertex_t u = 0; u != num_vertices; ++u){
        if((distances[u] != std::numeric_limits<float>::max()) && (distances2[u] != std::numeric_limits<float>::max())
                && ((distances[u] + distances2[u]) < min_dist)) {
            target = u;
            min_dist = distances[u] + distances2[u];
        }
    }

    //Construit l'itinéraire
    if(min_dist != std::numeric_limits<float>::max()) {
        result = this->geo_ref.build_path(target, this->predecessors);
        auto path2 = this->geo_ref.build_path(target, this->predecessors2);
        for(auto p = path2.path_items.rbegin(); p != path2.path_items.rend(); ++p) {
            result.path_items.push_back(*p);
            result.length+= p->length;
        }
        for(auto c = path2.coordinates.rbegin(); c != path2.coordinates.rend(); ++c) {
            result.coordinates.push_back(*c);
        }
    }

    result.coordinates.push_front(departure.projected);
    result.coordinates.push_back(destination.projected);
    return result;
}
}}
