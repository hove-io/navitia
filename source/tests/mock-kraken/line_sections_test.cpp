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

/*
  Test on when to display the line section disruptions

A           B           C           D           E           F
o --------- o --------- o --------- o --------- o --------- o   <- route:line:1:1

                        XXXXXXXXXXXXXXXXXXXXXXXXX        <- impact on route:line:1:1 and route:line:1:3 on [C, E]

  We also add another route route:line:1:2 that pass through the same stops areas but different stop points

  We also add another route route:line:1:3 that only pass on (A, B, F) and that shouldn't be impacted

  We finally add another line line_2 that pass on the same stops areas
  */

namespace bg = boost::gregorian;
using btp = boost::posix_time::time_period;

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    ed::builder b("20170101");
    b.sa("A", 0., 1.)("A_1", 0., 1.)("A_2", 0., 1.);
    b.sa("B", 0., 2.)("B_1", 0., 2.)("B_2", 0., 2.);
    b.sa("C", 0., 3.)("C_1", 0., 3.)("C_2", 0., 3.)("C_3", 0., 1.);
    b.sa("D", 0., 4.)("D_1", 0., 4.)("D_2", 0., 4.)("D_3", 0., 4.);
    b.sa("E", 0., 5.)("E_1", 0., 5.)("E_2", 0., 5.);
    b.sa("F", 0., 6.)("F_1", 0., 6.)("F_2", 0., 6.);

    b.vj("line:1", "111111111111111111111111")
        .route("route:line:1:1")
        .name("vj:1:1")("A_1", "08:00"_t)("B_1", "08:15"_t)("C_1", "08:45"_t)("D_1", "09:00"_t)("D_3", "09:05"_t)(
            "E_1", "09:15"_t)("F_1", "09:30"_t);

    b.vj("line:1", "111111111111111111111111")
        .route("route:line:1:2")
        .name("vj:1:2")("A_2", "09:00"_t)("B_2", "09:15"_t)("C_2", "09:45"_t)("D_2", "10:00"_t)("E_2", "10:15"_t)(
            "F_2", "10:30"_t);

    b.vj("line:1", "111111111111111111111111")
        .route("route:line:1:3")
        .name("vj:1:3")("A_1", "10:00"_t)("B_1", "10:15"_t)("F_1", "11:30"_t);

    b.vj("line:2", "111111111111111111111111")
        .route("route:line:2:1")
        .name("vj:2")("A_1", "11:00"_t)("B_1", "11:15"_t)("F_1", "12:30"_t);

    b.vj("line:3", "111111111111111111111111").route("route:line:3:1").name("vj:3")("C_1", "11:00"_t)("A_1", "12:30"_t);

    b.vj_with_network("network:other", "line:B", "111111111111111111111111", "", true, "")("C_3", "08:10"_t, "08:11"_t)(
        "stop_area:other", "08:20"_t, "08:21"_t);

    b.make();
    b.build_autocomplete();
    b.data->meta->production_date = bg::date_period(bg::date(2017, 1, 1), bg::days(30));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line_1")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
            .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
            .on_line_section("line:1", "C", "E", {"route:line:1:1", "route:line:1:3"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line_1_other_effect")
            .severity(nt::disruption::Effect::OTHER_EFFECT)
            .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
            .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
            .on_line_section("line:1", "E", "F", {"route:line:1:1", "route:line:1:3"}, *b.data->pt_data)
            .on_line_section("line:1", "F", "E", {"route:line:1:1", "route:line:1:3"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line_1_one_stop_detour")
            .severity(nt::disruption::Effect::DETOUR)
            .application_periods(btp("20170106T000000"_dt, "20170108T000000"_dt))
            .publish(btp("20170106T000000"_dt, "20170108T000000"_dt))
            .on_line_section("line:1", "B", "B", {"route:line:1:1", "route:line:1:3"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line_1_two_stop_reduced")
            .severity(nt::disruption::Effect::REDUCED_SERVICE)
            .application_periods(btp("20170109T000000"_dt, "20170111T000000"_dt))
            .publish(btp("20170109T000000"_dt, "20170111T000000"_dt))
            .on_line_section("line:1", "C", "D", {"route:line:1:1", "route:line:1:3"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line_section_on_line_2")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_line_section("line:2", "B", "F", {"route:line:2:1"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    mock_kraken kraken(b, argc, argv);

    return 0;
}
