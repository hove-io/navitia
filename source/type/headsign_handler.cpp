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

#include "headsign_handler.h"

#include "type/meta_vehicle_journey.h"
#include "type/serialization.h"
#include "type/stop_time.h"
#include "utils/functions.h"
#include "utils/serialization_flat_map.h"
#include "utils/serialization_unordered_map.h"
#include "utils/serialization_unordered_set.h"

namespace navitia {
namespace type {

template <class Archive>
void HeadsignHandler::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& headsign_changes& headsign_mvj;
}
SERIALIZABLE(HeadsignHandler)

void HeadsignHandler::change_name_and_register_as_headsign(VehicleJourney& vj, const std::string& new_name) {
    std::string prev_name = vj.headsign;
    vj.headsign = new_name;
    headsign_mvj[vj.headsign].insert(vj.meta_vj);
    if (prev_name != vj.headsign) {
        update_headsign_mvj_after_remove(vj, prev_name);
    }
}

void HeadsignHandler::update_headsign_mvj_after_remove(const VehicleJourney& vj, const std::string& removed_headsign) {
    // unregister meta-vj from headsign only if necessary
    bool need_remove = true;
    vj.meta_vj->for_all_vjs([&](const VehicleJourney& cur_vj) {
        if (has_headsign_or_name(cur_vj, removed_headsign)) {
            need_remove = false;
        }
    });
    if (!need_remove) {
        return;
    }

    auto it_headsign_mvj = headsign_mvj.find(removed_headsign);
    if (it_headsign_mvj == headsign_mvj.end()) {
        return;
    }
    it_headsign_mvj->second.erase(vj.meta_vj);
    if (it_headsign_mvj->second.empty()) {
        headsign_mvj.erase(it_headsign_mvj);
    }
}

const std::string& HeadsignHandler::get_headsign(const VehicleJourney* vj) const {
    return vj->headsign;
}

const std::string& HeadsignHandler::get_headsign(const StopTime& stop_time) const {
    // if no headsign map for vj: return name
    if (!navitia::contains(headsign_changes, stop_time.vehicle_journey)) {
        return stop_time.vehicle_journey->headsign;
    }

    // otherwise use headsign change map
    const auto& map_stop_time_headsign_change = headsign_changes.at(stop_time.vehicle_journey);
    auto order_stop_time = stop_time.order();

    // if no headsign change stored: return name
    if (map_stop_time_headsign_change.empty()) {
        return stop_time.vehicle_journey->headsign;
    }

    // get next change
    auto it_headsign = map_stop_time_headsign_change.upper_bound(order_stop_time);
    // if next change is the first: return name
    if (it_headsign == map_stop_time_headsign_change.begin()) {
        return stop_time.vehicle_journey->headsign;
    }

    // get previous change and return headsign
    if (it_headsign == map_stop_time_headsign_change.end()) {
        return map_stop_time_headsign_change.rbegin()->second;
    }
    return (--it_headsign)->second;
}

std::set<std::string> HeadsignHandler::get_all_headsigns(const VehicleJourney* vj) {
    std::set<std::string> res;
    const auto& it_changes = headsign_changes.find(vj);
    if (!vj || it_changes == std::end(headsign_changes)) {
        return res;
    }
    for (const auto& change : it_changes->second) {
        // ignore last headsign (vj.name) as it does not concern a stop_time
        if (change.first.val < vj->stop_time_list.size()) {
            res.insert(change.second);
        }
    }
    return res;
}

bool HeadsignHandler::has_headsign_or_name(const VehicleJourney& vj, const std::string& headsign) const {
    if (vj.headsign == headsign) {
        return true;
    }

    const auto it_vj_changes = headsign_changes.find(&vj);
    if (it_vj_changes == headsign_changes.end()) {
        return false;
    }

    auto has_headsign = [&](const std::pair<RankStopTime, std::string>& it_change) {
        return it_change.second == headsign;
    };
    return navitia::contains_if(it_vj_changes->second, has_headsign);
}

std::vector<const VehicleJourney*> HeadsignHandler::get_vj_from_headsign(const std::string& headsign) const {
    const auto& it_vj_set = headsign_mvj.find(headsign);
    if (it_vj_set == headsign_mvj.end()) {
        return {};
    }

    std::vector<const VehicleJourney*> res;
    for (const MetaVehicleJourney* mvj : it_vj_set->second) {
        mvj->for_all_vjs([&](const VehicleJourney& vj) {
            if (has_headsign_or_name(vj, headsign)) {
                res.push_back(&vj);
            }
        });
    }
    return res;
}

void HeadsignHandler::affect_headsign_to_stop_time(const StopTime& stop_time, const std::string& headsign) {
    const VehicleJourney* vj = stop_time.vehicle_journey;
    assert(find_or_default(vj->headsign, headsign_mvj).count(vj->meta_vj));
    std::string prev_headsign_for_stop_time = get_headsign(stop_time);
    if (headsign == prev_headsign_for_stop_time) {
        return;
    }

    auto order = stop_time.order();
    auto& vj_headsign_changes = headsign_changes[vj];
    // erase change if exists
    if (navitia::contains(vj_headsign_changes, order)) {
        vj_headsign_changes.erase(order);
    }
    // only if headsign erase is not enough: store change
    if (get_headsign(stop_time) != headsign) {
        vj_headsign_changes[order] = headsign;
    }
    // if next stop_time's value is prev_headsign_for_stop_time: store the change back to this
    // previous headsign. Done even if last, as we could add stop_time in the future
    if (!navitia::contains(vj_headsign_changes, order + 1)) {
        vj_headsign_changes[order + 1] = prev_headsign_for_stop_time;
    }
    // if next stop_time changed to  headsign: erase mext change to merge
    else if (vj_headsign_changes[order + 1] == headsign) {
        vj_headsign_changes.erase(order + 1);
    }
    // if map for this vj is empty (all headsigns == vj.name): clean map
    if (vj_headsign_changes.empty()) {
        headsign_changes.erase(vj);
    }

    // register meta-vj from headsign
    headsign_mvj[headsign].insert(vj->meta_vj);
    update_headsign_mvj_after_remove(*vj, prev_headsign_for_stop_time);
}

void HeadsignHandler::forget_vj(const VehicleJourney* /*unused*/) {
    // currently we never want to forget VJ
    // it would be MANDATORY to do it if we added realtime vjs to the headsigns handler,
    // but for the moment we only index base schedule VJ
    // if that changes be sure to add the mechanism here to remove the useless VJ before we delete it
}

}  // namespace type
}  // namespace navitia
