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

#include "adminref.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/geometry.hpp>
#include <utility>

namespace navitia {
namespace georef {

Admin::Admin(idx_t idx,
             const std::string& uri,
             const std::string& name,
             int level,
             std::string insee,
             std::string label,
             const nt::GeographicalCoord& coord,
             Postal_codes postal_codes)
    : Header(idx, uri),
      Nameable(name),
      level(level),
      insee(std::move(insee)),
      label(std::move(label)),
      coord(coord),
      postal_codes(std::move(postal_codes)) {}

std::string Admin::get_range_postal_codes() {
    std::string post_code;
    // the label is the range of the postcode
    // ex: Tours (37000;37100;37200) -> Tours (37000-37200)
    //     Cosne-d'Allier (03430)    -> Cosne-d'Allier (03430)
    if (!this->postal_codes.empty()) {
        try {
            std::vector<int> postal_codes_int;
            for (const std::string& str_post_code : this->postal_codes) {
                postal_codes_int.push_back(boost::lexical_cast<int>(str_post_code));
            }
            auto result = std::minmax_element(postal_codes_int.begin(), postal_codes_int.end());
            if (*result.first == *result.second) {
                post_code = this->postal_codes[result.first - postal_codes_int.begin()];
            } else {
                post_code = this->postal_codes[result.first - postal_codes_int.begin()] + "-"
                            + this->postal_codes[result.second - postal_codes_int.begin()];
            }
        } catch (const boost::bad_lexical_cast&) {
            post_code = this->postal_codes_to_string();
        }
    }
    return post_code;
}

std::string Admin::postal_codes_to_string() const {
    return boost::algorithm::join(this->postal_codes, ";");
}

AdminRtree build_admins_tree(const std::vector<Admin*> admins) {
    AdminRtree admins_tree;
    double min[2] = {0., 0.};
    double max[2] = {0., 0.};
    for (auto* admin : admins) {
        if (!admin->boundary.empty()) {
            boost::geometry::model::box<nt::GeographicalCoord> box;
            boost::geometry::envelope(admin->boundary, box);
            min[0] = box.min_corner().lon();
            min[1] = box.min_corner().lat();
            max[0] = box.max_corner().lon();
            max[1] = box.max_corner().lat();
        }
        admins_tree.Insert(min, max, admin);
    }
    return admins_tree;
}
}  // namespace georef
}  // namespace navitia
