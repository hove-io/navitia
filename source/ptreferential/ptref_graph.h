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

#include "type/type_interfaces.h"

#include <boost/graph/adjacency_list.hpp>

namespace navitia {
namespace ptref {

struct Edge {
    float weight;
    Edge() : weight(1) {}
    Edge(float w) : weight(w) {}
};

struct Jointures {
    using Graph = boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, type::Type_e, Edge>;
    using vertex_t = boost::graph_traits<Graph>::vertex_descriptor;
    using edge_t = boost::graph_traits<Graph>::edge_descriptor;

    navitia::flat_enum_map<type::Type_e, vertex_t> vertex_map{};
    Graph g;
    Jointures();
};

/// Find the path between types.  `res[t]` is the successor of `t` in
/// the path. If `res[t] == t`, we are at the begin of the path,
/// i.e. we arrived at the source, or there is no path to the source.
///
/// For example, to find the path from Route to CommercialMode:
/// ```
/// const auto succ = find_path(Type_e::CommercialMode);
/// std::vector<Type_e> res;
/// for (auto cur = Type_e::Route; succ.at(cur) != cur; cur = succ.at(cur)) {
///     res.push_back(succ.at(cur));
/// }
/// // the path is Route -> Line -> CommercialMode
/// BOOST_CHECK_EQUAL_RANGE(res, std::vector<Type_e>({Type_e::Line, Type_e::CommercialMode}));
/// ```
std::map<type::Type_e, type::Type_e> find_path(type::Type_e source);

}  // namespace ptref
}  // namespace navitia
