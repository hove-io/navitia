/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

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
#define BOOST_TEST_MODULE test_realtime
#include <boost/test/unit_test.hpp>
#include "type/gtfs-realtime.pb.h"
#include "type/pt_data.h"
#include "type/kirin.pb.h"
#include "kraken/realtime.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "kraken/apply_disruption.h"
#include "disruption/traffic_reports_api.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace ntest = navitia::test;

static const std::string feed_id = "42";
static const std::string feed_id_1 = "44";
static const pt::ptime timestamp = "20150101T1337"_dt;

static transit_realtime::TripUpdate
make_cancellation_message(const std::string& vj_uri, const std::string& date) {
    transit_realtime::TripUpdate trip_update;
    auto trip = trip_update.mutable_trip();
    trip->set_trip_id(vj_uri);
    trip->set_start_date(date);
    trip->set_schedule_relationship(transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED);
    trip_update.SetExtension(kirin::trip_message, "cow on the tracks");

    return trip_update;
}


BOOST_AUTO_TEST_CASE(simple_train_cancellation) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);

    transit_realtime::TripUpdate trip_update = make_cancellation_message("vj:1", "20150928");
    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    // we should not have created any objects save for one validity_pattern
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    //the rt vp must be empty
    BOOST_CHECK_EQUAL(vj->rt_validity_pattern()->days, navitia::type::ValidityPattern::year_bitset());

    //check the disruption created
    const auto& impacts = vj->meta_vj->get_impacts();
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    auto impact = impacts.front();
    BOOST_REQUIRE(impact);
    BOOST_REQUIRE_EQUAL(impact->messages.size(), 1);
    BOOST_CHECK_EQUAL(impact->messages.front().text, "cow on the tracks");
    BOOST_CHECK(impact->aux_info.stop_times.empty());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    // we should not have created any objects save for one validity_pattern
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());
}

/*
 * the vj:1 is not in service the 09/29, so the cancellation on this day should not change anything
 *
 */
BOOST_AUTO_TEST_CASE(train_cancellation_on_unused_day) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);

    transit_realtime::TripUpdate trip_update = make_cancellation_message("vj:1", "20150929");
    const auto& pt_data = b.data->pt_data;

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());
}

