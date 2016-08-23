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
#include <boost/graph/dijkstra_shortest_paths.hpp>

namespace navitia { namespace routing {

struct BoundBox {
    type::GeographicalCoord max = type::GeographicalCoord(-180, -90);
    type::GeographicalCoord min = type::GeographicalCoord(180, 90);

    void set_box(const type::GeographicalCoord& coord,
                 const double offset) {
        const auto lon_max = std::max(coord.lon() + offset, this->max.lon());
        const auto lat_max = std::max(coord.lat() + offset, this->max.lat());
        const auto lon_min = std::min(coord.lon() - offset, this->min.lon());
        const auto lat_min = std::min(coord.lat() - offset, this->min.lat());
        this->max = type::GeographicalCoord(lon_max, lat_max);
        this->min = type::GeographicalCoord(lon_min, lat_min);
    }
};

const auto source_e = georef::ProjectionData::Direction::Source;
const auto target_e = georef::ProjectionData::Direction::Target;

static navitia::time_duration set_duration(const georef::ProjectionData& projected_edge,
                                           const type::GeographicalCoord& center,
                                           const std::vector<navitia::time_duration>& distances,
                                           const double speed,
                                           const double max_duration) {
    if (!projected_edge.found) {
        return bt::pos_infin;
    }
    const auto distance_source = distances[projected_edge[source_e]];
    const auto distance_target = distances[projected_edge[target_e]];
    const auto coslat = cos(center.lat());
    const auto distance_center = sqrt(projected_edge.projected.approx_sqr_distance(center, coslat));
    const auto duration_center = navitia::milliseconds( distance_center / speed * 1e3);
    const auto proj_source = navitia::milliseconds(projected_edge.distances[source_e] / speed * 1e3);
    const auto proj_target = navitia::milliseconds(projected_edge.distances[target_e] / speed * 1e3);
    navitia::time_duration duration;
    duration = std::min(distance_source + proj_source,
                        distance_target + proj_target + duration_center);
    if (duration.total_seconds() > max_duration) {
        duration = bt::pos_infin;
    }
    return duration;
}

static std::string print_single_coord(const SingleCoord& coord,
                                      const std::string& type) {
    std::stringstream ss;
    ss << R"(")" << type << R"(":{)";
    ss << R"("min_)" << type << R"(":)" << coord.min_coord;
    ss << R"(,"middle_)" << type << R"(":)" << coord.min_coord + coord.step / 2;
    ss << R"(,"max_)" << type << R"(":)" << coord.min_coord + coord.step;
    ss << "}";
    return ss.str();
}

static void print_lat(std::stringstream& ss,
                      const SingleCoord lat) {
    ss << "{";
    ss << print_single_coord(lat, "lat");
    ss << "}";
}

static void print_datetime(std::stringstream& ss,
                           navitia::time_duration duration) {
    ss << R"({"duration":)";
    if (duration.is_pos_infinity()) {
        ss << R"(null)";
    } else {
        ss << duration.total_seconds();
    }
    ss << "}";
}

static void print_body(std::stringstream& ss,
                       std::pair <SingleCoord, std::vector<navitia::time_duration>> pair) {
    ss <<"{";
    ss << print_single_coord(pair.first, "lon");
    ss << R"(,"row":[)";
    separated_by_coma(ss, print_datetime, pair.second);
    ss << R"(]})";
}

std::string print_grid(const HeatMap& heat_map) {
    std::stringstream header;
    std::stringstream body;
    header << R"({"header":[)";
    separated_by_coma(header, print_lat, heat_map.header);
    body << R"(],"body":[)";
    separated_by_coma(body, print_body, heat_map.body);
    body << "]}";
    return header.str() + body.str();
}

static HeatMap fill_heat_map(const uint step,
                             const BoundBox& box,
                             const double height_step,
                             const double width_step,
                             const georef::GeoRef& worker,
                             const double min_dist,
                             const double max_duration,
                             const double speed,
                             const std::vector<navitia::time_duration>& distances,
                             const nt::idx_t offset) {
    std::vector<SingleCoord> header;
    std::vector<std::pair <SingleCoord, std::vector<navitia::time_duration>>> body;
    for (uint i = 0; i < step; i++) {
        std::vector<navitia::time_duration> local_duration;
        auto lon = SingleCoord(box.min.lon() + i * width_step, width_step);
        for (uint j = 0; j < step; j++) {
            if (i == 0) {
                header.push_back(SingleCoord((box.min.lat() + j * height_step), height_step));
            }
            auto start_coord = type::GeographicalCoord(lon.min_coord + width_step / 2, header[j].min_coord + height_step / 2);
            auto projected_edge = georef::ProjectionData(start_coord, worker, offset, worker.pl, min_dist);
            auto dur = set_duration(projected_edge, start_coord, distances, speed, max_duration);
            local_duration.push_back(dur);
        }
        auto local_body = std::make_pair(lon, local_duration);
        body.push_back(std::move(local_body));
    }
    return HeatMap(header, body);
}

static std::string build_grid(const georef::GeoRef& worker,
                              BoundBox& box,
                              const nt::Mode_e& mode,
                              const std::vector<navitia::time_duration>& distances,
                              const double speed,
                              const double max_duration) {
    const auto step = 150;
    nt::idx_t offset = worker.offsets[mode];
    double width_step = (box.max.lon() - box.min.lon()) / step;
    double height_step = (box.max.lat() - box.min.lat()) / step;
    auto min_dist = std::max(500., width_step * N_DEG_TO_DISTANCE);
    min_dist = std::max(min_dist, height_step * N_DEG_TO_DISTANCE);
    auto heat_map = fill_heat_map(step, box, height_step, width_step, worker, min_dist, max_duration,
                                  speed, distances, offset);
    return print_grid(heat_map);
}

static double walking_distance(const DateTime& max_duration,
                               const DateTime& duration,
                               const double speed) {
    return (max_duration - duration) * speed / type::GeographicalCoord::EARTH_RADIUS_IN_METERS * N_RAD_TO_DEG;
}

static std::vector<navitia::time_duration> init_distance(const georef::GeoRef & worker,
                                                         const std::vector<type::StopPoint*>& stop_points,
                                                         const DateTime& init_dt,
                                                         const RAPTOR& raptor,
                                                         const type::Mode_e& mode,
                                                         const type::GeographicalCoord& coord_origin,
                                                         const bool clockwise,
                                                         const DateTime& bound,
                                                         const double speed,
                                                         const DateTime& duration) {
    std::vector<navitia::time_duration> distances;
    size_t n = boost::num_vertices(worker.graph);
    distances.assign(n, bt::pos_infin);
    nt::idx_t offset = worker.offsets[mode];
    auto proj = georef::ProjectionData(coord_origin, worker, offset, worker.pl);
    if (proj.found) {
        distances[proj[source_e]] = navitia::seconds(duration + proj.distances[source_e] / speed);
        distances[proj[target_e]] = navitia::seconds(duration + proj.distances[target_e] / speed);
    }
    for(const type::StopPoint* sp: stop_points) {
        SpIdx sp_idx(*sp);
        const auto& best_lbl = raptor.best_labels_pts[sp_idx];
        if (in_bound(best_lbl, bound, clockwise)) {
            const auto& projections = worker.projected_stop_points[sp->idx];
            const auto& proj = projections[mode];
            if(proj.found) {
                const double duration = best_lbl - init_dt;
                distances[proj[source_e]] = navitia::seconds(duration + proj.distances[source_e] / speed);
                distances[proj[target_e]] = navitia::seconds(duration + proj.distances[target_e] / speed);
            }
        }
    }
    return distances;
}

static std::vector<georef::vertex_t> init_vertex(const georef::GeoRef & worker,
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
    for(const type::StopPoint* sp: stop_points) {
        SpIdx sp_idx(*sp);
        const auto& best_lbl = raptor.best_labels_pts[sp_idx];
        if (in_bound(best_lbl, bound, clockwise)) {
            const auto& projections = worker.projected_stop_points[sp->idx];
            const auto& proj = projections[mode];
            if(proj.found) {
                initialized_points.push_back(proj[source_e]);
                initialized_points.push_back(proj[target_e]);
            }
        }
    }
    return initialized_points;
}

static BoundBox find_boundary_box(const georef::GeoRef & worker,
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
    for(const type::StopPoint* sp: stop_points) {
        SpIdx sp_idx(*sp);
        const auto& best_lbl = raptor.best_labels_pts[sp_idx];
        if (in_bound(best_lbl, bound, clockwise)) {
            const auto& projections = worker.projected_stop_points[sp->idx];
            const auto& proj = projections[mode];
            if(proj.found) {
                const double duration = best_lbl - init_dt;
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
                                   const DateTime bound) {
    const auto& stop_points = raptor.data.pt_data->stop_points;
    std::vector<georef::vertex_t> predecessors;
    size_t n = boost::num_vertices(worker.graph);
    predecessors.resize(n);
    auto box = find_boundary_box(worker, stop_points, init_dt, raptor, mode, coord_origin,
                                 clockwise, bound, duration, speed);
    auto init_points = init_vertex(worker, stop_points, raptor, mode, coord_origin,
                                   clockwise, bound);
    auto distances = init_distance(worker, stop_points, init_dt, raptor, mode, coord_origin,
                                   clockwise, bound, speed, duration);
    auto start = init_points.begin();
    auto end = init_points.end();
    double speed_factor = speed / georef::default_speed[mode];
    auto visitor = georef::distance_visitor(navitia::seconds(duration), distances);
    auto index_map = boost::identity_property_map();
    using filtered_graph = boost::filtered_graph<georef::Graph, boost::keep_all, georef::TransportationModeFilter>;
    // We cannot make a dijkstra multi start with old boost version
#if BOOST_VERSION >= 105500
    try {
        boost::dijkstra_shortest_paths_no_init(filtered_graph(worker.graph, {},
                                                              georef::TransportationModeFilter(mode, worker)),
                                               start, end, &predecessors[0], &distances[0],
                                               boost::get(&georef::Edge::duration, worker.graph),
                                               index_map,
                                               std::less<navitia::time_duration>(),
                                               georef::SpeedDistanceCombiner(speed_factor),
                                               navitia::seconds(0),
                                               visitor);
    } catch (georef::DestinationFound){};
#endif
    return build_grid(worker, box, mode, distances, speed, duration);
}

}} //namespace navitia::routing
