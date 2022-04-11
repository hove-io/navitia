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

#include "georef/street_network.h"
#include "type/pt_data.h"
#include "type/datetime.h"
#include "type/accessibility_params.h"

#include <boost/optional.hpp>

namespace navitia {
namespace routing {

using type::idx_t;

struct NotFound {};

/** Représente un horaire associé à un validity pattern
 *
 * Il s'agit donc des horaires théoriques
 */
struct ValidityPatternTime {
    idx_t vp_idx;
    int hour;

    template <class T>
    bool operator<(T other) const {
        return hour < other.hour;
    }

    ValidityPatternTime() : vp_idx(type::invalid_idx), hour(-1) {}
    ValidityPatternTime(idx_t vp_idx, int hour) : vp_idx(vp_idx), hour(hour) {}
};

enum class ItemType { public_transport, walking, stay_in, waiting, boarding, alighting };

inline std::ostream& operator<<(std::ostream& ss, ItemType t) {
    switch (t) {
        case ItemType::public_transport:
            return ss << "public_transport";
        case ItemType::walking:
            return ss << "transfer";
        case ItemType::stay_in:
            return ss << "stay_in";
        case ItemType::waiting:
            return ss << "waiting";
        case ItemType::boarding:
            return ss << "boarding";
        case ItemType::alighting:
            return ss << "alighting";
    }
    return ss;
}

/** Étape d'un itinéraire*/
struct PathItem {
    boost::posix_time::ptime arrival = boost::posix_time::pos_infin;
    boost::posix_time::ptime departure = boost::posix_time::pos_infin;
    std::vector<boost::posix_time::ptime> arrivals;
    std::vector<boost::posix_time::ptime> departures;
    std::vector<const nt::StopTime*> stop_times;  // empty if not public transport

    /**
     * if public transport, contains the list of visited stop_point
     * for transfers, contains the departure and arrival stop points
     */
    std::vector<const navitia::type::StopPoint*> stop_points;

    const navitia::type::StopPointConnection* connection;

    ItemType type;

    PathItem(ItemType t = ItemType::public_transport,
             boost::posix_time::ptime departure = boost::posix_time::pos_infin,
             boost::posix_time::ptime arrival = boost::posix_time::pos_infin)
        : arrival(arrival), departure(departure), type(t) {}

    std::string print() const;

    const navitia::type::VehicleJourney* get_vj() const {
        if (stop_times.empty())
            return nullptr;
        return stop_times.front()->vehicle_journey;
    }
};

std::ostream& operator<<(std::ostream& ss, const PathItem& t);
/** Un itinéraire complet */
struct Path {
    navitia::time_duration duration = boost::posix_time::pos_infin;
    uint32_t nb_changes = std::numeric_limits<uint32_t>::max();
    boost::posix_time::ptime request_time = boost::posix_time::pos_infin;
    std::vector<PathItem> items;
    type::EntryPoint origin;

    // for debug purpose, we store the reader's computed values
    navitia::time_duration sn_dur = 0_s;           // street network duration
    navitia::time_duration transfer_dur = 0_s;     // walking duration during transfer
    navitia::time_duration min_waiting_dur = 0_s;  // minimal waiting duration on every transfers
    uint8_t nb_vj_extentions = 0;                  // number of vehicle journey extentions (I love useless comments!)
    uint8_t nb_sections = 0;

    void print() const {
        for (auto item : items)
            std::cout << item.print() << std::endl;
    }
};

std::ostream& operator<<(std::ostream& ss, const Path& t);

bool operator==(const PathItem& a, const PathItem& b);

bool is_same_stop_point(const type::EntryPoint&, const type::StopPoint&);

/**
 * Choose if we must use a crowfly or a streetnework for a section.
 * This function is called for the first and last section of a journey.
 *
 * @param point: the object where we are going/from where we are coming:
 *               the requested origin for the first section or the destination for the last section
 * @param stop_point: for the first section, the stop point where we are going,
 *                    or for the last section the stop point from where we come
 * @param street_network_path: the street network path between point and stop_point
 * @param data: reference datas of the instance
 * @param free_radius: distance to which stop points become free to crowfly to from the entry point.
 * @param distance_to_stop_point: duration from the entry point to the stop point.
 */
bool use_crow_fly(const type::EntryPoint& point,
                  const type::StopPoint& stop_point,
                  const georef::Path& street_network_path,
                  const type::Data& data,
                  const uint32_t free_radius = 0,
                  boost::optional<time_duration> distance_to_stop_point = boost::none);
}  // namespace routing
}  // namespace navitia
