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

#pragma once
#include "type/type_interfaces.h"
#include "type/fwd_type.h"
#include "type/geographical_coord.h"
#include "type/odt_properties.h"

#include <boost/container/flat_set.hpp>

#include <set>

namespace navitia {
namespace type {

struct StopPoint;
struct StopArea;

struct Route : public Header, Nameable, HasMessages {
    const static Type_e type = Type_e::Route;
    Line* line = nullptr;
    StopArea* destination = nullptr;
    MultiLineString shape;
    std::string direction_type;

    std::vector<DiscreteVehicleJourney*> discrete_vehicle_journey_list;
    std::vector<FrequencyVehicleJourney*> frequency_vehicle_journey_list;
    std::set<Dataset*> dataset_list;
    boost::container::flat_set<StopPoint*> stop_point_list;
    boost::container::flat_set<StopArea*> stop_area_list;

    type::hasOdtProperties get_odt_properties() const;

    template <typename T>
    void for_each_vehicle_journey(const T func) const {
        // call the functor for each vj.
        // if func return false, we stop
        for (auto* vj : discrete_vehicle_journey_list) {
            if (!func(*vj)) {
                return;
            }
        }
        for (auto* vj : frequency_vehicle_journey_list) {
            if (!func(*vj)) {
                return;
            }
        }
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);

    Indexes get(Type_e type, const PT_Data& data) const;
    bool operator<(const Route& other) const;

    std::string get_label() const;
};

}  // namespace type
}  // namespace navitia
