/* Copyright © 2001-2016, Canal TP and/or its affiliates. All rights reserved.

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

#include "type/geographical_coord.h"
#include "routing/heat_map.h"
#include "raptor.h"
#include "isochrone.h"
#include "raptor_api.h"

#include <vector>

namespace navitia {
namespace routing {

const auto source_e = georef::ProjectionData::Direction::Source;
const auto target_e = georef::ProjectionData::Direction::Target;

static std::string print_single_coord(const SingleCoord& coord, const std::string& type) {
    std::stringstream ss;
    ss << R"(")"
       << "cell_" << type << R"(":{)";
    ss << R"("min_)" << type << R"(":)" << coord.min_coord;
    ss << R"(,"center_)" << type << R"(":)" << coord.min_coord + coord.step / 2;
    ss << R"(,"max_)" << type << R"(":)" << coord.min_coord + coord.step;
    ss << "}";
    return ss.str();
}

static void print_lat(std::stringstream& ss, const SingleCoord lat) {
    ss << "{";
    ss << print_single_coord(lat, "lat");
    ss << "}";
}

static void print_datetime(std::stringstream& ss, navitia::time_duration duration) {
    if (duration.is_pos_infinity()) {
        ss << R"(null)";
    } else {
        ss << duration.total_seconds();
    }
}

static void print_body(std::stringstream& ss, std::pair<SingleCoord, std::vector<navitia::time_duration>> pair) {
    ss << "{";
    ss << print_single_coord(pair.first, "lon");
    ss << R"(,"duration":[)";
    separated_by_coma(ss, print_datetime, pair.second);
    ss << R"(]})";
}

std::string print_grid(const HeatMap& heat_map) {
    std::stringstream ss;
    ss << R"({"line_headers":[)";
    separated_by_coma(ss, print_lat, heat_map.header);
    ss << R"(],"lines":[)";
    separated_by_coma(ss, print_body, heat_map.body);
    ss << "]}";
    return ss.str();
}

static std::pair<int, int> find_rank(const BoundBox& box,
                                     const type::GeographicalCoord& coord,
                                     const double height_step,
                                     const double width_step) {
    const auto lon_rank = floor((coord.lon() - box.min.lon()) / width_step);
    const auto lat_rank = floor((coord.lat() - box.min.lat()) / height_step);
    return std::make_pair(lon_rank, lat_rank);
}

struct Projection {
    boost::optional<double> distance;
    georef::vertex_t source;
    georef::vertex_t target;
    Projection(double distance, georef::vertex_t source, georef::vertex_t target)
        : distance(distance), source(source), target(target) {}

    Projection() : distance(boost::none) {}
};

struct Boundary {
    size_t max_lon;
    size_t max_lat;
    size_t min_lon;
    size_t min_lat;
    Boundary(size_t max_lon, size_t max_lat, size_t min_lon, size_t min_lat)
        : max_lon(max_lon), max_lat(max_lat), min_lon(min_lon), min_lat(min_lat) {}
};

static int get_rank(int source, int target, int offset, size_t step) {
    int rank = std::min(source, target) + offset;
    rank = std::max(rank, 0);
    return std::min<size_t>(rank, step - 1);
}

static Boundary find_boundary(const std::pair<int, int>& rank_source,
                              const std::pair<int, int>& rank_target,
                              const size_t offset_lon,
                              const size_t offset_lat,
                              const size_t step) {
    auto end_lon_box = get_rank(rank_source.first, rank_target.first, +offset_lon, step);
    auto end_lat_box = get_rank(rank_source.second, rank_target.second, +offset_lat, step);
    auto begin_lon_box = get_rank(rank_source.first, rank_target.first, -offset_lon, step);
    auto begin_lat_box = get_rank(rank_source.second, rank_target.second, -offset_lat, step);
    return Boundary(end_lon_box, end_lat_box, begin_lon_box, begin_lat_box);
}

static std::vector<std::vector<Projection>> find_projection(BoundBox box,
                                                            const double height_step,
                                                            const double width_step,
                                                            const georef::GeoRef& worker,
                                                            const double min_dist,
                                                            const HeatMap& heat_map,
                                                            const size_t step) {
    std::vector<std::vector<Projection>> dist_pixel = {step, {step, Projection()}};
    const size_t offset_lon = floor(min_dist / (width_step * N_DEG_TO_DISTANCE)) + 1;
    const size_t offset_lat = floor(min_dist / (height_step * N_DEG_TO_DISTANCE)) + 1;
    auto begin = std::lower_bound(
        worker.pl.items.begin(), worker.pl.items.end(), box.min.lon(),
        [](const proximitylist::ProximityList<georef::vertex_t>::Item& i, double min) { return i.coord.lon() < min; });

    if (begin == worker.pl.items.end()) {
        return dist_pixel;
    }

    auto end = std::upper_bound(
        begin, worker.pl.items.end(), box.max.lon(),
        [](double max, const proximitylist::ProximityList<georef::vertex_t>::Item& i) { return max <= i.coord.lon(); });

    const auto coslat = cos(begin->coord.lat() * type::GeographicalCoord::N_DEG_TO_RAD);
    for (auto it = begin; it != end; ++it) {
        const auto& source = it->coord;
        if (!box.contains(source)) {
            continue;
        }
        const auto rank_source = find_rank(box, source, height_step, width_step);
        BOOST_FOREACH (georef::edge_t e, boost::out_edges(it->element, worker.graph)) {
            const auto v = target(e, worker.graph);
            const auto& target = worker.graph[v].coord;
            const auto rank_target = find_rank(box, target, height_step, width_step);
            const auto boundary = find_boundary(rank_source, rank_target, offset_lon, offset_lat, step);
            for (uint lon_rank = boundary.min_lon; lon_rank <= boundary.max_lon; lon_rank++) {
                for (uint lat_rank = boundary.min_lat; lat_rank <= boundary.max_lat; lat_rank++) {
                    auto center = type::GeographicalCoord(heat_map.body[lon_rank].first.min_coord + width_step / 2,
                                                          heat_map.header[lat_rank].min_coord + height_step / 2);
                    auto proj = center.approx_project(source, target, coslat);
                    auto length = double(proj.second);
                    if (length < min_dist
                        && (!dist_pixel[lon_rank][lat_rank].distance
                            || length < *dist_pixel[lon_rank][lat_rank].distance)) {
                        dist_pixel[lon_rank][lat_rank].distance = length;
                        dist_pixel[lon_rank][lat_rank].source = it->element;
                        dist_pixel[lon_rank][lat_rank].target = v;
                    }
                }
            }
        }
    }
    return dist_pixel;
}

HeatMap fill_heat_map(const BoundBox& box,
                      const double height_step,
                      const double width_step,
                      const georef::GeoRef& worker,
                      const double min_dist,
                      const double max_duration,
                      const double speed,
                      const std::vector<navitia::time_duration>& distances,
                      const size_t step) {
    auto heat_map = HeatMap(step, box, height_step, width_step);
    auto projection = find_projection(box, height_step, width_step, worker, min_dist, heat_map, step);
    for (size_t i = 0; i < step; i++) {
        for (size_t j = 0; j < step; j++) {
            auto& duration = heat_map.body[i].second[j];
            if (projection[i][j].distance) {
                auto center = type::GeographicalCoord(heat_map.body[i].first.min_coord + width_step / 2,
                                                      heat_map.header[j].min_coord + height_step / 2);
                const auto source = worker.graph[projection[i][j].source].coord;
                const auto target = worker.graph[projection[i][j].target].coord;
                const auto coslat = cos(center.lat() * type::GeographicalCoord::N_DEG_TO_RAD);
                const auto duration_to_source =
                    distances[projection[i][j].source]
                    + navitia::milliseconds(sqrt(center.approx_sqr_distance(source, coslat)) / speed * 1e3);
                const auto duration_to_target =
                    distances[projection[i][j].target]
                    + navitia::milliseconds(sqrt(center.approx_sqr_distance(target, coslat)) / speed * 1e3);
                const auto new_duration = std::min(duration_to_source, duration_to_target);
                if (new_duration.total_seconds() < max_duration) {
                    duration = new_duration;
                } else {
                    duration = bt::pos_infin;
                }
            } else {
                duration = bt::pos_infin;
            }
        }
    }
    return heat_map;
}

static std::string build_grid(const georef::GeoRef& worker,
                              const BoundBox& box,
                              const std::vector<navitia::time_duration>& distances,
                              const double speed,
                              const double max_duration,
                              const uint resolution) {
    double width_step = (box.max.lon() - box.min.lon()) / resolution;
    double height_step = (box.max.lat() - box.min.lat()) / resolution;
    auto min_dist = std::max(500., width_step * N_DEG_TO_DISTANCE);
    min_dist = std::max(min_dist, height_step * N_DEG_TO_DISTANCE);
    auto heat_map =
        fill_heat_map(box, height_step, width_step, worker, min_dist, max_duration, speed, distances, resolution);
    return print_grid(heat_map);
}

static double walking_distance(const DateTime& max_duration, const DateTime& duration, const double speed) {
    return (max_duration - duration) * speed / type::GeographicalCoord::EARTH_RADIUS_IN_METERS * N_RAD_TO_DEG;
}

std::vector<navitia::time_duration> init_distance(const georef::GeoRef& worker,
                                                  const std::vector<type::StopPoint*>& stop_points,
                                                  const DateTime& init_dt,
                                                  const RAPTOR& raptor,
                                                  const type::Mode_e& mode,
                                                  const type::GeographicalCoord& coord_origin,
                                                  const bool clockwise,
                                                  const DateTime& bound,
                                                  const double speed) {
    std::vector<navitia::time_duration> distances;
    size_t n = boost::num_vertices(worker.graph);
    distances.assign(n, bt::pos_infin);
    nt::idx_t offset = worker.offsets[mode];
    auto proj = georef::ProjectionData(coord_origin, worker, offset, worker.pl);
    if (proj.found) {
        distances[proj[source_e]] =
            std::min(distances[proj[source_e]], time_duration(seconds(proj.distances[source_e] / speed)));
        distances[proj[target_e]] =
            std::min(distances[proj[target_e]], time_duration(seconds(proj.distances[target_e] / speed)));
    }
    for (const type::StopPoint* sp : stop_points) {
        SpIdx sp_idx(*sp);
        const auto& best_lbl = raptor.best_labels_pts[sp_idx];
        if (in_bound(best_lbl, bound, clockwise)) {
            const auto& projections = worker.projected_stop_points[sp->idx];
            const auto& proj = projections[mode];
            if (proj.found) {
                const double duration = clockwise ? best_lbl - init_dt : init_dt - best_lbl;
                distances[proj[source_e]] = std::min(
                    distances[proj[source_e]], time_duration(seconds(duration + proj.distances[source_e] / speed)));
                distances[proj[target_e]] = std::min(
                    distances[proj[target_e]], time_duration(seconds(duration + proj.distances[target_e] / speed)));
            }
        }
    }
    return distances;
}

static std::vector<georef::vertex_t> init_vertex(const georef::GeoRef& worker,
                                                 const std::vector<type::StopPoint*>& stop_points,
                                                 const RAPTOR& raptor,
                                                 const type::Mode_e& mode,
                                                 const type::GeographicalCoord& coord_origin,
                                                 const bool clockwise,
                                                 const DateTime& bound) {
    std::vector<georef::vertex_t> initialized_points;
    nt::idx_t offset = worker.offsets[mode];
    auto proj = georef::ProjectionData(coord_origin, worker, offset, worker.pl);
    if (proj.found) {
        initialized_points.push_back(proj[source_e]);
        initialized_points.push_back(proj[target_e]);
    }
    for (const type::StopPoint* sp : stop_points) {
        SpIdx sp_idx(*sp);
        const auto& best_lbl = raptor.best_labels_pts[sp_idx];
        if (in_bound(best_lbl, bound, clockwise)) {
            const auto& projections = worker.projected_stop_points[sp->idx];
            const auto& proj = projections[mode];
            if (proj.found) {
                initialized_points.push_back(proj[source_e]);
                initialized_points.push_back(proj[target_e]);
            }
        }
    }
    return initialized_points;
}

static BoundBox find_boundary_box(const georef::GeoRef& worker,
                                  const std::vector<type::StopPoint*>& stop_points,
                                  const DateTime& init_dt,
                                  const RAPTOR& raptor,
                                  const type::Mode_e& mode,
                                  const type::GeographicalCoord& coord_origin,
                                  const bool clockwise,
                                  const DateTime& bound,
                                  const DateTime& max_duration,
                                  const double speed) {
    auto box = BoundBox();
    const auto distance_500m = (500 / type::GeographicalCoord::EARTH_RADIUS_IN_METERS) * N_RAD_TO_DEG;
    nt::idx_t offset = worker.offsets[mode];
    auto proj = georef::ProjectionData(coord_origin, worker, offset, worker.pl);
    if (proj.found) {
        box.set_box(coord_origin, walking_distance(max_duration, 0, speed) + distance_500m);
    }
    for (const type::StopPoint* sp : stop_points) {
        SpIdx sp_idx(*sp);
        const auto& best_lbl = raptor.best_labels_pts[sp_idx];
        if (in_bound(best_lbl, bound, clockwise)) {
            const auto& projections = worker.projected_stop_points[sp->idx];
            const auto& proj = projections[mode];
            if (proj.found) {
                const double duration = clockwise ? best_lbl - init_dt : init_dt - best_lbl;
                box.set_box(sp->coord, walking_distance(max_duration, duration, speed) + distance_500m);
            }
        }
    }
    return box;
}

std::string build_raster_isochrone(const georef::GeoRef& worker,
                                   const double& speed,
                                   const type::Mode_e& mode,
                                   const DateTime init_dt,
                                   RAPTOR& raptor,
                                   const type::GeographicalCoord& coord_origin,
                                   const DateTime duration,
                                   const bool clockwise,
                                   const DateTime bound,
                                   const uint resolution) {
    const auto& stop_points = raptor.data.pt_data->stop_points;
    std::vector<georef::vertex_t> predecessors;
    size_t n = boost::num_vertices(worker.graph);
    predecessors.resize(n);
    auto box =
        find_boundary_box(worker, stop_points, init_dt, raptor, mode, coord_origin, clockwise, bound, duration, speed);
    auto init_points = init_vertex(worker, stop_points, raptor, mode, coord_origin, clockwise, bound);
    auto distances = init_distance(worker, stop_points, init_dt, raptor, mode, coord_origin, clockwise, bound, speed);
    auto start = init_points.begin();
    auto end = init_points.end();
    float speed_factor = float(speed) / georef::default_speed[mode];
    auto visitor = georef::dijkstra_distance_visitor(navitia::seconds(duration), distances);
    auto index_map = boost::identity_property_map();
    using filtered_graph = boost::filtered_graph<georef::Graph, boost::keep_all, georef::TransportationModeFilter>;
    try {
        boost::dijkstra_shortest_paths_no_init(
            filtered_graph(worker.graph, {}, georef::TransportationModeFilter(mode, worker)), start, end,
            &predecessors[0], &distances[0], boost::get(&georef::Edge::duration, worker.graph), index_map,
            std::less<navitia::time_duration>(), georef::SpeedDistanceCombiner(speed_factor), navitia::seconds(0),
            visitor);
    } catch (georef::DestinationFound) {
    }
    return build_grid(worker, box, distances, speed, duration, resolution);
}

}  // namespace routing
}  // namespace navitia
