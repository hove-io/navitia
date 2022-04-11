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
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "type/type_interfaces.h"
#include "type/stop_point.h"

using namespace navitia::type;

BOOST_AUTO_TEST_CASE(wcb_required_false_properties_false) {
    Properties required_properties = 0;
    StopPoint sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(wcb_required_false_properties_true) {
    Properties required_properties = 0;
    StopPoint sp;
    sp.set_property(hasProperties::WHEELCHAIR_BOARDING);
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(wcb_required_true_properties_false) {
    Properties required_properties = 1 << hasProperties::WHEELCHAIR_BOARDING;
    StopPoint sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), false);
}
BOOST_AUTO_TEST_CASE(wcb_required_true_properties_true) {
    Properties required_properties = 1 << hasProperties::WHEELCHAIR_BOARDING;
    StopPoint sp;
    sp.set_property(hasProperties::WHEELCHAIR_BOARDING);
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(wcb_required_false_properties_false_const) {
    Properties required_properties = 0;
    StopPoint tmp_sp;
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(wcb_required_false_properties_true_const) {
    Properties required_properties = 0;
    StopPoint tmp_sp;
    tmp_sp.set_property(hasProperties::WHEELCHAIR_BOARDING);
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(wcb_required_true_properties_false_const) {
    Properties required_properties = 1 << hasProperties::WHEELCHAIR_BOARDING;
    StopPoint tmp_sp;
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), false);
}
BOOST_AUTO_TEST_CASE(wcb_required_true_properties_true_const) {
    Properties required_properties = 1 << hasProperties::WHEELCHAIR_BOARDING;
    StopPoint tmp_sp;
    tmp_sp.set_property(hasProperties::WHEELCHAIR_BOARDING);
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(ba_required_false_properties_false) {
    Properties required_properties = 0;
    StopPoint sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(ba_required_false_properties_true) {
    Properties required_properties = 0;
    StopPoint sp;
    sp.set_property(hasProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(ba_required_true_properties_false) {
    Properties required_properties = 1 << hasProperties::BIKE_ACCEPTED;
    StopPoint sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), false);
}
BOOST_AUTO_TEST_CASE(ba_required_true_properties_true) {
    Properties required_properties = 0;
    StopPoint sp;
    sp.set_property(hasProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(ba_required_false_properties_false_const) {
    Properties required_properties = 0;
    StopPoint tmp_sp;
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(ba_required_false_properties_true_const) {
    Properties required_properties = 0;
    StopPoint tmp_sp;
    tmp_sp.set_property(hasProperties::BIKE_ACCEPTED);
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
BOOST_AUTO_TEST_CASE(ba_required_true_properties_false_const) {
    Properties required_properties = 1 << hasProperties::BIKE_ACCEPTED;
    StopPoint tmp_sp;
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), false);
}
BOOST_AUTO_TEST_CASE(ba_required_true_properties_true_const) {
    Properties required_properties = 1 << hasProperties::BIKE_ACCEPTED;
    StopPoint tmp_sp;
    tmp_sp.set_property(hasProperties::BIKE_ACCEPTED);
    const StopPoint sp = tmp_sp;
    BOOST_CHECK_EQUAL(sp.accessible(required_properties), true);
}
