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
#define BOOST_TEST_MODULE test_path_finder

#include "georef/path_finder.h"
#include "georef/georef.h"
#include "type/time_duration.h"

#include <boost/test/unit_test.hpp>

using namespace navitia::georef;

using pitc = PathItem::TransportCaracteristic;
using ntm = navitia::type::Mode_e;

namespace {

struct get_transportation_mode_item_to_update_params {
    const pitc transportation;
    const bool append_to_begin;
    const pitc expected_result;

    get_transportation_mode_item_to_update_params(const pitc& tc, bool atb, const pitc& er)
        : transportation(tc), append_to_begin(atb), expected_result(er) {}
};

struct crowfly_duration_params {
    const ntm mode;
    const double value;
    const float speed_factor;
    const navitia::time_duration expected_result;

    crowfly_duration_params(const ntm m, const double v, const float sf, const navitia::time_duration er)
        : mode(m), value(v), speed_factor(sf), expected_result(er) {}
};

class PathFinderTest : public PathFinder {
public:
    PathFinderTest(const GeoRef& geo_ref) : PathFinder(geo_ref) {}

    void test_get_transportation_mode_item_to_update(const get_transportation_mode_item_to_update_params& p) const {
        auto const result = get_transportation_mode_item_to_update(p.transportation, p.append_to_begin);
        BOOST_CHECK(result == p.expected_result);
    }

    void test_crowfly_duration(const crowfly_duration_params& p) {
        this->mode = p.mode;
        this->speed_factor = p.speed_factor;
        auto const result = crow_fly_duration(p.value);
        BOOST_CHECK_EQUAL(result, p.expected_result);
    }
};
}  // namespace

BOOST_AUTO_TEST_CASE(get_transportation_mode_item_to_update) {
    GeoRef geo_ref;
    PathFinderTest pf(geo_ref);

    const std::vector<get_transportation_mode_item_to_update_params> params = {
        {pitc::Walk, true, pitc::Walk},
        {pitc::Car, true, pitc::Car},
        {pitc::Bike, false, pitc::Bike},
        {pitc::BssTake, true, pitc::Walk},
        {pitc::BssTake, false, pitc::Bike},
        {pitc::BssPutBack, true, pitc::Bike},
        {pitc::BssPutBack, false, pitc::Walk},
        {pitc::CarLeaveParking, true, pitc::Walk},
        {pitc::CarLeaveParking, false, pitc::Car},
        {pitc::CarPark, true, pitc::Car},
        {pitc::CarPark, false, pitc::Walk}};

    for (auto const& p : params) {
        pf.test_get_transportation_mode_item_to_update(p);
    }
}

BOOST_AUTO_TEST_CASE(crowfly_duration) {
    GeoRef geo_ref;
    PathFinderTest pf(geo_ref);

    const std::vector<crowfly_duration_params> params = {{ntm::Walking, 50., 1.78f, navitia::seconds(25)},
                                                         {ntm::Bike, 50., 0.485f, navitia::seconds(25.)},
                                                         {ntm::Car, 50., 0.175f, navitia::seconds(25.)},
                                                         {ntm::Bss, 50., 1.78f, navitia::seconds(25.)},
                                                         {ntm::CarNoPark, 50., 0.18f, navitia::seconds(25.)}};

    for (auto const& p : params) {
        pf.test_crowfly_duration(p);
    }
}