BOOST_AUTO_TEST_CASE(simple_train_cancellation_routing) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);

    transit_realtime::TripUpdate trip_update = make_cancellation_message("vj:1", "20150928");
    const auto& pt_data = b.data->pt_data;

    pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    auto raptor = std::make_unique<navitia::routing::RAPTOR>(*(b.data));

    auto compute = [&](nt::RTLevel level, navitia::routing::RAPTOR& raptor) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    //on the theoric level, we should get one solution
    auto res = compute(nt::RTLevel::Base, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    //on the realtime level, we should also get one solution, since for the moment there is no cancellation
    res = compute(nt::RTLevel::RealTime, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    navitia::test::handle_realtime_test(feed_id, timestamp, trip_update, *b.data, raptor);

    //on the theoric level, we should still get one solution
    res = compute(nt::RTLevel::Base, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    //on the realtime we should now have no solution
    res = compute(nt::RTLevel::RealTime, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 0);
}

BOOST_AUTO_TEST_CASE(train_cancellation_with_choice_routing) {
    ed::builder b("20150928");
    b.vj("A", "111", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    b.vj("B")("stop1", "08:00"_t)("stop2", "09:30"_t);

    transit_realtime::TripUpdate trip_update = make_cancellation_message("vj:1", "20150928");
    const auto& pt_data = b.data->pt_data;

    pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    auto raptor = std::make_unique<navitia::routing::RAPTOR>(*(b.data));

    auto compute = [&](nt::RTLevel level, navitia::routing::RAPTOR& raptor) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    //on the theoric and realtime level, we should arrive at 9:00 (with line A)
    auto res = compute(nt::RTLevel::Base, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);
    res = compute(nt::RTLevel::RealTime, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);

    // we cancel the vj1
    navitia::test::handle_realtime_test(feed_id, timestamp, trip_update, *b.data, raptor);

    // on the theoric, nothing has changed
    res = compute(nt::RTLevel::Base, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);

    // but on the realtime we now arrive at 09:30
    res = compute(nt::RTLevel::RealTime, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0930"_dt);
}

BOOST_AUTO_TEST_CASE(train_delayed) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0810"_pts, "20150928T0810"_pts),
                    std::make_tuple("stop2", "20150928T0910"_pts, "20150928T0910"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("bob", timestamp, trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    //check the disruption created
    const auto& impacts = vj->meta_vj->get_impacts();
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    auto impact = impacts.front();
    BOOST_REQUIRE(impact);
    BOOST_CHECK(impact->messages.empty());
    const auto& stus = impact->aux_info.stop_times;
    BOOST_REQUIRE_EQUAL(stus.size(), 2);
    BOOST_CHECK_EQUAL(stus[0].cause, "birds on the tracks");
    BOOST_CHECK_EQUAL(stus[1].cause, "birds on the tracks");

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("bob", timestamp, trip_update, *b.data);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    BOOST_REQUIRE_EQUAL(impacts.size(), 1);  //we should still have only one impact

    // we add a third time the same message but with a different id, it should not change anything
    // but for the number of impacts in the meta vj
    navitia::handle_realtime("bobette", timestamp, trip_update, *b.data);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 2);

    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0910"_dt);
}

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(train_delayed_expected_failure, 1)

BOOST_AUTO_TEST_CASE(train_delayed_expected_failure) {

    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0810"_pts, "20150928T0810"_pts),
                    std::make_tuple("stop2", "20150928T0910"_pts, "20150928T0910"_pts)
            });
    b.data->build_uri();
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    /*****************Expected Failure********************/
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2); // Current size is 3, hopefully it should be 2
    /*****************************************************/

}

BOOST_AUTO_TEST_CASE(two_different_delays_on_same_vj) {

    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t)("stop3", "10:01"_t);

    transit_realtime::TripUpdate trip_update_1 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0810"_pts, "20150928T0810"_pts),
                    std::make_tuple("stop2", "20150928T0910"_pts, "20150928T0910"_pts), // <--- delayed
                    std::make_tuple("stop3", "20150928T1001"_pts, "20150928T1001"_pts)
            });

    transit_realtime::TripUpdate trip_update_2 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0810"_pts, "20150928T0810"_pts),
                    std::make_tuple("stop2", "20150928T0910"_pts, "20150928T0910"_pts), // <--- delayed
                    std::make_tuple("stop3", "20150928T1030"_pts, "20150928T1030"_pts)  // <--- delayed
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_1, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    pt_data->index();
    b.finish();
    b.data->build_raptor();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to),
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0910"_dt);

    }

    // we add a second time the realtime message
    navitia::handle_realtime(feed_id_1, timestamp, trip_update_2, *b.data);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 3);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    pt_data->index();
    b.finish();
    b.data->build_raptor();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to),
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0910"_dt);

        res = compute(nt::RTLevel::Base, "stop1", "stop3");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T1001"_dt);

        res = compute(nt::RTLevel::RealTime, "stop1", "stop3");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T1030"_dt);
    }


}

BOOST_AUTO_TEST_CASE(train_pass_midnight_delayed) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "23:00"_t)("stop2", "23:55"_t);

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T2330"_pts, "20150928T2330"_pts),
                    std::make_tuple("stop2", "20150929T0025"_pts, "20150929T0025"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T2355"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0025"_dt);
}

BOOST_AUTO_TEST_CASE(add_two_delay_disruption) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "23:00"_t)("stop2", "23:55"_t);
    b.vj("B", "000001", "", true, "vj:2")("stop3", "22:00"_t)("stop4", "22:30"_t);

    transit_realtime::TripUpdate trip_update_A = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T2330"_pts, "20150928T2330"_pts),
                    std::make_tuple("stop2", "20150929T0025"_pts, "20150929T0025"_pts)
            });

    transit_realtime::TripUpdate trip_update_B = ntest::make_delay_message("vj:2",
            "20150928",
            {
                    std::make_tuple("stop3", "20150928T2230"_pts, "20150928T2230"_pts),
                    std::make_tuple("stop4", "20150928T2300"_pts, "20150928T2300"_pts)
            });

    b.data->build_uri();

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj_A = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_EQUAL(vj_A->base_validity_pattern(), vj_A->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_A, *b.data);

    // We should have 3 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 3);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 2);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj_A->base_validity_pattern(), vj_A->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id_1, timestamp, trip_update_B, *b.data);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    auto vj_B = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_NE(vj_B->base_validity_pattern(), vj_B->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
        return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to),
                "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T2355"_dt);

    res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0025"_dt);


    res = compute(nt::RTLevel::Base, "stop3", "stop4");
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T2230"_dt);

    res = compute(nt::RTLevel::RealTime, "stop3", "stop4");
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T2300"_dt);

}

