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
#include "utils/flat_enum_map.h"
#include "utils/idx_map.h"
#include "type/fwd_type.h"

#include <boost/container/flat_set.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <bitset>
#include <iostream>
#include <utility>

namespace navitia {
namespace type {

using idx_t = navitia::idx_t;
const idx_t invalid_idx = std::numeric_limits<idx_t>::max();

#define ITERATE_NAVITIA_PT_TYPES(FUN)       \
    FUN(ValidityPattern, validity_patterns) \
    FUN(Line, lines)                        \
    FUN(LineGroup, line_groups)             \
    FUN(VehicleJourney, vehicle_journeys)   \
    FUN(StopPoint, stop_points)             \
    FUN(StopArea, stop_areas)               \
    FUN(Network, networks)                  \
    FUN(PhysicalMode, physical_modes)       \
    FUN(CommercialMode, commercial_modes)   \
    FUN(Company, companies)                 \
    FUN(Route, routes)                      \
    FUN(Contributor, contributors)          \
    FUN(Calendar, calendars)                \
    FUN(Dataset, datasets)

enum class Type_e {
    ValidityPattern = 0,
    Line = 1,
    JourneyPattern = 2,
    VehicleJourney = 3,
    StopPoint = 4,
    StopArea = 5,
    Network = 6,
    PhysicalMode = 7,
    CommercialMode = 8,
    Connection = 9,
    JourneyPatternPoint = 10,
    Company = 11,
    Route = 12,
    POI = 13,
    StopPointConnection = 15,
    Contributor = 16,

    // Objets spéciaux qui ne font pas partie du référentiel TC
    StopTime = 17,
    Address = 18,
    Coord = 19,
    Unknown = 20,
    Way = 21,
    Admin = 22,
    POIType = 23,
    Calendar = 24,
    LineGroup = 25,
    MetaVehicleJourney = 26,
    Impact = 27,
    Dataset = 28,
    size = 29,
    AccessPoint = 30,
    PathWay = 31
};

using WeekPattern = std::bitset<7>;

enum class OdtLevel_e { scheduled = 0, with_stops = 1, zonal = 2, all = 3 };

struct Nameable {
    std::string name;
    bool visible = true;

    Nameable() = default;
    Nameable(std::string name) : name(std::move(name)) {}
};

class PT_Data;

using Indexes = boost::container::flat_set<idx_t>;

inline Indexes make_indexes(std::initializer_list<idx_t> l) {
    Indexes indexes;
    indexes.reserve(l.size());
    for (const auto& v : l) {
        indexes.insert(v);
    }
    return indexes;
}

struct Header {
    idx_t idx = invalid_idx;  // Index of the object in the main structure
    std::string uri;          // unique indentifier of the object

    Header() = default;
    Header(idx_t idx, std::string uri) : idx(idx), uri(std::move(uri)) {}
    Indexes get(Type_e, const PT_Data&) const { return Indexes{}; }
};

using Properties = std::bitset<10>;
struct hasProperties {
    static const uint8_t WHEELCHAIR_BOARDING = 0;
    static const uint8_t SHELTERED = 1;
    static const uint8_t ELEVATOR = 2;
    static const uint8_t ESCALATOR = 3;
    static const uint8_t BIKE_ACCEPTED = 4;
    static const uint8_t BIKE_DEPOT = 5;
    static const uint8_t VISUAL_ANNOUNCEMENT = 6;
    static const uint8_t AUDIBLE_ANNOUNVEMENT = 7;
    static const uint8_t APPOPRIATE_ESCORT = 8;
    static const uint8_t APPOPRIATE_SIGNAGE = 9;

