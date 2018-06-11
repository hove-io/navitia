/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "utils/init.h"       // init_app
#include "ed/build_helper.h"  // builder
#include "tests/utils_test.h" // _t
#include "mock_kraken.h"      // mock kraken

/**
 * @brief This data aims to highlight the min_nb_journeys option on a journey request.
 *
 * The data for testing :
 *
 * start : sp1
 * stop  : sp2
 *
 * Vehicle journey with multiple stops times
 * date : 2018 03 09
 *
 *                   sp1                   sp2
 *            /-------x---------------------x--------\
 *           /      - 8:01                - 8:20      \
 *          /       - 8:02                - 8:21       \
 *   Start x        - 8:03                - 8:22        x Stop
 *         |        - 8:04                - 8:23        |
 *         |                                            |
 *         |          We add an other way to test       |
 *         \         similar journeys filtering         /
 *          \        (Faster but with more walking)    /
 *           \       sp1                   sp2        /
 *            \-------x---------------------x--------/
 *                 - 8:05                - 8:15
 */

int main(int argc, const char* const argv[]) {

    navitia::init_app();

    // Create Builder
    ed::builder b("20180309");

    // Create Area
    b.sa("stop_area:sa1")("stop_point:sa1:s1", 2.39592, 48.84848, false)("stop_point:sa1:s2", 2.39592, 48.84858, false);
    b.sa("stop_area:sa3")("stop_point:sa3:s1", 2.36381, 48.86650, false)("stop_point:sa3:s2", 2.36381, 48.86550, false);

    // jouney 1
    b.vj("A", "111111", "", false, "vj1")("stop_point:sa1:s1", "8:00"_t, "8:01"_t)("stop_point:sa3:s1", "8:20"_t, "8:21"_t);
    b.vj("A", "111111", "", false, "vj2")("stop_point:sa1:s1", "8:00"_t, "8:02"_t)("stop_point:sa3:s1", "8:21"_t, "8:22"_t);
    b.vj("A", "111111", "", false, "vj3")("stop_point:sa1:s1", "8:00"_t, "8:03"_t)("stop_point:sa3:s1", "8:22"_t, "8:23"_t);
    b.vj("A", "111111", "", false, "vj4")("stop_point:sa1:s1", "8:00"_t, "8:04"_t)("stop_point:sa3:s1", "8:23"_t, "8:24"_t);

    // Line A
    b.lines["A"]->code = "A";
	b.lines["A"]->color = "3ACCDC";
	b.lines["A"]->text_color = "FFFFFF";

    // journey 2
    b.vj("B", "111111", "", false, "vj5")("stop_point:sa1:s2", "8:00"_t, "8:05"_t)("stop_point:sa3:s2", "8:15"_t, "8:16"_t);

    // Line B
    b.lines["B"]->code = "B";
	b.lines["B"]->color = "5AC8BC";
	b.lines["B"]->text_color = "FFFFFF";

    // build data
    b.data->complete();
    b.manage_admin();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();

    // mock kraken
    mock_kraken kraken(b, "min_nb_journeys_test", argc, argv);
    return 0;
}
