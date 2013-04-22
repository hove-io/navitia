#include "street_network.h"
#include "type/data.h"
#include "georef.h"

namespace ng = navitia::georef;

namespace navitia { namespace streetnetwork {

StreetNetwork::StreetNetwork(const ng::GeoRef &geo_ref) : geo_ref(geo_ref){}

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


std::vector< std::pair<type::idx_t, double> > StreetNetwork::find_nearest_stop_points(const type::GeographicalCoord & start_coord, const proximitylist::ProximityList<type::idx_t> & pl, double radius, bool use_second,nt::idx_t offset){
    // On cherche le segment le plus proche des coordonnées
    ng::ProjectionData nearest_edge = ng::ProjectionData(start_coord, this->geo_ref, this->geo_ref.pl);

    if(!nearest_edge.found)
        return std::vector< std::pair<nt::idx_t, double> >();

    // On trouve tous les élements à moins radius mètres en vol d'oiseau
    std::vector< std::pair<nt::idx_t, type::GeographicalCoord> > elements = pl.find_within(start_coord, radius);
    if(elements.empty())
        return std::vector< std::pair<nt::idx_t, double> >();

    // Incérmentation de début d'exploration dans graphe selon le mode : marche, vélo ou voiture
    nearest_edge.inc_vertex(offset);

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

std::vector< std::pair<type::idx_t, double> >
StreetNetwork::find_nearest_stop_points(const ng::ProjectionData & start, double radius,
                                        const std::vector< std::pair<type::idx_t, type::GeographicalCoord> > & elements,
                                        std::vector<float> & dist,
                                        std::vector<ng::vertex_t> & preds,
                                        std::map<type::idx_t, ng::ProjectionData> & idx_proj,nt::idx_t offset){
    std::vector< std::pair<type::idx_t, double> > result;
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

    double max = std::numeric_limits<float>::max();

    for(auto element: elements){
//        const ng::ProjectionData & projection = geo_ref.projected_stop_points[element.first];
        ng::ProjectionData projection = geo_ref.projected_stop_points[element.first];
        // Est-ce que le stop point a pu être raccroché au street network
        if(projection.found){
            // Incérmentation de début d'exploration dans graphe selon le mode : marche, vélo ou voiture
            projection.inc_vertex(offset);
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


ng::Path StreetNetwork::get_path(type::idx_t idx, bool use_second){
    ng::Path result;
    if(!use_second){
        if(!departure_launched() || (distances[idx] == std::numeric_limits<float>::max() && this->idx_projection.find(idx) == idx_projection.end()))
            return result;

        ng::ProjectionData projection = idx_projection[idx];

        if(distances[projection.source] + projection.source_distance < distances[projection.target] + projection.target_distance){
            result = this->geo_ref.build_path(projection.source, this->predecessors);
            result.length = distances[projection.source] + projection.source_distance;
        }
        else{
            result = this->geo_ref.build_path(projection.target, this->predecessors);
            result.length = distances[projection.target] + projection.target_distance;
        }
        result.coordinates.push_front(departure.projected);
    } else {
        if(!arrival_launched() || (distances2[idx] == std::numeric_limits<float>::max() && this->idx_projection2.find(idx) == idx_projection2.end()))
            return result;

        ng::ProjectionData projection = idx_projection2[idx];

        if(distances2[projection.source] + projection.source_distance < distances2[projection.target] + projection.target_distance){
            result = this->geo_ref.build_path(projection.source, this->predecessors2);
            result.length = distances2[projection.source] + projection.source_distance;
        }
        else{
            result = this->geo_ref.build_path(projection.target, this->predecessors2);
            result.length = distances2[projection.target] + projection.target_distance;
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

    double min_dist = std::numeric_limits<float>::max();
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
