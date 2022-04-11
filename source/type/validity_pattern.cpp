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
#include "validity_pattern.h"
#include "datetime.h"
#include "type/serialization.h"
#include "utils/exception.h"

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/serialization/bitset.hpp>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

namespace navitia {
namespace type {

template <class Archive>
void ValidityPattern::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& beginning_date& days& idx& uri;
}
SERIALIZABLE(ValidityPattern)

bool ValidityPattern::is_valid(int day) const {
    if (day < 0) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                        "Validity pattern not valid, the day " << day << " is too early");
        return false;
    }
    if (size_t(day) >= days.size()) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                        "Validity pattern not valid, the day " << day << " is late");
        return false;
    }
    return true;
}

int ValidityPattern::slide(boost::gregorian::date day) const {
    return (day - beginning_date).days();
}

void ValidityPattern::add(boost::gregorian::date day) {
    long duration = slide(day);
    add(duration);
}

void ValidityPattern::add(int duration) {
    if (is_valid(duration)) {
        days[duration] = true;
    }
}

void ValidityPattern::add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days) {
    for (auto current_date = start; current_date < end; current_date = current_date + boost::gregorian::days(1)) {
        navitia::weekdays week_day = navitia::get_weekday(current_date);
        if (active_days[week_day]) {
            add(current_date);
        } else {
            remove(current_date);
        }
    };
}

void ValidityPattern::remove(boost::gregorian::date date) {
    long duration = slide(date);
    remove(duration);
}

void ValidityPattern::remove(int day) {
    if (is_valid(day)) {
        days[day] = false;
    }
}

std::string ValidityPattern::str() const {
    return days.to_string();
}

bool ValidityPattern::check(boost::gregorian::date day) const {
    long duration = slide(day);
    if (duration < 0 || duration >= long(days.size())) {
        throw navitia::recoverable_exception("date " + boost::gregorian::to_iso_extended_string(day)
                                             + " out of bound, cannot check validity pattern");
    }
    return ValidityPattern::check(duration);
}

bool ValidityPattern::check(unsigned int day) const {
    //    BOOST_ASSERT(is_valid(day));
    return days[day];
}

bool ValidityPattern::check2(unsigned int day) const {
    //    BOOST_ASSERT(is_valid(day));
    if (day == 0) {
        return days[day] || days[day + 1];
    }

    return days[day - 1] || days[day] || days[day + 1];
}

bool ValidityPattern::uncheck2(unsigned int day) const {
    //    BOOST_ASSERT(is_valid(day));
    if (day == 0) {
        return !days[day] && !days[day + 1];
    }

    return !days[day - 1] && !days[day] && !days[day + 1];
}
}  // namespace type
}  // namespace navitia
