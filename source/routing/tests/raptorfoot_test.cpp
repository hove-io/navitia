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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>

#include "routing/raptor.h"
#include "street_network/types.h"
#include "ed/build_helper.h"

using namespace navitia;
using namespace routing::raptor;

/**
 *
 *            9 de marche  B(8h20)
 *           ______________________x_____________
 *          /                                    \
 *         /                                      \
 *        /                                        \ Z(8h30)
 * A(8h) x                                          x
 *        \                                        / Z(9h30)
 *         \                                      /
 *          \______________________x_____________/
 *           1  de marche    C(8h10)
 *
 *
 *     Je veux remonter 2 solutions : 1) Depart de B(8h20), arrivée en Z à 8h30, 10 minutes de marche
 *                                    2) Depart de C(8h10), arrivée en Z à 9h03, 1 minute de marche
 *
 */
BOOST_AUTO_TEST_CASE(marche_depart) {
    navitia::streetnetwork::StreetNetwork sn;
    navitia::streetnetwork::GraphBuilder bsn(sn);

    navitia::type::GeographicalCoord A(0, 0);
    navitia::type::GeographicalCoord B(0.005, 0.005);
    navitia::type::GeographicalCoord C(-0.001, -0.001);
    navitia::type::GeographicalCoord D(0.005, 0.0010);

    navitia::type::GeographicalCoord Z(0.0020, 0);

    bsn("A", A)("B", B)("C", C)("D", D)("Z", Z);
    bsn("A", "B")("B", "A")("A", "C")("C", "A")("A", "D")("D", "A")("Z", "Z");

    ed::builder b("20120614");
    b.sa("B", B);
    b.sa("C", C);
    b.sa("Z", Z);
    b.vj("t1")("B", 8 * 3600 + 60 * 20)("Z", 8 * 3600 + 30 * 60);
    b.vj("t2")("C", 8 * 3600 + 60 * 10)("Z", 9 * 3600 + 30 * 60);
}

/**
 *
 *
 *
 *
 *          B(8h10)    10 min de marche      C(8h30)
 *          x______________________________x
 *   8h5   /                                \
 *        /                                  \ Z(8h45)
 * A(8h) x                                    x
 *       \                                   / Z(9h45)
 *   8h10 \                                 /
 *         \_______________________________/
 *
 *
 *
 *
 * Je veux remonter deux solutions : 1) Départ de A(8h05), arrivée en Z(8h45), temps de marche : 1 minutes, 1
 * correspondance 2) Départ de A(8h10), arrivee en Z(9h45), temps de marche : 0 minute, 0 correspondance
 *
 */

BOOST_AUTO_TEST_CASE(marche_milieu) {
    navitia::streetnetwork::StreetNetwork sn;
    navitia::streetnetwork::GraphBuilder bsn(sn);

    navitia::type::GeographicalCoord A(0, 0);
    navitia::type::GeographicalCoord B(0.0020, 0.0020);
    navitia::type::GeographicalCoord C(0.0010, 0.0010);
    navitia::type::GeographicalCoord Z(0.0020, 0);

    bsn("A", A)("B", B)("C", C)("Z", Z);
    bsn("A", "A")("C", "C")("B", "B")("Z", "Z");

    ed::builder b("20120614");
    b.sa("A", A);
    b.sa("B", B);
    b.sa("C", C);
    b.sa("Z", Z);
    b.vj("t1")("A", 8 * 3600 + 60 * 5)("B", 8 * 3600 + 10 * 60);
    b.vj("t2")("C", 8 * 3600 + 60 * 30)("Z", 8 * 3600 + 45 * 60);
    b.vj("t3")("A", 8 * 3600 + 60 * 10)("Z", 9 * 3600 + 45 * 60);

    b.connection("B", "C", 10);
}

/**
 *
 *
 *                          B(8h20)
 *           _______________x
 *          /                \ 10 minutes de marche
 *    8h05 /                  \
 *        /                    \
 * A(8h) x                      xZ
 *        \                    /(9h30)
 *         \__________________/
 *
 *
 *
 * Je veux remonter 2 solutions : 1) Départ en A à 8h05, arrivée en Z à 8h30, 10 minutes de marche
 *                                2) Départ en A à 8h10, arrivée en Z à 9h30, 0 minute de marche
 *
 */

BOOST_AUTO_TEST_CASE(marche_fin) {
    navitia::streetnetwork::StreetNetwork sn;
    navitia::streetnetwork::GraphBuilder bsn(sn);

    navitia::type::GeographicalCoord A(0, 0);
    navitia::type::GeographicalCoord B(0.005, 0.005);
    navitia::type::GeographicalCoord Z(0.0020, 0);

    bsn("A", A)("B", B)("Z", Z);
    bsn("B", "Z")("A", "A")("B", "B")("Z", "Z");

    ed::builder b("20120614");
    b.sa("A", A);
    b.sa("B", B);
    b.sa("Z", Z);
    b.vj("t1")("A", 8 * 3600 + 60 * 5)("B", 8 * 3600 + 20 * 60);
    b.vj("t2")("A", 8 * 3600 + 60 * 10)("Z", 9 * 3600 + 30 * 60);
}

/**
 *
 *
 *
 *                                          C(8h15)            D(8h30)
 *                                           x__________________x____________
 *            9min de marche      B(8h10)   /  10min de marche               \
 *          _____________________x_________/                                  \
 *         /                                                                   \ Z(8h45)
 * A(8h)x /                                                                     x
 *        \                                                                     /Z(9h45)
 *         \_____________________x ____________________________________________/
 *            0min de marche    E(8h10)
 *
 * Je veux remonter deux solutions : 1) Départ de B(8h10), arrivée en Z(8h45), temps de marche : 15 minutes
 *                                   2) Départ de E(8h10), arrivee en Z(9h45), temps de marche : 1 minute
 *
 */

