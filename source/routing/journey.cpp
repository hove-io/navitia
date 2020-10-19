/* Copyright © 2001-2018, Canal TP and/or its affiliates. All rights reserved.

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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "journey.h"

#include "type/stop_time.h"

using namespace navitia::routing;

Journey::Section::Section(const type::StopTime& in,
                          const DateTime in_dt,
                          const type::StopTime& out,
                          const DateTime out_dt)
    : get_in_st(&in), get_in_dt(in_dt), get_out_st(&out), get_out_dt(out_dt) {}

bool Journey::Section::operator==(const Section& rhs) const {
    return get_in_st->order() == rhs.get_in_st->order() && get_in_st->vehicle_journey == rhs.get_in_st->vehicle_journey
           && get_in_dt == rhs.get_in_dt && get_out_st->order() == rhs.get_out_st->order()
           && get_out_st->vehicle_journey == rhs.get_out_st->vehicle_journey && get_out_dt == rhs.get_out_dt;
}

bool Journey::is_pt() const {
    return !sections.empty();
}

bool Journey::better_on_dt(const Journey& that,
                           bool request_clockwise,
                           const navitia::time_duration transfer_penalty) const {
    const int nb_transfer = sections.size() + nb_vj_extentions;
    const int that_nb_transfer = that.sections.size() + that.nb_vj_extentions;
    // we consider that an extra transfer is worthwhile
    // only if it reduces the arrival time by at least transfer_penalty
    const DateTime penalized_arrival = arrival_dt + transfer_penalty * nb_transfer;
    const DateTime that_penalized_arrival = that.arrival_dt + transfer_penalty * that_nb_transfer;
    // Similary, we consider that an extra transfer (hence a bigger sections.size() ) is worthwhile
    // only if it increases the departure time by at least transfer_penalty
    const DateTime penalized_departure = departure_dt - transfer_penalty * nb_transfer;
    const DateTime that_penalized_departure = that.departure_dt - transfer_penalty * that_nb_transfer;

    if (request_clockwise) {
        // if the (non-penalized) arrival times are the same, we compare the penalized departure times
        if (arrival_dt != that.arrival_dt) {
            return penalized_arrival <= that_penalized_arrival;
        }

        if (penalized_departure != that_penalized_departure) {
            return penalized_departure >= that_penalized_departure;
        }
    } else {
        if (departure_dt != that.departure_dt) {
            return penalized_departure >= that_penalized_departure;
        }
        if (penalized_arrival != that_penalized_arrival) {
            return penalized_arrival <= that_penalized_arrival;
        }
    }

    return min_waiting_dur >= that.min_waiting_dur;
}

bool Journey::better_on_transfer(const Journey& that) const {
    if (sections.size() != that.sections.size()) {
        return sections.size() <= that.sections.size();
    }
    if (nb_vj_extentions != that.nb_vj_extentions) {
        return nb_vj_extentions <= that.nb_vj_extentions;
    }

    return true;
}
bool Journey::better_on_sn(const Journey& that, const navitia::time_duration transfer_penalty) const {
    const int nb_transfer = sections.size() + nb_vj_extentions;
    const int that_nb_transfer = that.sections.size() + that.nb_vj_extentions;
    // we consider that the transfer duration as well as the street network duration are
    // walking duration
    // we consider that an extra transfer  is worthwhile
    // only if it reduces the walking duration by at least transfer_penalty
    return sn_dur + transfer_dur + transfer_penalty * nb_transfer
           <= that.sn_dur + that.transfer_dur + transfer_penalty * that_nb_transfer;
    ;
}

bool Journey::operator==(const Journey& rhs) const {
    return departure_dt == rhs.departure_dt && arrival_dt == rhs.arrival_dt && sections == rhs.sections;
}

bool Journey::operator!=(const Journey& rhs) const {
    return !(*this == rhs);
}

size_t SectionHash::operator()(const Journey::Section& s, size_t seed) const {
    boost::hash_combine(seed, s.get_in_st->order().val);
    boost::hash_combine(seed, s.get_in_st->vehicle_journey);
    boost::hash_combine(seed, s.get_in_dt);
    boost::hash_combine(seed, s.get_out_st->order().val);
    boost::hash_combine(seed, s.get_out_st->vehicle_journey);
    boost::hash_combine(seed, s.get_out_dt);

    return seed;
}

size_t JourneyHash::operator()(const Journey& j) const {
    size_t seed = 0;
    SectionHash sect_hash;

    for (const auto& s : j.sections) {
        boost::hash_combine(seed, sect_hash(s, seed));
    }

    return seed;
}
