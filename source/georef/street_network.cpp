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


std::vector< std::pair<nt::idx_t, double> > StreetNetwork::find_nearest(const type::GeographicalCoord & start_coord, const proximitylist::ProximityList<nt::idx_t> & pl, double radius, bool use_second) {
    ng::ProjectionData start;
    try{
        start = ng::ProjectionData(start_coord, this->geo_ref, this->geo_ref.pl);
    }catch(ng::DestinationFound){}


    // On trouve tous les élements à moins radius mètres en vol d'oiseau
    std::vector< std::pair<nt::idx_t, type::GeographicalCoord> > elements;
    try{
        elements = pl.find_within(start_coord, radius);
    }catch(ng::DestinationFound){
        return std::vector< std::pair<nt::idx_t, double> >();
    }

    if(!use_second) {
        departure_launch = true;
        return find_nearest(start, radius, elements, distances, predecessors, idx_projection);
    }
    else{
        arrival_launch = true;
        return find_nearest(start, radius, elements, distances2, predecessors2, idx_projection2);
    }
}

std::vector< std::pair<type::idx_t, double> > StreetNetwork::find_nearest(const ng::ProjectionData & start,
                                                                          double radius,
                                                                          const std::vector< std::pair<nt::idx_t, type::GeographicalCoord> > & elements , std::vector<float> & dist, std::vector<ng::vertex_t> & preds, std::map<type::idx_t, ng::ProjectionData> & idx_proj){
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

    proximitylist::ProximityList<ng::vertex_t> temp_pl;

    size_t num_vertices = boost::num_vertices(geo_ref.graph);
    double max_dist = std::numeric_limits<double>::max();
    for(ng::vertex_t u = 0; u != num_vertices; ++u){
        if(dist[u] < max_dist)
            temp_pl.add(this->geo_ref.graph[u].coord, u);
    }
    temp_pl.build();

    std::vector< std::pair<nt::idx_t, double> > result;
    // À chaque fois on regarde la distance réelle en suivant le filaire de voirie
    for(auto element : elements){

        ng::ProjectionData current;
        try{
            current = ng::ProjectionData(element.second, this->geo_ref, temp_pl);
            idx_proj[element.first] = current;
        }catch(ng::DestinationFound){}

        ng::vertex_t best;
        double best_distance;
        if(dist[current.source] < dist[current.target]){
            best = current.source;
            best_distance = current.source_distance + dist[best];
        } else {
            best = current.target;
            best_distance = current.target_distance + dist[best];
        }

        if(best_distance < radius){
            result.push_back(std::make_pair(element.first, best_distance));
        }
    }
    return result;
}

std::vector< std::pair<type::idx_t, double> > StreetNetwork::find_nearest_stop_points(const type::GeographicalCoord & start_coord, const proximitylist::ProximityList<type::idx_t> & pl, double radius, bool use_second){
    ng::ProjectionData projection;
    try{
        projection = ng::ProjectionData(start_coord, this->geo_ref, this->geo_ref.pl);
    }catch(ng::DestinationFound){}

    std::vector< std::pair<nt::idx_t, type::GeographicalCoord> > elements;
    try{
        elements = pl.find_within(start_coord, radius);
    }catch(ng::DestinationFound){
        return std::vector< std::pair<nt::idx_t, double> >();
    }

    if(!use_second) {
        departure_launch = true;
        start = projection;
        return find_nearest_stop_points(projection, radius, elements, distances, predecessors, idx_projection);
    }
    else {
        arrival_launch = true;
        destination = projection;
        return find_nearest_stop_points(projection, radius, elements, distances2, predecessors2, idx_projection2);
    }
}

std::vector< std::pair<type::idx_t, double> > StreetNetwork::find_nearest_stop_points(const ng::ProjectionData & start, double radius, const std::vector< std::pair<type::idx_t, type::GeographicalCoord> > & elements,
                                                                                            std::vector<float> & dist,
                                                                                            std::vector<ng::vertex_t> & preds,
                                                                                            std::map<type::idx_t, ng::ProjectionData> & idx_proj){
    std::vector< std::pair<type::idx_t, double> > result;
    geo_ref.init(dist, preds);
    idx_proj.clear();

    // On lance un dijkstra depuis les deux nœuds de départ
    dist[start.source] = start.source_distance;
    try{
        if(start.source < boost::num_vertices(this->geo_ref.graph))
            geo_ref.dijkstra(start.source, dist, preds, distance_visitor(radius, dist));
    }catch(ng::DestinationFound){}
    dist[start.target] = start.target_distance;
    try{
        if(start.target < boost::num_vertices(this->geo_ref.graph))
            geo_ref.dijkstra(start.target, dist, preds, distance_visitor(radius, dist));
    }catch(ng::DestinationFound){}

    double max = std::numeric_limits<float>::max();

    for(auto element: elements){
        const ng::ProjectionData & projection = geo_ref.projected_stop_points[element.first];
        // Est-ce que le stop point a pu être raccroché au street network
        if(projection.source < boost::num_vertices(this->geo_ref.graph) && projection.source < boost::num_vertices(this->geo_ref.graph)){
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
        result.coordinates.push_front(start.projected);
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

    result.coordinates.push_front(start.projected);
    result.coordinates.push_back(destination.projected);
    return result;
}
}}
