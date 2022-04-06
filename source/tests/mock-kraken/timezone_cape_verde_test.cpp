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

#include "utils/init.h"
#include "ed/build_helper.h"
#include "kraken/apply_disruption.h"
#include "mock_kraken.h"
#include "tests/utils_test.h"

namespace bg = boost::gregorian;
using btp = boost::posix_time::time_period;

/*
 * A dataset with a negative time offset. Cape Verde is UTC-01 without
 * DST.
 */
int main(int argc, const char* const argv[]) {
    navitia::init_app();

    const auto date = "20170101";
    const auto begin = boost::gregorian::date_from_iso_string(date);
    const boost::gregorian::date_period production_date = {begin, begin + boost::gregorian::years(1)};
    navitia::type::TimeZoneHandler::dst_periods timezones = {{-static_cast<int32_t>("01:00"_t), {production_date}}};
    ed::builder b(
        "20170101",
        [](ed::builder& b) {
            // UTC and local pass midnight vj
            b.vj("line:1", "000010")
                .name("vj:1:1")("A", "23:00"_t)("B", "23:30"_t)("C", "24:00"_t)("D", "24:30"_t)("E", "25:00"_t)(
                    "F", "25:30"_t)("G", "26:00"_t);

            // Only local pass midnight vj
            b.vj("line:1", "000100")
                .name("vj:1:2")("C", "00:00"_t)("D", "00:30"_t)("E", "01:00"_t)("F", "01:30"_t)("G", "02:00"_t);

            /* The X line is used to test first/last departure of a line
             *
             * Times here are UTC
             * */
            b.vj("X", "11100111")
                .route("route_1")
                .name("X:vj1")("X_S1", "08:00"_t)("X_S2", "16:00"_t)("X_S3", "24:30"_t)("X_S4", "24:40"_t)
                .make();
            b.vj("X", "11100111")
                .route("route_1")
                .name("X:vj2")("X_S1", "09:00"_t)("X_S2", "17:00"_t)("X_S3", "25:30"_t)("X_S4", "25:40"_t)
                .make();
            b.vj("X", "11100111")
                .route("route_1")
                .name("X:vj3")("X_S1", "10:00"_t)("X_S2", "18:00"_t)("X_S3", "26:30"_t)("X_S4", "26:40"_t)
                .make();

            b.vj("X", "00011000")
                .route("route_1")
                .name("X:vj4")("X_S1", "08:05"_t)("X_S2", "16:05"_t)("X_S3", "24:45"_t)("X_S4", "24:55"_t)
                .make();
            b.vj("X", "00011000")
                .route("route_1")
                .name("X:vj5")("X_S1", "09:05"_t)("X_S2", "17:05"_t)("X_S3", "25:45"_t)("X_S4", "25:55"_t)
                .make();
            b.vj("X", "00011000")
                .route("route_1")
                .name("X:vj6")("X_S1", "10:05"_t)("X_S2", "18:05"_t)("X_S3", "26:45"_t)("X_S4", "26:55"_t)
                .make();

            b.vj("X", "11111111")
                .route("route_2")
                .name("X:vj7")("X_S4", "08:55"_t)("X_S3", "09:05"_t)("X_S2", "17:05"_t)("X_S1", "25:45"_t)
                .make();
            b.vj("X", "11111111")
                .route("route_2")
                .name("X:vj8")("X_S4", "09:55"_t)("X_S3", "10:05"_t)("X_S2", "18:05"_t)("X_S1", "26:45"_t)
                .make();

            auto line_X = b.lines.find("X")->second;

            // the opening time should be the earliest departure of all vjs of the line and it's stored as
            // local time
            line_X->opening_time = boost::make_optional(boost::posix_time::duration_from_string("07:00"));
            // the closing time should be the latest arrival of all vjs of the line and it's stored as local
            // time
            line_X->closing_time = boost::make_optional(boost::posix_time::duration_from_string("26:55"));

            b.build_autocomplete();
        },
        false, "hove", "Atlantic/Cape_Verde", timezones);

    b.data->meta->production_date = bg::date_period(bg::date(2017, 1, 1), bg::days(30));

    mock_kraken kraken(b, argc, argv);

    return 0;
}
