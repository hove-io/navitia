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

#include "type/data.h"
#include "routing.h"
#include "boost/dynamic_bitset.hpp"
#include <boost/heap/d_ary_heap.hpp>

namespace navitia { namespace routing {

struct out_stop_point {
    idx_t stop_point_idx;
    uint32_t duration;
};

struct heap_compare {
    const std::vector<uint32_t> & durations;
    heap_compare(const std::vector<uint32_t> & durations) : durations(durations){}
    bool operator()(idx_t a, idx_t b) const {
        return durations[a] >= durations[b];
    }
};

typedef boost::heap::d_ary_heap<idx_t, boost::heap::arity<4>, boost::heap::compare<heap_compare>, boost::heap::constant_time_size<false>,boost::heap::mutable_<true>  > MyHeap;
struct StaticRaptor : public AbstractRouter {

    const type::PT_Data & data;

    boost::dynamic_bitset<> marked_stop;
    std::vector<uint32_t> best_durations;
    std::vector< std::vector<out_stop_point> > out_edges;
    MyHeap heap;
    std::vector<MyHeap::handle_type> handles;


    /// Calcule les temps minimal pour parcourir une route
    void compute_route_durations();

    void init(idx_t departure_idx);

    void mark(idx_t stop_point_idx);

     StaticRaptor(const type::Data & data) :
        data(data.pt_data),
        marked_stop(this->data.stop_points.size()),
        best_durations(this->data.stop_points.size(), std::numeric_limits<uint32_t>::max()),
        out_edges(this->data.stop_points.size()),
        heap(heap_compare(this->best_durations)),
        handles(this->data.stop_points.size())
    {
        compute_route_durations();
    }

    Path compute(idx_t departure_idx, idx_t arrival_idx, int, int , senscompute sens = partirapres);
};

}
}
