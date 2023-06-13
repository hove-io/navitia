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

#include "routing/labels.h"
#include "utils/boost_patched/range/combine.hpp"

namespace navitia {
namespace routing {

Labels::Labels() = default;
Labels::Labels(const std::vector<type::StopPoint*> stop_points) : labels(stop_points) {}

Labels::Labels(const Map& dt_pts, const Map& dt_transfers, const Map& walkings, const Map& walking_transfers) {
    const auto& zip = boost::combine(dt_pts, dt_transfers.values(), walkings.values(), walking_transfers.values());
    labels.resize(zip.size());
    for (const auto z : zip) {
        const auto sp_idx = z.get<0>().first;
        labels[sp_idx] = Label{z.get<0>().second, z.get<1>(), z.get<2>(), z.get<3>()};
    }
}

std::array<Labels::Map, 4> Labels::inrow_labels() {
    std::array<Map, 4> row_labels;

    for (auto& row_label : row_labels)
        row_label.resize(labels.size());

    for (const auto l : labels) {
        const auto& sp_idx = l.first;
        const auto& label = l.second;

        row_labels[0][sp_idx] = label.dt_pt;
        row_labels[1][sp_idx] = label.dt_transfer;
        row_labels[2][sp_idx] = label.walking_duration_pt;
        row_labels[3][sp_idx] = label.walking_duration_transfer;
    }

    return row_labels;
}

void Labels::fill_values(DateTime pts, DateTime transfert, DateTime walking, DateTime walking_transfert) {
    Label default_label{pts, transfert, walking, walking_transfert};
    boost::fill(labels.values(), default_label);
}

void Labels::init(const std::vector<type::StopPoint*>& stops, DateTime val) {
    Label init_label{val, val, DateTimeUtils::not_valid, DateTimeUtils::not_valid};
    labels.assign(stops, init_label);
}

}  // namespace routing
}  // namespace navitia