    bool wheelchair_boarding() { return _properties[WHEELCHAIR_BOARDING]; }
    bool wheelchair_boarding() const { return _properties[WHEELCHAIR_BOARDING]; }
    bool sheltered() { return _properties[SHELTERED]; }
    bool sheltered() const { return _properties[SHELTERED]; }
    bool elevator() { return _properties[ELEVATOR]; }
    bool elevator() const { return _properties[ELEVATOR]; }
    bool escalator() { return _properties[ESCALATOR]; }
    bool escalator() const { return _properties[ESCALATOR]; }
    bool bike_accepted() { return _properties[BIKE_ACCEPTED]; }
    bool bike_accepted() const { return _properties[BIKE_ACCEPTED]; }
    bool bike_depot() { return _properties[BIKE_DEPOT]; }
    bool bike_depot() const { return _properties[BIKE_DEPOT]; }
    bool visual_announcement() { return _properties[VISUAL_ANNOUNCEMENT]; }
    bool visual_announcement() const { return _properties[VISUAL_ANNOUNCEMENT]; }
    bool audible_announcement() { return _properties[AUDIBLE_ANNOUNVEMENT]; }
    bool audible_announcement() const { return _properties[AUDIBLE_ANNOUNVEMENT]; }
    bool appropriate_escort() { return _properties[APPOPRIATE_ESCORT]; }
    bool appropriate_escort() const { return _properties[APPOPRIATE_ESCORT]; }
    bool appropriate_signage() { return _properties[APPOPRIATE_SIGNAGE]; }
    bool appropriate_signage() const { return _properties[APPOPRIATE_SIGNAGE]; }

    bool accessible(const Properties& required_properties) const {
        auto mismatched = required_properties & ~_properties;
        return mismatched.none();
    }
    bool accessible(const Properties& required_properties) {
        auto mismatched = required_properties & ~_properties;
        return mismatched.none();
    }

    void set_property(uint8_t property) { _properties.set(property, true); }
    void set_properties(const Properties& other) { this->_properties = other; }

    Properties properties() const { return this->_properties; }
    void unset_property(uint8_t property) { _properties.set(property, false); }
    bool property(uint8_t property) const { return _properties[property]; }
    idx_t to_ulog() { return _properties.to_ulong(); }

    Properties _properties;
};

using VehicleProperties = std::bitset<8>;
struct hasVehicleProperties {
    static const uint8_t WHEELCHAIR_ACCESSIBLE = 0;
    static const uint8_t BIKE_ACCEPTED = 1;
    static const uint8_t AIR_CONDITIONED = 2;
    static const uint8_t VISUAL_ANNOUNCEMENT = 3;
    static const uint8_t AUDIBLE_ANNOUNCEMENT = 4;
    static const uint8_t APPOPRIATE_ESCORT = 5;
    static const uint8_t APPOPRIATE_SIGNAGE = 6;
    static const uint8_t SCHOOL_VEHICLE = 7;

    bool wheelchair_accessible() { return _vehicle_properties[WHEELCHAIR_ACCESSIBLE]; }
    bool wheelchair_accessible() const { return _vehicle_properties[WHEELCHAIR_ACCESSIBLE]; }
    bool bike_accepted() { return _vehicle_properties[BIKE_ACCEPTED]; }
    bool bike_accepted() const { return _vehicle_properties[BIKE_ACCEPTED]; }
    bool air_conditioned() { return _vehicle_properties[AIR_CONDITIONED]; }
    bool air_conditioned() const { return _vehicle_properties[AIR_CONDITIONED]; }
    bool visual_announcement() { return _vehicle_properties[VISUAL_ANNOUNCEMENT]; }
    bool visual_announcement() const { return _vehicle_properties[VISUAL_ANNOUNCEMENT]; }
    bool audible_announcement() { return _vehicle_properties[AUDIBLE_ANNOUNCEMENT]; }
    bool audible_announcement() const { return _vehicle_properties[AUDIBLE_ANNOUNCEMENT]; }
    bool appropriate_escort() { return _vehicle_properties[APPOPRIATE_ESCORT]; }
    bool appropriate_escort() const { return _vehicle_properties[APPOPRIATE_ESCORT]; }
    bool appropriate_signage() { return _vehicle_properties[APPOPRIATE_SIGNAGE]; }
    bool appropriate_signage() const { return _vehicle_properties[APPOPRIATE_SIGNAGE]; }
    bool school_vehicle() { return _vehicle_properties[SCHOOL_VEHICLE]; }
    bool school_vehicle() const { return _vehicle_properties[SCHOOL_VEHICLE]; }

