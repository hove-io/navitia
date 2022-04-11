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
#include "type_interfaces.h"

#include <boost/date_time/gregorian/gregorian.hpp>

#include <bitset>

namespace navitia {
namespace type {

struct ValidityPattern : public Header {
    const static Type_e type = Type_e::ValidityPattern;

private:
    bool is_valid(int day) const;

public:
    using year_bitset = std::bitset<366>;
    year_bitset days;
    boost::gregorian::date beginning_date;

    ValidityPattern() = default;
    ValidityPattern(const boost::gregorian::date& beginning_date) : beginning_date(beginning_date) {}
    ValidityPattern(const boost::gregorian::date& beginning_date, const std::string& vp)
        : days(vp), beginning_date(beginning_date) {}

    int slide(boost::gregorian::date day) const;
    void add(boost::gregorian::date day);
    void add(int duration);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date date);
    void remove(int day);
    void reset() { days.reset(); }
    bool empty() const { return days.none(); }
    std::string str() const;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int);

    bool check(boost::gregorian::date day) const;
    bool check(unsigned int day) const;
    bool check2(unsigned int day) const;
    bool uncheck2(unsigned int day) const;
    // void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    bool operator<(const ValidityPattern& other) const { return this < &other; }
    bool operator==(const ValidityPattern& other) const {
        return (this->beginning_date == other.beginning_date) && (this->days == other.days);
    }
};

}  // namespace type
}  // namespace navitia
