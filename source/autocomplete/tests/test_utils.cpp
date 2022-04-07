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

#include <boost/test/unit_test.hpp>
#include "type/type_interfaces.h"
#include "autocomplete/utils.h"
#include "tests/utils_test.h"

using namespace navitia::autocomplete;

using namespace navitia::type;

namespace navitia {
namespace type {
static std::ostream& operator<<(std::ostream& os, const Type_e& type) {
    return os << navitia::type::static_data::get()->captionByType(type);
}
}  // namespace type
}  // namespace navitia

BOOST_AUTO_TEST_CASE(autocomplete_build_type_group) {
    auto result = build_type_groups({Type_e::StopArea});
    std::vector<Type_e> expected;
    BOOST_REQUIRE_EQUAL(result.size(), 1);
    expected = {Type_e::StopArea};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[0].begin(), result[0].end(), expected.begin(), expected.end());

    result = build_type_groups({Type_e::StopArea, Type_e::Admin});
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    expected = {Type_e::Admin};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[0].begin(), result[0].end(), expected.begin(), expected.end());
    expected = {Type_e::StopArea};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[1].begin(), result[1].end(), expected.begin(), expected.end());

    result = build_type_groups({Type_e::Address, Type_e::StopArea, Type_e::Admin, Type_e::POI});
    BOOST_REQUIRE_EQUAL(result.size(), 4);
    expected = {Type_e::Admin};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[0].begin(), result[0].end(), expected.begin(), expected.end());
    expected = {Type_e::StopArea};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[1].begin(), result[1].end(), expected.begin(), expected.end());
    expected = {Type_e::POI};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[2].begin(), result[2].end(), expected.begin(), expected.end());
    expected = {Type_e::Address};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[3].begin(), result[3].end(), expected.begin(), expected.end());

    result = build_type_groups({Type_e::Network, Type_e::Line, Type_e::Route});
    BOOST_REQUIRE_EQUAL(result.size(), 3);
    expected = {Type_e::Network};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[0].begin(), result[0].end(), expected.begin(), expected.end());
    expected = {Type_e::Line};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[1].begin(), result[1].end(), expected.begin(), expected.end());
    expected = {Type_e::Route};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[2].begin(), result[2].end(), expected.begin(), expected.end());

    result = build_type_groups({Type_e::Network, Type_e::VehicleJourney, Type_e::JourneyPattern});
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    expected = {Type_e::Network};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[0].begin(), result[0].end(), expected.begin(), expected.end());
    expected = {Type_e::VehicleJourney, Type_e::JourneyPattern};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(result[1].begin(), result[1].end(), expected.begin(), expected.end());
}
