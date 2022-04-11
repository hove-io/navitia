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
#include "utils_test.h"
#include "mock_kraken.h"
#include "time_tables/tests/departure_board_test_data.h"

/*
 * tests for departure board
 *
 * Also used with realtime proxy
 *
 *          A:s
 *           v RouteA
 *           v
 *           v
 *         |-----------|      RouteB
 *B:s > > >| S1> > > > |> > > > B:e
 *         | v         |
 *         | v         |
 *         | S2        |
 *         |-----------|
 *           v         SA1
 *           v
 *           v
 *          A:e
 *
 *
 */
int main(int argc, const char* const argv[]) {
    navitia::init_app();

    departure_board_fixture data_set;

    mock_kraken kraken(data_set.b, argc, argv);

    return 0;
}