    bool accessible(const VehicleProperties& required_vehicles) const {
        auto mismatched = required_vehicles & ~_vehicle_properties;
        return mismatched.none();
    }
    bool accessible(const VehicleProperties& required_vehicles) {
        auto mismatched = required_vehicles & ~_vehicle_properties;
        return mismatched.none();
    }

    void set_vehicle(uint8_t vehicle) { _vehicle_properties.set(vehicle, true); }
    void set_vehicles(const VehicleProperties& other) { this->_vehicle_properties = other; }
    VehicleProperties vehicles() const { return this->_vehicle_properties; }

    void unset_vehicle(uint8_t vehicle) { _vehicle_properties.set(vehicle, false); }
    bool vehicle(uint8_t vehicle) const { return _vehicle_properties[vehicle]; }
    idx_t to_ulog() { return _vehicle_properties.to_ulong(); }

    VehicleProperties _vehicle_properties;
};

namespace disruption {
struct Impact;
}
struct Line;

struct HasMessages {
protected:
    std::vector<boost::weak_ptr<disruption::Impact>> impacts;

public:
    void add_impact(const boost::shared_ptr<disruption::Impact>& i) { impacts.emplace_back(i); }

    std::vector<boost::shared_ptr<disruption::Impact>> get_applicable_messages(
        const boost::posix_time::ptime& current_time,
        const boost::posix_time::time_period& action_period) const;

    bool has_applicable_message(const boost::posix_time::ptime& current_time,
                                const boost::posix_time::time_period& action_period,
                                const std::vector<disruption::ActiveStatus>& filter_status = {},
                                const Line* line = nullptr) const;

    bool has_publishable_message(const boost::posix_time::ptime& current_time) const;

    std::vector<boost::shared_ptr<disruption::Impact>> get_publishable_messages(
        const boost::posix_time::ptime& current_time) const;

    std::vector<boost::shared_ptr<disruption::Impact>> get_impacts() const;

    void remove_impact(const boost::shared_ptr<disruption::Impact>& impact) {
        auto it = std::find_if(impacts.begin(), impacts.end(),
                               [&impact](const boost::weak_ptr<disruption::Impact>& i) { return i.lock() == impact; });
        if (it != impacts.end()) {
            impacts.erase(it);
        }
    }

    void clean_weak_impacts();
};

enum class Mode_e {
    Walking = 0,    // Marche à pied
    Bike = 1,       // Vélo
    Car = 2,        // Voiture
    Bss = 3,        // Vls
    CarNoPark = 4,  // used for ridesharing typicaly
    // Note: if a new transportation mode is added, don't forget to update the associated enum_size_trait<type::Mode_e>
};

inline std::ostream& operator<<(std::ostream& os, const Mode_e& mode) {
    switch (mode) {
        case Mode_e::Walking:
            return os << "walking";
        case Mode_e::Bike:
            return os << "bike";
        case Mode_e::Car:
            return os << "car";
        case Mode_e::Bss:
            return os << "bss";
        case Mode_e::CarNoPark:
            return os << "car_no_park";
        default:
            return os << "[unknown mode]";
    }
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
template <>
inline Type_e get_type_e<Line>() {
    return Type_e::Line;
}
template <>
inline Type_e get_type_e<AccessPoint>() {
    return Type_e::AccessPoint;
}

}  // namespace type
// trait to access the number of elements in the Mode_e enum
template <>
struct enum_size_trait<type::Mode_e> {
    static constexpr typename get_enum_type<type::Mode_e>::type size() { return 5; }
};

}  // namespace navitia
