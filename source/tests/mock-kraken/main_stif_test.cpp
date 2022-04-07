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
/*
 * Here we test prepare a data for stif test
 */
int main(int argc, const char* const argv[]) {
    navitia::init_app();

    ed::builder b("20140614", [](ed::builder& b) {
        b.vj("A")("stopA", "08:00"_t)("stopB", "10:00"_t);
        b.vj("A")("stopA", "10:00"_t)("stopB", "12:00"_t);
        b.vj("B")("stopA", "09:00"_t)("stopC", "10:00"_t);
        b.vj("C")("stopC", "10:30"_t)("stopB", "11:00"_t);
        b.connection("stopC", "stopC", 0);

        b.vj("P", "11111111", "", true, "", "", "", "physical_mode:Bus")("stopP", "15:00"_t)("stopQ", "16:00"_t);
        b.vj("Q", "11111111", "", true, "", "", "", "physical_mode:Bus")("stopQ", "16:00"_t)("stopR", "17:00"_t);
        b.vj("R", "11111111", "", true, "", "", "", "physical_mode:Bus")("stopR", "17:00"_t)("stopS", "18:00"_t);
        b.vj("S", "11111111", "", true, "", "", "", "physical_mode:Bus")("stopS", "18:00"_t)("stopT", "19:00"_t);
        b.vj("T", "11111111", "", true, "", "", "", "physical_mode:Bus")("stopP", "15:00"_t)("stopT", "20:00"_t);
        b.connection("stopQ", "stopQ", 0);
        b.connection("stopR", "stopR", 0);
        b.connection("stopS", "stopS", 0);

        b.connection("stopT", "stopT", 0);
        b.vj("U", "11111111", "", true, "", "", "", "physical_mode:Tramway")("stopT", "19:00"_t)("stopU",
                                                                                                 "19:30"_t);  // Tram
        b.vj("V", "11111111", "", true, "", "", "", "physical_mode:Bus")("stopU", "19:30"_t)("stopV",
                                                                                             "20:00"_t);  // Bus
        b.vj("W", "11111111", "", true, "", "", "", "physical_mode:Bus")("stopV", "20:00"_t)("stopW",
                                                                                             "20:30"_t);  // Bus

        b.connection("stopU", "stopU", 0);
        b.connection("stopV", "stopV", 0);
        b.vj("PW", "11111111", "", true, "", "", "physical_mode:Bus")("stopP", "15:00"_t)("stopW", "21:00"_t);  // Bus

        // Add tickets
        b.add_ticket("P-Ticket", "P", 100, "P-Ticket comment");
        b.add_ticket("Q-Ticket", "Q", 100, "Q-Ticket comment");
        b.add_ticket("R-Ticket", "R", 100, "R-Ticket comment");
        b.add_ticket("S-Ticket", "S", 100, "S-Ticket comment");
        b.add_ticket("T-Ticket", "T", 99, "T-Ticket comment");
    });

    mock_kraken kraken(b, argc, argv);
    return 0;
}
