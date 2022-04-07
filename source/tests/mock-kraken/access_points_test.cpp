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
#include "mock_kraken.h"
#include "type/pt_data.h"
#include "tests/utils_test.h"

using namespace navitia::georef;

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    ed::builder b("20210101", [](ed::builder& b) {
        b.sa("A", 2.361795, 48.871871)("spA", 2.361795, 48.871871);
        b.sa("B", 2.362795, 48.872871)("spB", 2.362795, 48.872871);
        b.sa("C", 2.363795, 48.873871)("spC", 2.363795, 48.873871);
        b.vj("A")("spA", "08:00"_t)("spB", "10:00"_t);
        b.vj("B")("spC", "09:00"_t)("spD", "11:00"_t);
        b.vj("C")("spA", "10:00"_t)("spC", "12:00"_t);
    });

    b.data->complete();
    b.make();

    b.add_access_point("spA", "AP1", true, true, 10, 23, 48.874871, 2.364795, "access_point_code_1");
    b.add_access_point("spA", "AP2", true, false, 13, 26, 48.875871, 2.365795, "access_point_code_2");
    b.add_access_point("spC", "AP3", true, false, 12, 36, 48.876871, 2.366795, "access_point_code_3");
    b.add_access_point("spC", "AP2", true, false, 13, 26, 48.875871, 2.365795, "access_point_code_4");

    mock_kraken kraken(b, argc, argv);

    return 0;
}
