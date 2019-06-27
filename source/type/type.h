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

#include "type_interfaces.h"
#include "type/time_duration.h"
#include "datetime.h"
#include "rt_level.h"
#include "validity_pattern.h"
#include "timezone_manager.h"
#include "geographical_coord.h"
#include "utils/flat_enum_map.h"
#include "utils/exception.h"
#include "utils/functions.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>

#include <boost/weak_ptr.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/bitset.hpp>
#include "utils/serialization_vector.h"
#include <boost/serialization/export.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/optional.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include "type/fwd_type.h"
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
#include "type/vehicle_journey.h"
#include "type/stop_time.h"

namespace navitia {
namespace type {

std::ostream& operator<<(std::ostream& os, const Mode_e& mode);

/**
 * A meta vj is a shell around some vehicle journeys
 *
 * It has 2 purposes:
 *
 *  - to store the adapted and real time vj
 *
 *  - sometime we have to split a vj.
 *    For example we have to split a vj because of dst (day saving light see gtfs parser for that)
 *    the meta vj can thus make the link between the split vjs
 *    *NOTE*: An IMPORTANT prerequisite is that ALL theoric vj have the same local time
 *            (even if the UTC time is different because of DST)
 *            That prerequisite is very important for calendar association and departure board over period
 *
 *
 */
struct MetaVehicleJourney : public Header, HasMessages {
    const static Type_e type = Type_e::MetaVehicleJourney;
    const TimeZoneHandler* tz_handler = nullptr;

    // impacts not directly on this vj, by example an impact on a line will impact the vj, so we add the impact here
    // because it's not really on the vj
    std::vector<boost::weak_ptr<disruption::Impact>> modified_by;

    /// map of the calendars that nearly match union of the validity pattern
    /// of the theoric vj, key is the calendar name
    std::map<std::string, AssociatedCalendar*> associated_calendars;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);

    FrequencyVehicleJourney* create_frequency_vj(const std::string& uri,
                                                 const std::string& name,
                                                 const RTLevel,
                                                 const ValidityPattern& canceled_vp,
                                                 Route*,
                                                 std::vector<StopTime>,
                                                 PT_Data&);
    DiscreteVehicleJourney* create_discrete_vj(const std::string& uri,
                                               const std::string& name,
                                               const RTLevel,
                                               const ValidityPattern& canceled_vp,
                                               Route*,
                                               std::vector<StopTime>,
                                               PT_Data&);

    void clean_up_useless_vjs(PT_Data&);

    template <typename T>
    void for_all_vjs(T fun) const {
        for (const auto rt_vjs : rtlevel_to_vjs_map) {
            auto& vjs = rt_vjs.second;
            boost::for_each(vjs, [&](const std::unique_ptr<VehicleJourney>& vj) { fun(*vj); });
        }
    }

    const std::vector<std::unique_ptr<VehicleJourney>>& get_vjs_at(RTLevel rt_level) const {
        return rtlevel_to_vjs_map[rt_level];
    }

    const std::vector<std::unique_ptr<VehicleJourney>>& get_base_vj() const {
        return rtlevel_to_vjs_map[RTLevel::Base];
    }
    const std::vector<std::unique_ptr<VehicleJourney>>& get_adapted_vj() const {
        return rtlevel_to_vjs_map[RTLevel::Adapted];
    }
    const std::vector<std::unique_ptr<VehicleJourney>>& get_rt_vj() const {
        return rtlevel_to_vjs_map[RTLevel::RealTime];
    }

    void cancel_vj(RTLevel level,
                   const std::vector<boost::posix_time::time_period>& periods,
                   PT_Data& pt_data,
                   const Route* filtering_route = nullptr);

    VehicleJourney* get_base_vj_circulating_at_date(const boost::gregorian::date& date) const;

    const std::string& get_label() const { return uri; }  // for the moment the label is just the uri

    Indexes get(Type_e type, const PT_Data& data) const;

    void push_unique_impact(const boost::shared_ptr<disruption::Impact>& impact);

    bool is_already_modified_by(const boost::shared_ptr<disruption::Impact>& impact);

private:
    template <typename VJ>
    VJ* impl_create_vj(const std::string& uri,
                       const std::string& name,
                       const RTLevel,
                       const ValidityPattern& canceled_vp,
                       Route*,
                       std::vector<StopTime>,
                       PT_Data&);

    navitia::flat_enum_map<RTLevel, std::vector<std::unique_ptr<VehicleJourney>>> rtlevel_to_vjs_map;
};

struct static_data {
private:
    static static_data* instance;

public:
    static static_data* get();
    // static std::string getListNameByType(Type_e type);
    static boost::posix_time::ptime parse_date_time(const std::string& s);
    static Type_e typeByCaption(const std::string& type_str);
    static std::string captionByType(Type_e type);
    boost::bimap<Type_e, std::string> types_string;
    static Mode_e modeByCaption(const std::string& mode_str);
    boost::bimap<boost::bimaps::multiset_of<Mode_e>, boost::bimaps::set_of<std::string>> modes_string;
};

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

template <typename T>
inline Type_e get_type_e() {
    static_assert(!std::is_same<T, T>::value, "get_type_e unimplemented");
    return Type_e::Unknown;
}
template <>
inline Type_e get_type_e<PhysicalMode>() {
    return Type_e::PhysicalMode;
}
template <>
inline Type_e get_type_e<CommercialMode>() {
    return Type_e::CommercialMode;
}
template <>
inline Type_e get_type_e<Contributor>() {
    return Type_e::Contributor;
}
template <>
inline Type_e get_type_e<Network>() {
    return Type_e::Network;
}
template <>
inline Type_e get_type_e<Route>() {
    return Type_e::Route;
}
template <>
inline Type_e get_type_e<StopArea>() {
    return Type_e::StopArea;
}
template <>
inline Type_e get_type_e<StopPoint>() {
    return Type_e::StopPoint;
}

}  // namespace type

// trait to access the number of elements in the Mode_e enum
template <>
struct enum_size_trait<type::Mode_e> {
    static constexpr typename get_enum_type<type::Mode_e>::type size() { return 5; }
};

}  // namespace navitia
