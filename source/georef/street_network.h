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

#pragma once
#include "georef.h"
#include "dijkstra_path_finder.h"
#include "type/time_duration.h"

namespace navitia {
namespace georef {

/** Structure managing the computation on the streetnetwork */
struct StreetNetwork {
    StreetNetwork(const GeoRef& geo_ref);

    void init(const type::EntryPoint& start_coord, boost::optional<const type::EntryPoint&> end_coord = {});

    bool departure_launched() const;
    bool arrival_launched() const;

    routing::map_stop_point_duration find_nearest_stop_points(const navitia::time_duration& radius,
                                                              const proximitylist::ProximityList<type::idx_t>& pl,
                                                              bool use_second);

    navitia::time_duration get_distance(type::idx_t target_idx, bool use_second = false);

    Path get_path(type::idx_t idx, bool use_second = false);

    /**
     * Build the direct path between the start and the end
     **/
    Path get_direct_path(const type::EntryPoint& origin, const type::EntryPoint& destination);

    const GeoRef& geo_ref;
    DijkstraPathFinder departure_path_finder;
    DijkstraPathFinder arrival_path_finder;
    DijkstraPathFinder direct_path_finder;
};

}  // namespace georef
}  // namespace navitia
