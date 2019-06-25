/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "path_finder.h"

#include "utils/logger.h"

#include <boost/math/constants/constants.hpp>

namespace navitia {
namespace georef {

PathFinder::~PathFinder() = default;

navitia::time_duration PathFinder::crow_fly_duration(const double distance) const {
    // For BSS we want the default speed of walking, because on extremities we walk !
    const auto mode_ = mode == nt::Mode_e::Bss ? nt::Mode_e::Walking : mode;
    return navitia::seconds(distance / double(default_speed[mode_] * speed_factor));
}

navitia::time_duration PathFinder::path_duration_on_same_edge(const ProjectionData& p1, const ProjectionData& p2) {
    // Don't compute distance between p1 and p2, instead use distance from one of the vertex, to speed up the process
    // (especially if we use geometries). We make sure to use the distance from the same vertex by checking if p1 and p2
    // are not projected on reversed edges.
    bool is_reversed(p1[source_e] != p2[source_e]
                     || (p1[source_e] == p1[target_e] && p1.edge.geom_idx != p2.edge.geom_idx));
    return crow_fly_duration(p1.real_coord.distance_to(p1.projected)
                             + fabs(p1.distances[target_e] - p2.distances[is_reversed ? source_e : target_e])
                             + p2.projected.distance_to(p2.real_coord));
}

nt::LineString PathFinder::path_coordinates_on_same_edge(const Edge& e,
                                                         const ProjectionData& p1,
                                                         const ProjectionData& p2) {
    nt::LineString result;
    if (e.geom_idx != nt::invalid_idx) {
        const Way* way = this->geo_ref.ways[e.way_idx];
        /*
         * Check if we want source or target distance (handle reverse edges).
         * The edge e in parameter is representing p1, we are always using target_e for this one.
         * If p2 is projected on the reversed edge, we are using the distance from source_e in order to compare distance
         * to the same vertex. If the distance to the target from the starting point is lower than the one from the
         * ending point we are reversing the geometry.
         */
        bool edge_dest_reversed(p1[source_e] != p2[source_e]
                                || (p1[source_e] == p1[target_e] && p1.edge.geom_idx != p2.edge.geom_idx));
        bool reverse(p1.distances[target_e] < p2.distances[edge_dest_reversed ? source_e : target_e]);
        const nt::GeographicalCoord& startBlade = (reverse ? p2.projected : p1.projected);
        const nt::GeographicalCoord& endBlade = (reverse ? p1.projected : p2.projected);
        result = type::split_line_at_point(type::split_line_at_point(way->geoms[e.geom_idx], startBlade, true),
                                           endBlade, false);
        if (reverse)
            std::reverse(result.begin(), result.end());
    }

    if (result.empty()) {
        result.push_back(p1.projected);
        result.push_back(p2.projected);
    }

    return result;
}

PathFinder::PathFinder(const GeoRef& gref) : geo_ref(gref), color(boost::num_vertices(geo_ref.graph)) {}

void PathFinder::init(const type::GeographicalCoord& start_coord, nt::Mode_e mode, const float speed_factor) {
    computation_launch = false;
    // we look for the nearest edge from the start coordinate
    // in the right transport mode (walk, bike, car, ...) (ie offset)
    this->mode = mode;
    this->speed_factor = speed_factor;  // the speed factor is the factor we have to multiply the edge cost with
    nt::idx_t offset = this->geo_ref.offsets[mode];
    this->start_coord = start_coord;
    starting_edge = ProjectionData(start_coord, this->geo_ref, offset, this->geo_ref.pl);

    distance_to_entry_point.clear();
    // we initialize the distances to the maximum value
    size_t n = boost::num_vertices(geo_ref.graph);
    distances.assign(n, bt::pos_infin);
    // for the predecessors no need to clean the values, the important one will be updated during search
    predecessors.resize(n);
    index_in_heap_map.resize(n);

    if (starting_edge.found) {
        // durations initializations
        distances[starting_edge[source_e]] = crow_fly_duration(
            starting_edge.distances[source_e]);  // for the projection, we use the default walking speed.
        distances[starting_edge[target_e]] = crow_fly_duration(starting_edge.distances[target_e]);
        predecessors[starting_edge[source_e]] = starting_edge[source_e];
        predecessors[starting_edge[target_e]] = starting_edge[target_e];

        if (starting_edge[target_e] != starting_edge[source_e]) {  // if we're on a useless edge we do not enhance
            // small enchancement, if the projection is done on a node, we disable the crow fly
            if (starting_edge.distances[source_e] < 0.01) {
                predecessors[starting_edge[target_e]] = starting_edge[source_e];
                distances[starting_edge[target_e]] = bt::pos_infin;
            } else if (starting_edge.distances[target_e] < 0.01) {
                predecessors[starting_edge[source_e]] = starting_edge[target_e];
                distances[starting_edge[source_e]] = bt::pos_infin;
            }
        }
    }

    if (color.n != n) {
        color = boost::two_bit_color_map<>(n);
    }
}

std::pair<navitia::time_duration, ProjectionData::Direction> PathFinder::find_nearest_vertex(
    const ProjectionData& target,
    bool handle_on_node) const {
    constexpr auto max = bt::pos_infin;
    if (!target.found)
        return {max, source_e};

    if (distances[target[source_e]] == max)  // if one distance has not been reached, both have not been reached
        return {max, source_e};

    if (handle_on_node) {
        // handle if the projection is done on a node
        if (target.distances[source_e] < 0.01) {
            return {distances[target[source_e]], source_e};
        } else if (target.distances[target_e] < 0.01) {
            return {distances[target[target_e]], target_e};
        }
    }

    auto source_dist = distances[target[source_e]] + crow_fly_duration(target.distances[source_e]);
    auto target_dist = distances[target[target_e]] + crow_fly_duration(target.distances[target_e]);

    if (target_dist < source_dist)
        return {target_dist, target_e};

    return {source_dist, source_e};
}

Path PathFinder::get_path(type::idx_t idx) {
    if (!computation_launch)
        return {};
    ProjectionData projection = this->geo_ref.projected_stop_points[idx][mode];

    auto nearest_edge = find_nearest_vertex(projection);

    return get_path(projection, nearest_edge);
}

PathItem::TransportCaracteristic PathFinder::get_transportation_mode_item_to_update(
    const PathItem::TransportCaracteristic& previous_transportation,
    bool append_to_begin) const {
    switch (previous_transportation) {
        case georef::PathItem::TransportCaracteristic::Walk:
        case georef::PathItem::TransportCaracteristic::Car:
        case georef::PathItem::TransportCaracteristic::Bike:
            return previous_transportation;
            // if we were switching between walking and biking, we need to take either
            // the previous or the next transportation mode depending on 'append_to_begin'
        case georef::PathItem::TransportCaracteristic::BssTake:
            return (append_to_begin ? georef::PathItem::TransportCaracteristic::Walk
                                    : georef::PathItem::TransportCaracteristic::Bike);
        case georef::PathItem::TransportCaracteristic::BssPutBack:
            return (append_to_begin ? georef::PathItem::TransportCaracteristic::Bike
                                    : georef::PathItem::TransportCaracteristic::Walk);
        case georef::PathItem::TransportCaracteristic::CarLeaveParking:
            return (append_to_begin ? georef::PathItem::TransportCaracteristic::Walk
                                    : georef::PathItem::TransportCaracteristic::Car);
        case georef::PathItem::TransportCaracteristic::CarPark:
            return (append_to_begin ? georef::PathItem::TransportCaracteristic::Car
                                    : georef::PathItem::TransportCaracteristic::Walk);
        default:
            throw navitia::recoverable_exception("unhandled transportation carac case");
    }
}

void PathFinder::add_custom_projections_to_path(Path& p,
                                                bool append_to_begin,
                                                const ProjectionData& projection,
                                                ProjectionData::Direction d) const {
    auto item_to_update = [append_to_begin](Path& p) -> PathItem& {
        return (append_to_begin ? p.path_items.front() : p.path_items.back());
    };
    auto add_in_path = [append_to_begin](Path& p, const PathItem& item) {
        return (append_to_begin ? p.path_items.push_front(item) : p.path_items.push_back(item));
    };

    Edge start_edge = projection.edge;
    nt::LineString coords_to_add;
    /*
        Cut the projected edge.
        We have a point p and a direction d.
        Our chunk is something like
                 _______/\           target_e
        start_e /         \__p______/
        The third parameter tell if we want the geometry before or after p.
        If the direction is target_e, we want the end, otherwise the start.
    */
    if (start_edge.way_idx != nt::invalid_idx) {
        Way* way = geo_ref.ways[start_edge.way_idx];
        if (start_edge.geom_idx != nt::invalid_idx) {
            coords_to_add =
                type::split_line_at_point(way->geoms[start_edge.geom_idx], projection.projected, d == target_e);
        }
    }
    if (coords_to_add.empty()) {
        coords_to_add.push_back(projection.projected);
    }

    auto duration = crow_fly_duration(projection.distances[d]);
    // we need to update the total length
    p.duration += duration;

    // we either add the starting coordinate to the first path item or create a new path item if it was another way
    nt::idx_t first_way_idx = (p.path_items.empty() ? type::invalid_idx : item_to_update(p).way_idx);
    if (start_edge.way_idx != first_way_idx || first_way_idx == type::invalid_idx) {
        // there can be an item with no way, so we will update this item
        if (!p.path_items.empty() && item_to_update(p).way_idx == type::invalid_idx) {
            item_to_update(p).way_idx = start_edge.way_idx;
            item_to_update(p).duration += duration;
        } else {
            PathItem item;
            item.way_idx = start_edge.way_idx;
            item.duration = duration;

            if (!p.path_items.empty()) {
                // still complexifying stuff... TODO: simplify this
                // we want the projection to be done with the previous transportation mode
                item.transportation =
                    get_transportation_mode_item_to_update(item_to_update(p).transportation, append_to_begin);
            }
            add_in_path(p, item);
        }
    } else {
        // we just need to update the duration
        item_to_update(p).duration += duration;
    }

    /*
        Define the way we want to add the coordinates to the item to update
        There is 4 possibilities :
         - If we append to the beginning :
           * if the projection is directed to target_e we're adding the coords to the beginning as is,
           * otherwise we're reversing the coords before adding them
         - If we append to the end :
           * if the projection is directed to source_e we're adding the coords to the end as is,
           * otherwise we're reversing the coords before adding them
    */
    auto& coord_list = item_to_update(p).coordinates;
    if (append_to_begin) {
        if (coord_list.empty() || coord_list.front() != projection.projected) {
            if (d == target_e)
                coord_list.insert(coord_list.begin(), coords_to_add.begin(), coords_to_add.end());
            else
                coord_list.insert(coord_list.begin(), coords_to_add.rbegin(), coords_to_add.rend());
        }
    } else {
        if (coord_list.empty() || coord_list.back() != projection.projected) {
            if (d == target_e)
                coord_list.insert(coord_list.end(), coords_to_add.rbegin(), coords_to_add.rend());
            else
                coord_list.insert(coord_list.end(), coords_to_add.begin(), coords_to_add.end());
        }
    }
}

Path PathFinder::get_path(const ProjectionData& target,
                          const std::pair<navitia::time_duration, ProjectionData::Direction>& nearest_edge) {
    if (!computation_launch || !target.found || nearest_edge.first == bt::pos_infin)
        return {};

    Path result = this->build_path(target[nearest_edge.second]);
    auto base_duration = result.duration;
    add_projections_to_path(result, true);
    // we need to put the end projections too
    add_custom_projections_to_path(result, false, target, nearest_edge.second);

    /*
     * If we are on the same edge a non direct path might be faster, like if we have :
     *
     *                      |              .--------C-.
     * ,-----------------A--|              |          |
     * |                    |      or      |          |--------------
     * '-----------------B--|              |          |
     *                      |              '--------D-'
     *
     * Even if A and B are on the same edge it's faster to use the result of build_path taking the vertical edge.
     * It's a bit different for an edge with the same vertex for source and target. For C and D we need to only take the
     * one edge but passing by the vertex. To take a direct path on the same edge we need to :
     *  - have two point projected on the same edge,
     *  AND
     *   - have an edge with a different vertex for source and target AND have build_path return a duration of 0 second
     *   OR
     *   - have a path duration on the same edge smaller than the duration of the complete path returned by Dijkstra
     */
    if (is_projected_on_same_edge(starting_edge, target)
        && ((!base_duration.total_seconds() && starting_edge[source_e] != starting_edge[target_e])
            || path_duration_on_same_edge(starting_edge, target) <= result.duration)) {
        result.path_items.clear();
        result.duration = {};
        PathItem item;
        item.duration = path_duration_on_same_edge(starting_edge, target);

        auto edge_pair = boost::edge(starting_edge[source_e], starting_edge[target_e], geo_ref.graph);
        if (!edge_pair.second) {
            throw navitia::exception("impossible to find an edge");
        }

        item.way_idx = starting_edge.edge.way_idx;
        item.transportation = geo_ref.get_caracteristic(edge_pair.first);
        nt::LineString geom = path_coordinates_on_same_edge(starting_edge.edge, starting_edge, target);
        item.coordinates.insert(item.coordinates.begin(), geom.begin(), geom.end());
        result.path_items.push_back(item);
        result.duration += item.duration;
    }

    return result;
}

void PathFinder::add_projections_to_path(Path& p, bool append_to_begin) const {
    // we need to find out which side of the projection has been used to compute the right length
    assert(!p.path_items.empty());

    const auto& path_item_to_consider = append_to_begin ? p.path_items.front() : p.path_items.back();
    const auto& coord_to_consider =
        append_to_begin ? path_item_to_consider.coordinates.front() : path_item_to_consider.coordinates.back();

    ProjectionData::Direction direction;
    // If source and target are the same keep the closest one
    if (starting_edge[source_e] == starting_edge[target_e]) {
        direction = starting_edge.distances[source_e] < starting_edge.distances[target_e] ? source_e : target_e;
    } else if (coord_to_consider == geo_ref.graph[starting_edge[source_e]].coord) {
        direction = source_e;
    } else if (coord_to_consider == geo_ref.graph[starting_edge[target_e]].coord) {
        direction = target_e;
    } else {
        throw navitia::exception("by construction, should never happen");
    }

    add_custom_projections_to_path(p, append_to_begin, starting_edge, direction);
}

Path PathFinder::build_path(vertex_t best_destination) const {
    std::vector<vertex_t> reverse_path;
    while (best_destination != predecessors[best_destination]) {
        reverse_path.push_back(best_destination);
        best_destination = predecessors[best_destination];
    }
    reverse_path.push_back(best_destination);

    return create_path(geo_ref, reverse_path, true, speed_factor);
}

static edge_t get_best_edge(vertex_t u, vertex_t v, const GeoRef& georef) {
    const auto& g = georef.graph;
    boost::optional<edge_t> best_edge;
    for (auto range = out_edges(u, g); range.first != range.second; ++range.first) {
        if (target(*range.first, g) != v) {
            continue;
        }
        if (!best_edge || g[*range.first].duration < g[*best_edge].duration) {
            best_edge = *range.first;
        }
    }
    if (!best_edge) {
        throw navitia::exception("impossible to find an edge");
    }
    return *best_edge;
}

bool is_projected_on_same_edge(const ProjectionData& p1, const ProjectionData& p2) {
    // On the same edge if both use the same two vertices, are on the same way with the same duration
    return (((p1[source_e] == p2[source_e] && p1[target_e] == p2[target_e])
             || (p1[source_e] == p2[target_e] && p1[target_e] == p2[source_e]))
            && p1.edge.duration == p2.edge.duration && p1.edge.way_idx == p2.edge.way_idx);
}

Path create_path(const GeoRef& geo_ref,
                 const std::vector<vertex_t>& reverse_path,
                 bool add_one_elt,
                 float speed_factor) {
    Path p;

    // On reparcourt tout dans le bon ordre
    nt::idx_t last_way = type::invalid_idx;
    boost::optional<PathItem::TransportCaracteristic> last_transport_carac{};
    PathItem path_item;
    path_item.coordinates.push_back(geo_ref.graph[reverse_path.back()].coord);

    for (size_t i = reverse_path.size(); i > 1; --i) {
        bool path_item_changed = false;
        vertex_t v = reverse_path[i - 2];
        vertex_t u = reverse_path[i - 1];
        edge_t e = get_best_edge(u, v, geo_ref);

        Edge edge = geo_ref.graph[e];
        PathItem::TransportCaracteristic transport_carac = geo_ref.get_caracteristic(e);
        if ((edge.way_idx != last_way && last_way != type::invalid_idx)
            || (last_transport_carac && transport_carac != *last_transport_carac)) {
            p.path_items.push_back(path_item);
            path_item = PathItem();
            path_item_changed = true;
        }

        nt::GeographicalCoord coord = geo_ref.graph[v].coord;
        if (edge.geom_idx != nt::invalid_idx) {
            auto geometry = geo_ref.ways[edge.way_idx]->geoms[edge.geom_idx];
            path_item.coordinates.insert(path_item.coordinates.end(), geometry.begin(), geometry.end());
        } else
            path_item.coordinates.push_back(coord);
        last_way = edge.way_idx;
        last_transport_carac = transport_carac;
        path_item.way_idx = edge.way_idx;
        path_item.transportation = transport_carac;
        path_item.duration += edge.duration / speed_factor;
        p.duration += edge.duration / speed_factor;
        if (path_item_changed) {
            // we update the last path item
            path_item.angle = compute_directions(p, coord);
        }
    }
    // in some case we want to add even if we have only one vertex (which means there is no valid edge)
    size_t min_nb_elt_to_add = add_one_elt ? 1 : 2;
    if (reverse_path.size() >= min_nb_elt_to_add) {
        p.path_items.push_back(path_item);
    }
    return p;
}

/**
 * Compute the angle between the last segment of the path and the next point
 *
 * A-------B
 *  \)
 *   \
 *    \
 *     C
 *
 *                       l(AB)² + l(AC)² - l(BC)²
 *the angle ABC = cos-1(_________________________)
 *                          2 * l(AB) * l(AC)
 *
 * with l(AB) = length OF AB
 *
 * The computed angle is 180 - the angle ABC, ie we compute the turn angle of the path
 */
int compute_directions(const navitia::georef::Path& path, const nt::GeographicalCoord& c_coord) {
    if (path.path_items.empty()) {
        return 0;
    }
    nt::GeographicalCoord b_coord, a_coord;
    const PathItem& last_item = path.path_items.back();
    a_coord = last_item.coordinates.back();
    if (last_item.coordinates.size() > 1) {
        b_coord = last_item.coordinates[last_item.coordinates.size() - 2];
    } else {
        if (path.path_items.size() < 2) {
            return 0;  // we don't have 2 previous coordinate, we can't compute an angle
        }
        const PathItem& previous_item = *(++(path.path_items.rbegin()));
        b_coord = previous_item.coordinates.back();
    }
    if (a_coord == b_coord || b_coord == c_coord || a_coord == c_coord) {
        return 0;
    }

    double len_ab = a_coord.distance_to(b_coord);
    double len_bc = b_coord.distance_to(c_coord);
    double len_ac = a_coord.distance_to(c_coord);

    double numerator = pow(len_ab, 2) + pow(len_ac, 2) - pow(len_bc, 2);
    double ab_lon = b_coord.lon() - a_coord.lon();
    double bc_lat = c_coord.lat() - b_coord.lat();
    double ab_lat = b_coord.lat() - a_coord.lat();
    double bc_lon = c_coord.lon() - b_coord.lon();

    double denominator = 2 * len_ab * len_ac;
    double raw_angle = acos(numerator / denominator);

    double det = ab_lon * bc_lat - ab_lat * bc_lon;

    // conversion into angle
    raw_angle *= 360 / (2 * boost::math::constants::pi<double>());

    int rounded_angle = std::round(raw_angle);

    rounded_angle = 180 - rounded_angle;
    if (det < 0)
        rounded_angle *= -1.0;

    return rounded_angle;
}

}  // namespace georef
}  // namespace navitia
