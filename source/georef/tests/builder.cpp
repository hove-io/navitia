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

#include "builder.h"

namespace navitia {
namespace georef {

GraphBuilder& GraphBuilder::add_vertex(std::string node_name, double x, double y) {
    auto it = this->vertex_map.find(node_name);
    vertex_t v;
    type::GeographicalCoord coord;
    coord.set_xy(x, y);
    if (it == this->vertex_map.end()) {
        v = boost::add_vertex(this->geo_ref.graph);
        vertex_map[node_name] = v;
    } else {
        v = it->second;
    }

    this->geo_ref.graph[v].coord = coord;
    return *this;
}

GraphBuilder& GraphBuilder::add_edge(std::string source_name,
                                     std::string target_name,
                                     navitia::time_duration dur,
                                     bool bidirectionnal) {
    vertex_t source, target;
    auto it = this->vertex_map.find(source_name);
    if (it == this->vertex_map.end())
        this->add_vertex(source_name, 0, 0);
    source = this->vertex_map[source_name];

    it = this->vertex_map.find(target_name);
    if (it == this->vertex_map.end())
        this->add_vertex(target_name, 0, 0);
    target = this->vertex_map[target_name];

    Edge edge;
    edge.duration = dur.is_negative() ? navitia::seconds(0) : dur;

    Way* way = new Way();
    way->idx = this->geo_ref.ways.size();
    this->geo_ref.ways.push_back(way);
    edge.way_idx = way->idx;

    boost::add_edge(source, target, edge, this->geo_ref.graph);
    if (bidirectionnal)
        boost::add_edge(target, source, edge, this->geo_ref.graph);

    return *this;
}

GraphBuilder& GraphBuilder::add_geom(edge_t edge_ref, const nt::LineString& geom) {
    auto& edge = this->geo_ref.graph[edge_ref];
    if (edge.way_idx == nt::invalid_idx) {
        Way* way = new Way();
        way->idx = this->geo_ref.ways.size();
        this->geo_ref.ways.push_back(way);
        edge.way_idx = way->idx;
    }

    Way* way = this->geo_ref.ways.at(edge.way_idx);
    edge.geom_idx = way->geoms.size();
    way->geoms.push_back(geom);

    return *this;
}

vertex_t GraphBuilder::get(const std::string& node_name) {
    return this->vertex_map[node_name];
}

edge_t GraphBuilder::get(const std::string& source_name, const std::string& target_name) {
    vertex_t source = this->get(source_name);
    vertex_t target = this->get(target_name);
    edge_t e;
    bool b;
    boost::tie(e, b) = boost::edge(source, target, this->geo_ref.graph);
    if (!b)
        throw proximitylist::NotFound();
    else
        return e;
}

}  // namespace georef
}  // namespace navitia
