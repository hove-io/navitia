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

#include "type/calendar.h"
#include "type/line.h"
#include "type/pt_data.h"
#include "type/serialization.h"

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
namespace navitia {
namespace type {

Calendar::Calendar(boost::gregorian::date beginning_date) : validity_pattern(beginning_date) {}

template <class Archive>
void Calendar::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& name& idx& uri& week_pattern& active_periods& exceptions& validity_pattern;
}
SERIALIZABLE(Calendar)

template <class Archive>
void AssociatedCalendar::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& calendar& exceptions;
}
SERIALIZABLE(AssociatedCalendar)

void Calendar::build_validity_pattern(boost::gregorian::date_period production_period) {
    // initialisation of the validity pattern from the active periods and the exceptions
    for (boost::gregorian::date_period period : this->active_periods) {
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
        if (exd.type == ExceptionDate::ExceptionType::sub) {
            validity_pattern.remove(exd.date);
        } else if (exd.type == ExceptionDate::ExceptionType::add) {
            validity_pattern.add(exd.date);
        }
    }
}

Indexes Calendar::get(Type_e type, const PT_Data& data) const {
    Indexes result;
    switch (type) {
        case Type_e::Line: {
            // if the method is slow, adding a list of lines in calendar
            for (Line* line : data.lines) {
                for (Calendar* cal : line->calendar_list) {
                    if (cal == this) {
                        result.insert(line->idx);
                        break;
                    }
                }
            }
        } break;
        default:
            break;
    }
    return result;
}

std::string to_string(ExceptionDate::ExceptionType t) {
    switch (t) {
        case ExceptionDate::ExceptionType::add:
            return "Add";
        case ExceptionDate::ExceptionType::sub:
            return "Sub";
        default:
            throw navitia::exception("unhandled exception type");
    }
}

}  // namespace type
}  // namespace navitia
