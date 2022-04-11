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
#define BOOST_TEST_MODULE disruption_periods_test
#include <boost/test/unit_test.hpp>
#include "tests/utils_test.h"
#include "ed/build_helper.h"
#include "kraken/worker.h"
#include "kraken/apply_disruption.h"

using btp = boost::posix_time::time_period;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

/*
 * Fixture to test when we display a disruption and the status of this disruption
 *
 * The dataset is very simple, there is one VJ and we impact it.
 *
 * The publication period and application period of the impact are shown below:

[------------------------------------------------------------------------------------]  production period

                                                Impact
                                    <--------------------------------->                 publication period

                                                     (------------)                     application period

 the production period is the whole 2017 year
 the impact is publishable  from 02/2017 to 06/2017
 the impact is active (application period) from 03/2017 to 04/2017
 */
struct disruption_periods_fixture {
    disruption_periods_fixture()
        : w(navitia::kraken::Configuration()),
          b("20170101", [](ed::builder& b) { b.vj("1").valid_all_days()("A", "9:00"_t)("B", "10:00"_t); }) {
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line_1_closed")
                                      .severity(nt::disruption::Effect::UNKNOWN_EFFECT)  // just an information
                                      .application_periods(btp("20170301T100000"_dt, "20170401T100000"_dt))
                                      .publish(btp("20170201T100000"_dt, "20170601T100000"_dt))
                                      .on(nt::Type_e::Line, "1", *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);
    }

    std::vector<const pbnavitia::Impact*> get_impacts(const pbnavitia::Request& req) {
        w.dispatch(req, *b.data);
        const auto& resp = w.pb_creator.get_response();
        std::vector<const pbnavitia::Impact*> res;
        for (const pbnavitia::Impact& i : resp.impacts()) {
            res.push_back(&i);
        }
        return res;
    }

    navitia::Worker w;
    ed::builder b;
    uint64_t before_publish = "20170115T080000"_pts;
    uint64_t inside_publish_before_application = "20170215T080000"_pts;
    uint64_t inside_publish_inside_application = "20170315T080000"_pts;
    uint64_t inside_publish_after_application = "20170415T080000"_pts;
};

// create a pt ref request for all lines
static pbnavitia::Request create_pt_request(uint64_t current_dt) {
    pbnavitia::Request req;
    req.set__current_datetime(current_dt);
    req.set_requested_api(pbnavitia::PTREFERENTIAL);
    auto* ptref = req.mutable_ptref();
    ptref->set_requested_type(pbnavitia::LINE);
    ptref->set_filter("");
    ptref->set_depth(2);
    ptref->set_count(100);
    return req;
}

// create a journey request
static pbnavitia::Request create_journey_request(uint64_t current_dt, uint64_t journey_dt) {
    pbnavitia::Request req;
    req.set__current_datetime(current_dt);
    req.set_requested_api(pbnavitia::PLANNER);
    pbnavitia::JourneysRequest* j = req.mutable_journeys();
    j->set_clockwise(true);
    j->set_wheelchair(true);
    j->set_realtime_level(pbnavitia::BASE_SCHEDULE);
    j->set_max_duration(std::numeric_limits<int32_t>::max());
    j->set_max_transfers(42);
    j->add_datetimes(journey_dt);
    auto sn_params = j->mutable_streetnetwork_params();
    sn_params->set_origin_mode("walking");
    sn_params->set_destination_mode("walking");
    sn_params->set_walking_speed(1);
    sn_params->set_bike_speed(1);
    sn_params->set_car_speed(1);
    sn_params->set_bss_speed(1);
    pbnavitia::LocationContext* from = j->add_origin();
    from->set_place("A");
    from->set_access_duration(0);
    pbnavitia::LocationContext* to = j->add_destination();
    to->set_place("B");
    to->set_access_duration(0);

    return req;
}

/*
 * for any query made outside the publication period of the disruption, we shouldn't see the impact
[------------------|------------------------------------------------------------------]  production period
                   |
                   |                             Impact
                   |                 <--------------------------------->                 publication period
                   |
                   |                                  (------------)                     application period
                   |
                   |
                  now

 */
BOOST_FIXTURE_TEST_CASE(query_before_pub_period, disruption_periods_fixture) {
    auto now = before_publish;

    BOOST_REQUIRE_EQUAL(get_impacts(create_pt_request(now)).size(), 0);

    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, before_publish)).size(), 0);
    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, inside_publish_before_application)).size(), 0);
    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, inside_publish_inside_application)).size(), 0);
    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, inside_publish_after_application)).size(), 0);
}
/*
 * for a ptref query made inside the publication period, we should see the impact
 * and the status should be 'future' since the status is only compared to 'now'
[------------------------------------------|------------------------------------------]  production period
                                           |
                                           |     Impact
                                    <------|--------------------------->                 publication period
                                           |
                                           |          (------------)                     application period
                                           |
                                          now

 */
