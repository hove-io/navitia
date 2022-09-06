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
     * route 11-1: AA BB CC DD EE FF GG HH II
     * route 11-2: II HH GG FF EE DD CC BB AA
     *
     * route 100: A1 B1 C1 D1 E1 F1 G1 H1 I1
     * route 101-1: P1 Q1 R1 S1 T1 U1 V1
     * route 101-2: V1 U1 T1 S1 R1 Q1 P1
     *
     *
     *
     * line RER-B : denfert - port_royal - luxembourg - chatelet - gare_du_nord - aulnay - villepinte - parc_des_expos -
     * cdg
     *
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
        b.sa("stopAreaHH", 0., 26.)("stopHH", 0., 26.);
        b.sa("stopAreaHH", 0., 27.)("stopHH", 0., 27.);

        b.sa("stopAreaA1", 0., 30.)("stopA1", 0., 30.);
        b.sa("stopAreaB1", 0., 31.)("stopB1", 0., 31.);
        b.sa("stopAreaC1", 0., 32.)("stopC1", 0., 32.);
        b.sa("stopAreaD1", 0., 33.)("stopD1", 0., 33.);
        b.sa("stopAreaE1", 0., 34.)("stopE1", 0., 34.);
        b.sa("stopAreaF1", 0., 35.)("stopF1", 0., 35.);
        b.sa("stopAreaG1", 0., 36.)("stopG1", 0., 36.);
        b.sa("stopAreaH1", 0., 37.)("stopH1", 0., 37.);
        b.sa("stopAreaI1", 0., 38.)("stopI1", 0., 38.);

        b.sa("stopAreaP1", 0., 40.)("stopP1", 0., 40.);
        b.sa("stopAreaQ1", 0., 41.)("stopQ1", 0., 41.);
        b.sa("stopAreaR1", 0., 42.)("stopR1", 0., 42.);
        b.sa("stopAreaS1", 0., 43.)("stopS1", 0., 43.);
        b.sa("stopAreaT1", 0., 44.)("stopT1", 0., 44.);
        b.sa("stopAreaU1", 0., 45.)("stopU1", 0., 45.);
        b.sa("stopAreaV1", 0., 46.)("stopV1", 0., 46.);
        b.sa("stopAreaW1", 0., 88.)("stopW1", 0., 88.);

        b.sa("denfert_area", 0., 47.)("denfert", 0., 47.);
        b.sa("port_royal_area", 0., 48.)("port_royal", 0., 48.);
        b.sa("luxembourg_area", 0., 49.)("luxembourg", 0., 49.);
        b.sa("chatelet_area", 0., 50.)("chatelet", 0., 50.);
        b.sa("gare_du_nord_area", 0., 51.)("gare_du_nord", 0., 51.);
        b.sa("aulnay_area", 0., 52.)("aulnay", 0., 52.);
        b.sa("villepinte_area", 0., 53.)("villepinte", 0., 53.);
        b.sa("parc_des_expos_area", 0., 54.)("parc_des_expos", 0., 54.);
        b.sa("cdg_area", 0., 55.)("cdg", 0., 55.);
        b.sa("denfert_area", 0., 56.)("denfert", 0., 56.);

        b.vj("line:1", "111111111111", "", true, "vj:1")
            .route("route1")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopD", "08:15"_t)(
                "stopE", "08:20"_t)("stopF", "08:25"_t)("stopG", "08:30"_t)("stopH", "08:35"_t)("stopI", "08:40"_t);
        b.vj("line:1", "111111111111", "", true, "vj:2")
            .route("route2")("stopI", "08:00"_t)("stopH", "08:05"_t)("stopG", "08:10"_t)("stopF", "08:15"_t)(
                "stopE", "08:20"_t)("stopD", "08:25"_t)("stopC", "08:30"_t)("stopB", "08:35"_t)("stopA", "08:40"_t);
        b.vj("line:1", "111111111111", "", true, "vj:3")
            .route("route3")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopJ", "08:10"_t)("stopK", "08:15"_t)(
                "stopL", "08:20"_t)("stopF", "08:25"_t)("stopG", "08:30"_t)("stopH", "08:35"_t)("stopI", "08:40"_t);
        b.vj("line:1", "111111111111", "", true, "vj:4")
            .route("route4")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopD", "08:15"_t)(
                "stopM", "08:20"_t)("stopN", "08:25"_t)("stopO", "08:30"_t)("stopG", "08:35"_t)("stopH", "08:40"_t)(
                "stopI", "08:40"_t);
        b.vj("line:1", "111111111111", "", true, "vj:5")
            .route("route5")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopP", "08:15"_t)(
                "stopQ", "08:20"_t)("stopR", "08:25"_t);
        b.vj("line:1", "111111111111", "", true, "vj:6")
            .route("route6")("stopR", "08:00"_t)("stopQ", "08:05"_t)("stopP", "08:10"_t)("stopC", "08:15"_t)(
                "stopB", "08:20"_t)("stopA", "08:25"_t);
        b.vj("line:2", "111111111111", "", true, "vj:2-1")
            .route("route2-1")("stopW", "08:00"_t)("stopX", "08:05"_t)("stopY", "08:10"_t)("stopZ", "08:15"_t);

        b.vj("line:11", "111111111111", "", true, "vj:11-1")
            .route("route11-1")("stopAA", "08:00"_t)("stopBB", "08:05"_t)("stopCC", "08:10"_t)("stopDD", "08:15"_t)(
                "stopEE", "08:20"_t)("stopFF", "08:25"_t)("stopGG", "08:30"_t)("stopHH", "08:35"_t)("stopII",
                                                                                                    "08:40"_t);
        b.vj("line:11", "111111111111", "", true, "vj:11-2")
            .route("route11-2")("stopII", "08:00"_t)("stopHH", "08:05"_t)("stopGG", "08:10"_t)("stopFF", "08:15"_t)(
                "stopEE", "08:20"_t)("stopDD", "08:25"_t)("stopCC", "08:30"_t)("stopBB", "08:35"_t)("stopAA",
                                                                                                    "08:40"_t);

        b.vj("line:100", "111111111111", "", true, "vj:100-1")
            .route("route100-1")("stopA1", "08:00"_t)("stopB1", "08:05"_t)("stopC1", "08:10"_t)("stopD1", "08:15"_t)(
                "stopE1", "08:20"_t)("stopF1", "08:25"_t)("stopG1", "08:30"_t)("stopH1", "08:35"_t)("stopI1",
                                                                                                    "08:40"_t);

        b.vj("line:101", "111111111111", "", true, "vj:101-1")
            .route("route101-1")("stopP1", "08:00"_t)("stopQ1", "08:05"_t)("stopR1", "08:10"_t)("stopS1", "08:15"_t)(
                "stopT1", "08:20"_t)("stopU1", "08:25"_t)("stopV1", "08:30"_t)("stopW1", "08:35"_t);
        b.vj("line:101", "111111", "", true, "vj:101-2")
            .route("route101-2")("stopW1", "07:55"_t)("stopV1", "08:00"_t)("stopU1", "08:05"_t)("stopT1", "08:10"_t)(
                "stopS1", "08:15"_t)("stopR1", "08:20"_t)("stopQ1", "08:25"_t)("stopP1", "08:30"_t);

        b.vj("line:RER-B", "111111111111", "", true, "vj:rer_b_nord")
            .route("route_rer_b_nord")("denfert", "08:00"_t)("port_royal", "08:05"_t)("luxembourg", "08:10"_t)(
                "chatelet", "08:15"_t)("gare_du_nord", "08:20"_t)("aulnay", "08:25"_t)("villepinte", "08:30"_t)(
                "parc_des_expos", "08:35"_t)("cdg", "08:40"_t);
        b.vj("line:RER-B", "111111111111", "", true, "vj:rer_b_sud")
            .route("route_rer_b_sud")("cdg", "08:00"_t)("parc_des_expos", "08:05"_t)("villepinte", "08:10"_t)(
                "aulnay", "08:15"_t)("gare_du_nord", "08:20"_t)("chatelet", "08:25"_t)("luxembourg", "08:30"_t)(
                "port_royal", "08:35"_t)("denfert", "08:40"_t);
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
              |        X        X        e        |
     A x----- B ------ C ------ D ------ E ------ F ------ G ------ H -----x I
              s        |        |                          |
                       |        |                          |
                       |        |                          |
                       |        |------- M ------ N ------ O
                       |                 X        X        e
                       |
                       P ------ Q -----x R
                       s        X      e

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

    // new impact with two rail_sections and severity level REDUCED_SERVICE on route11-1 and route11-2
    // route11-1: AA  BB  CC DD  EE  FF  GG HH II
    // rail_section: Start CC - End FF / Blocked: DD, EE
    // route11-2: II  HH  GG FF  EE  DD  CC BB AA
    // rail_section: Start FF - End CC / Blocked: EE, DD
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line11")
                                  .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_rail_section("line:11", "stopAreaCC", "stopAreaFF",
                                                   {std::make_pair("stopAreaDD", 3), std::make_pair("stopAreaEE", 4)},
                                                   {"route11-1"}, *b.data->pt_data)
                                  .on_rail_section("line:11", "stopAreaFF", "stopAreaCC",
                                                   {std::make_pair("stopAreaDD", 4), std::make_pair("stopAreaEE", 3)},
                                                   {"route11-2"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // new impact with two rail_sections and severity level DETOUR on route11-1 and route11-2
    // route11-1: AA  BB  CC DD  EE  FF  GG HH II
    // rail_section: Start CC - End FF / Blocked: DD, EE
    // route11-2: II  HH  GG FF  EE  DD  CC BB AA
    // rail_section: Start FF - End CC / Blocked: EE, DD
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_bis_on_line11")
                                  .severity(nt::disruption::Effect::DETOUR)
                                  .application_periods(btp("20170106T000000"_dt, "20170110T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_rail_section("line:11", "stopAreaCC", "stopAreaFF",
                                                   {std::make_pair("stopAreaDD", 3), std::make_pair("stopAreaEE", 4)},
                                                   {"route11-1"}, *b.data->pt_data)
                                  .on_rail_section("line:11", "stopAreaFF", "stopAreaCC",
                                                   {std::make_pair("stopAreaDD", 4), std::make_pair("stopAreaEE", 3)},
                                                   {"route11-2"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // new rail_section disruption NO_SERVICE on route 100
    // A1  B1  C1 D1  E1  F1  G1 H1 I1
    // Start C1
    // End   F1
    // Blocked: D1, E1
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line100")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_rail_section("line:100", "stopAreaC1", "stopAreaF1",
                                                   {std::make_pair("stopAreaD1", 1), std::make_pair("stopAreaE1", 2)},
                                                   {"route100-1"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // new impact with two rail_sections and severity level NO_SERVICE on route101-1 and route101-2
    // route101-1: P1   Q1  R1  S1  T1  U1  V1  W1
    // rail_section: Start S1 / End   V1 / Blocked: T1, U1
    // route101-2: W1   V1  U1  T1  S1  R1  Q1  P1
    // rail_section: Start V1 / End   S1 / Blocked: U1, T1
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line101")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_rail_section("line:101", "stopAreaS1", "stopAreaV1",
                                                   {std::make_pair("stopAreaT1", 1), std::make_pair("stopAreaU1", 2)},
                                                   {"route101-1"}, *b.data->pt_data)
                                  .on_rail_section("line:101", "stopAreaV1", "stopAreaS1",
                                                   {std::make_pair("stopAreaT1", 2), std::make_pair("stopAreaU1", 1)},
                                                   {"route101-2"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // rail_section NO_SERVICE on rer B between gare_du_nord and parc_des_expos on both routes
    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "rail_section_on_rer_b")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
            .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
            .on_rail_section("line:RER-B", "gare_du_nord_area", "parc_des_expos_area",
                             {std::make_pair("aulnay_area", 1), std::make_pair("villepinte_area", 2)},
                             {"route_rer_b_nord"}, *b.data->pt_data)
            .on_rail_section("line:RER-B", "parc_des_expos_area", "gare_du_nord_area",
                             {std::make_pair("aulnay_area", 2), std::make_pair("villepinte_area", 1)},
                             {"route_rer_b_sud"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    // line_section NO_SERVICE on rer B on port_royal
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line_section_on_rer_b_port_royal")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20170101T000000"_dt, "20170105T000000"_dt))
                                  .publish(btp("20170101T000000"_dt, "20170110T000000"_dt))
                                  .on_line_section("line:RER-B", "port_royal_area", "port_royal_area",
                                                   {"route_rer_b_sud", "route_rer_b_nord"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    mock_kraken kraken(b, argc, argv);

    return 0;
}
