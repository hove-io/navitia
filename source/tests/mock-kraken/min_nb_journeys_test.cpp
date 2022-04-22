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

#include "utils/init.h"        // init_app
#include "ed/build_helper.h"   // builder
#include "tests/utils_test.h"  // _t
#include "mock_kraken.h"       // mock kraken

/**
 * @brief This data aims to highlight the min_nb_journeys option on a journey request.
 *
 * The data for testing :
 *
 *
 * Vehicle journey with multiple stops times
 * date : 2018 03 09
 *
 *                sa1:sp1   (A Line & C line)   sa3:sp1
 *            /-------x--------------------------x--------\
 *           /                                             \
 *          /                                               \
 *   Start x                                                 x Stop
 *          \                                               /
 *           \    sa1:sp2                       sa3:sp2    /
 *            \-------x--------------------------x--------/
 *                          (B Line)
 *
 *
 *  A line validity pattern 00111111 :
 *  - 08:01:00       - 08:20:00
 *  - 08:02:00       - 08:21:00
 *  - 08:03:00       - 08:22:00
 *  - 08:04:00       - 08:23:00
 *
 *
 *  B line validity pattern 00111111 :
 *  - 08:05:00       - 08:15:00
 *
 *
 *  C line validity pattern 01000000 :
 *  - 08:00:00      - 08:05:00
 *  - 08:10:00      - 08:15:00
 *  - 08:20:00      - 08:25:00
 *  - 08:30:00      - 08:35:00
 *  - 08:40:00      - 08:45:00
 *  - ... x20
 *  - 08:00:00      - 08:05:00 (24H after the first)
 *
 */

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    // Create Builder
    ed::builder b("20180309");

    // Create Area
    b.sa("stop_area:sa1")("stop_point:sa1:s1", 2.39592, 48.84848, false)("stop_point:sa1:s2", 2.39592, 48.84858, false);
    b.sa("stop_area:sa3")("stop_point:sa3:s1", 2.36381, 48.86650, false)("stop_point:sa3:s2", 2.36381, 48.86550, false);

    // jouney 1
    b.vj("A", "111111", "", false, "vj1")("stop_point:sa1:s1", "8:00"_t, "8:01"_t)("stop_point:sa3:s1", "8:20"_t,
                                                                                   "8:21"_t);
    b.vj("A", "111111", "", false, "vj2")("stop_point:sa1:s1", "8:00"_t, "8:02"_t)("stop_point:sa3:s1", "8:21"_t,
                                                                                   "8:22"_t);
    b.vj("A", "111111", "", false, "vj3")("stop_point:sa1:s1", "8:00"_t, "8:03"_t)("stop_point:sa3:s1", "8:22"_t,
                                                                                   "8:23"_t);
    b.vj("A", "111111", "", false, "vj4")("stop_point:sa1:s1", "8:00"_t, "8:04"_t)("stop_point:sa3:s1", "8:23"_t,
                                                                                   "8:24"_t);

    // Line A
    b.lines["A"]->code = "A";
    b.lines["A"]->color = "3ACCDC";
    b.lines["A"]->text_color = "FFFFFF";

    // journey 2
    // We add an other way to test similar journeys filtering
    // (Faster but with more walking)

    // Line B
    b.vj("B", "111111", "", false, "vj5")("stop_point:sa1:s2", "8:00"_t, "8:05"_t)("stop_point:sa3:s2", "8:15"_t,
                                                                                   "8:16"_t);
    b.lines["B"]->code = "B";
    b.lines["B"]->color = "5AC8BC";
    b.lines["B"]->text_color = "FFFFFF";

    // journey 3
    // adding new vjs to test timeframe_duration and max_nb_journeys
    // All those vj start their service on 20180315

    // Line C
    auto dep_time = "08:00:00"_t;
    auto arr_time = "08:05:00"_t;
    for (int nb = 0; nb < 20; ++nb) {
        b.vj("C", "01000000", "", false, "vjC_" + std::to_string(nb))(
            "stop_point:sa1:s1", dep_time + nb * "00:10:00"_t)("stop_point:sa3:s1", arr_time + nb * "00:10:00"_t);
    }
    // Add a VJ 24H after to test the time frame duration max limit (24H)
    b.vj("C", "10000000", "", false, "vjC_out_of_24_bound")("stop_point:sa1:s1", "08:00:00"_t)("stop_point:sa3:s1",
                                                                                               "08:05:00"_t);
    b.lines["C"]->code = "C";
    b.lines["C"]->color = "AAC2B1";
    b.lines["C"]->text_color = "FFFFFF";

    // journey 3
    b.vj("N", "111111", "", false, "vj6")("stop_point:sa1:s1", "15:00"_t, "15:01"_t)("stop_point:sa3:s1", "15:04"_t,
                                                                                     "15:05"_t);

    // Line N
    b.lines["N"]->code = "N";
    b.lines["N"]->color = "BC5AC8";
    b.lines["N"]->text_color = "FFFFFF";

    // Add tickets
    b.add_ticket("A-Ticket", "A", 100, "A-Ticket comment");
    b.add_ticket("B-Ticket", "B", 120, "B-Ticket comment");

    // build data
    b.data->complete();
    b.manage_admin();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->build_uri();

    // mock kraken
    mock_kraken kraken(b, argc, argv);
    return 0;
}
