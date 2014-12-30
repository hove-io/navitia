/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>

#include "type/type.h"
#include "ed/build_helper.h"
#include "type/pt_data.h"

struct aggregate_odt_fixture {
    ed::builder b;
    navitia::type::JourneyPattern* jp ;
    navitia::type::Route* route;
    navitia::type::Line* line;
    aggregate_odt_fixture() : b("20140210"){
        jp = new navitia::type::JourneyPattern();
        b.data->pt_data->journey_patterns_map["jp1"] = jp;
        b.data->pt_data->journey_patterns.push_back(jp);
        route = new navitia::type::Route();
        b.data->pt_data->routes_map["route1"] = route;
        b.data->pt_data->routes.push_back(route);
        line = new navitia::type::Line();

        jp = new navitia::type::JourneyPattern();
        b.data->pt_data->journey_patterns.push_back(jp);
        b.data->pt_data->journey_patterns_map["none_odt"] = jp;

        jp = new navitia::type::JourneyPattern();
        jp->odt_properties.set_virtual_odt();
        b.data->pt_data->journey_patterns.push_back(jp);
        b.data->pt_data->journey_patterns_map["virtual_odt"] = jp;

        jp = new navitia::type::JourneyPattern();
        jp->odt_properties.set_zonal_odt();
        b.data->pt_data->journey_patterns.push_back(jp);
        b.data->pt_data->journey_patterns_map["zonal_odt"] = jp;

        regular_vj = std::make_unique<navitia::type::DiscreteVehicleJourney>();
        regular_vj->vehicle_journey_type = navitia::type::VehicleJourneyType::regular;

        virtual_with_stop_time_vj = std::make_unique<navitia::type::DiscreteVehicleJourney>();
        virtual_with_stop_time_vj->vehicle_journey_type = navitia::type::VehicleJourneyType::virtual_with_stop_time;

        virtual_without_stop_time_vj = std::make_unique<navitia::type::DiscreteVehicleJourney>();
        virtual_without_stop_time_vj->vehicle_journey_type = navitia::type::VehicleJourneyType::virtual_without_stop_time;

        stop_point_to_stop_point_vj = std::make_unique<navitia::type::DiscreteVehicleJourney>();
        stop_point_to_stop_point_vj->vehicle_journey_type = navitia::type::VehicleJourneyType::stop_point_to_stop_point;

        address_to_stop_point_vj = std::make_unique<navitia::type::DiscreteVehicleJourney>();
        address_to_stop_point_vj->vehicle_journey_type = navitia::type::VehicleJourneyType::adress_to_stop_point;

        odt_point_to_point_vj = std::make_unique<navitia::type::DiscreteVehicleJourney>();
        odt_point_to_point_vj->vehicle_journey_type = navitia::type::VehicleJourneyType::odt_point_to_point;
    }

    std::unique_ptr<navitia::type::DiscreteVehicleJourney> regular_vj;
    std::unique_ptr<navitia::type::DiscreteVehicleJourney> virtual_with_stop_time_vj;
    std::unique_ptr<navitia::type::DiscreteVehicleJourney> virtual_without_stop_time_vj;
    std::unique_ptr<navitia::type::DiscreteVehicleJourney> stop_point_to_stop_point_vj;
    std::unique_ptr<navitia::type::DiscreteVehicleJourney> address_to_stop_point_vj;
    std::unique_ptr<navitia::type::DiscreteVehicleJourney> odt_point_to_point_vj;

};