BOOST_AUTO_TEST_CASE(add_blocking_disruption_and_delay_disruption) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    transit_realtime::TripUpdate trip_update_A = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0810"_pts, "20150928T0810"_pts),
                    std::make_tuple("stop2", "20150928T0910"_pts, "20150928T0910"_pts)
            });
    b.data->build_uri();

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj_A = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_EQUAL(vj_A->base_validity_pattern(), vj_A->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_A, *b.data);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj_A->base_validity_pattern(), vj_A->rt_validity_pattern());

    using btp = boost::posix_time::time_period;
    const auto& disrup = b.impact(nt::RTLevel::RealTime)
                     .severity(nt::disruption::Effect::NO_SERVICE)
                     .on(nt::Type_e::MetaVehicleJourney, "vj:1")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    const auto& vj = pt_data->vehicle_journeys_map.at("vj:1");
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::apply_disruption(disrup, *b.data->pt_data, *b.data->meta);

    pt_data->index();
    b.finish();
    b.data->build_raptor();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to),
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);

        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 0); // <-- No VJ is availble
    }

    navitia::delete_disruption(std::string(disrup.uri), *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 3);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    //no cleanup for the moment, but it would be nice
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // but the vp should be equals again
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());
    pt_data->index();
    b.finish();
    b.data->build_raptor();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to),
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);

        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0910"_dt);
    }
}

BOOST_AUTO_TEST_CASE(invalid_delay) {
    // we add a non valid delay, it shoudl be rejected and no disruption added
    ed::builder b("20150928");
    auto vj = b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t).make();
    b.data->build_uri();

    transit_realtime::TripUpdate wrong_st_order = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    //stop1 is after stop2, it's not valid
                    std::make_tuple("stop1", "20150928T1010"_pts, "20150928T1010"_pts),
                    std::make_tuple("stop2", "20150928T0910"_pts, "20150928T0910"_pts)
            });

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    navitia::handle_realtime(feed_id, timestamp, wrong_st_order, *b.data);

    //there should be no disruption added
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    // we test with a wrongly formated stoptime
    transit_realtime::TripUpdate dep_before_arr = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0810"_pts, "20150928T0810"_pts),
                    //departure is before arrival, it's not valid too
                    std::make_tuple("stop2", "20150928T0910"_pts, "20150928T0900"_pts)
            });

    navitia::handle_realtime(feed_id, timestamp, dep_before_arr, *b.data);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    //we test with a first stop time before the day of the disrupted vj
    transit_realtime::TripUpdate wrong_first_st = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150926T0810"_pts, "20150927T0210"_pts),
                    //departure is before arrival, it's not valid too
                    std::make_tuple("stop2", "20150927T0310"_pts, "20150927T0400"_pts)
            });

    navitia::handle_realtime(feed_id, timestamp, wrong_first_st, *b.data);

    //there should be no disruption added
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);
}

/* testing a vj delayed to the day after is accepted and correctly handled
 * (only passing same disruption 2 times)
 * Disruptions sent:
 * D0: +1 day (two times)
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0610"_pts, "20150929T0610"_pts),
                    std::make_tuple("stop2", "20150929T0710"_pts, "20150929T0710"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1DayD0_bis", timestamp, trip_update, *b.data);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0710"_dt);
}

/* testing a vj delayed to the day after and passing midnight is accepted and correctly handled
 * (only passing same disruption 2 times)
 * Disruptions sent:
 * D0: +1 day pass-midnight (two times)
 */
BOOST_AUTO_TEST_CASE(train_delayed_pass_midnight_day_after) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T1710"_pts, "20150929T1710"_pts),
                    std::make_tuple("stop2", "20150930T0110"_pts, "20150930T0110"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayPassMidnightD0", timestamp, trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1DayPassMidnightD0_bis", timestamp, trip_update, *b.data);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                hour, day, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base, "08:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    // we cannot find a vj whose departure is 24 hours later the requested time hour
    res = compute(nt::RTLevel::RealTime, "8:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 0);


    res = compute(nt::RTLevel::RealTime, "8:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150930T0110"_dt);
}

