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
#define BOOST_TEST_MODULE code_container_test

#include "type/code_container.h"
#include "tests/utils_test.h"
#include <boost/test/unit_test.hpp>

namespace nt = navitia::type;
using VJ = nt::VehicleJourney;
using ObjsVJ = nt::CodeContainer::Objs<VJ>;

BOOST_AUTO_TEST_CASE(code_container_test) {
    nt::CodeContainer cc;
    VJ* vj1 = reinterpret_cast<VJ*>(0x42);
    VJ* vj2 = reinterpret_cast<VJ*>(0x1337);

    BOOST_CHECK_EQUAL(cc.get_codes(vj1), nt::CodeContainer::Codes{});
    BOOST_CHECK_EQUAL(cc.get_objs<VJ>("bob", "bobette"), ObjsVJ{});
    cc.add(vj1, "bob", "bobette");
    BOOST_CHECK_EQUAL(cc.get_codes(vj1), (nt::CodeContainer::Codes{{"bob", {"bobette"}}}));
    BOOST_CHECK_EQUAL(cc.get_objs<VJ>("bob", "bobette"), ObjsVJ{vj1});
    cc.add(vj1, "bob", "bobitto");
    BOOST_CHECK_EQUAL(cc.get_codes(vj1), (nt::CodeContainer::Codes{{"bob", {"bobette", "bobitto"}}}));
    BOOST_CHECK_EQUAL(cc.get_objs<VJ>("bob", "bobette"), ObjsVJ{vj1});
    cc.add(vj1, "foo", "bar");
    BOOST_CHECK_EQUAL(cc.get_codes(vj1), (nt::CodeContainer::Codes{{"bob", {"bobette", "bobitto"}}, {"foo", {"bar"}}}));
    cc.add(vj2, "bob", "bobette");
    BOOST_CHECK_EQUAL(cc.get_objs<VJ>("bob", "bobette"), (ObjsVJ{vj1, vj2}));
}
