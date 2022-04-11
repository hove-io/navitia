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

#include "georef/georef.h"

namespace navitia {
namespace georef {

/**
 * Easy way to build a street network graph
 * Mainly used by unit tests
 */
struct GraphBuilder {
    /// street network graph to build
    GeoRef geo_ref;

    /// node by names
    std::map<std::string, vertex_t> vertex_map;

    void init() {
        geo_ref.init();
        geo_ref.build_proximity_list();
    }

    /// upsert a node
    GraphBuilder& add_vertex(std::string node_name, double x, double y);

    /// upsert an edge
    GraphBuilder& add_edge(std::string source_name,
                           std::string target_name,
                           navitia::time_duration dur = {},
                           bool bidirectionnal = false);

    GraphBuilder& add_geom(edge_t edge_ref, const nt::LineString& geom);

    /// Syntaxic sugar on node creation
    GraphBuilder& operator()(std::string node_name, double x, double y) { return add_vertex(node_name, x, y); }

    /// Syntaxic sugar on edge creation
    GraphBuilder& operator()(std::string source_name,
                             std::string target_name,
                             navitia::time_duration dur = {},
                             bool bidirectionnal = false) {
        return add_edge(source_name, target_name, dur, bidirectionnal);
    }

    GraphBuilder& operator()(edge_t edge_ref, const nt::LineString& geom) { return add_geom(edge_ref, geom); }

    vertex_t get(const std::string& node_name);
    edge_t get(const std::string& source_name, const std::string& target_name);
};

}  // namespace georef
}  // namespace navitia
