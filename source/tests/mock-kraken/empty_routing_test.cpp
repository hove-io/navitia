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
#include "routing/tests/routing_api_test_data.h"
#include "mock_kraken.h"

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    // for some tests we want an empty region but with a bounding box
    ed::builder b = {"20120614", ed::builder::make_builder, true, "empty routing"};

    routing_api_data<normal_speed_provider> routing_data;

    // we give a very large bounding box (it must overlap with the shape of main_routing_test)
    std::stringstream ss;
    ss << "POLYGON((" << routing_data.S.lon() - 1 << " " << routing_data.S.lat() - 1 << ", " << routing_data.S.lon() - 1
       << " " << routing_data.R.lat() + 1e-3 << ", " << routing_data.R.lon() + 1e-3 << " "
       << routing_data.R.lat() + 1e-3 << ", " << routing_data.R.lon() + 1e-3 << " " << routing_data.S.lat() - 1 << ", "
       << routing_data.S.lon() - 1 << " " << routing_data.S.lat() - 1 << "))";
    b.data->meta->shape = ss.str();

    mock_kraken kraken(b, argc, argv);

    return 0;
}
