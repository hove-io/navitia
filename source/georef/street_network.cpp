#include "street_network.h"
#include "type/data.h"
#include "georef.h"
#include <chrono>

namespace navitia { namespace georef {

bt::time_duration get_walking_duration(const double val) {
    return bt::seconds(val / default_speed[type::Mode_e::Walking]);
}

StreetNetwork::StreetNetwork(const GeoRef &geo_ref) :
    geo_ref(geo_ref),
    departure_path_finder(geo_ref),
    arrival_path_finder(geo_ref)
{}

void StreetNetwork::init(const type::EntryPoint& start, boost::optional<const type::EntryPoint&> end) {
    departure_path_finder.init(start.coordinates, start.streetnetwork_params.mode, start.streetnetwork_params.speed_factor);

    if (end) {
        arrival_path_finder.init((*end).coordinates, (*end).streetnetwork_params.mode, (*end).streetnetwork_params.speed_factor);
    }
}

bool StreetNetwork::departure_launched() const {return departure_path_finder.computation_launch;}
bool StreetNetwork::arrival_launched() const {return arrival_path_finder.computation_launch;}

std::vector<std::pair<type::idx_t, bt::time_duration>>
StreetNetwork::find_nearest_stop_points(bt::time_duration radius, const proximitylist::ProximityList<type::idx_t>& pl, bool use_second) {
    // delegate to the arrival or departure pathfinder
    // results are store to build the routing path after the transportation routing computation
    return (use_second ? arrival_path_finder : departure_path_finder).find_nearest_stop_points(radius, pl);
}

bt::time_duration StreetNetwork::get_distance(type::idx_t target_idx, bool use_second) {
    return (use_second ? arrival_path_finder : departure_path_finder).get_distance(target_idx);
}

Path StreetNetwork::get_path(type::idx_t idx, bool use_second) {
    Path result;
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
            //no direction for the first elt
            result.path_items.front().angle = 0;
            result.path_items.back().coordinates.push_back(arrival_path_finder.starting_edge.projected);
        }
    }

    return result;
}

Path StreetNetwork::get_direct_path() {
    if(!departure_launched() || !arrival_launched())
        return {};
    //Cherche s'il y a des nœuds en commun, et retient le chemin le plus court
    size_t num_vertices = boost::num_vertices(geo_ref.graph);

    bt::time_duration min_dist = bt::pos_infin;
    vertex_t target = std::numeric_limits<size_t>::max();
    for(vertex_t u = 0; u != num_vertices; ++u) {
        if((departure_path_finder.distances[u] != bt::pos_infin)
                && (arrival_path_finder.distances[u] != bt::pos_infin)
                && ((departure_path_finder.distances[u] + arrival_path_finder.distances[u]) < min_dist)) {
            target = u;
            min_dist = departure_path_finder.distances[u] + arrival_path_finder.distances[u];
        }
    }

    //Construit l'itinéraire
    if(min_dist == bt::pos_infin)
        return {};

    Path result = this->geo_ref.combine_path(target, departure_path_finder.predecessors, arrival_path_finder.predecessors);
    departure_path_finder.add_projections_to_path(result, true);
    arrival_path_finder.add_projections_to_path(result, false);

    result.path_items.front().angle = 0;

    return result;
}

PathFinder::PathFinder(const GeoRef& gref) : geo_ref(gref) {}

void PathFinder::init(const type::GeographicalCoord& start_coord, nt::Mode_e mode, const float speed_factor) {
    computation_launch = false;
    // we look for the nearest edge from the start coorinate in the right transport mode (walk, bike, car, ...) (ie offset)
    this->mode = mode;
    this->speed_factor = speed_factor; //the speed factor is the factor we have to multiply the edge cost with
    nt::idx_t offset = this->geo_ref.offsets[mode];
    this->start_coord = start_coord;
    starting_edge = ProjectionData(start_coord, this->geo_ref, offset, this->geo_ref.pl);

    //we initialize the distances to the maximum value
    size_t n = boost::num_vertices(geo_ref.graph);
    distances.assign(n, bt::pos_infin);
    //for the predecessors no need to clean the values, the important one will be updated during search
    predecessors.resize(n);

    if (starting_edge.found) {
        //durations initializations
        distances[starting_edge.source] = get_walking_duration(starting_edge.source_distance); //for the projection, we use the default walking speed.
        distances[starting_edge.target] = get_walking_duration(starting_edge.target_distance);
        predecessors[starting_edge.source] = starting_edge.source;
        predecessors[starting_edge.target] = starting_edge.target;
    }
}

