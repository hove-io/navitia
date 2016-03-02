/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

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
#include <bitset>
#include <iostream>
#include "utils/idx_map.h"

namespace navitia {
namespace type {

typedef navitia::idx_t idx_t;
const idx_t invalid_idx = std::numeric_limits<idx_t>::max();

#define ITERATE_NAVITIA_PT_TYPES(FUN)\
    FUN(ValidityPattern, validity_patterns)\
    FUN(Line, lines)\
    FUN(LineGroup, line_groups)\
    FUN(VehicleJourney, vehicle_journeys)\
    FUN(StopPoint, stop_points)\
    FUN(StopArea, stop_areas)\
    FUN(Network, networks)\
    FUN(PhysicalMode, physical_modes)\
    FUN(CommercialMode, commercial_modes)\
    FUN(Company, companies)\
    FUN(Route, routes)\
    FUN(Contributor, contributors)\
    FUN(Calendar, calendars) \
    FUN(Dataset, datasets)

enum class Type_e {
    ValidityPattern                 = 0,
    Line                            = 1,
    JourneyPattern                  = 2,
    VehicleJourney                  = 3,
    StopPoint                       = 4,
    StopArea                        = 5,
    Network                         = 6,
    PhysicalMode                    = 7,
    CommercialMode                  = 8,
    Connection                      = 9,
    JourneyPatternPoint             = 10,
    Company                         = 11,
    Route                           = 12,
    POI                             = 13,
    StopPointConnection             = 15,
    Contributor                     = 16,

    // Objets spéciaux qui ne font pas partie du référentiel TC
    StopTime                        = 17,
    Address                         = 18,
    Coord                           = 19,
    Unknown                         = 20,
    Way                             = 21,
    Admin                           = 22,
    POIType                         = 23,
    Calendar                        = 24,
    LineGroup                       = 25,
    MetaVehicleJourney              = 26,
    Impact                          = 27,
    Dataset                         = 28,
    size                            = 29
};

enum class Mode_e {
    Walking = 0,    // Marche à pied
    Bike = 1,       // Vélo
    Car = 2,        // Voiture
    Bss = 3         // Vls
    //Note: if a new transportation mode is added, don't forget to update the associated enum_size_trait<type::Mode_e>
};

inline std::ostream& operator<<(std::ostream& os, const Mode_e& mode) {
    switch (mode) {
    case Mode_e::Walking: return os << "walking";
    case Mode_e::Bike: return os << "bike";
    case Mode_e::Car: return os << "car";
    case Mode_e::Bss: return os << "bss";
    default: return os << "[unknown mode]";
    }
}

enum class OdtLevel_e {
    scheduled = 0,
    with_stops = 1,
    zonal = 2,
    all = 3
};


struct Nameable {
    std::string name;
    bool visible = true;
};

struct PT_Data;

struct Header {
    idx_t idx = invalid_idx; // Index of the object in the main structure
    std::string uri; // unique indentifier of the object
    std::vector<idx_t> get(Type_e, const PT_Data &) const {return std::vector<idx_t>();}
};

typedef std::bitset<10> Properties;
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

    bool wheelchair_boarding() {return _properties[WHEELCHAIR_BOARDING];}
    bool wheelchair_boarding() const {return _properties[WHEELCHAIR_BOARDING];}
    bool sheltered() {return _properties[SHELTERED];}
    bool sheltered() const {return _properties[SHELTERED];}
    bool elevator() {return _properties[ELEVATOR];}
    bool elevator() const {return _properties[ELEVATOR];}
    bool escalator() {return _properties[ESCALATOR];}
    bool escalator() const {return _properties[ESCALATOR];}
    bool bike_accepted() {return _properties[BIKE_ACCEPTED];}
    bool bike_accepted() const {return _properties[BIKE_ACCEPTED];}
    bool bike_depot() {return _properties[BIKE_DEPOT];}
    bool bike_depot() const {return _properties[BIKE_DEPOT];}
    bool visual_announcement() {return _properties[VISUAL_ANNOUNCEMENT];}
    bool visual_announcement() const {return _properties[VISUAL_ANNOUNCEMENT];}
    bool audible_announcement() {return _properties[AUDIBLE_ANNOUNVEMENT];}
    bool audible_announcement() const {return _properties[AUDIBLE_ANNOUNVEMENT];}
    bool appropriate_escort() {return _properties[APPOPRIATE_ESCORT];}
    bool appropriate_escort() const {return _properties[APPOPRIATE_ESCORT];}
    bool appropriate_signage() {return _properties[APPOPRIATE_SIGNAGE];}
    bool appropriate_signage() const {return _properties[APPOPRIATE_SIGNAGE];}

