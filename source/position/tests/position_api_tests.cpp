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
#define BOOST_TEST_MODULE position_test
#include <boost/test/unit_test.hpp>

#include "ed/build_helper.h"
#include "position/position_api.h"
#include "type/pb_converter.h"
#include "utils/logger.h"
#include "type/pt_data.h"
#include "tests/utils_test.h"
#include "kraken/apply_disruption.h"

using namespace navitia;
using boost::posix_time::time_period;
namespace ntest = navitia::test;
using ntest::RTStopTime;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace {

class positionTestFixture {
public:
    ed::builder b;
    const type::Data& data;

    positionTestFixture() : b("20210912"), data(*b.data) {
        b.vj_with_network("network:R", "line:A", "1111111", "", true, "")("stop_area:stop1", "10:00"_t, "10:00"_t)(
            "stop_area:stop2", "11:00"_t, "11:00"_t);
        b.vj_with_network("network:R", "line:S", "1111111", "", true, "")("stop_area:stop5", "10:30"_t, "10:30"_t)(
            "stop_area:stop6", "15:00"_t, "15:00"_t);

        b.vj_with_network("network:K", "line:KA", "1111111", "", true, "KA")(
            "stop_area:stop_k_0", "14:00"_t, "14:00"_t)("stop_area:stop_k_1", "15:00"_t, "15:00"_t)(
            "stop_area:stop_k_2", "16:00"_t, "16:00"_t);

        b.vj_with_network("network:K", "line:KA", "1111111", "", true, "KB")(
            "stop_area:stop_k_0", "14:45"_t, "14:45"_t)("stop_area:stop_k_1", "15:45"_t, "15:45"_t);
        /*
            Test Validate VJs
                        15:00               15:10           15:30           15:40         15:45      15:50
            VJ: AA        |****************************************************|
            VJ: BB                          |**********************************************************|
            VJ: CC                                            |****************************|

         */
        b.vj_with_network("network:AA", "line:AA", "1111111", "", true, "AA")("stop_area:stop01", "15:00"_t, "15:00"_t)(
            "stop_area:stop02", "15:40"_t, "15:40"_t);
        b.vj_with_network("network:AA", "line:AA", "1111111", "", true, "BB")("stop_area:stop01", "15:10"_t, "15:10"_t)(
            "stop_area:stop02", "15:50"_t, "15:50"_t);
        b.vj_with_network("network:AA", "line:AA", "1111111", "", true, "CC")("stop_area:stop01", "15:30"_t, "15:30"_t)(
            "stop_area:stop02", "15:45"_t, "15:45"_t);

        b.vj_with_network("midnight:AA", "midnight:AA", "0000011", "", true, "midnight")(
            "midnight:stop01", "23:45"_t, "23:45"_t)("midnight:stop02", "24:30"_t, "24:30"_t);

        auto& vj = data.pt_data->vehicle_journeys_map.at("vehicle_journey:line:S:1");
        data.pt_data->codes.add(vj, "source", "network:R-line:S");

        vj = data.pt_data->vehicle_journeys_map.at("vehicle_journey:line:A:0");
        data.pt_data->codes.add(vj, "source", "network:R-line:A");

        b.make();
    }
};

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_by_network, positionTestFixture) {
    const ptime current_datetime = "20210913T100500"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:R")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:A");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(),
                        "vehicle_journey:line:A:0");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).type(), "source");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).value(),
                        "network:R-line:A");
}

BOOST_FIXTURE_TEST_CASE(test_many_vehicle_position_by_network, positionTestFixture) {
    const ptime current_datetime = "20210913T103500"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:R")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 2);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:A");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(),
                        "vehicle_journey:line:A:0");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).type(), "source");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).value(),
                        "network:R-line:A");

    vehicle_position = resp.vehicle_positions(1);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:S");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(),
                        "vehicle_journey:line:S:1");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).type(), "source");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).value(),
                        "network:R-line:S");
}

