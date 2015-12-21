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
#include "timezone_manager.h"
#include "type.h"
#include "meta_data.h"

namespace navitia {
namespace type {

TimeZoneHandler::TimeZoneHandler(const MetaData& meta_data,
                                 const std::map<int16_t, std::vector<boost::gregorian::date_period>>& offsets_periods) {

    for (const auto& utc_shit_and_periods: offsets_periods) {
        ValidityPattern vp(meta_data.production_date.begin());

        auto offset = utc_shit_and_periods.first;
        for (const auto& period: utc_shit_and_periods.second) {
            for (boost::gregorian::day_iterator it(period.begin()); it < period.end(); ++it) {
                vp.add(*it);
            }
        }
        time_changes.push_back({std::move(vp), offset});
    }
}

int16_t TimeZoneHandler::get_utc_offset(boost::gregorian::date day) const {
    for (const auto& vp_shift: time_changes) {
        if (vp_shift.first.check(day)) { return vp_shift.second; }
    }
    // the time_changes should be a partition of the production period, so this should not happen
    throw navitia::exception("day " + boost::gregorian::to_iso_string(day) + " not in production period");
}

int16_t TimeZoneHandler::get_utc_offset(int day) const {
    for (const auto& vp_shift: time_changes) {
        if (vp_shift.first.check(day)) { return vp_shift.second; }
    }
    // the time_changes should be a partition of the production period, so this should not happen
    throw navitia::exception("day " + std::to_string(day) + " not in production period");
}

int16_t TimeZoneHandler::get_first_utc_offset(const ValidityPattern& vp) const {
    for (const auto& vp_shift: time_changes) {
        // we check if the vj intersect
        if ((vp_shift.first.days & vp.days).any()) { return vp_shift.second; }
    }
    // by construction, this should not be possible
    throw navitia::exception("no intersection with a validitypattern found");
}

const TimeZoneHandler*
TimeZoneManager::get_or_create(const MetaData& meta,
                               const std::map<int16_t, std::vector<boost::gregorian::date_period>>& offsets) {
    if (! tz_handler) {
        tz_handler = std::make_unique<TimeZoneHandler>(meta, offsets);
    }
    return tz_handler.get();
}
}
}
