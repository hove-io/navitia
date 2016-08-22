/* Copyright © 2001-2016, Canal TP and/or its affiliates. All rights reserved.
  
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
#define BOOST_TEST_MODULE direct_path_test
#include <boost/test/unit_test.hpp>
#include "routing/tests/routing_api_test_data.h"
#include "kraken/data_manager.h"
#include "kraken/worker.h"
#include "kraken/configuration.h"


BOOST_AUTO_TEST_CASE(direct_path_test) {
    routing_api_data<normal_speed_provider> routing_data;
    DataManager<navitia::type::Data> data_manager;
    data_manager.set_data(routing_data.b.data.release());
    navitia::Worker w(data_manager, navitia::kraken::Configuration());

    pbnavitia::Request req;
    auto* dp_req = req.mutable_direct_path();

    auto* origin = dp_req->mutable_origin();
    origin->set_place(routing_data.origin.uri);
    origin->set_access_duration(0);

    auto* destination = dp_req->mutable_destination();
    destination->set_place(routing_data.destination.uri);
    destination->set_access_duration(0);

    dp_req->set_datetime(1470241573);
    dp_req->set_clockwise(true);

    auto* sn_params = dp_req->mutable_streetnetwork_params();
    sn_params->set_origin_mode("walking");
    sn_params->set_destination_mode("walking");
    sn_params->set_walking_speed(1.12);
    sn_params->set_bss_speed(4.1);
    sn_params->set_bike_speed(4.1);
    sn_params->set_car_speed(11.11);
    sn_params->set_origin_filter("");
    sn_params->set_destination_filter("");
    sn_params->set_max_walking_duration_to_pt(30*60);
    sn_params->set_max_bss_duration_to_pt(30*60);
    sn_params->set_max_bike_duration_to_pt(30*60);
    sn_params->set_max_car_duration_to_pt(30*60);
    sn_params->set_enable_direct_path(true);

    // walking
    sn_params->set_origin_mode("walking");
    auto res = w.direct_path(req);
    BOOST_REQUIRE_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).street_network().path_items_size(), 3);
    BOOST_CHECK_EQUAL(res.journeys(0).durations().total(), 275);

    // bss
    sn_params->set_origin_mode("bss");
    res = w.direct_path(req);
    BOOST_REQUIRE_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections_size(), 5);
    BOOST_CHECK_EQUAL(res.journeys(0).durations().total(), 234);

    // bike
    sn_params->set_origin_mode("bike");
    res = w.direct_path(req);
    BOOST_REQUIRE_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).street_network().path_items_size(), 7);
    BOOST_CHECK_EQUAL(res.journeys(0).durations().total(), 58);

    // car
    sn_params->set_origin_mode("car");
    res = w.direct_path(req);
    BOOST_REQUIRE_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections_size(), 3);
    BOOST_CHECK_EQUAL(res.journeys(0).durations().total(), 121);
}
