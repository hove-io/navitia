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

#include "request_handle.h"

#include "ptreferential/ptreferential.h"
#include "type/meta_data.h"

namespace navitia {
namespace timetables {

RequestHandle::RequestHandle(PbCreator& pb_creator,
                             const pt::ptime datetime,
                             uint32_t duration,
                             const boost::optional<const std::string>& calendar_id,
                             const bool clockwise)
    : pb_creator(pb_creator), date_time(DateTimeUtils::inf), max_datetime(DateTimeUtils::inf) {
    if (!calendar_id) {
        // we only have to check the production period if we do not have a calendar,
        // since if we have one we are only interested in the time, not the date
        // Because of UTC and local date_time problem, we allow one more day before production_date.begin()
        if (!pb_creator.data->meta->production_date.contains(datetime.date())
            && !pb_creator.data->meta->production_date.contains(
                   (datetime + boost::posix_time::seconds(86399)).date())) {
            pb_creator.fill_pb_error(pbnavitia::Error::date_out_of_bounds, "date is out of bound");
        }
    }

    if (!pb_creator.has_error()) {
        // Because of UTC and local date_time problem, we allow one more day before production_date.begin()
        auto diff = (datetime.date() - pb_creator.data->meta->production_date.begin()).days();
        if (diff == -1) {
            date_time = DateTimeUtils::set(0, 0);
            duration = duration + datetime.time_of_day().total_seconds();
            uint32_t one_day = 24 * 60 * 60;
            duration = (duration > one_day) ? duration - one_day : 0;
        } else {
            date_time = DateTimeUtils::set((datetime.date() - pb_creator.data->meta->production_date.begin()).days(),
                                           datetime.time_of_day().total_seconds());
        }
        if (clockwise) {
            // in clockwise set max_dt to the end_date at midnight if dt + duration is bigger
            auto max_prod_dt = DateTimeUtils::set(pb_creator.data->meta->production_date.length().days(), 0);
            max_datetime = std::min(date_time + duration, max_prod_dt);
        } else {
            // in !clockwise make sure we don't substract duration to dt if it's bigger since they are unsigned
            max_datetime = duration > date_time ? 0 : date_time - duration;
        }
    }
}

void RequestHandle::init_jpp(const std::string& request, const std::vector<std::string>& forbidden_uris) {
    const auto jpp_t = type::Type_e::JourneyPatternPoint;

    try {
        const auto& jpps = ptref::make_query(jpp_t, request, forbidden_uris, *pb_creator.data);
        for (const auto idx : jpps) {
            journey_pattern_points.emplace_back(idx);
        }
        total_result = journey_pattern_points.size();
    } catch (const ptref::ptref_error& ptref_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + ptref_error.more);
    }
}

}  // namespace timetables
}  // namespace navitia
