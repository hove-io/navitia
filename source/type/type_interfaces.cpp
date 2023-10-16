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

#include "type/type_interfaces.h"
#include "utils/functions.h"
#include "type/type.pb.h"
#include "type/message.h"

namespace navitia {
namespace type {

std::vector<boost::shared_ptr<disruption::Impact>> HasMessages::get_applicable_messages(
    const boost::posix_time::ptime& current_time,
    const boost::posix_time::time_period& action_period) const {
    std::vector<boost::shared_ptr<disruption::Impact>> result;

    for (const auto& impact : this->impacts) {
        auto impact_acquired = impact.lock();
        if (!impact_acquired) {
            continue;  // pointer might still have become invalid
        }
        if (impact_acquired->is_valid(current_time, action_period)) {
            result.push_back(impact_acquired);
        }
    }

    return result;
}

std::vector<boost::shared_ptr<disruption::Impact>> HasMessages::get_impacts() const {
    std::vector<boost::shared_ptr<disruption::Impact>> result;
    for (const auto& impact : impacts) {
        auto impact_sptr = impact.lock();
        if (impact_sptr == nullptr) {
            continue;
        }
        result.push_back(impact_sptr);
    }
    return result;
}

std::vector<boost::shared_ptr<disruption::Impact>> HasMessages::get_publishable_messages(
    const boost::posix_time::ptime& current_time) const {
    std::vector<boost::shared_ptr<disruption::Impact>> result;

    for (const auto& impact : this->impacts) {
        auto impact_acquired = impact.lock();
        if (!impact_acquired) {
            continue;  // pointer might still have become invalid
        }
        if (impact_acquired->disruption->is_publishable(current_time)) {
            result.push_back(impact_acquired);
        }
    }
    return result;
}

bool HasMessages::has_applicable_message(const boost::posix_time::ptime& current_time,
                                         const boost::posix_time::time_period& action_period,
                                         const std::vector<disruption::ActiveStatus>& filter_status,
                                         const Line* line) const {
    for (const auto& i : this->impacts) {
        auto impact = i.lock();
        if (!impact) {
            continue;  // pointer might still have become invalid
        }
        if (line) {
            if ((impact->is_only_line_section() && !impact->is_line_section_of(*line))
                || (impact->is_only_rail_section() && !impact->is_rail_section_of(*line))) {
                continue;
            }
        }
        if (impact->is_valid(current_time, action_period)) {
            // if filter empty == no filter
            // else we return true only if active status is wanted
            if (filter_status.empty()
                || std::find(filter_status.begin(), filter_status.end(), impact->get_active_status(current_time))
                       != filter_status.end()) {
                return true;
            }
        }
    }
    return false;
}

bool HasMessages::has_publishable_message(const boost::posix_time::ptime& current_time) const {
    for (const auto& impact : this->impacts) {
        auto impact_acquired = impact.lock();
        if (!impact_acquired) {
            continue;  // pointer might still have become invalid
        }
        if (impact_acquired->disruption->is_publishable(current_time)) {
            return true;
        }
    }
    return false;
}

void HasMessages::clean_weak_impacts() {
    clean_up_weak_ptr(impacts);
}

}  // namespace type
}  // namespace navitia