BOOST_FIXTURE_TEST_CASE(query_inside_pub_period, disruption_periods_fixture) {
    auto now = inside_publish_before_application;
    const auto impacts = get_impacts(create_pt_request(now));
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    BOOST_CHECK_EQUAL(impacts[0]->status(), pbnavitia::ActiveStatus::future);
}

/*
 for a journey query made in the publication period for a date before the application period,
 we shouldn't be able to find the impact
[------------------------------------------|------------------------------------------]  production period
                                           |
                                           |     Impact
                                    <------|--------------------------->                 publication period
                                           |
                                           |          (------------)                     application period
                                           |
                                          now
                                             {----}
                                          action period (ie period of the journey

 */
BOOST_FIXTURE_TEST_CASE(journey_inside_pub_period_before_application, disruption_periods_fixture) {
    auto now = inside_publish_before_application;
    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, before_publish)).size(), 0);
    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, inside_publish_before_application)).size(), 0);

    // same if the journey is for a date after the application period, we should'nt see the impact
    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, inside_publish_after_application)).size(), 0);
}

/*
 for a journey query made in the publication period for a date inside the application period,
 we should be able to see the impact and it's status should be 'future'
[------------------------------------------|------------------------------------------]  production period
                                           |
                                           |     Impact
                                    <------|--------------------------->                 publication period
                                           |
                                           |          (------------)                     application period
                                           |
                                          now
                                                          {----}
                                                      action period (ie period of the journey

 */
BOOST_FIXTURE_TEST_CASE(journey_now_inside_pub_period_journey_inside_application, disruption_periods_fixture) {
    auto now = inside_publish_before_application;
    const auto impacts = get_impacts(create_journey_request(now, inside_publish_inside_application));
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    BOOST_CHECK_EQUAL(impacts[0]->status(), pbnavitia::ActiveStatus::future);
}

/*
if ptref is queried inside the application period, we display the impact and it's status is 'active'

[-----------------------------------------------------------|-------------------------]  production period
                                                            |
                                                Impact      |
                                    <-----------------------|---------->                 publication period
                                                            |
                                                     (------|------)                     application period
                                                            |
                                                           now

 */
BOOST_FIXTURE_TEST_CASE(pt_inside_pub_period_inside_application, disruption_periods_fixture) {
    auto now = inside_publish_inside_application;
    const auto impacts = get_impacts(create_pt_request(now));
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    BOOST_CHECK_EQUAL(impacts[0]->status(), pbnavitia::ActiveStatus::active);
}

/*
if the journey is requested the day of the disruption for a day before the application period,
we do not display the impact

[-----------------------------------------------------------|-------------------------]  production period
                                                            |
                                                Impact      |
                                    <-----------------------|---------->                 publication period
                                                            |
                                                     (------|------)                     application period
                                                            |
                                                           now
                                         {----}
                                action period (ie period of the journey

 */
BOOST_FIXTURE_TEST_CASE(journey_now_inside_pub_period_before_application, disruption_periods_fixture) {
    auto now = inside_publish_inside_application;

    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, inside_publish_before_application)).size(), 0);

    // same if the action period is after the application period
    BOOST_REQUIRE_EQUAL(get_impacts(create_journey_request(now, inside_publish_after_application)).size(), 0);
}

/*
if the journey is requested the day of the disruption for the day of the disruption, we display the impact
and it's status is 'active'
[-----------------------------------------------------------|-------------------------]  production period
                                                            |
                                                Impact      |
                                    <-----------------------|---------->                 publication period
                                                            |
                                                     (------|------)                     application period
                                                            |
                                                           now
                                                          {----}
                                                      action period (ie period of the journey

 */
BOOST_FIXTURE_TEST_CASE(journey_inside_pub_period_inside_application, disruption_periods_fixture) {
    auto now = inside_publish_inside_application;
    const auto impacts = get_impacts(create_journey_request(now, inside_publish_inside_application));
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    BOOST_CHECK_EQUAL(impacts[0]->status(), pbnavitia::ActiveStatus::active);
}

/*
if ptref is queried inside the publication period, but after the application period, we display the impact
and it's status is 'past'
[-------------------------------------------------------------------|-----------------]  production period
                                                                    |
                                                Impact              |
                                    <-------------------------------|-->                 publication period
                                                                    |
                                                     (------------) |                    application period
                                                                    |
                                                                   now

 */
BOOST_FIXTURE_TEST_CASE(pt_inside_pub_period_after_application, disruption_periods_fixture) {
    auto now = inside_publish_after_application;
    const auto impacts = get_impacts(create_pt_request(now));
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    BOOST_CHECK_EQUAL(impacts[0]->status(), pbnavitia::ActiveStatus::past);
}
