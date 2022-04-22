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

#pragma once
#include "georef/georef_types.h"
#include "georef/edge.h"

namespace navitia {
namespace georef {

/** When given a coordinate, we have to associate it with the street network.
 *
 * This structure handle this.
 *
 * It contains
 *   - 2 possible nodes (each end of the edge where the coordinate has been projected)
 *   - the coordinate of the projection
 *   - the 2 distances between the projected point and the ends (NOTE, this is not the distance between the coordinate
 * and the ends)
 *
 */
struct ProjectionData {
    // enum used to acces the nodes and the distances
    enum class Direction { Source = 0, Target, size };
    // 2 possible nodes (each end of the edge where the coordinate has been projected)
    flat_enum_map<Direction, vertex_t> vertices;

    // The edge we projected on. Needed since we can't be sure to get the right edge with only the source and the target
    // because of parallel edges.
    Edge edge;

    // has the projection been successful?
    bool found = false;

    // The coordinate projected on the edge
    type::GeographicalCoord projected;

    // the original coordinate before projection
    type::GeographicalCoord real_coord;

    // Distance between the projected point and the ends
    flat_enum_map<Direction, double> distances{{{-1, -1}}};

    ProjectionData() = default;
    // Project the coordinate on the graph corresponding to the transportation mode of the offset
    ProjectionData(const type::GeographicalCoord& coord, const GeoRef& sn, type::Mode_e mode = type::Mode_e::Walking);

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& vertices& projected& distances& found& real_coord& edge;
    }

    void init(const type::GeographicalCoord& coord, const GeoRef& sn, const edge_t& nearest_edge);

    // syntaxic sugar
    vertex_t operator[](Direction d) const {
        if (!found) {
            throw proximitylist::NotFound();
        }

        return vertices[d];
    }
};

}  // namespace georef
}  // namespace navitia
