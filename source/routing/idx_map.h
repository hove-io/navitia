/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.
  
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

#include <vector>
#include <iterator>
#include <boost/range/iterator_range_core.hpp>

namespace navitia { namespace routing {

typedef uint32_t idx_t;
const idx_t invalid_idx = std::numeric_limits<idx_t>::max();

// Strong typing of index with a phantom type!
template<typename T> struct Idx {
    inline explicit Idx(): val(invalid_idx) {}
    inline explicit Idx(const idx_t& v): val(v) {}
    inline explicit Idx(const T& o): val(o.idx) {}
    inline bool is_valid() const { return val != invalid_idx; }
    inline bool operator==(const Idx& other) const { return val == other.val; }
    inline bool operator!=(const Idx& other) const { return val != other.val; }
    inline bool operator<(const Idx& other) const { return val < other.val; }
    inline friend std::ostream& operator<<(std::ostream& os, const Idx& idx) {
        return os << idx.val;
    }

    idx_t val; // the value of the index
};

template<typename K, typename I>
class IdxMapIterator: public boost::iterator_facade<
    IdxMapIterator<K, I>,
    std::pair<const K, typename std::iterator_traits<I>::reference>,
    boost::random_access_traversal_tag,
    std::pair<const K, typename std::iterator_traits<I>::reference> >
{
public:
    typedef typename std::iterator_traits<I> traits;
    typedef typename traits::difference_type difference_type;
    
    inline IdxMapIterator() {}
    inline IdxMapIterator(const idx_t& i, I it): idx(i), iterator(it) {}

private:
    friend class boost::iterator_core_access;
    
    inline void increment() { ++idx; ++iterator; }
    inline void decrement() { --idx; --iterator; }
    inline void advance(difference_type n) { idx += n; iterator += n; }
    inline difference_type distance_to(const IdxMapIterator& other) {
        return iterator - other.iterator;
    }
    inline bool equal(const IdxMapIterator& other) const {
        return iterator == other.iterator;
    }
    inline std::pair<const K, typename traits::reference> dereference() const {
        return {K(idx), *iterator};
    }

    idx_t idx;
    I iterator;
};

// A HashMap with optimal hash!
template<typename T, typename V> struct IdxMap {
    typedef Idx<T> key_type;
    typedef V mapped_type;
    typedef std::vector<V> container;
    typedef IdxMapIterator<key_type, typename container::iterator> iterator;
    typedef IdxMapIterator<key_type, typename container::const_iterator> const_iterator;

    inline IdxMap() = default;
    inline IdxMap(size_t nb, const V& val = V()): map(nb, val) {}

    // initialize the map with the number of element
    inline void assign(size_t nb, const V& val = V()) { map.assign(nb, val); }

    // resize the map
    inline void resize(size_t nb) { map.resize(nb); }

    // accessors
    inline size_t size() const { return map.size(); }
    inline const V& operator[](const Idx<T>& idx) const { return map[idx.val]; }
    inline V& operator[](const Idx<T>& idx) { return map[idx.val]; }

    // iterator getters
    inline iterator begin() { return iterator(0, map.begin()); }
    inline iterator end() { return iterator(map.size(), map.end()); }
    inline const_iterator begin() const { return iterator(0, map.cbegin()); }
    inline const_iterator end() const { return iterator(map.size(), map.cend()); }
    inline const_iterator cbegin() const { return iterator(0, map.cbegin()); }
    inline const_iterator cend() const { return iterator(map.size(), map.cend()); }

    // iterate on const values
    inline boost::iterator_range<typename std::vector<V>::const_iterator>
    values() const { return boost::make_iterator_range(map.cbegin(), map.cend()); }

    // iterate on values
    inline boost::iterator_range<typename std::vector<V>::iterator>
    values() { return boost::make_iterator_range(map.begin(), map.end()); }

private:
    container map;
};

}} // namespace navitia::routing
