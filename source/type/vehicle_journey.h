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
#include "type/rt_level.h"
#include "validity_pattern.h"

#include <boost/serialization/split_member.hpp>

#include <set>

namespace navitia {
namespace type {

// TODO ODT NTFSv0.3: remove that when we stop to support NTFSv0.1
enum class VehicleJourneyType {
    regular = 0,                    // ligne régulière
    virtual_with_stop_time = 1,     // TAD virtuel avec horaires
    virtual_without_stop_time = 2,  // TAD virtuel sans horaires
    stop_point_to_stop_point = 3,   // TAD rabattement arrêt à arrêt
    adress_to_stop_point = 4,       // TAD rabattement adresse à arrêt
    odt_point_to_point = 5          // TAD point à point (Commune à Commune)
};

/**
 * A VehicleJourney is an abstract class with 2 subclasses
 *
 *  - DiscreteVehicleJourney
 * The 'classic' VJ, with expanded stop times
 *
 *  - FrequencyVehicleJourney
 * A frequency VJ, with a start, an end and frequency (headway)
 *
 * The Route owns 2 differents list for the VJs
 */
struct VehicleJourney : public Header, Nameable, hasVehicleProperties {
    const static Type_e type = Type_e::VehicleJourney;
    Route* route = nullptr;
    PhysicalMode* physical_mode = nullptr;
    Company* company = nullptr;
    std::vector<StopTime> stop_time_list;

    // These variables are used in the case of an extension of service
    // They indicate what's the vj you can take directly after or before this one
    // They have the same block id
    VehicleJourney* next_vj = nullptr;
    VehicleJourney* prev_vj = nullptr;
    // associated meta vj
    MetaVehicleJourney* meta_vj = nullptr;
    std::string odt_message;  // TODO It seems a VJ can have either a comment or an odt_message but never both, so we
                              // could use only the 'comment' to store the odt_message
    std::string headsign;

    // TODO ODT NTFSv0.3: remove that when we stop to support NTFSv0.1
    VehicleJourneyType vehicle_journey_type = VehicleJourneyType::regular;

    // all times are stored in UTC
    // however, sometime we do not have a date to convert the time to a local value (in jormungandr)
    // For example for departure board over a period (calendar)
    // thus we might need the shift to convert all stop times of the vehicle journey to local
    int32_t utc_to_local_offset() const;  // in seconds

    RTLevel realtime_level = RTLevel::Base;
    bool is_base_schedule() const { return realtime_level == RTLevel::Base; }
    // number of days of delay compared to base-vj vp (case of a delayed vj in realtime or adapted)
    size_t shift = 0;
    // validity pattern for all RTLevel
    flat_enum_map<RTLevel, ValidityPattern*> validity_patterns = {{{nullptr, nullptr, nullptr}}};
    ValidityPattern* get_validity_pattern_at(RTLevel level) const { return validity_patterns[level]; }
    ValidityPattern* base_validity_pattern() const { return get_validity_pattern_at(RTLevel::Base); }
    ValidityPattern* adapted_validity_pattern() const { return get_validity_pattern_at(RTLevel::Adapted); }
    ValidityPattern* rt_validity_pattern() const { return get_validity_pattern_at(RTLevel::RealTime); }
    // base-schedule validity pattern canceled by this vj (to get corresponding vjs, use meta-vj)
    ValidityPattern get_base_canceled_validity_pattern() const;

    // return the base vj corresponding to this vj, return nullptr if nothing found
    const VehicleJourney* get_corresponding_base() const;

    // Return the smallest time within its stop_times
    uint32_t earliest_time() const;

    /*
     *
     *
     *  Day     1              2               3               4               5               6             ...
     *          -----------------------------------------------------------------------------------------------------
     * SP_bob           8:30         8:30             8:30           8:30           8:30             8:30   ... (vj)
     * Period_bob   |-----------------------------------------------|
     *             6:00                                            6:00
     *
     * Let's say we have a vj passes every day at 8:30 on a stop point SP_bob.
     * Now given a Period_bob and the VJ stops at SP_bob only during this Period_bob, what's the validity pattern of the
     * VJ if SP_bob must be served?
     *
     * In this case, the vehicle does stop at the Day1, 2, 3, *BUT* not the Day4
     * Then the validity pattern of this stop_time in this period is "00111" (Reminder: the vp string is inverted)
     *
     * */
    // compute the validity pattern of the vj stop at that stop_point, given the realtime level and a period
    ValidityPattern get_vp_of_sp(const StopPoint& sp,
                                 RTLevel rt_level,
                                 const boost::posix_time::time_period& period) const;

    // Return the vp for all the stops of the section
    ValidityPattern get_vp_for_section(const std::set<RankStopTime>& section,
                                       RTLevel rt_level,
                                       const boost::posix_time::time_period& period) const;

    const StopTime& get_stop_time(const RankStopTime& order) const;

    // return all the sections of the base vj between the 2 stop areas
    std::set<RankStopTime> get_sections_ranks(const StopArea*, const StopArea*) const;

    // return all the sections of the base vj between the 2 stop areas
    std::set<RankStopTime> get_no_service_sections_ranks(const StopArea*) const;

    // return the time period of circulation of the vj for one day
    boost::posix_time::time_period execution_period(const boost::gregorian::date& date) const;
    Dataset* dataset = nullptr;

    std::string get_direction() const;
    bool has_datetime_estimated() const;
    bool has_zonal_stop_point() const;
    bool has_odt() const;

    bool has_boarding() const;
    bool has_landing() const;
    Indexes get(Type_e type, const PT_Data& data) const;
    std::vector<boost::shared_ptr<disruption::Impact>> get_impacts() const;

    bool operator<(const VehicleJourney& other) const;

    template <class Archive>
    void save(Archive& ar, const unsigned int) const;
    template <class Archive>
    void load(Archive& ar, const unsigned int);
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    virtual ~VehicleJourney();
    // TODO remove the virtual there, but to do that we need to remove the prev/next_vj since boost::serialiaze needs to
    // make a virtual call for those
private:
    /*
     * Note: the destructor has not been defined as virtual because we don't need those classes to
     * be virtual.
     * the JP owns 2 differents lists so no virtual call must be made on destruction
     */
    VehicleJourney() = default;
    VehicleJourney(const VehicleJourney&) = default;
    friend class boost::serialization::access;
    friend struct DiscreteVehicleJourney;
    friend struct FrequencyVehicleJourney;
};

struct DiscreteVehicleJourney : public VehicleJourney {
    ~DiscreteVehicleJourney() override;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

struct FrequencyVehicleJourney : public VehicleJourney {
    uint32_t start_time = std::numeric_limits<uint32_t>::max();    // first departure hour
    uint32_t end_time = std::numeric_limits<uint32_t>::max();      // last departure hour
    uint32_t headway_secs = std::numeric_limits<uint32_t>::max();  // Seconds between each departure.
    ~FrequencyVehicleJourney() override;

    bool is_valid(int day, const RTLevel rt_level) const;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};
}  // namespace type
}  // namespace navitia
