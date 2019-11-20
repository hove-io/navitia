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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "type/type_interfaces.h"
#include "type/time_duration.h"
#include "type/validity_pattern.h"
#include "type/timezone_manager.h"
#include "type/geographical_coord.h"
#include "type/stop_point.h"
#include "type/connection.h"
#include "type/calendar.h"
#include "type/stop_area.h"
#include "type/network.h"
#include "type/contributor.h"
#include "type/dataset.h"
#include "type/company.h"
#include "type/commercial_mode.h"
#include "type/physical_mode.h"
#include "type/line.h"
#include "type/route.h"
#include "type/meta_vehicle_journey.h"
#include "type/vehicle_journey.h"
#include "type/stop_time.h"
#include "type/static_data.h"

#include <vector>

namespace navitia {
namespace type {

/**
 * fallback params
 */
struct StreetNetworkParams {
    Mode_e mode = Mode_e::Walking;
    idx_t offset = 0;
    float speed_factor = 1;
    Type_e type_filter =
        Type_e::Unknown;  // filtre sur le départ/arrivée : exemple les arrêts les plus proches à une site type
    std::string uri_filter;
    float radius_filter = 150;
    void set_filter(const std::string& param_uri);

    navitia::time_duration max_duration = 1_s;
    bool enable_direct_path = true;
};
/**
  Accessibility management
  */
struct AccessibiliteParams {
    Properties properties;                 // Accessibility StopPoint, Connection, ...
    VehicleProperties vehicle_properties;  // Accessibility VehicleJourney

    AccessibiliteParams() {}

    bool operator<(const AccessibiliteParams& other) const {
        if (properties.to_ulong() != other.properties.to_ulong()) {
            return properties.to_ulong() < other.properties.to_ulong();
        } else if (vehicle_properties.to_ulong() != other.vehicle_properties.to_ulong()) {
            return vehicle_properties.to_ulong() < other.vehicle_properties.to_ulong();
        }
        return false;
    }
};

/** Type pour gérer le polymorphisme en entrée de l'API
 *
 * Les objets on un identifiant universel de type stop_area:872124
 * Ces identifiants ne devraient pas être générés par le média
 * C'est toujours NAViTiA qui le génère pour être repris tel quel par le média
 */
struct EntryPoint {
    Type_e type;      //< Le type de l'objet
    std::string uri;  //< Le code externe de l'objet
    int house_number;
    int access_duration;
    GeographicalCoord coordinates;             // < coordonnées du point d'entrée
    StreetNetworkParams streetnetwork_params;  // < paramètres de rabatement du point d'entrée

    /// Construit le type à partir d'une chaîne
    EntryPoint(const Type_e type, const std::string& uri);
    EntryPoint(const Type_e type, const std::string& uri, int access_duration);

    EntryPoint() : type(Type_e::Unknown), house_number(-1), access_duration(0) {}
    bool set_mode(const std::string& mode) {
        if (mode == "walking") {
            streetnetwork_params.mode = Mode_e::Walking;
        } else if (mode == "bike") {
            streetnetwork_params.mode = Mode_e::Bike;
        } else if (mode == "bss") {
            streetnetwork_params.mode = Mode_e::Bss;
        } else if (mode == "car") {
            streetnetwork_params.mode = Mode_e::Car;
        } else if ((mode == "ridesharing") || (mode == "car_no_park")) {
            streetnetwork_params.mode = Mode_e::CarNoPark;
        } else {
            return false;
        }
        return true;
    }

    static bool is_coord(const std::string&);
};

using EntryPoints = std::vector<EntryPoint>;

template <typename T>
std::string get_admin_name(const T* v) {
    std::string admin_name = "";
    for (auto admin : v->admin_list) {
        if (admin->level == 8) {
            admin_name += " (" + admin->name + ")";
        }
    }
    return admin_name;
}

}  // namespace type

}  // namespace navitia
