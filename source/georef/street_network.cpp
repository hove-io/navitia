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

#include "street_network.h"

#include "georef.h"
#include "type/data.h"

#include <chrono>

namespace navitia {
namespace georef {

StreetNetwork::StreetNetwork(const GeoRef& geo_ref)
    : geo_ref(geo_ref), departure_path_finder(geo_ref), arrival_path_finder(geo_ref), direct_path_finder(geo_ref) {}

void StreetNetwork::init(const type::EntryPoint& start, const boost::optional<const type::EntryPoint&>& end) {
    departure_path_finder.init(start.coordinates, start.streetnetwork_params.mode,
                               start.streetnetwork_params.speed_factor);
    if (end) {
        arrival_path_finder.init((*end).coordinates, (*end).streetnetwork_params.mode,
                                 (*end).streetnetwork_params.speed_factor);
    }
}

bool StreetNetwork::departure_launched() const {
    return departure_path_finder.computation_launch;
}
bool StreetNetwork::arrival_launched() const {
    return arrival_path_finder.computation_launch;
}

routing::map_stop_point_duration StreetNetwork::find_nearest_stop_points(
    const navitia::time_duration& radius,
    const proximitylist::ProximityList<type::idx_t>& pl,
    bool use_second) {
    // delegate to the arrival or departure pathfinder
    // results are store to build the routing path after the transportation routing computation
    return (use_second ? arrival_path_finder : departure_path_finder).find_nearest_stop_points(radius, pl);
}

navitia::time_duration StreetNetwork::get_distance(type::idx_t target_idx, bool use_second) {
    return (use_second ? arrival_path_finder : departure_path_finder).get_distance(target_idx);
}

Path StreetNetwork::get_path(type::idx_t idx, bool use_second) {
    Path result;
    if (!use_second) {
        result = departure_path_finder.get_path(idx);

        if (!result.path_items.empty()) {
            result.path_items.front().coordinates.push_front(departure_path_finder.starting_edge.projected);
        }
    } else {
        result = arrival_path_finder.get_path(idx);

        // we have to reverse the path
        std::reverse(result.path_items.begin(), result.path_items.end());
        int last_angle = 0;
        const auto& speed_factor = arrival_path_finder.speed_factor;
        for (auto& item : result.path_items) {
            std::reverse(item.coordinates.begin(), item.coordinates.end());

            // we have to reverse the directions too
            // the first direction become 0,
            // and we 'shift' all directions to the next path_item after reverting them
            int current_angle = -1 * item.angle;
            item.angle = last_angle;
            last_angle = current_angle;

            // FIXME: ugly temporary fix
            // while we don't use a boost::reverse_graph, the easiest way to handle
            // the bss rent/putback section in the arrival section is to swap them
            if (item.transportation == PathItem::TransportCaracteristic::BssTake) {
                item.transportation = PathItem::TransportCaracteristic::BssPutBack;
                item.duration = geo_ref.default_time_bss_putback / speed_factor;
            } else if (item.transportation == PathItem::TransportCaracteristic::BssPutBack) {
                item.transportation = PathItem::TransportCaracteristic::BssTake;
                item.duration = geo_ref.default_time_bss_pickup / speed_factor;
            }
            if (item.transportation == PathItem::TransportCaracteristic::CarPark) {
                item.transportation = PathItem::TransportCaracteristic::CarLeaveParking;
                item.duration = geo_ref.default_time_parking_leave / speed_factor;
            } else if (item.transportation == PathItem::TransportCaracteristic::CarLeaveParking) {
                item.transportation = PathItem::TransportCaracteristic::CarPark;
                item.duration = geo_ref.default_time_parking_park / speed_factor;
            }
        }

        if (!result.path_items.empty()) {
            // no direction for the first elt
            result.path_items.back().coordinates.push_back(arrival_path_finder.starting_edge.projected);
        }
    }

    return result;
}

Path StreetNetwork::get_direct_path(const type::EntryPoint& origin, const type::EntryPoint& destination) {
    auto dest_mode = origin.streetnetwork_params.mode;
    if (dest_mode == type::Mode_e::Car) {
        // on direct path with car we want to arrive on the walking graph
        dest_mode = type::Mode_e::Walking;
    }
    const auto dest_edge = ProjectionData(destination.coordinates, geo_ref, dest_mode);
    if (!dest_edge.found) {
        return Path();
    }
    const auto max_dur = origin.streetnetwork_params.max_duration + destination.streetnetwork_params.max_duration;
    direct_path_finder.init(origin.coordinates, dest_edge.projected, origin.streetnetwork_params.mode,
                            origin.streetnetwork_params.speed_factor);

    direct_path_finder.start_distance_or_target_astar(max_dur, dest_edge.projected,
                                                      {dest_edge[source_e], dest_edge[target_e]});
    const auto dest_vertex = direct_path_finder.find_nearest_vertex(dest_edge, true);
    const auto res = direct_path_finder.get_path(dest_edge, dest_vertex);
    if (res.duration > max_dur) {
        return Path();
    }
    return res;
}
}  // namespace georef
}  // namespace navitia
