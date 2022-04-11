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
#define BOOST_TEST_MODULE test_disruption
#include <boost/test/unit_test.hpp>
#include "routing_api_test_data.h"
#include "kraken/data_manager.h"
#include "kraken/configuration.h"
#include "kraken/worker.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(ok_before_and_after_disruption) {
    routing_api_data<normal_speed_provider> data;
    DataManager<navitia::type::Data> data_manager;
    data_manager.set_data(data.b.data.release());
    navitia::kraken::Configuration conf;
    navitia::Worker w(conf);

    // a basic request
    pbnavitia::Request req;
    req.set_requested_api(pbnavitia::PLANNER);
    pbnavitia::JourneysRequest* j = req.mutable_journeys();
    j->set_clockwise(true);
    j->set_max_duration(1337);
    j->set_max_transfers(42);
    j->set_wheelchair(false);
    j->set_realtime_level(pbnavitia::RTLevel::ADAPTED_SCHEDULE);
    j->add_datetimes(navitia::test::to_posix_timestamp("20120614T080000"));
    j->mutable_streetnetwork_params()->set_origin_mode("walking");
    j->mutable_streetnetwork_params()->set_destination_mode("walking");
    j->mutable_streetnetwork_params()->set_walking_speed(1.14);
    j->mutable_streetnetwork_params()->set_max_walking_duration_to_pt(15 * 60);
    pbnavitia::LocationContext* from = j->add_origin();
    from->set_place("coord:0.0000898312;0.0000898312");  // coord of S
    from->set_access_duration(0);
    pbnavitia::LocationContext* to = j->add_destination();
    to->set_place("coord:0.00188646;0.00071865");  // coord of R
    to->set_access_duration(0);

    // we ask for a journey
    pbnavitia::Response resp;
    {
        const auto d = data_manager.get_data();
        w.dispatch(req, *d);
        resp = w.pb_creator.get_response();
    }
    BOOST_REQUIRE(resp.journeys_size() != 0);
    const auto nb_before = resp.journeys_size();

    // a clone and set
    auto data_cloned = data_manager.get_data_clone();
    data_cloned->build_relations();
    data_cloned->build_raptor();
    data_cloned->build_proximity_list();
    data_manager.set_data(data_cloned);

    // we ask for a journey, we should have the same thing
    {
        const auto d = data_manager.get_data();
        w.dispatch(req, *d);
        resp = w.pb_creator.get_response();
    }
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), nb_before);
}
