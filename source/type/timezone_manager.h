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
#include "validity_pattern.h"

#include <boost/date_time/gregorian/gregorian.hpp>

namespace navitia {
namespace type {

struct MetaData;
class TimeZoneHandler {
    /*
     * we store on the production period all the times changes,
     * the periods are represented by validity patterns
     *
     *
     *              production period
     *       [------------------------------------]
     *       [  DST    )[    non DST     )[   DST )
     * vp1:  ***********                  *********      <-- offset n
     * vp2:             ******************               <-- offset m
     *
     * offsets are in seconds
     */
    std::vector<std::pair<const ValidityPattern, int32_t>> time_changes;

public:
    std::string tz_name;
    using dst_periods = std::map<int32_t, std::vector<boost::gregorian::date_period>>;
    TimeZoneHandler(std::string name, const boost::gregorian::date& production_period_beg, const dst_periods&);
    TimeZoneHandler() = default;
    int32_t get_utc_offset(boost::gregorian::date day) const;
    int32_t get_utc_offset(int day) const;
    int32_t get_utc_offset(const ValidityPattern& vp) const;
    dst_periods get_periods_and_shift() const;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& tz_name& time_changes;
    }
};

class TimeZoneManager {
    std::map<std::string, std::unique_ptr<TimeZoneHandler>> tz_handlers;

public:
    const TimeZoneHandler* get_or_create(const std::string&,
                                         const boost::gregorian::date& production_period_beg,
                                         const std::map<int32_t, std::vector<boost::gregorian::date_period>>&);

    const TimeZoneHandler* get(const std::string& name) const;
    const TimeZoneHandler* get_first_timezone() const;
    size_t get_nb_timezones() const { return tz_handlers.size(); }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& tz_handlers;
    }
};

}  // namespace type
}  // namespace navitia
