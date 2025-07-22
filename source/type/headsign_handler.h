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

#include "type/fwd_type.h"

#include <boost/container/flat_map.hpp>

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>

/**
 * Headsign handler
 *
 * Contains and manages :
 * - headsign at a given stop_time for VJ (default value for headsign at a stop_time is vj.headsign)
 * - metaVJ from given headsign (one of the VJ in metaVJ contains given headsign)
 */
namespace navitia {
namespace type {

struct HeadsignHandler {
    // changes vj's headsign and registers vj under its new headsign for headsign-to-vj map
    // (remove previous headsign from headsign-to-vj map if necessary)
    void change_vj_headsign_and_register(VehicleJourney& vj, const std::string& new_headsign);
    void affect_headsign_to_stop_time(const StopTime& stop_time, const std::string& headsign);

    const std::string& get_headsign(const StopTime& stop_time) const;
    const std::string& get_headsign(const VehicleJourney* vj) const;
    std::set<std::string> get_all_headsigns(const VehicleJourney* vj);
    std::vector<const VehicleJourney*> get_vj_from_headsign(const std::string& headsign) const;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);

    void forget_vj(const VehicleJourney*);

protected:
    bool has_headsign_or_name(const VehicleJourney& vj, const std::string& headsign) const;
    void update_headsign_mvj_after_remove(const VehicleJourney& vj, const std::string& removed_headsign);

    // for each VJ, map containing index of stop time and the new headsign (until next change)
    // as new stop_time might be added in the future, if map_vj_map_stop_time_headsign_change[vj]
    // exists, last change is always vj.headsign (and might happen after last stop_time)
    // This gives following structure (vj.headsign = A) :
    // stop times       : 1 2 3 4 5 6 7 8 (potential 9)
    // headsigns        : A A A B B C B B
    // headsign_changes :       B   C B   A
    std::unordered_map<const VehicleJourney*, boost::container::flat_map<RankStopTime, std::string>> headsign_changes;
    // headsign to meta-vj map
    std::unordered_map<std::string, std::unordered_set<const MetaVehicleJourney*>> headsign_mvj;
};

}  // namespace type
}  // namespace navitia
