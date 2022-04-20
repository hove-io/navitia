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
 * A dataset with a positive time offset. Hongkong is UTC+08 without
 * DST.
 */
int main(int argc, const char* const argv[]) {
    navitia::init_app();

    const auto date = "20170101";
    const auto begin = boost::gregorian::date_from_iso_string(date);
    const boost::gregorian::date_period production_date = {begin, begin + boost::gregorian::years(1)};
    navitia::type::TimeZoneHandler::dst_periods timezones = {{"08:00"_t, {production_date}}};
    ed::builder b(
        "20170101",
        [](ed::builder& b) {
            /* The X line is used to test first/last departure of a line
             *
             * Times here are UTC
             * */
            b.vj("X", "11100111")
                .route("route_1")
                .name("X:vj1")
                // 06:00 UTC+08       06:30 UTC+08        23:50 UTC+08        00:30 UTC+08
                ("X_S1", "22:00"_t)("X_S2", "22:30"_t)("X_S3", "39:50"_t)("X_S4", "40:30"_t)
                .make();
            b.vj("X", "11100111")
                .route("route_1")
                .name("X:vj2")("X_S1", "22:10"_t)("X_S2", "22:40"_t)("X_S3", "40:00"_t)("X_S4", "40:40"_t)
                .make();
            b.vj("X", "11100111")
                .route("route_1")
                .name("X:vj3")("X_S1", "22:20"_t)("X_S2", "22:50"_t)("X_S3", "40:10"_t)("X_S4", "40:50"_t)
                .make();

            b.vj("X", "00011000")
                .route("route_1")
                .name("X:vj4")("X_S1", "22:05"_t)("X_S2", "22:35"_t)("X_S3", "39:55"_t)("X_S4", "40:35"_t)
                .make();
            b.vj("X", "00011000")
                .route("route_1")
                .name("X:vj5")("X_S1", "22:15"_t)("X_S2", "22:45"_t)("X_S3", "40:05"_t)("X_S4", "40:45"_t)
                .make();
            b.vj("X", "00011000")
                .route("route_1")
                .name("X:vj6")("X_S1", "22:25"_t)("X_S2", "22:55"_t)("X_S3", "40:15"_t)("X_S4", "40:55"_t)
                .make();

            b.vj("X", "11111111")
                .route("route_2")
                .name("X:vj7")("X_S4", "22:00"_t)("X_S3", "22:40"_t)("X_S2", "39:40"_t)("X_S1", "40:20"_t)
                .make();
            b.vj("X", "11111111")
                .route("route_2")
                .name("X:vj8")("X_S4", "22:05"_t)("X_S3", "22:45"_t)("X_S2", "39:45"_t)("X_S1", "40:25"_t)
                .make();

            auto line_X = b.lines.find("X")->second;

            // opening time should be the earliest departure of all vjs of the line and it's stored as local
            // time
            line_X->opening_time = boost::make_optional(boost::posix_time::duration_from_string("06:00"));
            // closing time should be the latest arrival of all vjs of the line and it's stored as local time
            line_X->closing_time = boost::make_optional(boost::posix_time::duration_from_string("00:55"));
        },
        false, "canal tp", "Asia/Hong_Kong", timezones);

    b.data->meta->production_date = bg::date_period(bg::date(2017, 1, 1), bg::days(30));

    mock_kraken kraken(b, argc, argv);

    return 0;
}
