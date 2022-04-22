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
#include "type/type_interfaces.h"
#include "type/validity_pattern.h"

#include <utils/exception.h>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <vector>
#include <string>
#include <bitset>

namespace navitia {
namespace type {
struct ExceptionDate {
    enum class ExceptionType {
        sub = 0,  // remove
        add = 1   // add
    };
    ExceptionType type;
    boost::gregorian::date date;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& type& date;
    }
    inline bool operator<(const ExceptionDate& that) const {
        if (this->type < that.type)
            return true;
        if (that.type < this->type)
            return false;
        return this->date < that.date;
    }
    inline bool operator==(const ExceptionDate& that) const {
        return this->type == that.type && this->date == that.date;
    }
};
inline std::ostream& operator<<(std::ostream& os, const ExceptionDate& ed) {
    switch (ed.type) {
        case ExceptionDate::ExceptionType::add:
            os << "excl ";
            break;
        case ExceptionDate::ExceptionType::sub:
            os << "incl ";
            break;
    }
    return os << ed.date;
}

std::string to_string(ExceptionDate::ExceptionType t);

inline ExceptionDate::ExceptionType to_exception_type(const std::string& str) {
    if (str == "Add") {
        return ExceptionDate::ExceptionType::add;
    }
    if (str == "Sub") {
        return ExceptionDate::ExceptionType::sub;
    }
    throw navitia::exception("unhandled exception type: " + str);
}

struct Calendar : public Nameable, public Header {
    const static Type_e type = Type_e::Calendar;
    navitia::type::WeekPattern week_pattern;
    std::vector<boost::gregorian::date_period> active_periods;
    std::vector<ExceptionDate> exceptions;

    ValidityPattern validity_pattern;  // computed validity pattern

    Calendar() = default;
    Calendar(boost::gregorian::date beginning_date);

    // we limit the validity pattern to the production period
    void build_validity_pattern(boost::gregorian::date_period production_period);

    bool operator<(const Calendar& other) const { return this->uri < other.uri; }

    Indexes get(Type_e type, const PT_Data& data) const;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};
struct AssociatedCalendar {
    /// calendar matched
    const Calendar* calendar;

    /// exceptions to this association (not to be mixed up with the exceptions in the calendar)
    /// the calendar exceptions change it's validity pattern
    /// the AssociatedCalendar exceptions are the differences between the vj validity pattern and the calendar's
    std::vector<ExceptionDate> exceptions;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

}  // namespace type
}  // namespace navitia
