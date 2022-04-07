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
#define BOOST_TEST_MODULE test_fare
#include <boost/test/unit_test.hpp>
#include "ed/connectors/fare_parser.h"
#include "ed/connectors/fare_utils.h"
#include "utils/logger.h"
#include "utils/base64_encode.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

using namespace navitia::fare;
using namespace ed::connectors;

BOOST_AUTO_TEST_CASE(parse_state_test) {
    State state;

    // * correspond au state vide, toujours vrai
    BOOST_CHECK(parse_state("*") == state);
    BOOST_CHECK(parse_state("") == state);

    BOOST_CHECK(parse_state("mode=metro").mode == navitia::encode_uri("metro"));

    BOOST_CHECK(parse_state("zone=1").zone == "1");

    // on ignore les espaces
    BOOST_CHECK(parse_state(" mode = metro  ").mode == navitia::encode_uri("metro"));
    BOOST_CHECK(parse_state("line=l1").line == navitia::encode_uri("l1"));
    BOOST_CHECK(parse_state(" mode = Metro ").mode == navitia::encode_uri("Metro"));
    // parse_state("stop_area=chatelet").stop_area;
    std::cout << parse_state("stoparea=chatelet").stop_area << std::endl;
    std::cout << navitia::encode_uri("chatelet") << std::endl;
    BOOST_CHECK(parse_state("stoparea=chatelet").stop_area == navitia::encode_uri("chatelet"));

    // Qu'est-ce qui se passe avec des boulets ?
    BOOST_CHECK_THROW(parse_state("mode=Metro=foo"), std::exception);

    // On ne respecte pas la grammaire => exception
    BOOST_CHECK_THROW(parse_state("coucou=moo"), invalid_key);

    // On essaye de parser des choses plus compliquées
    State state2 = parse_state("mode=metro&stoparea=chatelet");
    BOOST_CHECK(state2.mode == navitia::encode_uri("metro"));
    BOOST_CHECK(state2.stop_area == navitia::encode_uri("chatelet"));

    // Si un attribut est en double
    BOOST_CHECK_THROW(parse_state("mode=foo&mode=bar"), invalid_key);

    // On ne veut rien d'autre que de l'égalité
    BOOST_CHECK_THROW(parse_state("mode < bli"), invalid_key);
}

BOOST_AUTO_TEST_CASE(parse_condition_test) {
    BOOST_CHECK_THROW(parse_condition("moo"), invalid_condition);

    Condition cond = parse_condition(" key = value ");
    BOOST_CHECK(cond.value == "value");
    BOOST_CHECK(cond.key == "key");
    BOOST_CHECK(cond.comparaison == Comp_e::EQ);

    BOOST_CHECK(parse_condition("coucou <= moo").comparaison == Comp_e::LTE);
    BOOST_CHECK(parse_condition("coucou >= moo").comparaison == Comp_e::GTE);
    BOOST_CHECK(parse_condition("coucou != moo").comparaison == Comp_e::NEQ);
    BOOST_CHECK(parse_condition("coucou < moo").comparaison == Comp_e::LT);
    BOOST_CHECK(parse_condition("coucou > moo").comparaison == Comp_e::GT);

    BOOST_CHECK(parse_conditions("coucoun<= bli& foo =azw &abc>=123").size() == 3);
}