/* testing a vj delayed to the day after (1 day late)
 * then a second disruption on the same day is pushed, bringing back the vj to just 1 hour late
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D0: +1 hour
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_one_hour) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0901"_pts, "20150928T0901"_pts),
                    std::make_tuple("stop2", "20150928T1001"_pts, "20150928T1001"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1HourD0", timestamp, second_trip_update, *b.data);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T1001"_dt);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
            "06:00"_t, 1, navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, 2_min, true);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0901"_dt);
}

/* testing a vj delayed to the day after (1 day late)
 * then a second disruption on the same day is pushed, bringing back the vj to normal
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D0: back to normal
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_back_to_normal) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0801"_pts, "20150928T0801"_pts),
                    std::make_tuple("stop2", "20150928T0901"_pts, "20150928T0901"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("backToNormalD0", timestamp, second_trip_update, *b.data);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
            "06:00"_t, 1, navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, 2_min, true);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0901"_dt);
}

/* testing a vj delayed to the day after (1 day late)
 * then a second disruption on the day after is pushed, delaying the vj of D+1 of 1 hour
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D1: +1 hour
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_one_hour_on_next_day) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = ntest::make_delay_message("vj:1",
            "20150929",
            {
                    std::make_tuple("stop1", "20150929T0901"_pts, "20150929T0901"_pts),
                    std::make_tuple("stop2", "20150929T1001"_pts, "20150929T1001"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1HourD1", timestamp, second_trip_update, *b.data);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 5);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                hour, day, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base, "08:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime, "10:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0810"_dt);

    res = compute(nt::RTLevel::RealTime, "09:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T1001"_dt);

}

/* testing a vj delayed to the day after (1 day late)
 * then a second disruption on the same day is pushed, canceling the vj
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D0: cancel
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_cancel) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0610"_pts, "20150929T0610"_pts),
                    std::make_tuple("stop2", "20150929T0710"_pts, "20150929T0710"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = make_cancellation_message("vj:1", "20150928");
    b.data->build_uri();

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD0", timestamp, second_trip_update, *b.data);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                hour, day, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base, "08:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime, "09:02"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0901"_dt);
}

/* testing a vj delayed to the day after (1 day late)
 * then a second disruption on the day after is pushed, canceling the vj
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D1: cancel
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_day_after_cancel) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0610"_pts, "20150929T0610"_pts),
                    std::make_tuple("stop2", "20150929T0710"_pts, "20150929T0710"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = make_cancellation_message("vj:1", "20150929");
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1dayD0", timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD1", timestamp, second_trip_update, *b.data);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                hour, day, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base, "08:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime, "08:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0710"_dt);
}

/* testing a vj canceled first day, then a vj canceled second day
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: cancel
 * D1: cancel
 */
BOOST_AUTO_TEST_CASE(train_canceled_first_day_then_cancel_second_day) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = make_cancellation_message("vj:1", "20150928");
    transit_realtime::TripUpdate second_trip_update = make_cancellation_message("vj:1", "20150929");
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("cancelD0", timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD1", timestamp, second_trip_update, *b.data);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
            "09:02"_t, 1, navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, 2_min,true);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150930T0901"_dt);
}

/* testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +10 hours
 * D0: cancel
 */
BOOST_AUTO_TEST_CASE(train_delayed_10_hours_then_canceled) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T1801"_pts, "20150928T1801"_pts),
                    std::make_tuple("stop2", "20150928T1901"_pts, "20150928T1901"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = make_cancellation_message("vj:1", "20150928");
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay10HoursD0", timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD0", timestamp, second_trip_update, *b.data);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    pt_data->index();
    b.finish();
    b.data->build_raptor();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                hour, day, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base, "08:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime, "09:02"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0901"_dt);
}

/* testing a vj delayed on first day
 * then a second delay on the second day after is pushed
 * testing that we retrieve impacts correctly
 * Disruptions sent:
 * D0: +1 hour
 * D1: +2 hour
 * D2: cancel
 */
