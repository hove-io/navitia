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
#include "type/pt_data.h"
#include "type/datetime.h"
#include "routing/raptor_utils.h"

#include <boost/foreach.hpp>
#include <boost/dynamic_bitset.hpp>
namespace navitia { namespace routing {

/** Données statiques qui ne sont pas modifiées pendant le calcul */
struct dataRAPTOR {

    struct Connections {
        struct Connection {
            DateTime duration;
            type::idx_t sp_idx;
        };
        inline const std::vector<Connection>& get_forward(type::idx_t sp) const {
            return forward_connections[sp];
        }
        inline const std::vector<Connection>& get_backward(type::idx_t sp) const {
            return backward_connections[sp];
        }
        void load(const navitia::type::PT_Data &data);

    private:
        std::vector<std::vector<Connection>> forward_connections;
        std::vector<std::vector<Connection>> backward_connections;
    };
    Connections connections;

    struct JppsFromSp {
        struct JppIdxOrder {
            type::idx_t idx;
            type::idx_t jp_idx;
            int order;
        };
        inline const std::vector<JppIdxOrder>& operator[](type::idx_t sp) const {
            return jpps_from_sp[sp];
        }
        void load(const navitia::type::PT_Data &data);
        void filter_jpps(const boost::dynamic_bitset<>& valid_jpps);

    private:
        std::vector<std::vector<JppIdxOrder>> jpps_from_sp;
    };
    JppsFromSp jpps_from_sp;

    // arrival_times (resp. departure_times) are the different arrival
    // (resp. departure) times of each stop time sorted by
    //     lex(jp, jpp, arrival_time (resp. departure_time)).
    //
    // Then, you have:
    //     for all i, st_forward[i]->departure_time == departure_times[i]
    //     for all i, st_backward[i]->arrival_time == arrival_times[i]
    //
    // st_forward and st_backward contains StopTime* from
    // std::vector<StopTime> (in VehicleJourney).  This is safe as
    // pt_data MUST be const for a given dataRAPTOR (else, you'll have
    // much troubles, and not only from here).
    std::vector<DateTime> arrival_times;
    std::vector<DateTime> departure_times;
    std::vector<const type::StopTime*> st_forward;
    std::vector<const type::StopTime*> st_backward;

    std::vector<size_t> first_stop_time;
    std::vector<size_t> nb_trips;
    Labels labels_const;
    Labels labels_const_reverse;
    vector_idx boardings_const;
    std::vector<boost::dynamic_bitset<> > jp_validity_patterns;
    std::vector<boost::dynamic_bitset<> > jp_adapted_validity_pattern;


    dataRAPTOR() {}
    void load(const navitia::type::PT_Data &data);
};

}}

