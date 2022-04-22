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
#include "type/timezone_manager.h"
#include "type/calendar.h"
#include "type/vehicle_journey.h"

#include <boost/range/algorithm/for_each.hpp>

namespace navitia {
namespace type {

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
                                                 const std::string& headsign,
                                                 const RTLevel,
                                                 const ValidityPattern& canceled_vp,
                                                 Route*,
                                                 std::vector<StopTime>,
                                                 PT_Data&);
    DiscreteVehicleJourney* create_discrete_vj(const std::string& uri,
                                               const std::string& name,
                                               const std::string& headsign,
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
                       const std::string& headsign,
                       const RTLevel,
                       const ValidityPattern& canceled_vp,
                       Route*,
                       std::vector<StopTime>,
                       PT_Data&);

    navitia::flat_enum_map<RTLevel, std::vector<std::unique_ptr<VehicleJourney>>> rtlevel_to_vjs_map;
};

}  // namespace type

}  // namespace navitia
