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
#include "static_data.h"

#include "utils/exception.h"

#include <boost/assign.hpp>

namespace navitia {
namespace type {

static_data* static_data::instance = nullptr;

static_data* static_data::get() {
    if (instance == nullptr) {
        auto* temp = new static_data();

        boost::assign::insert(temp->types_string)(Type_e::ValidityPattern, "validity_pattern")(Type_e::Line, "line")(
            Type_e::LineGroup, "line_group")(Type_e::JourneyPattern, "journey_pattern")(
            Type_e::VehicleJourney, "vehicle_journey")(Type_e::StopPoint, "stop_point")(Type_e::StopArea, "stop_area")(
            Type_e::Network, "network")(Type_e::PhysicalMode, "physical_mode")(
            Type_e::CommercialMode, "commercial_mode")(Type_e::Connection, "connection")(
            Type_e::JourneyPatternPoint, "journey_pattern_point")(Type_e::Company, "company")(Type_e::Route, "route")(
            Type_e::POI, "poi")(Type_e::POIType, "poi_type")(Type_e::Contributor, "contributor")(
            Type_e::Calendar, "calendar")(Type_e::MetaVehicleJourney, "trip")(Type_e::Impact, "disruption")(
            Type_e::Dataset, "dataset")(Type_e::AccessPoint, "access_point");

        boost::assign::insert(temp->modes_string)(Mode_e::Walking, "walking")(Mode_e::Bike, "bike")(Mode_e::Car, "car")(
            Mode_e::Bss, "bss")(Mode_e::CarNoPark, "ridesharing")(Mode_e::CarNoPark, "car_no_park")(Mode_e::CarNoPark,
                                                                                                    "taxi");
        instance = temp;
    }
    return instance;
}

Type_e static_data::typeByCaption(const std::string& type_str) {
    const auto it_type = instance->types_string.right.find(type_str);
    if (it_type == instance->types_string.right.end()) {
        throw navitia::recoverable_exception("The type " + type_str + " is not managed by kraken");
    }
    return it_type->second;
}

std::string static_data::captionByType(Type_e type) {
    return instance->types_string.left.at(type);
}

Mode_e static_data::modeByCaption(const std::string& mode_str) {
    const auto it_mode = instance->modes_string.right.find(mode_str);
    if (it_mode == instance->modes_string.right.end()) {
        throw navitia::recoverable_exception("The mode " + mode_str + " is not managed by kraken");
    }
    return it_mode->second;
}

}  // namespace type

}  // namespace navitia