std::vector<std::pair<type::idx_t, bt::time_duration>>
PathFinder::find_nearest_stop_points(bt::time_duration radius, const proximitylist::ProximityList<type::idx_t>& pl) {
    if (! starting_edge.found)
        return {};

    // On trouve tous les élements à moins radius mètres en vol d'oiseau
    float crow_flies_dist = radius.total_seconds() * speed_factor * georef::default_speed[mode];
    std::vector< std::pair<nt::idx_t, type::GeographicalCoord> > elements = pl.find_within(start_coord, crow_flies_dist);
    if(elements.empty())
        return {};

    computation_launch = true;
    std::vector<std::pair<type::idx_t, bt::time_duration>> result;

    // On lance un dijkstra depuis les deux nœuds de départ
    try {
        dijkstra(starting_edge.source, distance_visitor(radius, distances));
    } catch(DestinationFound){}

    try {
        dijkstra(starting_edge.target, distance_visitor(radius, distances));
    } catch(DestinationFound){}

    const auto max = bt::pos_infin;

    for (auto element: elements) {
        ProjectionData projection = this->geo_ref.projected_stop_points[element.first][mode];
        // Est-ce que le stop point a pu être raccroché au street network
        if(projection.found){
            bt::time_duration best_dist = max;
            if (distances[projection.source] < max) {
                best_dist = distances[projection.source] + get_walking_duration(projection.source_distance);
            }
            if (distances[projection.target] < max) {
                best_dist = std::min(best_dist, distances[projection.target] + get_walking_duration(projection.target_distance));
            }
            if (best_dist < radius) {
                result.push_back(std::make_pair(element.first, best_dist));
            }
        }
    }
    return result;
}

bt::time_duration PathFinder::get_distance(type::idx_t target_idx) {
    constexpr auto max = bt::pos_infin;

    if (! starting_edge.found)
        return max;
    assert(boost::edge(starting_edge.source, starting_edge.target, geo_ref.graph).second);

    ProjectionData target = this->geo_ref.projected_stop_points[target_idx][mode];

    auto nearest_edge = update_path(target);

    return nearest_edge.first;
}

std::pair<bt::time_duration, vertex_t> PathFinder::find_nearest_vertex(const ProjectionData& target) const {
    constexpr auto max = bt::pos_infin;
    if (! target.found)
        return {max, {}};

    if (distances[target.source] == max) //if one distance has not been reached, both have not been reached
        return {max, {}};

    auto source_dist = distances[target.source] + get_walking_duration(target.source_distance);
    auto target_dist = distances[target.target] + get_walking_duration(target.target_distance);

    if (target_dist < source_dist)
        return {target_dist, target.target};

    return {source_dist, target.source};
}

Path PathFinder::get_path(type::idx_t idx) {
    if (! computation_launch)
        return {};
    ProjectionData projection = this->geo_ref.projected_stop_points[idx][mode];

    auto nearest_edge = find_nearest_vertex(projection);

    return get_path(projection, nearest_edge);
}

Path PathFinder::get_path(const ProjectionData& target, std::pair<bt::time_duration, vertex_t> nearest_edge) {
    if (! computation_launch || ! target.found || nearest_edge.first == bt::pos_infin)
        return {};

    auto result = this->geo_ref.build_path(nearest_edge.second, this->predecessors);
    add_projections_to_path(result, true);

    result.duration = nearest_edge.first;

    //we need to put the end projections too
    edge_t end_e = boost::edge(target.source, target.target, geo_ref.graph).first;
    Edge end_edge = geo_ref.graph[end_e];
    nt::idx_t last_way_idx = result.path_items.back().way_idx;
    if (end_edge.way_idx != last_way_idx) {
        PathItem item;
        item.way_idx = end_edge.way_idx;
        result.path_items.push_back(item);
    }
    auto& back_coord_list = result.path_items.back().coordinates;
    if (back_coord_list.empty() || back_coord_list.back() != target.projected)
        back_coord_list.push_back(target.projected);

    return result;
}

