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

#include "routing/labels.h"
#include "routing/journey_pattern_container.h"
#include "utils/boost_patched/range/combine.hpp"

namespace navitia {
namespace routing {

Labels::Labels() {}
Labels::Labels(const std::vector<JourneyPatternPoint> jpps) : labels(jpps) {}

void Labels::fill_values(DateTime pts, DateTime transfert, DateTime walking, DateTime walking_transfert) {
    Label default_label{pts, transfert, walking, walking_transfert};
    boost::fill(labels.values(), default_label);
}

void Labels::init(const std::vector<JourneyPatternPoint>& jpps, DateTime val) {
    Label init_label{val, val, DateTimeUtils::not_valid, DateTimeUtils::not_valid};
    labels.assign(jpps, init_label);
}

}  // namespace routing
}  // namespace navitia
