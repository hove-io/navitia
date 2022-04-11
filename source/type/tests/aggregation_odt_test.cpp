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

#include "ed/build_helper.h"
#include "type/pt_data.h"
#include "type/base_pt_objects.h"

struct aggregate_odt_fixture {
    ed::builder b;
    navitia::type::VehicleJourney* regular_vj;
    navitia::type::VehicleJourney* virtual_with_stop_time_vj;
    navitia::type::VehicleJourney* virtual_without_stop_time_vj;
    navitia::type::VehicleJourney* stop_point_to_stop_point_vj;
    navitia::type::VehicleJourney* address_to_stop_point_vj;
    navitia::type::VehicleJourney* odt_point_to_point_vj;

    aggregate_odt_fixture() : b("20140210") {
        {  // possible in none type
            regular_vj = b.vj("none")("1", 8000, 8000)("2", 9000, 9000)("3", 10000, 10000).make();

            virtual_with_stop_time_vj = b.vj("none")("1", 8000, 8000)("2", 9000, 9000)("3", 10000, 10000).make();
            for (auto& st : virtual_with_stop_time_vj->stop_time_list) {
                st.set_odt(true);
            }
        }

        {  // possible in mixed type
            b.vj("mixed")("1", 8000, 8000)("2", 9000, 9000)("3", 10000, 10000);

            auto* vj = b.vj("mixed")("1", 8000, 8000)("2", 9000, 9000)("3", 10000, 10000).make();
            for (auto& st : vj->stop_time_list) {
                st.set_odt(true);
            }

            virtual_without_stop_time_vj = b.vj("mixed")("1", 8000, 8000)("2", 9000, 9000)("3", 10000, 10000).make();
            for (auto& st : virtual_without_stop_time_vj->stop_time_list) {
                st.set_odt(true);
                st.set_date_time_estimated(true);
            }

            stop_point_to_stop_point_vj = b.vj("mixed")("1", 8000, 8000)("2", 9000, 9000).make();
            for (auto& st : stop_point_to_stop_point_vj->stop_time_list) {
                st.set_odt(true);
                st.set_date_time_estimated(true);
            }
        }

        {  // possible in zonal
            b.vj("zonal")("1", 8000, 8000)("2", 9000, 9000)("3", 10000, 10000);

            auto* vj = b.vj("zonal")("1", 8000, 8000)("2", 9000, 9000)("3", 10000, 10000).make();
            for (auto& st : vj->stop_time_list) {
                st.set_odt(true);
            }

            vj = b.vj("zonal")("1", 8000, 8000)("2", 9000, 9000)("3", 10000, 10000).make();
            for (auto& st : vj->stop_time_list) {
                st.set_odt(true);
                st.set_date_time_estimated(true);
            }

            vj = b.vj("zonal")("1", 8000, 8000)("2", 9000, 9000).make();
            for (auto& st : vj->stop_time_list) {
                st.set_odt(true);
                st.set_date_time_estimated(true);
            }

            address_to_stop_point_vj = b.vj("zonal")("zone1", 8000, 8000)("2", 9000, 9000).make();
            address_to_stop_point_vj->stop_time_list.at(0).set_odt(true);
            address_to_stop_point_vj->stop_time_list.at(0).set_date_time_estimated(true);
            address_to_stop_point_vj->stop_time_list.at(0).set_odt(true);
            address_to_stop_point_vj->stop_time_list.at(0).set_date_time_estimated(true);

            odt_point_to_point_vj = b.vj("zonal")("zone1", 8000, 8000)("zone2", 9000, 9000).make();
            odt_point_to_point_vj->stop_time_list.at(0).set_odt(true);
            odt_point_to_point_vj->stop_time_list.at(0).set_date_time_estimated(true);
            odt_point_to_point_vj->stop_time_list.at(1).set_odt(true);
            odt_point_to_point_vj->stop_time_list.at(1).set_date_time_estimated(true);

            b.data->build_uri();

            b.data->pt_data->stop_points_map.at("zone1")->is_zonal = true;
            b.data->pt_data->stop_points_map.at("zone2")->is_zonal = true;

            b.data->pt_data->sort_and_index();
            b.data->aggregate_odt();
        }
    }
};