BOOST_AUTO_TEST_CASE(get_impacts_on_vj) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0910"_pts, "20150928T0910"_pts),
                    std::make_tuple("stop2", "20150928T1010"_pts, "20150928T1010"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = ntest::make_delay_message("vj:1",
            "20150929",
            {
                    std::make_tuple("stop1", "20150929T1010"_pts, "20150929T1010"_pts),
                    std::make_tuple("stop2", "20150929T1110"_pts, "20150929T1110"_pts)
            });
    transit_realtime::TripUpdate third_trip_update = make_cancellation_message("vj:1", "20150930");

    b.data->build_uri();

    const auto& pt_data = b.data->pt_data;
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(vj->get_impacts().size(), 0);

    navitia::handle_realtime("delay1hourD0", timestamp, first_trip_update, *b.data);

    // get vj realtime for d0 and check it's on day 0
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_rt_vj().size(), 1);
    const auto vj_rt_d0 = vj->meta_vj->get_rt_vj()[0].get();
    BOOST_CHECK(vj_rt_d0->get_validity_pattern_at(vj_rt_d0->realtime_level)->check(0));

    BOOST_REQUIRE_EQUAL(vj->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_REQUIRE_EQUAL(vj_rt_d0->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d0->get_impacts()[0]->uri, "delay1hourD0");

    navitia::handle_realtime("delay2hourD1", timestamp, second_trip_update, *b.data);

    // get vj realtime for d1 and check it's on day 1
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 3);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_rt_vj().size(), 2);
    const auto vj_rt_d1 = vj->meta_vj->get_rt_vj()[1].get();
    BOOST_CHECK(vj_rt_d1->get_validity_pattern_at(vj_rt_d1->realtime_level)->check(1));

    BOOST_REQUIRE_EQUAL(vj->get_impacts().size(), 2);
    BOOST_CHECK_EQUAL(vj->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_CHECK_EQUAL(vj->get_impacts()[1]->uri, "delay2hourD1");
    BOOST_REQUIRE_EQUAL(vj_rt_d0->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d0->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_REQUIRE_EQUAL(vj_rt_d1->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d1->get_impacts()[0]->uri, "delay2hourD1");

    navitia::handle_realtime("cancelD3", timestamp, third_trip_update, *b.data);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 3);
    BOOST_REQUIRE_EQUAL(vj->get_impacts().size(), 3);
    BOOST_CHECK_EQUAL(vj->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_CHECK_EQUAL(vj->get_impacts()[1]->uri, "delay2hourD1");
    BOOST_CHECK_EQUAL(vj->get_impacts()[2]->uri, "cancelD3");
    BOOST_REQUIRE_EQUAL(vj_rt_d0->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d0->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_REQUIRE_EQUAL(vj_rt_d1->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d1->get_impacts()[0]->uri, "delay2hourD1");
}

BOOST_AUTO_TEST_CASE(traffic_reports_vehicle_journeys) {
    ed::builder b("20150928");
    b.vj_with_network("nt", "A", "000111", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    b.vj_with_network("nt", "A", "000111", "", true, "vj:2")("stop1", "08:10"_t)("stop2", "09:10"_t);
    b.vj_with_network("nt", "A", "000111", "", true, "vj:3")("stop1", "08:20"_t)("stop2", "09:20"_t);
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    transit_realtime::TripUpdate trip_update_vj2 = ntest::make_delay_message(
        "vj:2",
        "20150928",
        {
            std::make_tuple("stop1", "20150928T0910"_pts, "20150928T0910"_pts),
            std::make_tuple("stop2", "20150929T1010"_pts, "20150929T1010"_pts)
        });
    transit_realtime::TripUpdate trip_update_vj3 = make_cancellation_message("vj:3", "20150928");
    navitia::handle_realtime("trip_update_vj2", timestamp, trip_update_vj2, *b.data);
    navitia::handle_realtime("trip_update_vj3", timestamp, trip_update_vj3, *b.data);

    const auto resp = navitia::disruption::traffic_reports(*b.data, "20150928T0830"_pts, 1, 10, 0, "", {});
    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 1);
    BOOST_REQUIRE_EQUAL(resp.traffic_reports(0).vehicle_journeys_size(), 1);
    BOOST_CHECK_EQUAL(resp.traffic_reports(0).vehicle_journeys(0).uri(), "vj:3");
}

BOOST_AUTO_TEST_CASE(traffic_reports_vehicle_journeys_no_base) {
    ed::builder b("20150928");
    b.vj_with_network("nt", "A", "000110", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message(
        "vj:1",
        "20150928",
        {
            std::make_tuple("stop1", "20150928T0910"_pts, "20150928T0910"_pts),
            std::make_tuple("stop2", "20150929T1010"_pts, "20150929T1010"_pts)
        });
    navitia::handle_realtime("trip_update", timestamp, trip_update, *b.data);
    const auto resp = navitia::disruption::traffic_reports(*b.data, "20150928T0830"_pts, 1, 10, 0, "", {});
    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 0);
}

/* Test unknown stop point
 * Since the stop point is unknown, the trip update should be ignored
 *
 * */
BOOST_AUTO_TEST_CASE(unknown_stop_point) {

    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t)("stop3", "10:01"_t);

    transit_realtime::TripUpdate bad_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0810"_pts, "20150928T0810"_pts),
                    std::make_tuple("stop_unknown_toto", "20150928T0910"_pts, "20150928T0910"_pts), // <--- bad
                    std::make_tuple("stop3", "20150928T1001"_pts, "20150928T1001"_pts)
            });

    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, bad_trip_update, *b.data);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    pt_data->index();
    b.finish();
    b.data->build_raptor();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to),
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);
        
        // Nothing has changed, the result is the same than Base
        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    }

}

