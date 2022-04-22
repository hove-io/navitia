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
#include "timezone_manager.h"

#include "type/meta_data.h"
#include "type/validity_pattern.h"
#include "utils/exception.h"
#include "utils/logger.h"

#include <utility>

namespace navitia {
namespace type {

TimeZoneHandler::TimeZoneHandler(std::string name,
                                 const boost::gregorian::date& production_period_beg,
                                 const dst_periods& offsets_periods)
    : tz_name(std::move(name)) {
    for (const auto& utc_shift_and_periods : offsets_periods) {
        ValidityPattern vp(production_period_beg);

        auto offset = utc_shift_and_periods.first;
        for (const auto& period : utc_shift_and_periods.second) {
            for (boost::gregorian::day_iterator it(period.begin()); it < period.end(); ++it) {
                vp.add(*it);
            }
        }
        time_changes.emplace_back(std::move(vp), offset);
    }
}

int32_t TimeZoneHandler::get_utc_offset(boost::gregorian::date day) const {
    for (const auto& vp_shift : time_changes) {
        if (vp_shift.first.check(day)) {
            return vp_shift.second;
        }
    }
    // the time_changes should be a partition of the production period, so this should not happen
    throw navitia::recoverable_exception("day " + boost::gregorian::to_iso_string(day) + " not in production period");
}

int32_t TimeZoneHandler::get_utc_offset(int day) const {
    for (const auto& vp_shift : time_changes) {
        if (vp_shift.first.check(day)) {
            return vp_shift.second;
        }
    }
    // the time_changes should be a partition of the production period, so this should not happen
    throw navitia::recoverable_exception("day " + std::to_string(day) + " not in production period");
}

template <size_t N>
bool is_subset_of(const std::bitset<N>& lhs, const std::bitset<N>& rhs) {
    return (lhs & ~rhs).none();
}

int32_t TimeZoneHandler::get_utc_offset(const ValidityPattern& vp) const {
    // Most of the time, a validity pattern will not span accros DST.
    // It can be sometimes when we shift the validity pattern for
    // the first stop_time to be between 0 and 24.

    if (vp.days.none()) {
        return 0;  // vp is empty, utc shift is not important
    }

    for (const auto& vp_shift : time_changes) {
        // we check if the vp is included in the time change
        if (is_subset_of(vp.days, vp_shift.first.days)) {
            return vp_shift.second;
        }
    }

    // OK, we didn't find a perfect matching, trying with the best match
    size_t best_nb_common = 0;
    int32_t best_offset = 0;
    for (const auto& vp_shift : time_changes) {
        const auto nb_common = (vp.days & vp_shift.first.days).count();
        if (nb_common > best_nb_common) {
            best_nb_common = nb_common;
            best_offset = vp_shift.second;
        }
    }
    if (best_nb_common > 0) {
        return best_offset;
    }

    // by construction, this should not be possible
    // we add a debug log
    std::stringstream ss;
    ss << "vp " << vp.days << " does not match any tz periods: ";
    for (const auto& vp_shift : time_changes) {
        ss << " tz offset " << vp_shift.second << ": " << vp_shift.first.days;
    }
    LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"), ss.str());
    throw navitia::recoverable_exception("no intersection with a timezone found");
}

TimeZoneHandler::dst_periods TimeZoneHandler::get_periods_and_shift() const {
    dst_periods dst;
    namespace bg = boost::gregorian;
    for (const auto& vp_shift : time_changes) {
        const auto& bitset = vp_shift.first.days;
        const auto& beg_of_period = vp_shift.first.beginning_date;
        std::vector<bg::date_period> periods;
        bg::date last_period_beg;
        for (size_t i = 0; i < bitset.size(); ++i) {
            if (bitset.test(i)) {
                if (last_period_beg.is_not_a_date()) {
                    // begining of a period, we store the date
                    last_period_beg = beg_of_period + bg::days(i);
                }
            } else {
                // if we had a begin, we can add a period
                if (!last_period_beg.is_not_a_date()) {
                    periods.emplace_back(last_period_beg, beg_of_period + bg::days(i));
                }
                last_period_beg = bg::date();
            }
        }
        // we add the last build period
        if (!last_period_beg.is_not_a_date()) {
            periods.emplace_back(last_period_beg, beg_of_period + bg::days(bitset.size()));
        }
        dst[vp_shift.second] = periods;
    }

    return dst;
}

const TimeZoneHandler* TimeZoneManager::get_or_create(
    const std::string& name,
    const boost::gregorian::date& production_period_beg,
    const std::map<int32_t, std::vector<boost::gregorian::date_period>>& offsets) {
    auto it = tz_handlers.find(name);

    if (it == std::end(tz_handlers)) {
        tz_handlers[name] = std::make_unique<TimeZoneHandler>(name, production_period_beg, offsets);
        return tz_handlers[name].get();
    }
    return it->second.get();
}

const TimeZoneHandler* TimeZoneManager::get(const std::string& name) const {
    auto it = tz_handlers.find(name);
    if (it == std::end(tz_handlers)) {
        return nullptr;
    }
    return it->second.get();
}

const TimeZoneHandler* TimeZoneManager::get_first_timezone() const {
    if (tz_handlers.empty()) {
        return nullptr;
    }
    return tz_handlers.begin()->second.get();
}
}  // namespace type
}  // namespace navitia
