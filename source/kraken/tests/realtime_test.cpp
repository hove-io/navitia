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
#include "kraken/realtime.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "kraken/apply_disruption.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace nt = navitia::type;
namespace pt = boost::posix_time;

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

    return trip_update;
}

static transit_realtime::TripUpdate
make_delay_message(const std::string& vj_uri,
        const std::string& date,
        const std::vector<std::tuple<std::string, int, int>>& delayed_time_stops) {
    transit_realtime::TripUpdate trip_update;
    auto trip = trip_update.mutable_trip();
    trip->set_trip_id(vj_uri);
    trip->set_start_date(date);
    trip->set_schedule_relationship(transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED);
    auto st_update = trip_update.mutable_stop_time_update();

    for (const auto& delayed_st: delayed_time_stops) {
        auto stop_time = st_update->Add();
        auto arrival = stop_time->mutable_arrival();
        auto departure = stop_time->mutable_departure();
        stop_time->set_stop_id(std::get<0>(delayed_st));
        arrival->set_time(std::get<1>(delayed_st));
        departure->set_time(std::get<2>(delayed_st));
    }

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
    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    //on the theoric level, we should get one solution
    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    //on the realtime level, we should also get one solution, since for the moment there is no cancellation
    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    //on the theoric level, we should still get one solution
    res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    //on the realtime we should now have no solution
    res = compute(nt::RTLevel::RealTime);
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
    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    //on the theoric and realtime level, we should arrive at 9:00 (with line A)
    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);
    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);

    // we cancel the vj1
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    // on the theoric, nothing has changed
    res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);

    // but on the realtime we now arrive at 09:30
    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0930"_dt);
}

BOOST_AUTO_TEST_CASE(train_delayed) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate trip_update = make_delay_message("vj:1",
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
    navitia::handle_realtime(feed_id_1, timestamp, trip_update, *b.data);


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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
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

    transit_realtime::TripUpdate trip_update = make_delay_message("vj:1",
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

    transit_realtime::TripUpdate trip_update_1 = make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T0810"_pts, "20150928T0810"_pts),
                    std::make_tuple("stop2", "20150928T0910"_pts, "20150928T0910"_pts), // <--- delayed
                    std::make_tuple("stop3", "20150928T1001"_pts, "20150928T1001"_pts)
            });

    transit_realtime::TripUpdate trip_update_2 = make_delay_message("vj:1",
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
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
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
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
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

    transit_realtime::TripUpdate trip_update = make_delay_message("vj:1",
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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
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

    transit_realtime::TripUpdate trip_update_A = make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150928T2330"_pts, "20150928T2330"_pts),
                    std::make_tuple("stop2", "20150929T0025"_pts, "20150929T0025"_pts)
            });

    transit_realtime::TripUpdate trip_update_B = make_delay_message("vj:2",
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
                "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
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
    transit_realtime::TripUpdate trip_update_A = make_delay_message("vj:1",
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
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
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
                    "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
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

    transit_realtime::TripUpdate wrong_st_order = make_delay_message("vj:1",
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
    transit_realtime::TripUpdate dep_before_arr = make_delay_message("vj:1",
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
    transit_realtime::TripUpdate wrong_first_st = make_delay_message("vj:1",
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

/* testing that a vj delayed to the day after is accepted and correctly handled
 * (only passing same disruption 2 times)
 * Disruptions sent:
 * D0: +1 day (two times)
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate trip_update = make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
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
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id_1, timestamp, trip_update, *b.data);


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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0810"_dt);
}

/* testing that a vj delayed to the day after and passing midnight is accepted and correctly handled
 * (only passing same disruption 2 times)
 * Disruptions sent:
 * D0: +1 day pass-midnight (two times)
 */
BOOST_AUTO_TEST_CASE(train_delayed_pass_midnight_day_after) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate trip_update = make_delay_message("vj:1",
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

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id_1, timestamp, trip_update, *b.data);


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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
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

    transit_realtime::TripUpdate first_trip_update = make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = make_delay_message("vj:1",
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

    navitia::handle_realtime(feed_id, timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id_1, timestamp, second_trip_update, *b.data);


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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T1001"_dt);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
            "06:00"_t, 1, navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, true);
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

    transit_realtime::TripUpdate first_trip_update = make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = make_delay_message("vj:1",
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

    navitia::handle_realtime(feed_id, timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id_1, timestamp, second_trip_update, *b.data);


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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
            "06:00"_t, 1, navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, true);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0901"_dt);
}

/* testing that a vj delayed to the day after (1 day late)
 * then a second disruption on the day after is pushed, delaying the vj of D+1 of 1 hour
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D1: +1 hour
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_one_hour_on_next_day) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
            });
    transit_realtime::TripUpdate second_trip_update = make_delay_message("vj:1",
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

    navitia::handle_realtime(feed_id, timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id_1, timestamp, second_trip_update, *b.data);


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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0810"_dt);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"),
            "09:00"_t, 1, navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, true);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T1001"_dt);

}

/* testing that a vj delayed to the day after (1 day late)
 * then a second disruption on the same day is pushed, canceling the vj
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D0: cancel
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_cancel) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
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

    navitia::handle_realtime(feed_id, timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id_1, timestamp, second_trip_update, *b.data);


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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0901"_dt);
}

/* testing that a vj delayed to the day after (1 day late)
 * then a second disruption on the day after is pushed, canceling the vj
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D1: cancel
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_day_after_cancel) {
    ed::builder b("20150928");
    b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate first_trip_update = make_delay_message("vj:1",
            "20150928",
            {
                    std::make_tuple("stop1", "20150929T0710"_pts, "20150929T0710"_pts),
                    std::make_tuple("stop2", "20150929T0810"_pts, "20150929T0810"_pts)
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

    navitia::handle_realtime(feed_id, timestamp, first_trip_update, *b.data);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id_1, timestamp, second_trip_update, *b.data);


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
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0810"_dt);
}
