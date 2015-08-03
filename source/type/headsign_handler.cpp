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

#include "headsign_handler.h"
#include "utils/functions.h"

namespace navitia { namespace type {

void HeadsignHandler::change_name_and_register_as_headsign(VehicleJourney& vj,
                                                           const std::string& new_name) {
    std::string prev_name = vj.name;
    vj.name = new_name;
    headsign_mvj[vj.name].insert(vj.meta_vj);
    update_headsign_mvj_after_remove(vj, prev_name);
}

void HeadsignHandler::update_headsign_mvj_after_remove(const VehicleJourney& vj,
                                                       const std::string& removed_headsign) {
    // unregister meta-vj from headsign only if necessary
    bool meta_has_headsign = false;
    for (const auto& it_headsign_vj: get_vj_from_headsign(removed_headsign)) {
        if (it_headsign_vj->meta_vj == vj.meta_vj) {
            meta_has_headsign = true;
            break;
        }
    }
    if (!meta_has_headsign && navitia::contains(headsign_mvj, removed_headsign)) {
        headsign_mvj[removed_headsign].erase(vj.meta_vj);
        if (headsign_mvj[removed_headsign].empty()) {
            headsign_mvj.erase(removed_headsign);
        }
    }
}

const std::string& HeadsignHandler::get_headsign(const StopTime& stop_time) const{
    // if no headsign map for vj: return name
    if (!navitia::contains(map_vj_map_stop_time_headsign_change, stop_time.vehicle_journey)) {
        return stop_time.vehicle_journey->name;
    }

    // otherwise use headsign change map
    const auto& map_stop_time_headsign_change =
            map_vj_map_stop_time_headsign_change.at(stop_time.vehicle_journey);
    uint16_t order_stop_time = stop_time.journey_pattern_point->order;

    // if no headsign change stored: return name
    if (map_stop_time_headsign_change.empty()) {
        return stop_time.vehicle_journey->name;
    }

    // get next change
    auto it_headsign = map_stop_time_headsign_change.upper_bound(order_stop_time);
    // if next change is the first: return name
    if(it_headsign == map_stop_time_headsign_change.begin()) {
        return stop_time.vehicle_journey->name;
    }

    // get previous change and return headsign
    if (it_headsign == map_stop_time_headsign_change.end()) {
        return map_stop_time_headsign_change.rbegin()->second;
    }
    return (--it_headsign)->second;
}

bool HeadsignHandler::has_headsign_or_name(const VehicleJourney& vj,
                                           const std::string& headsign) const {
    if (vj.name == headsign) {
        return true;
    }

    if(!navitia::contains(map_vj_map_stop_time_headsign_change, &vj)) {
        return false;
    }

    for (const auto& change: map_vj_map_stop_time_headsign_change.at(&vj)) {
        if (change.second == headsign) {
            return true;
        }
    }
    return false;
}

std::vector<const VehicleJourney*>
HeadsignHandler::get_vj_from_headsign(const std::string& headsign) const {
    using namespace std;
    vector<const VehicleJourney*> res;
    const auto& it_vj_set = headsign_mvj.find(headsign);
    if (it_vj_set == headsign_mvj.end()) {
        return res;
    }

    for (const MetaVehicleJourney* mvj: it_vj_set->second) {
        vector<vector<VehicleJourney*>> vect_vect_vj{mvj->theoric_vj, mvj->adapted_vj, mvj->real_time_vj};
        for (const auto vect_vj: vect_vect_vj) {
            for (const VehicleJourney* vj: vect_vj) {
                if (has_headsign_or_name(*vj, headsign)) {
                    res.push_back(vj);
                }
            }
        }
    }
    return res;
}

void HeadsignHandler::affect_headsign_to_stop_time(const StopTime& stop_time,
                                                   const std::string& headsign) {
    const VehicleJourney* vj = stop_time.vehicle_journey;
    headsign_mvj[vj->name].insert(vj->meta_vj); // for safety
    std::string prev_headsign_for_stop_time = get_headsign(stop_time);
    if (headsign == prev_headsign_for_stop_time) {
        return;
    }

    uint16_t order = stop_time.journey_pattern_point->order;
    // erase change if exists
    if (navitia::contains(map_vj_map_stop_time_headsign_change[vj], order)) {
        map_vj_map_stop_time_headsign_change[vj].erase(order);
    }
    // only if headsign erase is not enough: store change
    if (get_headsign(stop_time) != headsign) {
        map_vj_map_stop_time_headsign_change[vj][order] = headsign;
    }
    // if next stop_time's value is prev_headsign_for_stop_time: store the change back to this
    // previous headsign. Done even if last, as we could add stop_time in the future
    if (!navitia::contains(map_vj_map_stop_time_headsign_change[vj], order + 1)) {
        map_vj_map_stop_time_headsign_change[vj][order + 1] = prev_headsign_for_stop_time;
    }
    // if next stop_time changed to  headsign: erase mext change to merge
    else if(map_vj_map_stop_time_headsign_change[vj][order + 1] == headsign) {
        map_vj_map_stop_time_headsign_change[vj].erase(order + 1);
    }
    // if map for this vj is empty (all headsigns == vj.name): clean map
    if (map_vj_map_stop_time_headsign_change[vj].empty()) {
        map_vj_map_stop_time_headsign_change.erase(vj);
    }

    // register meta-vj from headsign
    headsign_mvj[headsign].insert(vj->meta_vj);
    update_headsign_mvj_after_remove(*vj, prev_headsign_for_stop_time);
}

}} //namespace navitia::type
