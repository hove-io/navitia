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
#include "type/rt_level.h"
#include "type/datetime.h"
#include "type/vehicle_journey.h"  //required to inline order()
#include "utils/rank.h"

namespace navitia {
namespace type {

struct StopTime {
    static const uint8_t PICK_UP = 0;
    static const uint8_t DROP_OFF = 1;
    static const uint8_t ODT = 2;
    static const uint8_t IS_FREQUENCY = 3;
    static const uint8_t WHEELCHAIR_BOARDING = 4;
    static const uint8_t DATE_TIME_ESTIMATED = 5;
    static const uint8_t SKIPPED_STOP = 6;

    std::bitset<8> properties;
    uint16_t local_traffic_zone = std::numeric_limits<uint16_t>::max();

    /// for non frequency vj departure/arrival are the real departure/arrival
    /// for frequency vj they are relatives to the stoptime's vj's start_time
    uint32_t arrival_time = 0;    ///< seconds since midnight
    uint32_t departure_time = 0;  ///< seconds since midnight

    /// Representing times used in Raptor, taking into account boarding and alighting process duration
    uint32_t boarding_time = 0;   ///< seconds since midnight
    uint32_t alighting_time = 0;  ///< seconds since midnight

    VehicleJourney* vehicle_journey = nullptr;
    StopPoint* stop_point = nullptr;
    boost::shared_ptr<LineString> shape_from_prev;

    StopTime() = default;
    StopTime(uint32_t arr_time, uint32_t dep_time, StopPoint* stop_point)
        : arrival_time{arr_time}, departure_time{dep_time}, stop_point{stop_point} {}
    bool pick_up_allowed() const { return properties[PICK_UP]; }
    bool drop_off_allowed() const { return properties[DROP_OFF]; }
    bool skipped_stop() const { return properties[SKIPPED_STOP]; }
    bool odt() const { return properties[ODT]; }
    bool is_frequency() const { return properties[IS_FREQUENCY]; }
    bool date_time_estimated() const { return properties[DATE_TIME_ESTIMATED]; }

    inline void set_pick_up_allowed(bool value) { properties[PICK_UP] = value; }
    inline void set_drop_off_allowed(bool value) { properties[DROP_OFF] = value; }
    inline void set_skipped_stop(bool value) { properties[SKIPPED_STOP] = value; }
    inline void set_odt(bool value) { properties[ODT] = value; }
    inline void set_is_frequency(bool value) { properties[IS_FREQUENCY] = value; }
    inline void set_date_time_estimated(bool value) { properties[DATE_TIME_ESTIMATED] = value; }
    inline RankStopTime order() const {
        static_assert(std::is_same<decltype(vehicle_journey->stop_time_list), std::vector<StopTime>>::value,
                      "vehicle_journey->stop_time_list must be a std::vector<StopTime>");
        assert(vehicle_journey);
        // as vehicle_journey->stop_time_list is a vector, pointer
        // arithmetic gives us the order of the stop time in the
        // vector.
        return RankStopTime(this - &vehicle_journey->stop_time_list.front());
    }

    StopTime clone() const;

    /// can we start with this stop time (according to clockwise)
    bool valid_begin(bool clockwise) const { return clockwise ? pick_up_allowed() : drop_off_allowed(); }

    /// can we finish with this stop time (according to clockwise)
    bool valid_end(bool clockwise) const { return clockwise ? drop_off_allowed() : pick_up_allowed(); }

    bool is_odt_and_date_time_estimated() const { return (this->odt() && this->date_time_estimated()); }

    /// get the departure (resp arrival for anti clockwise) from the stoptime
    /// base_dt is the base time (from midnight for discrete / from start_time for frequencies)
    DateTime section_end(DateTime base_dt, bool clockwise) const {
        return clockwise ? arrival(base_dt) : departure(base_dt);
    }

    DateTime departure(DateTime base_dt) const { return base_dt + boarding_time; }
    DateTime arrival(DateTime base_dt) const { return base_dt + alighting_time; }

    DateTime base_dt(DateTime dt, bool clockwise) const { return dt - (clockwise ? boarding_time : alighting_time); }

    boost::posix_time::ptime get_arrival_utc(const boost::gregorian::date& circulating_day) const {
        auto timestamp = navitia::to_posix_timestamp(boost::posix_time::ptime{circulating_day})
                         + static_cast<uint64_t>(alighting_time);
        return boost::posix_time::from_time_t(timestamp);
    }

    uint32_t get_boarding_duration() const { return departure_time - boarding_time; }

    uint32_t get_alighting_duration() const { return alighting_time - arrival_time; }

    bool is_valid_day(u_int32_t day, const bool is_arrival, const RTLevel rt_level) const;

    const StopTime* get_base_stop_time() const;

    bool is_in_stop_area(const StopArea* stop_area) const;

    //  Check if a stop time has similar hours of departure/arrival/alighting/boarding_time etc...
    //  We don't want to rely on properties that might change with real time object.
    //  Note : This is usefull in the case of a StopTimeUpdate that owns a partially constructed Stop Time.
    bool is_similar(const StopTime& st) const;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& arrival_time& departure_time& boarding_time& alighting_time& vehicle_journey& stop_point& shape_from_prev&
            properties& local_traffic_zone;
    }
};
}  // namespace type
}  // namespace navitia
