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

#include "geographical_coord.h"

#include <RTree/RTree.h>
#include <boost/geometry.hpp>
#include <boost/serialization/deque.hpp>

namespace navitia {
namespace type {

namespace bg = boost::geometry;

template <typename T>
struct MultiPolygonMap {
    using Box = boost::geometry::model::box<GeographicalCoord>;
    using Value = std::pair<MultiPolygon, T>;
    using RT = RTree<const Value*, double, 2>;

    void insert(const MultiPolygon& key, T data) {
        pool.emplace_back(key, data);
        rtree_insert(pool.back());
    }

    template <class Archive>
    void save(Archive& ar, const unsigned int) const {
        ar& pool;
    }
    template <class Archive>
    void load(Archive& ar, const unsigned int) {
        ar& pool;
        for (const auto& elt : pool) {
            rtree_insert(elt);
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::vector<T> find(const GeographicalCoord& key) const {
        using result_type = std::vector<T>;
        result_type res;
        using context_type = std::pair<const GeographicalCoord&, result_type&>;
        context_type ctx = {key, res};
        const double coord[] = {key.lon(), key.lat()};
        auto callback = [](const Value* val, void* ctx_ptr) -> bool {
            context_type& ctx = *static_cast<context_type*>(ctx_ptr);
            if (boost::geometry::within(ctx.first, val->first)) {
                ctx.second.push_back(val->second);
            }
            return true;
        };
        rtree.Search(coord, coord, callback, &ctx);
        return res;
    }

private:
    void rtree_insert(const Value& elt) {
        const auto box = boost::geometry::return_envelope<Box>(elt.first);
        const auto& min_corner = box.min_corner();
        const double min[] = {min_corner.lon(), min_corner.lat()};
        const auto& max_corner = box.max_corner();
        const double max[] = {max_corner.lon(), max_corner.lat()};
        rtree.Insert(min, max, &elt);
    }
    mutable RT rtree;        // RTree::Search is not const!
    std::deque<Value> pool;  // using a deque as there is no iterator invalidation after push_back
};

}  // namespace type
}  // namespace navitia