BOOST_AUTO_TEST_CASE(marche_depart_milieu) {
    navitia::streetnetwork::StreetNetwork sn;
    navitia::streetnetwork::GraphBuilder bsn(sn);

    navitia::type::GeographicalCoord A(0, 0);
    navitia::type::GeographicalCoord B(0.0050, 0.0050);
    navitia::type::GeographicalCoord C(0.020, 0.010);
    navitia::type::GeographicalCoord D(0.030, 0.010, false);
    navitia::type::GeographicalCoord E(0.000100, -0.00010, false);

    navitia::type::GeographicalCoord Z(0.0040, 0, false);

    bsn("A", A)("B", B)("C", C)("D", D)("E", E)("Z", Z);
    bsn("A", "B")("B", "A")("A", "E")("E", "A")("C", "C")("D", "D")("Z", "Z");

    ed::builder b("20120614");
    b.sa("B", B);
    b.sa("C", C);
    b.sa("D", D);
    b.sa("E", E);
    b.sa("Z", Z);
    b.vj("t1")("B", 8 * 3600 + 60 * 10)("C", 8 * 3600 + 15 * 60);
    b.vj("t2")("D", 8 * 3600 + 60 * 30)("Z", 8 * 3600 + 45 * 60);
    b.vj("t3")("E", 8 * 3600 + 60 * 10)("Z", 9 * 3600 + 30 * 60);
    b.connection("C", "D", 10);
}

/**
 *
 *
 *
 *
 *                         B(8h10)          C(8h20)
 *                         x________________x
 *                        /                  \     5mins de marche
 * 5mins de marche       /                    \
 *                      /                      \ Z(8h30)
 *               A(8h)x                        x
 *                     \______________________/ Z(9h30)
 *
 *
 *
 *     Je veux remonter deux solutions : 1) Départ de B à 8h, arrivée en Z à 8h30, temps de marche : 10 minutes
 *                                       2) Départ de A à 8h, arrivée en Z à 9h30, temps de marche : 0 minutes
 *
 *
 *
 */
BOOST_AUTO_TEST_CASE(marche_depart_fin) {
    navitia::streetnetwork::StreetNetwork sn;
    navitia::streetnetwork::GraphBuilder bsn(sn);

    navitia::type::GeographicalCoord A(0, 0);
    navitia::type::GeographicalCoord B(0.005, 0.005);
    navitia::type::GeographicalCoord C(-0.001, -0.001);
    navitia::type::GeographicalCoord Z(0.002, 0.002);

    bsn("A", A)("B", B)("C", C)("Z", Z);
    bsn("A", "B")("B", "A")("C", "Z")("Z", "C")("Z", "Z");

    ed::builder b("20120614");
    b.sa("A", A);
    b.sa("B", B);
    b.sa("C", C);
    b.sa("Z", Z);
    b.vj("t1")("B", 8 * 3600 + 60 * 10)("C", 8 * 3600 + 20 * 60);
    b.vj("t2")("A", 8 * 3600 + 60 * 10)("Z", 9 * 3600 + 45 * 60);
}

/**
 *
 *                                      C(8h20)                   D(8h30)
 *                                      x_________________________x
 *                                     /    5 minutes de marche    \
 *                         B(8h10)    /                             \ E(8h40)
 *                         x_________/                               x_________________________
 *                        /                                           5mins de marche          \ Z(8h50)
 * 5mins de marche       /                                                                      x
 *                      /                                                                      / Z(9h30)
 *               A(8h)x                                                                       /
 *                     \_____________________________________________________________________/
 *
 *
 *
 *     Je veux remonter deux solutions : 1) Départ de B à 8h10, arrivée en Z à 8h50, temps de marche : 15 minutes
 *                                       2) Départ de A à 8h, arrivée en Z à 9h30, temps de marche : 0 minutes
 *
 *
 *
 */

BOOST_AUTO_TEST_CASE(marche_depart_milieu_fin) {
    navitia::streetnetwork::StreetNetwork sn;
    navitia::streetnetwork::GraphBuilder bsn(sn);

    navitia::type::GeographicalCoord A(0, 0, false);
    navitia::type::GeographicalCoord B(10, 10, false);
    navitia::type::GeographicalCoord C(20, 20, false);
    navitia::type::GeographicalCoord D(30, 20, false);
    navitia::type::GeographicalCoord E(40, 10, false);
    navitia::type::GeographicalCoord Z(50, 0, false);

    bsn("A", A)("B", B)("C", C)("D", D)("E", E)("Z", Z);
    bsn("A", "B", 5 * (5 / 60))("C", "D", 5 * (5 / 60))("E", "Z", 5 * (5 / 60));

    ed::builder b("20120614");
    b.sa("A", A);
    b.sa("B", B);
    b.sa("C", C);
    b.sa("D", D);
    b.sa("E", E);
    b.sa("Z", Z);
    b.vj("t1")("B", 8 * 3600 + 60 * 10)("C", 8 * 3600 + 20 * 60);
    b.vj("t2")("D", 8 * 3600 + 60 * 30)("E", 8 * 3600 + 40 * 60);
    b.vj("t3")("A", 8 * 3600)("Z", 9 * 3600 + 45 * 60);

    b.connection("C", "D", 5);
}
