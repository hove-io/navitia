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

#include "raptor_solution_filter.h"

namespace navitia { namespace routing {

std::vector<Path> filter_journeys(const std::vector<Path>& paths) {
    std::vector<Path> res;
    for (const auto& p: paths) {
        if (is_valid_journey(p)) {
            res.push_back(p);
        }
    }
    return res;
}

bool is_valid_journey(const Path& p) {

    //we don't want journeys with 2 zone ODT in transfers
    const type::VehicleJourney* previous_vj = nullptr;
    for (const auto& item: p.items) {
        auto vj = item.get_vj();

        if (! vj) { continue; }

        if (previous_vj) {
            if (previous_vj->is_zonal_odt() && vj->is_zonal_odt()) {
                return false;
            }
        }
        previous_vj = vj;
    }
    return true;
}

}}
