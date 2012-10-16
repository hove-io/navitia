#include "types.h"

#include <boost/foreach.hpp>
#include <fstream>
#include <unordered_map>

using navitia::type::idx_t;

namespace navitia{ namespace georef{

// Exception levée dès que l'on trouve une destination
struct DestinationFound{};

void Way::sort_house_number(){
    std::sort(this->house_number_right.begin(),this->house_number_right.end());
    std::sort(this->house_number_left.begin(),this->house_number_left.end());
}

nt::GeographicalCoord Way::nearest_geographical_coord(int number){
    nt::GeographicalCoord to_return;    
    HouseNumber hn_upper, hn_lower;
    if (number % 2 == 0){ // pair
        for(auto it=this->house_number_right.begin(); it != this->house_number_right.end(); ++it){
            if ((*it).number  < number){
                hn_lower = (*it);
            }else {
                hn_upper = (*it);
                break;
            }
        }
    }else{
        for(auto it=this->house_number_left.begin(); it != this->house_number_left.end(); ++it){
            if ((*it).number  < number){
                hn_lower = (*it);
            }else {
                hn_upper = (*it);
                break;
            }
        }
    }
    // l'extrapolation à faire :

    return to_return;
}

nt::GeographicalCoord Way::get_geographicalCoord_by_number(int number){
    nt::GeographicalCoord to_return;
    if (number % 2 == 0){ // Pair
        if (this->house_number_right.size() > 0){
            if (this->house_number_right.back().number <= number){
                to_return = this->house_number_right.back().coord;
            }else{
                if (this->house_number_right.front().number >= number){
                    to_return = this->house_number_right.front().coord;
                }else{
                    for(auto it=this->house_number_right.begin(); it != this->house_number_right.end(); ++it){
                        if ((*it).number  == number){
                            return (*it).coord;
                        }
                    }
                    // Dans le cas où on ne trouve pas le numéro
                    to_return = nearest_geographical_coord(number);
                }
            }
        }
    }else{ // Impair
        if (this->house_number_left.size() > 0){
            if (this->house_number_left.back().number <= number){
                to_return = this->house_number_left.back().coord;
            }else{
                if (this->house_number_left.front().number >= number){
                    to_return = this->house_number_left.front().coord;
                }else{
                    for(auto it=this->house_number_left.begin(); it != this->house_number_left.end(); ++it){
                        if ((*it).number  == number){
                            return (*it).coord;
                        }
                    }
                    // Dans le cas où on ne trouve pas le numéro
                    to_return = nearest_geographical_coord(number);
                }
            }
        }
    }    
    return to_return;
}

// Visiteur qui lève une exception dès qu'une des cibles souhaitées est atteinte
struct target_visitor : public boost::dijkstra_visitor<> {
    const std::vector<vertex_t> & destinations;
    target_visitor(const std::vector<vertex_t> & destinations) : destinations(destinations){}
    void finish_vertex(vertex_t u, const Graph&){
        if(std::find(destinations.begin(), destinations.end(), u) != destinations.end())
            throw DestinationFound();
    }
};

// Visiteur qui s'arrête au bout d'une certaine distance
struct distance_visitor : public boost::dijkstra_visitor<> {
    double max_distance;
    const std::vector<float> & distances;
    distance_visitor(float max_distance, const std::vector<float> & distances) : max_distance(max_distance), distances(distances){}
    void finish_vertex(vertex_t u, const Graph&){
        if(distances[u] > max_distance)
            throw DestinationFound();
    }
};



void GeoRef::init(std::vector<float> &distances, std::vector<vertex_t> &predecessors) const{
    size_t n = boost::num_vertices(this->graph);
    distances.assign(n, std::numeric_limits<float>::max());
    predecessors.resize(n);
}


Path GeoRef::build_path(vertex_t best_destination, std::vector<vertex_t> preds) const {
    Path p;
    std::vector<vertex_t> reverse_path;
    while(best_destination != preds[best_destination]){
        reverse_path.push_back(best_destination);
        best_destination = preds[best_destination];
    }
    reverse_path.push_back(best_destination);

    // On reparcourt tout dans le bon ordre
    nt::idx_t last_way =  type::invalid_idx;
    PathItem path_item;
    p.coordinates.push_back(graph[reverse_path.back()].coord);
    for(size_t i = reverse_path.size(); i > 1; --i){
        vertex_t v = reverse_path[i-2];
        vertex_t u = reverse_path[i-1];
        p.coordinates.push_back(graph[v].coord);

        edge_t e = boost::edge(u, v, graph).first;
        Edge edge = graph[e];
        if(edge.way_idx != last_way && last_way != type::invalid_idx){
            p.path_items.push_back(path_item);
            path_item = PathItem();
        }
        last_way = edge.way_idx;
        path_item.way_idx = edge.way_idx;
        path_item.segments.push_back(e);
        path_item.length += edge.length;
    }
    if(reverse_path.size() > 1)
        p.path_items.push_back(path_item);
    return p;
}

Path GeoRef::compute(std::vector<vertex_t> starts, std::vector<vertex_t> destinations, std::vector<double> start_zeros, std::vector<double> dest_zeros) const {
    if(starts.size() == 0 || destinations.size() == 0)
        throw proximitylist::NotFound();

    if(start_zeros.size() != starts.size())
        start_zeros.assign(starts.size(), 0);

    if(dest_zeros.size() != destinations.size())
        dest_zeros.assign(destinations.size(), 0);

    std::vector<vertex_t> preds;

    // Tableau des distances des nœuds à l'origine, par défaut à l'infini
    std::vector<float> dists;

    this->init(dists, preds);

    for(size_t i = 0; i < starts.size(); ++i){
        vertex_t start = starts[i];
        dists[start] = start_zeros[i];
        // On effectue un Dijkstra sans ré-initialiser les tableaux de distances et de prédécesseur
        try {
            this->dijkstra(start, dists, preds, target_visitor(destinations));
        } catch (DestinationFound) {}
    }

    // On cherche la destination la plus proche
    vertex_t best_destination = destinations.front();
    float best_distance = std::numeric_limits<float>::max();
    for(size_t i = 0; i < destinations.size(); ++i){
        vertex_t destination = destinations[i];
        dists[i] += dest_zeros[i];
        if(dists[destination] < best_distance) {
            best_distance = dists[destination];
            best_destination = destination;
        }
    }

    // Si un chemin existe
    if(best_distance < std::numeric_limits<float>::max()){
        Path p = build_path(best_destination, preds);
        p.length = best_distance;
        return p;
    } else {
        throw proximitylist::NotFound();
    }

}


ProjectionData::ProjectionData(const type::GeographicalCoord & coord, const GeoRef & sn, const proximitylist::ProximityList<vertex_t> &prox){

    edge_t edge;
    bool found = true;
    try {
        edge = sn.nearest_edge(coord, prox);
    } catch(proximitylist::NotFound) {
        found = false;
        this->source = std::numeric_limits<vertex_t>::max();
        this->target = std::numeric_limits<vertex_t>::max();
    }

    if(found) {
        // On cherche les coordonnées des extrémités de ce segment
        this->source = boost::source(edge, sn.graph);
        this->target = boost::target(edge, sn.graph);
        type::GeographicalCoord vertex1_coord = sn.graph[this->source].coord;
        type::GeographicalCoord vertex2_coord = sn.graph[this->target].coord;
        // On projette le nœud sur le segment
        this->projected = project(coord, vertex1_coord, vertex2_coord).first;
        // On calcule la distance « initiale » déjà parcourue avant d'atteindre ces extrémité d'où on effectue le calcul d'itinéraire
        this->source_distance = projected.distance_to(vertex1_coord);
        this->target_distance = projected.distance_to(vertex2_coord);
    }
}



Path GeoRef::compute(const type::GeographicalCoord & start_coord, const type::GeographicalCoord & dest_coord) const{
    ProjectionData start(start_coord, *this, this->pl);
    ProjectionData dest(dest_coord, *this, this->pl);

    Path p = compute({start.source, start.target},
                     {dest.source, dest.target},
                     {start.source_distance, start.target_distance},
                     {dest.source_distance, dest.target_distance});

    // On rajoute les bouts de coordonnées manquants à partir et vers le projeté de respectivement le départ et l'arrivée
    std::vector<type::GeographicalCoord> coords = {start.projected};
    coords.resize(p.coordinates.size() + 2);
    std::copy(p.coordinates.begin(), p.coordinates.end(), coords.begin() + 1);
    coords.back() = dest.projected;
    p.coordinates = coords;

    return p;
}


StreetNetworkWorker::StreetNetworkWorker(const GeoRef &geo_ref) :
    geo_ref(geo_ref)
{}

std::vector< std::pair<idx_t, double> > StreetNetworkWorker::find_nearest(const type::GeographicalCoord & start_coord, const proximitylist::ProximityList<idx_t> & pl, double radius, bool use_second) {
    ProjectionData start;
    try{
        start = ProjectionData(start_coord, this->geo_ref, this->geo_ref.pl);
    }catch(DestinationFound){}


    // On trouve tous les élements à moins radius mètres en vol d'oiseau
    std::vector< std::pair<idx_t, type::GeographicalCoord> > elements;
    try{
        elements = pl.find_within(start_coord, radius);
    }catch(DestinationFound){
        return std::vector< std::pair<idx_t, double> >();
    }

    if(!use_second)
        return find_nearest(start, radius, elements, distances, predecessors, idx_projection);
    else
        return find_nearest(start, radius, elements, distances2, predecessors2, idx_projection2);
}

std::vector< std::pair<type::idx_t, double> > StreetNetworkWorker::find_nearest(const ProjectionData & start, double radius, const std::vector< std::pair<idx_t, type::GeographicalCoord> > & elements , std::vector<float> & dist, std::vector<vertex_t> & preds, std::map<type::idx_t, ProjectionData> & idx_proj){
    geo_ref.init(dist, preds);
    idx_proj.clear();

    // On lance un dijkstra depuis les deux nœuds de départ
    dist[start.source] = start.source_distance;
    try{
        geo_ref.dijkstra(start.source, dist, preds, distance_visitor(radius, dist));
    }catch(DestinationFound){}
    dist[start.target] = start.target_distance;
    try{
        geo_ref.dijkstra(start.target, dist, preds, distance_visitor(radius, dist));
    }catch(DestinationFound){}

    proximitylist::ProximityList<vertex_t> temp_pl;

    size_t num_vertices = boost::num_vertices(geo_ref.graph);
    double max_dist = std::numeric_limits<double>::max();
    for(vertex_t u = 0; u != num_vertices; ++u){
        if(dist[u] < max_dist)
            temp_pl.add(this->geo_ref.graph[u].coord, u);
    }
    temp_pl.build();

    std::vector< std::pair<idx_t, double> > result;
    // À chaque fois on regarde la distance réelle en suivant le filaire de voirie
    for(auto element : elements){

        ProjectionData current;
        try{
            current = ProjectionData(element.second, this->geo_ref, temp_pl);
            idx_proj[element.first] = current;
        }catch(DestinationFound){}

        vertex_t best;
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

void GeoRef::build_proximity_list(){
    BOOST_FOREACH(vertex_t u, boost::vertices(this->graph)){
        pl.add(graph[u].coord, u);
    }
    pl.build();
}

void GeoRef::project_stop_points(const std::vector<type::StopPoint> & stop_points){
    this->projected_stop_points.reserve(stop_points.size());
    for(type::StopPoint stop_point : stop_points){
        this->projected_stop_points.push_back(ProjectionData(stop_point.coord, *this, this->pl));
    }
}

edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates) const {
    return this->nearest_edge(coordinates, this->pl);
}


edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates, const proximitylist::ProximityList<vertex_t> &prox) const {
    vertex_t u;
    try {
        u = prox.find_nearest(coordinates);
    } catch(proximitylist::NotFound) {
        throw proximitylist::NotFound();
    }

    type::GeographicalCoord coord_u, coord_v;
    coord_u = this->graph[u].coord;
    float dist = std::numeric_limits<float>::max();
    edge_t best;
    bool found = false;
    BOOST_FOREACH(edge_t e, boost::out_edges(u, this->graph)){
        vertex_t v = boost::target(e, this->graph);
        coord_v = this->graph[v].coord;
        // Petite approximation de la projection : on ne suit pas le tracé de la voirie !
        auto projected = project(coordinates, coord_u, coord_v);
        if(projected.second < dist){
            found = true;
            dist = projected.second;
            best = e;
        }
    }
    if(!found)
        throw proximitylist::NotFound();
    else
        return best;
}



std::pair<type::GeographicalCoord, float> project(type::GeographicalCoord point, type::GeographicalCoord segment_start, type::GeographicalCoord segment_end){
    std::pair<type::GeographicalCoord, float> result;
    result.first.degrees = point.degrees;

    float length = segment_start.distance_to(segment_end);
    float u;

    // On gère le cas où le segment est particulièrement court, et donc ça peut poser des problèmes (à cause de la division par length²)
    if(length < 1){ // moins d'un mètre, on projette sur une extrémité
        if(point.distance_to(segment_start) < point.distance_to(segment_end))
            u = 0;
        else
            u = 1;
    } else {
        u = ((point.x - segment_start.x)*(segment_end.x - segment_start.x)
             + (point.y - segment_start.y)*(segment_end.y - segment_start.y) )/
                (length * length);
    }

    // Les deux cas où le projeté tombe en dehors
    if(u < 0)
        result = std::make_pair(segment_start, segment_start.distance_to(point));
    else if(u > 1)
        result = std::make_pair(segment_end, segment_end.distance_to(point));
    else {
        result.first.x = segment_start.x + u * (segment_end.x - segment_start.x);
        result.first.y = segment_start.y + u * (segment_end.y - segment_start.y);
        result.second = point.distance_to(result.first);
    }

    return result;
}

std::vector< std::pair<type::idx_t, double> > StreetNetworkWorker::find_nearest_stop_points(const type::GeographicalCoord & start_coord, const proximitylist::ProximityList<type::idx_t> & pl, double radius, bool use_second){
    ProjectionData start;
    try{
        start = ProjectionData(start_coord, this->geo_ref, this->geo_ref.pl);
    }catch(DestinationFound){}

    std::vector< std::pair<idx_t, type::GeographicalCoord> > elements;
    try{
        elements = pl.find_within(start_coord, radius);
    }catch(DestinationFound){
        return std::vector< std::pair<idx_t, double> >();
    }

    if(!use_second)
        return find_nearest_stop_points(start, radius, elements, distances, predecessors, idx_projection);
    else
        return find_nearest_stop_points(start, radius, elements, distances2, predecessors2, idx_projection2);
}

std::vector< std::pair<type::idx_t, double> > StreetNetworkWorker::find_nearest_stop_points(const ProjectionData & start, double radius, const std::vector< std::pair<type::idx_t, type::GeographicalCoord> > & elements,
                                                                                            std::vector<float> & dist,
                                                                                            std::vector<vertex_t> & preds,
                                                                                            std::map<type::idx_t, ProjectionData> & idx_proj){
    std::vector< std::pair<type::idx_t, double> > result;
    geo_ref.init(dist, preds);
    idx_proj.clear();

    // On lance un dijkstra depuis les deux nœuds de départ
    dist[start.source] = start.source_distance;
    try{
        if(start.source < boost::num_vertices(this->geo_ref.graph))
            geo_ref.dijkstra(start.source, dist, preds, distance_visitor(radius, dist));
    }catch(DestinationFound){}
    dist[start.target] = start.target_distance;
    try{
        if(start.target < boost::num_vertices(this->geo_ref.graph))
            geo_ref.dijkstra(start.target, dist, preds, distance_visitor(radius, dist));
    }catch(DestinationFound){}

    double max = std::numeric_limits<float>::max();

    for(auto element: elements){
        const ProjectionData & projection = geo_ref.projected_stop_points[element.first];
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
    return result;
}


Path StreetNetworkWorker::get_path(type::idx_t idx, bool use_second){
    Path result;
    if(!use_second){
        if(this->idx_projection.find(idx) == idx_projection.end())
            return result;

        ProjectionData projection = idx_projection[idx];

        if(distances[projection.source] + projection.source_distance < distances[projection.target] + projection.target_distance){
            result = this->geo_ref.build_path(projection.source, this->predecessors);
            result.length = distances[projection.source] + projection.source_distance;
        }
        else{
            result = this->geo_ref.build_path(projection.target, this->predecessors);
            result.length = distances[projection.target] + projection.target_distance;
        }
    } else {
        if(this->idx_projection2.find(idx) == idx_projection2.end())
            return result;

        ProjectionData projection = idx_projection2[idx];

        if(distances2[projection.source] + projection.source_distance < distances2[projection.target] + projection.target_distance){
            result = this->geo_ref.build_path(projection.source, this->predecessors2);
            result.length = distances2[projection.source] + projection.source_distance;
        }
        else{
            result = this->geo_ref.build_path(projection.target, this->predecessors2);
            result.length = distances2[projection.target] + projection.target_distance;
        }
    }

    return result;
}

}}