    bool accessible(const Properties &required_properties) const{
        auto mismatched = required_properties & ~_properties;
        return mismatched.none();
    }
    bool accessible(const Properties &required_properties) {
        auto mismatched = required_properties & ~_properties;
        return mismatched.none();
    }

    void set_property(uint8_t property) { _properties.set(property, true); }
    void set_properties(const Properties &other) { this->_properties = other; }

    Properties properties() const { return this->_properties; }
    void unset_property(uint8_t property) { _properties.set(property, false); }
    bool property(uint8_t property) const { return _properties[property]; }
    idx_t to_ulog() { return _properties.to_ulong(); }

    Properties _properties;
};

typedef std::bitset<8> VehicleProperties;
struct hasVehicleProperties {
    static const uint8_t WHEELCHAIR_ACCESSIBLE = 0;
    static const uint8_t BIKE_ACCEPTED = 1;
    static const uint8_t AIR_CONDITIONED = 2;
    static const uint8_t VISUAL_ANNOUNCEMENT = 3;
    static const uint8_t AUDIBLE_ANNOUNCEMENT = 4;
    static const uint8_t APPOPRIATE_ESCORT = 5;
    static const uint8_t APPOPRIATE_SIGNAGE = 6;
    static const uint8_t SCHOOL_VEHICLE = 7;

    bool wheelchair_accessible() {return _vehicle_properties[WHEELCHAIR_ACCESSIBLE];}
    bool wheelchair_accessible() const {return _vehicle_properties[WHEELCHAIR_ACCESSIBLE];}
    bool bike_accepted() {return _vehicle_properties[BIKE_ACCEPTED];}
    bool bike_accepted() const {return _vehicle_properties[BIKE_ACCEPTED];}
    bool air_conditioned() {return _vehicle_properties[AIR_CONDITIONED];}
    bool air_conditioned() const {return _vehicle_properties[AIR_CONDITIONED];}
    bool visual_announcement() {return _vehicle_properties[VISUAL_ANNOUNCEMENT];}
    bool visual_announcement() const {return _vehicle_properties[VISUAL_ANNOUNCEMENT];}
    bool audible_announcement() {return _vehicle_properties[AUDIBLE_ANNOUNCEMENT];}
    bool audible_announcement() const {return _vehicle_properties[AUDIBLE_ANNOUNCEMENT];}
    bool appropriate_escort() {return _vehicle_properties[APPOPRIATE_ESCORT];}
    bool appropriate_escort() const {return _vehicle_properties[APPOPRIATE_ESCORT];}
    bool appropriate_signage() {return _vehicle_properties[APPOPRIATE_SIGNAGE];}
    bool appropriate_signage() const {return _vehicle_properties[APPOPRIATE_SIGNAGE];}
    bool school_vehicle() {return _vehicle_properties[SCHOOL_VEHICLE];}
    bool school_vehicle() const {return _vehicle_properties[SCHOOL_VEHICLE];}

    bool accessible(const VehicleProperties &required_vehicles) const{
        auto mismatched = required_vehicles & ~_vehicle_properties;
        return mismatched.none();
    }
    bool accessible(const VehicleProperties &required_vehicles) {
        auto mismatched = required_vehicles & ~_vehicle_properties;
        return mismatched.none();
    }

    void set_vehicle(uint8_t vehicle) { _vehicle_properties.set(vehicle, true); }
    void set_vehicles(const VehicleProperties &other) { this->_vehicle_properties = other; }
    VehicleProperties vehicles() const { return this->_vehicle_properties; }

    void unset_vehicle(uint8_t vehicle) { _vehicle_properties.set(vehicle, false); }
    bool vehicle(uint8_t vehicle) const { return _vehicle_properties[vehicle]; }
    idx_t to_ulog() { return _vehicle_properties.to_ulong(); }

    VehicleProperties _vehicle_properties;
};
}
}
