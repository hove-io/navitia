/* Copyright © 2001-2021, Canal TP and/or its affiliates. All rights reserved.

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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "type/fwd_type.h"
#include "type/stop_point.h"
#include "utils/idx_map.h"
#include "routing/raptor_utils.h"

#include <vector>
#include <array>

namespace navitia {
namespace routing {

struct Label {
    // All these vectors are indexed by sp_idx
    //
    // dt_pt stores the earliest arrival time to stop_point.
    // More precisely, at time dt_pt, we just alighted from
    // a vehicle going through stop_point, but we need to do a transfer
    // before being able to board a new vehicle.
    DateTime dt_pt;

    // dt_transfers[stop_point] stores the earliest time at which we are able
    // to board a vehicle leaving from stop_point (i.e. a transfer to stop_point has been done).
    DateTime dt_transfer;

    // walking_duration_pts[stop_point] stores the total walking duration (fallback + transfers)  of a
    // journey that alight to stop_point at DateTime dt_pt
    DateTime walking_duration_pt;

    // waling_duration_transfers[stop_point] stores the total walking duration (fallback + transfers) of a
    // journey that allows to board a vehicle at stop_point at DateTime transfers_pts[stop_point]
    DateTime walking_duration_transfer;
};

struct Labels {
    using LabelsMap = IdxMap<JourneyPatternPoint, Label>;

    Labels();
    Labels(const std::vector<JourneyPatternPoint> jpps);

    // initialize the structure according to the number of jpp
    void init_inf(const std::vector<JourneyPatternPoint>& jpps) { init(jpps, DateTimeUtils::inf); }
    void init_min(const std::vector<JourneyPatternPoint>& jpps) { init(jpps, DateTimeUtils::min); }

    const Label& operator[](JppIdx jpp_idx) const { return labels[jpp_idx]; }
    Label& operator[](JppIdx jpp_idx) { return labels[jpp_idx]; }

    LabelsMap::range values() { return labels.values(); }

    void fill_values(DateTime pts, DateTime transfert, DateTime walking, DateTime walking_transfert);

    void init(const std::vector<JourneyPatternPoint>& jpps, DateTime val);

private:
    LabelsMap labels;
};

}  // namespace routing
}  // namespace navitia