Path PathFinder::compute_path(const type::GeographicalCoord& target_coord) {
    ProjectionData dest(target_coord, geo_ref, geo_ref.pl);

    auto best_pair = update_path(dest);

    return get_path(dest, best_pair);
}

void PathFinder::add_projections_to_path(Path& p, bool append_to_begin) const {
    auto item_to_update = [append_to_begin](Path& p) -> PathItem& { return (append_to_begin ? p.path_items.front() : p.path_items.back()); };
    auto add_in_path = [append_to_begin](Path& p, const PathItem& item) {
        return (append_to_begin ? p.path_items.push_front(item) : p.path_items.push_back(item));
    };

    edge_t start_e = boost::edge(starting_edge.source, starting_edge.target, geo_ref.graph).first;
    Edge start_edge = geo_ref.graph[start_e];

    //we aither add the starting coordinate to the first path item or create a new path item if it was another way
    nt::idx_t first_way_idx = (p.path_items.empty() ? type::invalid_idx : item_to_update(p).way_idx);
    if (start_edge.way_idx != first_way_idx || first_way_idx == type::invalid_idx) {
        if (! p.path_items.empty() && item_to_update(p).way_idx == type::invalid_idx) { //there can be an item with no way, so we will update this item
            item_to_update(p).way_idx = start_edge.way_idx;
        }
        else {
            PathItem item;
            item.way_idx = start_edge.way_idx;
            if (!p.path_items.empty()) {
                //still complexifying stuff... TODO: simplify this
                //we want the projection to be done with the previous transportation mode
                switch (item_to_update(p).transportation) {
                case georef::PathItem::TransportCaracteristic::Walk:
                case georef::PathItem::TransportCaracteristic::Car:
                case georef::PathItem::TransportCaracteristic::Bike:
                    item.transportation = item_to_update(p).transportation;
                    break;
                    //if we were switching between walking and biking, we need to take either
                    //the previous or the next transportation mode depending on 'append_to_begin'
                case georef::PathItem::TransportCaracteristic::BssTake:
                    item.transportation = (append_to_begin ? georef::PathItem::TransportCaracteristic::Walk
                                                           : georef::PathItem::TransportCaracteristic::Bike);
                    break;
                case georef::PathItem::TransportCaracteristic::BssPutBack:
                    item.transportation = (append_to_begin ? georef::PathItem::TransportCaracteristic::Bike
                                                           : georef::PathItem::TransportCaracteristic::Walk);
                    break;
                default:
                    throw navitia::exception("unhandled transportation carac case");
                }
            }
            add_in_path(p, item);
        }
    }
    auto& coord_list = item_to_update(p).coordinates;
    if (append_to_begin) {
        if (coord_list.empty() || coord_list.front() != starting_edge.projected)
            coord_list.push_front(starting_edge.projected);
    }
    else {
        if (coord_list.empty() || coord_list.back() != starting_edge.projected)
            coord_list.push_back(starting_edge.projected);
    }
}

std::pair<bt::time_duration, vertex_t> PathFinder::update_path(const ProjectionData& target) {
    constexpr auto max = bt::pos_infin;
    if (! target.found)
        return {max, {}};
    assert(boost::edge(target.source, target.target, geo_ref.graph).second );

    computation_launch = true;

    if (distances[target.source] == max || distances[target.target] == max) {
        bool found = false;
        try {
            dijkstra(starting_edge.source, target_all_visitor({target.source, target.target}));
        } catch(DestinationFound) { found = true; }

        //if no way has been found, we can stop the search
        if ( ! found ) {
            LOG4CPLUS_WARN(log4cplus::Logger::getInstance("Logger"), "unable to find a way from start edge ["
                           << starting_edge.source << "-" << starting_edge.target
                           << "] to [" << target.source << "-" << target.target << "]");
            return {max, {}};
        }
        try {
            dijkstra(starting_edge.target, target_all_visitor({target.source, target.target}));
        } catch(DestinationFound) { found = true; }

    }
    //if we succeded in the first search, we must have found one of the other distances
    assert(distances[target.source] != max && distances[target.target] != max);

    return find_nearest_vertex(target);
}

}}
