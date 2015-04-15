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

#include "geographical_coord.h"
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace navitia { namespace type {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

template<typename T>
struct MultiPolygonMap {
    typedef boost::geometry::model::box<GeographicalCoord> Box;
    typedef std::tuple<Box, MultiPolygon, T> Value;
    typedef bgi::rtree<Value, bgi::quadratic<16>> RTree;

    void insert(const MultiPolygon& key, T data) {
        rtree.insert(Value(bg::return_envelope<Box>(key), key, data));
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & rtree;
    }

    boost::iterator_range<typename RTree::const_query_iterator>
    find(const GeographicalCoord& key) const {
        std::vector<T> res;
        auto within_key = [key](const Value& v) { return bg::within(key, std::get<1>(v)); };
        auto begin = rtree.qbegin(bgi::contains(key) && bgi::satisfies(within_key));
        return boost::make_iterator_range(begin, rtree.qend());
    }

    typename RTree::const_query_iterator begin() const {
        return rtree.qbegin(bgi::satisfies([](Value const&) { return true; }));
    }
    typename RTree::const_query_iterator end() const {
        return rtree.qend();
    }

private:
    RTree rtree;
};

}} // namespace navitia::type
