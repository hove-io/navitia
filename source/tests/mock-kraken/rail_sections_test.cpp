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

ed::builder create_complex_data_for_rail_section() {
    /*
     *
     *          |------- J ------ K ------ L --------
     *          |                                   |
     *          |                                   |
     * A x----- B ------ C ------ D ------ E ------ F ------ G ------ H -----x I
     *                   |        |                          |
     *                   |        |                          |
     *                   |        |                          |
     *                   |        |------- M ------ N ------ O
     *                   |
     *                   P ------ Q -----x R
     *
     * route 1 : A B C D E F G H I
     * route 2 : I H G F E D C B A
     * route 3 : A B J K L F G H I
     * route 4 : A B C D M N O G H I
     * route 5 : A B C P Q R
     * route 6 : R Q P C B A
     *
     * route 11: AA BB CC DD EE FF GG
     */
    ed::builder b("20170101", [](ed::builder& b) {
        b.sa("stopAreaA", 0., 1.)("stopA", 0., 1.);
        b.sa("stopAreaB", 0., 2.)("stopB", 0., 2.);
        b.sa("stopAreaC", 0., 3.)("stopC", 0., 3.);
        b.sa("stopAreaD", 0., 4.)("stopD", 0., 4.);
        b.sa("stopAreaE", 0., 5.)("stopE", 0., 5.);
        b.sa("stopAreaF", 0., 6.)("stopF", 0., 6.);
        b.sa("stopAreaG", 0., 7.)("stopG", 0., 7.);
        b.sa("stopAreaH", 0., 8.)("stopH", 0., 8.);
        b.sa("stopAreaI", 0., 9.)("stopI", 0., 9.);
        b.sa("stopAreaJ", 0., 10.)("stopJ", 0., 10.);
        b.sa("stopAreaK", 0., 11.)("stopK", 0., 11.);
        b.sa("stopAreaL", 0., 12.)("stopL", 0., 12.);
        b.sa("stopAreaM", 0., 13.)("stopM", 0., 13.);
        b.sa("stopAreaN", 0., 14.)("stopN", 0., 14.);
        b.sa("stopAreaO", 0., 15.)("stopO", 0., 15.);
        b.sa("stopAreaP", 0., 16.)("stopP", 0., 16.);
        b.sa("stopAreaQ", 0., 17.)("stopQ", 0., 17.);
        b.sa("stopAreaR", 0., 18.)("stopR", 0., 18.);
        b.sa("stopAreaW", 0., 15.)("stopW", 0., 15.);
        b.sa("stopAreaX", 0., 16.)("stopX", 0., 16.);
        b.sa("stopAreaY", 0., 17.)("stopY", 0., 17.);
        b.sa("stopAreaZ", 0., 18.)("stopZ", 0., 18.);

        b.sa("stopAreaAA", 0., 19.)("stopAA", 0., 19.);
        b.sa("stopAreaBB", 0., 20.)("stopBB", 0., 20.);
        b.sa("stopAreaCC", 0., 21.)("stopCC", 0., 21.);
        b.sa("stopAreaDD", 0., 22.)("stopDD", 0., 22.);
        b.sa("stopAreaEE", 0., 23.)("stopEE", 0., 23.);
        b.sa("stopAreaFF", 0., 24.)("stopFF", 0., 24.);
        b.sa("stopAreaGG", 0., 25.)("stopGG", 0., 25.);
        b.vj("line:1", "111111", "", true, "vj:1")
            .route("route1")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopD", "08:15"_t)(
                "stopE", "08:20"_t)("stopF", "08:25"_t)("stopG", "08:30"_t)("stopH", "08:35"_t)("stopI", "08:40"_t);
        b.vj("line:1", "111111", "", true, "vj:2")
            .route("route2")("stopI", "08:00"_t)("stopH", "08:05"_t)("stopG", "08:10"_t)("stopF", "08:15"_t)(
                "stopE", "08:20"_t)("stopD", "08:25"_t)("stopC", "08:30"_t)("stopB", "08:35"_t)("stopA", "08:40"_t);
        b.vj("line:1", "111111", "", true, "vj:3")
            .route("route3")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopJ", "08:10"_t)("stopK", "08:15"_t)(
                "stopL", "08:20"_t)("stopF", "08:25"_t)("stopG", "08:30"_t)("stopH", "08:35"_t)("stopI", "08:40"_t);
        b.vj("line:1", "111111", "", true, "vj:4")
            .route("route4")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopD", "08:15"_t)(
                "stopM", "08:20"_t)("stopN", "08:25"_t)("stopO", "08:30"_t)("stopG", "08:35"_t)("stopH", "08:40"_t)(
                "stopI", "08:40"_t);
        b.vj("line:1", "111111", "", true, "vj:5")
            .route("route5")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopP", "08:15"_t)(
                "stopQ", "08:20"_t)("stopR", "08:25"_t);
        b.vj("line:1", "111111", "", true, "vj:6")
            .route("route6")("stopR", "08:00"_t)("stopQ", "08:05"_t)("stopP", "08:10"_t)("stopC", "08:15"_t)(
                "stopB", "08:20"_t)("stopA", "08:25"_t);
        b.vj("line:2", "111111", "", true, "vj:2-1")
            .route("route2-1")("stopW", "08:00"_t)("stopX", "08:05"_t)("stopY", "08:10"_t)("stopZ", "08:15"_t);

        b.vj("line:11", "111111", "", true, "vj:11-1")
            .route("route11-1")("stopAA", "08:00"_t)("stopBB", "08:05"_t)("stopCC", "08:10"_t)("stopDD", "08:15"_t)(
                "stopEE", "08:20"_t)("stopFF", "08:25"_t)("stopGG", "08:30"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2017, 1, 1), bg::days(30));

    return b;
}

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    /**
     * We want to impact like this


              |------- J ------ K ------ L --------
              |                                   |
              |        X        X        X        |
     A x----- B ------ C ------ D ------ E ------ F ------ G ------ H -----x I
                       |        |                          |
                       |        |                          |
                       |        |                          |
                       |        |------- M ------ N ------ O
                       |                 X        X        X
                       |
                       P ------ Q -----x R
                       X        X      X

       X -> SA blocked : C-D-E-M-N-O-P-Q-R

       It can be the representation of a broken rail between B and C for all routes with B-C section

       We can't do this with a single disruption. We need 3 Disruptions :
       -> route1 - start_point B - End point E - blocked_stop_areas C D
       -> route4 - start_point B - End point O - blocked_stop_areas C D M N
       -> route5 - start_point P - End point R - blocked_stop_areas Q
    */
    ed::builder b = create_complex_data_for_rail_section();

    b.make();
    b.build_autocomplete();

    // new rail_section disruption on route 1
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-1")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_rail_section("line:1", "stopAreaB", "stopAreaE",
                                                   {
                                                       std::make_pair("stopAreaC", 1),
                                                       std::make_pair("stopAreaD", 2),
                                                   },
                                                   {"route1"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    // new rail_section disruption on route 4
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-2")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_rail_section("line:1", "stopAreaB", "stopAreaO",
                                                   {std::make_pair("stopAreaC", 1), std::make_pair("stopAreaD", 2),
                                                    std::make_pair("stopAreaM", 3), std::make_pair("stopAreaN", 4)},
                                                   {"route4"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    // new rail_section disruption on route 5
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-3")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_rail_section("line:1", "stopAreaP", "stopAreaR", {std::make_pair("stopAreaQ", 1)},
                                                   {"route5"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // new rail_section disruption on route 11
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line11")
                                  .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_rail_section("line:11", "stopAreaCC", "stopAreaFF",
                                                   {std::make_pair("stopAreaDD", 3), std::make_pair("stopAreaEE", 4)},
                                                   {"route11-1"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    mock_kraken kraken(b, argc, argv);

    return 0;
}