// Tests for VehicleJourney Object
BOOST_FIXTURE_TEST_CASE(vj_none_odt_test, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(regular_vj->has_datetime_estimated(), false);
    BOOST_CHECK_EQUAL(regular_vj->has_zonal_stop_point(), false);
}

BOOST_FIXTURE_TEST_CASE(vj_virtual_odt_test, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(virtual_with_stop_time_vj->has_datetime_estimated(), false);
    BOOST_CHECK_EQUAL(virtual_with_stop_time_vj->has_zonal_stop_point(), false);
}

BOOST_FIXTURE_TEST_CASE(vj_zonal_odt_test1, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(virtual_without_stop_time_vj->has_datetime_estimated(), true);
    BOOST_CHECK_EQUAL(virtual_without_stop_time_vj->has_zonal_stop_point(), false);
}

BOOST_FIXTURE_TEST_CASE(vj_zonal_odt_test2, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(stop_point_to_stop_point_vj->has_datetime_estimated(), true);
    BOOST_CHECK_EQUAL(stop_point_to_stop_point_vj->has_zonal_stop_point(), false);
}

BOOST_FIXTURE_TEST_CASE(vj_zonal_odt_test3, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(address_to_stop_point_vj->has_datetime_estimated(), true);
    BOOST_CHECK_EQUAL(address_to_stop_point_vj->has_zonal_stop_point(), true);
}

BOOST_FIXTURE_TEST_CASE(vj_zonal_odt_test4, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(odt_point_to_point_vj->has_datetime_estimated(), true);
    BOOST_CHECK_EQUAL(odt_point_to_point_vj->has_zonal_stop_point(), true);
}

// Tests for Route Object
BOOST_FIXTURE_TEST_CASE(route_none_odt, aggregate_odt_fixture) {
    const auto* route = b.data->pt_data->routes_map.at("none:0");

    BOOST_CHECK_EQUAL(route->get_odt_properties().is_scheduled(), true);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_with_stops(), true);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_estimated(), false);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_zonal(), false);
}

BOOST_FIXTURE_TEST_CASE(route_mixed_odt, aggregate_odt_fixture) {
    const auto* route = b.data->pt_data->routes_map.at("mixed:1");

    BOOST_CHECK_EQUAL(route->get_odt_properties().is_scheduled(), false);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_with_stops(), true);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_estimated(), true);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_zonal(), false);
}

BOOST_FIXTURE_TEST_CASE(route_zonal_odt, aggregate_odt_fixture) {
    const auto* route = b.data->pt_data->routes_map.at("zonal:2");

    BOOST_CHECK_EQUAL(route->get_odt_properties().is_scheduled(), false);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_with_stops(), false);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_estimated(), true);
    BOOST_CHECK_EQUAL(route->get_odt_properties().is_zonal(), true);
}

// tests for Line Object
BOOST_FIXTURE_TEST_CASE(line_none_odt, aggregate_odt_fixture) {
    const auto* line = b.data->pt_data->lines_map.at("none");

    BOOST_CHECK_EQUAL(line->get_odt_properties().is_scheduled(), true);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_with_stops(), true);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_estimated(), false);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_zonal(), false);
}

BOOST_FIXTURE_TEST_CASE(line_mixed_odt, aggregate_odt_fixture) {
    const auto* line = b.data->pt_data->lines_map.at("mixed");

    BOOST_CHECK_EQUAL(line->get_odt_properties().is_scheduled(), false);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_with_stops(), true);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_estimated(), true);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_zonal(), false);
}

BOOST_FIXTURE_TEST_CASE(line_zonal_odt, aggregate_odt_fixture) {
    const auto* line = b.data->pt_data->lines_map.at("zonal");

    BOOST_CHECK_EQUAL(line->get_odt_properties().is_scheduled(), false);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_with_stops(), false);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_estimated(), true);
    BOOST_CHECK_EQUAL(line->get_odt_properties().is_zonal(), true);
}