// Tests for VehicleJourney Object
BOOST_FIXTURE_TEST_CASE(vj_none_odt_test, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(regular_vj->is_none_odt(), true);
    BOOST_CHECK_EQUAL(regular_vj->is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(regular_vj->is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(vj_virtual_odt_test, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(virtual_with_stop_time_vj->is_none_odt(), false);
    BOOST_CHECK_EQUAL(virtual_with_stop_time_vj->is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(virtual_with_stop_time_vj->is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(vj_zonal_odt_test1, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(virtual_without_stop_time_vj->is_none_odt(), false);
    BOOST_CHECK_EQUAL(virtual_without_stop_time_vj->is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(virtual_without_stop_time_vj->is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(vj_zonal_odt_test2, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(stop_point_to_stop_point_vj->is_none_odt(), false);
    BOOST_CHECK_EQUAL(stop_point_to_stop_point_vj->is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(stop_point_to_stop_point_vj->is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(vj_zonal_odt_test3, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(address_to_stop_point_vj->is_none_odt(), false);
    BOOST_CHECK_EQUAL(address_to_stop_point_vj->is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(address_to_stop_point_vj->is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(vj_zonal_odt_test4, aggregate_odt_fixture) {
    BOOST_CHECK_EQUAL(odt_point_to_point_vj->is_none_odt(), false);
    BOOST_CHECK_EQUAL(odt_point_to_point_vj->is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(odt_point_to_point_vj->is_zonal_odt(), true);
}

// Tests for JourneyPattern Object
BOOST_FIXTURE_TEST_CASE(jp_without_vj, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), true);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(jp_none_odt_test, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.push_back(std::move(regular_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), true);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(jp_virtual_odt_test, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(regular_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_with_stop_time_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(jp_virtual_and_zonal_odt_test1, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(regular_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_with_stop_time_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_without_stop_time_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(jp_virtual_and_zonal_odt_test2, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(regular_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_with_stop_time_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_without_stop_time_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(stop_point_to_stop_point_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(jp_virtual_and_zonal_odt_test3, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(regular_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_with_stop_time_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_without_stop_time_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(stop_point_to_stop_point_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(address_to_stop_point_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(jp_virtual_and_zonal_odt_test4, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(regular_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_with_stop_time_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_without_stop_time_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(stop_point_to_stop_point_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(address_to_stop_point_vj));
    jp->discrete_vehicle_journey_list.push_back(std::move(odt_point_to_point_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), true);
}


BOOST_FIXTURE_TEST_CASE(jp_zonal_odt_test1, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(virtual_without_stop_time_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(jp_zonal_odt_test2, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(stop_point_to_stop_point_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(jp_zonal_odt_test3, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(address_to_stop_point_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(jp_zonal_odt_test4, aggregate_odt_fixture) {
    jp = b.data->pt_data->journey_patterns_map["jp1"];
    jp->discrete_vehicle_journey_list.clear();
    jp->discrete_vehicle_journey_list.push_back(std::move(odt_point_to_point_vj));
    jp->build_odt_properties();

    BOOST_CHECK_EQUAL(jp->odt_properties.is_regular(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(jp->odt_properties.is_zonal_odt(), true);
}

// Tests for Route Object
BOOST_FIXTURE_TEST_CASE(route_without_journey_pattern , aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    navitia::type::hasOdtProperties odt = route->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), true);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(route_none_odt, aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    route->journey_pattern_list.clear();
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["none_odt"]);
    route->get_odt_properties();
    navitia::type::hasOdtProperties odt = route->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), true);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(route_virtual_odt, aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    route->journey_pattern_list.clear();
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["none_odt"]);
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["virtual_odt"]);
    route->get_odt_properties();
    navitia::type::hasOdtProperties odt = route->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), false);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(route_virtual_and_zonal_odt, aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    route->journey_pattern_list.clear();
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["none_odt"]);
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["virtual_odt"]);
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["zonal_odt"]);
    route->get_odt_properties();
    navitia::type::hasOdtProperties odt = route->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), false);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(route_zonal_odt, aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    route->journey_pattern_list.clear();
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["zonal_odt"]);
    route->get_odt_properties();
    navitia::type::hasOdtProperties odt = route->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), false);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), true);
}

// tests for Line Object
BOOST_FIXTURE_TEST_CASE(line_without_route , aggregate_odt_fixture) {
    navitia::type::hasOdtProperties odt = line->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), true);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(line_none_odt , aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    route->journey_pattern_list.clear();
    line->route_list.clear();
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["none_odt"]);
    line->route_list.push_back(route);
    navitia::type::hasOdtProperties odt = line->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), true);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(line_virtual_odt , aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    route->journey_pattern_list.clear();
    line->route_list.clear();
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["none_odt"]);
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["virtual_odt"]);
    line->route_list.push_back(route);
    navitia::type::hasOdtProperties odt = line->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), false);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), false);
}

BOOST_FIXTURE_TEST_CASE(line_virtual_and_zonal_odt , aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    route->journey_pattern_list.clear();
    line->route_list.clear();
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["none_odt"]);
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["virtual_odt"]);
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["zonal_odt"]);
    line->route_list.push_back(route);
    navitia::type::hasOdtProperties odt = line->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), false);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), true);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), true);
}

BOOST_FIXTURE_TEST_CASE(line_zonal_odt , aggregate_odt_fixture) {
    route = b.data->pt_data->routes_map["route1"];
    route->journey_pattern_list.clear();
    line->route_list.clear();
    route->journey_pattern_list.push_back(b.data->pt_data->journey_patterns_map["zonal_odt"]);
    line->route_list.push_back(route);
    navitia::type::hasOdtProperties odt = line->get_odt_properties();
    BOOST_CHECK_EQUAL(odt.is_regular(), false);
    BOOST_CHECK_EQUAL(odt.is_virtual_odt(), false);
    BOOST_CHECK_EQUAL(odt.is_zonal_odt(), true);
}
