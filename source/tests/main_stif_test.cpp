/* Copyright © 2001-2016, Canal TP and/or its affiliates. All rights reserved.

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

#include "utils/init.h"
#include "mock_kraken.h"
#include "type/type.h"
#include "type/pt_data.h"
#include "tests/utils_test.h"

using namespace navitia::georef;
/*
 * Here we test prepare a data for stif test
 */
int main(int argc, const char* const argv[]) {
    navitia::init_app();

    ed::builder b = {"20140614"};

    b.generate_dummy_basis();

    b.vj("A")("stopA", "08:00"_t)("stopB", "10:00"_t);
    b.vj("A")("stopA", "10:00"_t)("stopB", "12:00"_t);
    b.vj("B")("stopA", "09:00"_t)("stopC", "10:00"_t);
    b.vj("C")("stopC", "10:30"_t)("stopB", "11:00"_t);
    b.connection("stopC", "stopC", 0);

    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();

    mock_kraken kraken(b, "main_stif_test", argc, argv);
    return 0;
}