BOOST_FIXTURE_TEST_CASE(test_vehicle_position_by_network_and_line, positionTestFixture) {
    const ptime current_datetime = "20210913T100000"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:R" and line.uri="line:A")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:A");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(),
                        "vehicle_journey:line:A:0");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).type(), "source");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).value(),
                        "network:R-line:A");
}

BOOST_FIXTURE_TEST_CASE(test_vehicle_position_by_network_and_stop_area, positionTestFixture) {
    const ptime current_datetime = "20210913T100000"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:R" and stop_area.uri="stop_area:stop1")",
                                         10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:A");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(),
                        "vehicle_journey:line:A:0");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).type(), "source");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes(0).value(),
                        "network:R-line:A");
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_by_network_many_position_by_line, positionTestFixture) {
    const ptime current_datetime = "20210913T144700"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:K")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:KA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 2);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:KA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 0);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().uri(), "vehicle_journey:KB");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().codes().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_0, positionTestFixture) {
    /*
    current_datetime            +
                    15:00               15:10           15:30           15:40         15:45      15:50
        VJ: AA        |****************************************************|
        VJ: BB                          |**********************************************************|
        VJ: CC                                            |****************************|

     */

    const ptime current_datetime = "20210913T150500"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_1, positionTestFixture) {
    /*
    current_datetime                             +
                    15:00               15:10           15:30           15:40         15:45      15:50
        VJ: AA        |****************************************************|
        VJ: BB                          |**********************************************************|
        VJ: CC                                            |****************************|

     */

    const ptime current_datetime = "20210913T152500"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 2);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 0);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().uri(), "vehicle_journey:BB");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().codes().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_2, positionTestFixture) {
    /*
    current_datetime                                             +
                    15:00               15:10           15:30           15:40         15:45      15:50
        VJ: AA        |****************************************************|
        VJ: BB                          |**********************************************************|
        VJ: CC                                            |****************************|

     */

    const ptime current_datetime = "20210913T153500"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 3);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 0);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().uri(), "vehicle_journey:BB");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().codes().size(), 0);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(2).vehicle_journey().uri(), "vehicle_journey:CC");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(2).vehicle_journey().codes().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_3, positionTestFixture) {
    /*
    current_datetime                                                            +
                    15:00               15:10           15:30           15:40         15:45      15:50
        VJ: AA        |****************************************************|
        VJ: BB                          |**********************************************************|
        VJ: CC                                            |****************************|

     */

    const ptime current_datetime = "20210913T154200"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 2);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:BB");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 0);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().uri(), "vehicle_journey:CC");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().codes().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_4, positionTestFixture) {
    /*
    current_datetime                                                                         +
                    15:00               15:10           15:30           15:40         15:45      15:50
        VJ: AA        |****************************************************|
        VJ: BB                          |**********************************************************|
        VJ: CC                                            |****************************|

     */

    const ptime current_datetime = "20210913T154700"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:BB");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_current_date_not_in_production_period, positionTestFixture) {
    // Start production date 20210912

    const ptime current_datetime = "20210910T154700"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_realtime_delay, positionTestFixture) {
    /*
    current_datetime            *
                    15:00               15:10           15:30           15:40         15:45      15:50
        VJ: AA        |****************************************************|
        VJ: BB                          |**********************************************************|
        VJ: BB modified    |--------------------------------------------------------------|
        VJ: CC                                            |****************************|

     */
    transit_realtime::TripUpdate trip_update =
        ntest::make_trip_update_message("BB", "20210913",
                                        {RTStopTime("stop_area:stop01", "20210913T1503"_pts).delay(7_min),
                                         RTStopTime("stop_area:stop02", "20210913T1543"_pts).delay(7_min)},
                                        transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS);

    navitia::handle_realtime("bob", "20210913T1500"_dt, trip_update, *b.data, true, true);
    const ptime current_datetime = "20210913T150400"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 2);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().codes().size(), 0);

    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().uri(),
                        "vehicle_journey:BB:modified:0:bob");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().codes().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_realtime_add_vj, positionTestFixture) {
    /*
    current_datetime            *
         14:00      15:00               15:10           15:30           15:40         15:45      15:50         16:00
VJ: AA                |****************************************************|
VJ: BB                                    |**********************************************************|
new vj added                  |--------------------------------------------------------------|
VJ: CC                                                    |****************************|
VJ: KA:   |******************************************************************************************************|
VJ: KB:         |******************************************************************************************|

     */
    transit_realtime::TripUpdate new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20210913",
        {
            RTStopTime("stop_area:stop01", "20210913T1507"_pts).added(),
            RTStopTime("stop_area:stop02", "20210913T1547"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, "", "", "", "", "trip_headsign");

    navitia::handle_realtime("bob", "20210913T1500"_dt, new_trip, *b.data, true, true);

    const ptime current_datetime = "20210913T150800"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, "", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 3);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:stop_area:stop01_stop_area:stop02");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(),
                        "vehicle_journey:vj_new_trip:modified:0:bob");

    vehicle_position = resp.vehicle_positions(1);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:AA");

    vehicle_position = resp.vehicle_positions(2);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:KA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 2);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:KA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().uri(), "vehicle_journey:KB");
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_realtime_del_vj, positionTestFixture) {
    /*
    current_datetime                              *
                    15:00               15:10           15:30           15:40         15:45      15:50
        VJ: AA        |****************************************************|
        VJ: BB: To del                    |**********************************************************|
        VJ: CC                                            |****************************|

     */
    transit_realtime::TripUpdate trip_update = ntest::make_trip_update_message(
        "BB", "20210913", {}, transit_realtime::Alert_Effect::Alert_Effect_NO_SERVICE, "", "", "", "");
    navitia::handle_realtime("bob", "20210913T1500"_dt, trip_update, *b.data, true, true);
    const ptime current_datetime = "20210913T151200"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="network:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);
    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:AA");
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_disruption_chaos, positionTestFixture) {
    /*
    current_datetime                              *
         14:00      15:00               15:10           15:30           15:40         15:45      15:50         16:00
VJ: AA                |****************************************************|
VJ: BB                                    |**********************************************************|
VJ: CC                                                    |****************************|

vjs of network:K
VJ: KA:   |******************************************************************************************************|
VJ: KB:         |******************************************************************************************|

     */
    auto period = time_period("20210913T060000"_dt, "20210913T180000"_dt);
    navitia::apply_disruption(b.disrupt(nt::RTLevel::Adapted, "network")
                                  .tag("network")
                                  .impact()
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(period)
                                  .publish(period)
                                  .on(nt::Type_e::Network, "network:K", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    const ptime current_datetime = "20210913T151200"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, "", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);

    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "line:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 2);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(), "vehicle_journey:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(1).vehicle_journey().uri(), "vehicle_journey:BB");
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_midnight_0, positionTestFixture) {
    /*
    current_datetime                              *
                              23:45                    24:00                        24:30
    VJ: midnight                |************************|****************************|


     */
    const ptime current_datetime = "20210913T234800"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="midnight:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);

    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "midnight:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(),
                        "vehicle_journey:midnight");
}

BOOST_FIXTURE_TEST_CASE(test_one_vehicle_position_is_active_midnight_1, positionTestFixture) {
    /*
    current_datetime                                             *
                              23:45                    24:00                        24:30
    VJ: midnight                |************************|****************************|


     */
    const ptime current_datetime = "20210914T001000"_dt;
    navitia::PbCreator pb_creator;
    pb_creator.init(&data, current_datetime, null_time_period);
    navitia::position::vehicle_positions(pb_creator, R"(network.uri="midnight:AA")", 10, 0, 0, {});
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_positions().size(), 1);

    auto vehicle_position = resp.vehicle_positions(0);

    BOOST_REQUIRE_EQUAL(vehicle_position.line().uri(), "midnight:AA");
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions().size(), 1);
    BOOST_REQUIRE_EQUAL(vehicle_position.vehicle_journey_positions(0).vehicle_journey().uri(),
                        "vehicle_journey:midnight");
}
}  // namespace
