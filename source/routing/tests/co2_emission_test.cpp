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
#define BOOST_TEST_MODULE co2_emission_test
#include <boost/test/unit_test.hpp>
#include "routing/raptor_api.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "georef/street_network.h"
#include "type/data.h"
#include "type/pb_converter.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace nr = navitia::routing;
namespace ntest = navitia::test;
namespace bt = boost::posix_time;
namespace ng = navitia::georef;

// co2_emission defined and > 0.0
BOOST_AUTO_TEST_CASE(co2_emission_higher_0) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614", [](ed::builder& b) {
        b.sa("stop_area:stop1", 4.2973, 46.945686);
        b.sa("stop_area:stop2", 4.373721, 46.665326);
        b.vj("A")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 8 * 3600 + 20 * 60,
                                                                             8 * 3600 + 21 * 60);
    });
    navitia::type::Data data;
    nr::RAPTOR raptor(*b.data);
    navitia::type::PhysicalMode* mt = b.data->pt_data->physical_modes_map["physical_mode:0x0"];
    navitia::type::VehicleJourney* vj = b.data->pt_data->vehicle_journeys.at(0);
    navitia::type::StopPoint* dep = b.data->pt_data->stop_points_map["stop_area:stop1"];
    navitia::type::StopPoint* arr = b.data->pt_data->stop_points_map["stop_area:stop2"];
    mt->co2_emission = 2.3;
    vj->physical_mode = mt;
    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");

    ng::StreetNetwork sn_worker(b.street_network);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T021000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 3);
    pbnavitia::Section section = resp.journeys(0).sections(1);
    pbnavitia::Co2Emission co2_emission = section.co2_emission();
    int32_t distance = dep->coord.distance_to(arr->coord);
    BOOST_REQUIRE_EQUAL(co2_emission.unit(), "gEC");
    BOOST_REQUIRE_EQUAL(co2_emission.value(), ((double(distance)) / 1000.0) * (*mt->co2_emission));
}

// co2_emission defined and = 0.0
BOOST_AUTO_TEST_CASE(co2_emission_equal_0) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614", [](ed::builder& b) {
        b.sa("stop_area:stop1", 4.2973, 46.945686);
        b.sa("stop_area:stop2", 4.373721, 46.665326);
        b.vj("A")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 8 * 3600 + 20 * 60,
                                                                             8 * 3600 + 21 * 60);
    });
    nr::RAPTOR raptor(*b.data);

    navitia::type::PhysicalMode* mt = b.data->pt_data->physical_modes_map["physical_mode:0x0"];
    navitia::type::VehicleJourney* vj = b.data->pt_data->vehicle_journeys[0];
    mt->co2_emission = 0.0;
    vj->physical_mode = mt;
    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");

    ng::StreetNetwork sn_worker(b.street_network);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T021000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 3);
    pbnavitia::Section section = resp.journeys(0).sections(1);
    pbnavitia::Co2Emission co2_emission = section.co2_emission();
    BOOST_REQUIRE_EQUAL(co2_emission.unit(), "gEC");
    BOOST_REQUIRE_EQUAL(co2_emission.value(), 0.);
}

// co2_emission defined and < 0.0
BOOST_AUTO_TEST_CASE(co2_emission_lower_0) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614", [](ed::builder& b) {
        b.sa("stop_area:stop1", 4.2973, 46.945686);
        b.sa("stop_area:stop2", 4.373721, 46.665326);
        b.vj("A")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 8 * 3600 + 20 * 60,
                                                                             8 * 3600 + 21 * 60);
    });
    nr::RAPTOR raptor(*b.data);
    navitia::type::PhysicalMode* mt = b.data->pt_data->physical_modes_map["physical_mode:0x1"];
    navitia::type::VehicleJourney* vj = b.data->pt_data->vehicle_journeys.at(0);
    vj->physical_mode = mt;
    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");
    ng::StreetNetwork sn_worker(b.street_network);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T021000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 3);
    pbnavitia::Section section = resp.journeys(0).sections(1);
    BOOST_REQUIRE_EQUAL(section.has_co2_emission(), false);
}
