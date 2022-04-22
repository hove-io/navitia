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

#include "type/time_duration.h"
#include "type/datetime.h"
#include "utils/exception.h"

#include <boost/functional/hash.hpp>
#include <unordered_set>

namespace navitia {

namespace type {
struct StopTime;
}

namespace routing {

struct Journey {
    struct Section {
        Section() = default;
        Section(const type::StopTime& in, const DateTime in_dt, const type::StopTime& out, const DateTime out_dt);
        const type::StopTime* get_in_st = nullptr;
        DateTime get_in_dt = 0;
        const type::StopTime* get_out_st = nullptr;
        DateTime get_out_dt = 0;
        bool operator==(const Section& rhs) const;
    };

    bool is_pt() const;

    bool better_on_dt(const Journey& that,
                      bool request_clockwise,
                      const navitia::time_duration arrival_transfer_penalty) const;
    bool better_on_transfer(const Journey& that) const;
    bool better_on_sn(const Journey& that, const navitia::time_duration walking_transfer_penalty) const;
    bool operator==(const Journey& rhs) const;
    bool operator!=(const Journey& rhs) const;
    friend std::ostream& operator<<(std::ostream& os, const Journey& j);

    std::vector<Section> sections;                   // the pt sections, with transfer between them
    navitia::time_duration sn_dur = 0_s;             // street network duration
    navitia::time_duration transfer_dur = 0_s;       // walking duration during transfer
    navitia::time_duration min_waiting_dur = 0_s;    // minimal waiting duration on every transfers
    navitia::time_duration total_waiting_dur = 0_s;  // sum of waiting duration over all transfers
    DateTime departure_dt = 0;                       // the departure dt of the journey, including sn
    DateTime arrival_dt = 0;                         // the arrival dt of the journey, including sn
    uint8_t nb_vj_extentions = 0;                    // number of vehicle journey extentions (I love useless comments!)
};

struct JourneyHash {
    size_t operator()(const Journey& j) const;
};

struct SectionHash {
    size_t operator()(const Journey::Section& s, size_t seed) const;
};

using JourneySet = std::unordered_set<Journey, JourneyHash>;

/**
 * @brief Get the pseudo-best journey
 *
 * Find journey with the earliest arrival (clockwise case) or the lastest departure (anti clockwise case)
 * When equal, real "best" uses more criteria, but this is enough for the current use
 *
 * @param journeys A container of journeys
 * @param clokwise Active clockwise or not
 * @return best jouney
 */
template <class Journeys>
const Journey& get_pseudo_best_journey(const Journeys& journeys, bool clockwise) {
    if (journeys.size() == 0)
        throw recoverable_exception("get_pseudo_best_journey takes a list of at least 1 journey");

    auto departure_comp = [](const Journey& j1, const Journey& j2) { return j1.departure_dt < j2.departure_dt; };

    auto arrival_comp = [](const Journey& j1, const Journey& j2) { return j1.arrival_dt < j2.arrival_dt; };

    const auto best = clockwise ? std::min_element(journeys.cbegin(), journeys.cend(), arrival_comp)
                                : std::max_element(journeys.cbegin(), journeys.cend(), departure_comp);

    return *best;
}

}  // namespace routing
}  // namespace navitia
