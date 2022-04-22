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

#include "types.h"

using namespace ed::types;

bool CommercialMode::operator<(const CommercialMode& other) const {
    return this->name < other.name || (this->name == other.name && this < &other);
}

bool PhysicalMode::operator<(const PhysicalMode& other) const {
    return this->name < other.name || (this->name == other.name && this < &other);
}

bool Line::operator<(const Line& other) const {
    if (this->commercial_mode == nullptr && other.commercial_mode != nullptr) {
        return true;
    }
    if (other.commercial_mode == nullptr && this->commercial_mode != nullptr) {
        return false;
    }
    if (this->commercial_mode == other.commercial_mode) {
        BOOST_ASSERT(this->uri != other.uri);
        return this->uri < other.uri;
    }
    return *(this->commercial_mode) < *(other.commercial_mode);
}

bool LineGroup::operator<(const LineGroup& other) const {
    if (this->name != other.name) {
        return this->name < other.name;
    }
    return this->idx < other.idx;
}

bool Route::operator<(const Route& other) const {
    if (this->line == other.line) {
        BOOST_ASSERT(this->uri != other.uri);
        return this->uri < other.uri;
    }
    return *(this->line) < *(other.line);
}

Calendar::Calendar(boost::gregorian::date beginning_date) : validity_pattern(beginning_date) {}

void Calendar::build_validity_pattern(boost::gregorian::date_period production_period) {
    // initialisation of the validity pattern from the active periods and the exceptions
    for (boost::gregorian::date_period period : this->period_list) {
        auto intersection_period = production_period.intersection(period);
        if (intersection_period.is_null()) {
            continue;
        }
        validity_pattern.add(intersection_period.begin(), intersection_period.end(), week_pattern);
    }

    for (navitia::type::ExceptionDate exd : this->exceptions) {
        if (!production_period.contains(exd.date)) {
            continue;
        }
        if (exd.type == navitia::type::ExceptionDate::ExceptionType::sub) {
            validity_pattern.remove(exd.date);
        } else if (exd.type == navitia::type::ExceptionDate::ExceptionType::add) {
            validity_pattern.add(exd.date);
        }
    }
}

bool StopArea::operator<(const StopArea& other) const {
    BOOST_ASSERT(this->uri != other.uri);
    //@TODO géré la gestion de la city
    return this->uri < other.uri;
}

bool StopPoint::operator<(const StopPoint& other) const {
    if (!this->stop_area) {
        return false;
    }
    if (!other.stop_area) {
        return true;
    }
    if (this->stop_area == other.stop_area) {
        BOOST_ASSERT(this->uri != other.uri);
        return this->uri < other.uri;
    }
    return *(this->stop_area) < *(other.stop_area);
}

bool StopPoint::operator!=(const StopPoint& sp) const {
    /*
     * We assume that URI are all unique accross stop points
     */
    return uri != sp.uri;
}

int VehicleJourney::earliest_time() const {
    const auto& st = std::min_element(stop_time_list.begin(), stop_time_list.end(), [](StopTime* st1, StopTime* st2) {
        return std::min(st1->boarding_time, st1->arrival_time) < std::min(st2->boarding_time, st2->arrival_time);
    });
    return std::min((*st)->boarding_time, (*st)->arrival_time);
}

bool VehicleJourney::operator<(const VehicleJourney& other) const {
    return this->uri < other.uri;
}

bool VehicleJourney::joins_on_different_stop_points(const ed::types::VehicleJourney& prev_vj) const {
    const auto* vj_first_st = stop_time_list.front();
    const auto* prev_vj_last_st = prev_vj.stop_time_list.back();

    return *vj_first_st->stop_point != *prev_vj_last_st->stop_point;
}

bool VehicleJourney::starts_with_stayin_on_same_stop_point() const {
    if (!prev_vj)
        return false;
    if (stop_time_list.empty() or prev_vj->stop_time_list.empty())
        return false;

    const auto* start_vj_last_sp = prev_vj->stop_time_list.back()->stop_point;
    const auto* end_vj_first_sp = stop_time_list.front()->stop_point;
    return start_vj_last_sp == end_vj_first_sp;
}

bool VehicleJourney::ends_with_stayin_on_same_stop_point() const {
    if (!next_vj)
        return false;
    if (stop_time_list.empty() or next_vj->stop_time_list.empty())
        return false;

    const auto* start_vj_last_sp = stop_time_list.back()->stop_point;
    const auto* end_vj_first_sp = next_vj->stop_time_list.front()->stop_point;
    return start_vj_last_sp == end_vj_first_sp;
}

bool StopPointConnection::operator<(const StopPointConnection& other) const {
    if (this->departure == other.departure) {
        if (this->destination == other.destination) {
            return this < &other;
        }
        return *(this->destination) < *(other.destination);
    }
    return *(this->departure) < *(other.departure);
}

bool StopTime::operator<(const StopTime& other) const {
    if (this->vehicle_journey != other.vehicle_journey) {
        return *(this->vehicle_journey) < *(other.vehicle_journey);
    }
    return this->order < other.order;
}
