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

#include "raptor.h"
#include "utils/multi_obj_pool.h"

namespace navitia { namespace routing {

struct Journey {
    struct Section {
        Section() = default;
        Section(const type::StopTime& in,
                const DateTime in_dt,
                const type::StopTime& out,
                const DateTime out_dt):
            get_in_st(&in), get_in_dt(in_dt), get_out_st(&out), get_out_dt(out_dt)
        {}
        const type::StopTime* get_in_st = nullptr;
        DateTime get_in_dt = 0;
        const type::StopTime* get_out_st = nullptr;
        DateTime get_out_dt = 0;
    };

    bool better_on_dt(const Journey& that, bool request_clockwise) const;
    bool better_on_transfer(const Journey& that, bool) const;
    bool better_on_sn(const Journey& that, bool) const;
    friend std::ostream& operator<<(std::ostream& os, const Journey& j);

    std::vector<Section> sections;// the pt sections, with transfer between them
    navitia::time_duration sn_dur = 0_s;// street network duration
    navitia::time_duration transfer_dur = 0_s;// walking duration during transfer
    navitia::time_duration min_waiting_dur = 0_s;// minimal waiting duration on every transfers
    DateTime departure_dt = 0;// the departure dt of the journey, including sn
    DateTime arrival_dt = 0;// the arrival dt of the journey, including sn
    uint8_t nb_vj_extentions = 0;// number of vehicle journey extentions (I love useless comments!)
};

// this structure compare 2 solutions.  It chooses which solutions
// will be kept at the end (only non dominated solutions will be kept
// by the pool).
struct Dominates {
    bool request_clockwise;
    Dominates(bool rc): request_clockwise(rc) {}
    bool operator()(const Journey& lhs, const Journey& rhs) const {
        return lhs.better_on_dt(rhs, request_clockwise)
            && lhs.better_on_transfer(rhs, request_clockwise)
            && lhs.better_on_sn(rhs, request_clockwise);
    }
};

typedef ParetoFront<Journey, Dominates> Solutions;

// deps (resp. arrs) are departure (resp. arrival) stop points and
// durations (not clockwise dependent).
void read_solutions(const RAPTOR& raptor,
                    Solutions& solutions, //all raptor solutions, modified by side effects
                    const bool clockwise,
                    const DateTime& departure_datetime,
                    const routing::map_stop_point_duration& deps,
                    const routing::map_stop_point_duration& arrs,
                    const type::RTLevel rt_level,
                    const type::AccessibiliteParams& accessibilite_params,
                    const navitia::time_duration& transfer_penalty,
                    const StartingPointSndPhase& end_point);

Path make_path(const Journey& journey, const type::Data& data);

}} // namespace navitia::routing
