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

#include "utils/init.h"
#include "ed/build_helper.h"
#include "kraken/apply_disruption.h"
#include "mock_kraken.h"
#include "tests/utils_test.h"

/*
  Test on when to display the line section disruptions

A           B           C           D           E           F
o --------- o --------- o --------- o --------- o --------- o   <- route_1_of_the_line_1

                        XXXXXXXXXXXXXXXXXXXXXXXXX               <- impact on the route_1_of_the_line_1 on [C, E]

  We also add another route route_2_of_the_line_1 that pass through the same stops areas but different stop points
  */

namespace bg = boost::gregorian;
using btp = boost::posix_time::time_period;

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    ed::builder b("20170101");
    b.sa("A")("A_1")("A_2");
    b.sa("B")("B_1")("B_2");
    b.sa("C")("C_1")("C_2");
    b.sa("D")("D_1")("D_2");
    b.sa("E")("E_1")("E_2");
    b.sa("F")("F_1")("F_2");

    b.vj("line:1")
            .route("route:line:1:1")
            .uri("vj:1:1")
            ("A_1", "08:00"_t)
            ("B_1", "08:15"_t)
            ("C_1", "08:45"_t)
            ("D_1", "09:00"_t)
            ("E_1", "09:15"_t)
            ("F_1", "09:30"_t);

    b.vj("line:1")
            .route("route:line:1:2")
            .uri("vj:1:2")
            ("A_2", "08:00"_t)
            ("B_2", "08:15"_t)
            ("C_2", "08:45"_t)
            ("D_2", "09:00"_t)
            ("E_2", "09:15"_t)
            ("F_2", "09:30"_t);

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2017, 1, 1), bg::days(30));

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line_section_on_line_1")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .application_periods(btp("20170101T000000"_dt, "20170106T090000"_dt))
                              .publish(btp("20170101T000000"_dt, "20170110T090000"_dt))
                              .on_line_section("line:1", "C", "E", {"route:line:1:1"})
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    mock_kraken kraken(b, "line_sections_test", argc, argv);

    return 0;
}
