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
#define BOOST_TEST_MODULE test_realtime
#include <boost/test/unit_test.hpp>
#include "type/gtfs-realtime.pb.h"
#include "type/pt_data.h"
#include "type/kirin.pb.h"
#include "kraken/realtime.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "routing/raptor_api.h"
#include "kraken/apply_disruption.h"
#include "disruption/traffic_reports_api.h"
#include "type/pb_converter.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger("logger", "TRACE"); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace ntest = navitia::test;
namespace tr = transit_realtime;
namespace ptref = navitia::ptref;

using ntest::RTStopTime;

static const std::string feed_id = "42";
static const std::string feed_id_1 = "44";
static const std::string comp_id_1 = "Comp_id_1";
static const pt::ptime timestamp = "20150101T1337"_dt;

static transit_realtime::TripUpdate make_cancellation_message(const std::string& vj_uri, const std::string& date) {
    return ntest::make_trip_update_message(vj_uri, date, {}, transit_realtime::Alert_Effect::Alert_Effect_NO_SERVICE,
                                           "", "", "cow.owner", "cow on the tracks");
}

static pbnavitia::Response compute_iti(const ed::builder& b,
                                       const char* datetime,
                                       const std::string& from,
                                       const std::string& to,
                                       const navitia::type::RTLevel level) {
    navitia::type::EntryPoint origin(b.data->get_type_of_id(from), from);
    navitia::type::EntryPoint destination(b.data->get_type_of_id(to), to);

    navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                  navitia::type::AccessibiliteParams(), {}, {}, sn_worker, level, 2_min);

    return pb_creator.get_response();
}

BOOST_AUTO_TEST_CASE(simple_train_cancellation) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    });

    transit_realtime::TripUpdate trip_update = make_cancellation_message("vj:1", "20150928");
    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true, true);

    // we should not have created any objects save for one validity_pattern
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // the rt vp must be empty
    BOOST_CHECK_EQUAL(vj->rt_validity_pattern()->days, navitia::type::ValidityPattern::year_bitset());

    // check the disruption created
    const auto& impacts = vj->meta_vj->get_impacts();
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    auto impact = impacts.front();
    BOOST_REQUIRE(impact);
    BOOST_REQUIRE_EQUAL(impact->messages.size(), 1);
    BOOST_CHECK_EQUAL(impact->messages.front().text, "cow on the tracks");
    BOOST_CHECK(impact->aux_info.stop_times.empty());

    // here we verify contributor:
    auto disruption = b.data->pt_data->disruption_holder.get_disruption(impact->uri);
    BOOST_REQUIRE_EQUAL(disruption->contributor, "cow.owner");

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true, true);

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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    });

    transit_realtime::TripUpdate trip_update = make_cancellation_message("vj:1", "20150929");
    const auto& pt_data = b.data->pt_data;

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true, true);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());
}

BOOST_AUTO_TEST_CASE(simple_train_cancellation_routing) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    });

    transit_realtime::TripUpdate trip_update = make_cancellation_message("vj:1", "20150928");
    const auto& pt_data = b.data->pt_data;

    auto raptor = std::make_unique<navitia::routing::RAPTOR>(*(b.data));

    auto compute = [&](nt::RTLevel level, navitia::routing::RAPTOR& raptor) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    // on the theoric level, we should get one solution
    auto res = compute(nt::RTLevel::Base, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    // on the realtime level, we should also get one solution, since for the moment there is no cancellation
    res = compute(nt::RTLevel::RealTime, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    navitia::test::handle_realtime_test(feed_id, timestamp, trip_update, *b.data, raptor);

    // on the theoric level, we should still get one solution
    res = compute(nt::RTLevel::Base, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    // on the realtime we should now have no solution
    res = compute(nt::RTLevel::RealTime, *raptor);
    BOOST_REQUIRE_EQUAL(res.size(), 0);
}

BOOST_AUTO_TEST_CASE(train_cancellation_with_choice_routing) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "111", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj("B")("stop1", "08:00"_t)("stop2", "09:30"_t);
    });

    transit_realtime::TripUpdate trip_update = make_cancellation_message("vj:1", "20150928");
    const auto& pt_data = b.data->pt_data;

    auto raptor = std::make_unique<navitia::routing::RAPTOR>(*(b.data));

    auto compute = [&](nt::RTLevel level, navitia::routing::RAPTOR& raptor) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    // on the theoric and realtime level, we should arrive at 9:00 (with line A)
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

        // to test company
        auto* cmp1 = new navitia::type::Company();
        cmp1->line_list.push_back(b.lines["A"]);
        cmp1->idx = b.data->pt_data->companies.size();
        cmp1->name = "Comp1";
        cmp1->uri = comp_id_1;
        b.data->pt_data->companies.push_back(cmp1);
        b.data->pt_data->companies_map[comp_id_1] = cmp1;
        b.lines["A"]->company_list.push_back(cmp1);
    });

    transit_realtime::TripUpdate trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0810"_pts).delay(9_min), RTStopTime("stop2", "20150928T0910"_pts).delay(9_min)},
        transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS, comp_id_1);

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("bob", timestamp, trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys[1]->company->uri, comp_id_1);
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys[1]->company->name, "Comp1");
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // check the disruption created
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
    navitia::handle_realtime("bob", timestamp, trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    BOOST_REQUIRE_EQUAL(impacts.size(), 1);  // we should still have only one impact

    // we add a third time the same message but with a different id, it should not change anything
    // but for the number of impacts in the meta vj
    navitia::handle_realtime("bobette", timestamp, trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 2);

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0910"_dt);

    // testing accessibility on the way, as VJ created is wheelchair-accessible and should stay accessible after
    // delay application
    nt::AccessibiliteParams accessibility_params;
    accessibility_params.properties.set(nt::hasProperties::WHEELCHAIR_BOARDING, true);
    accessibility_params.vehicle_properties.set(nt::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE, true);

    auto compute_wheelchair = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true, accessibility_params);
    };

    auto res_wheelchair = compute_wheelchair(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res_wheelchair.size(), 1);
    BOOST_CHECK_EQUAL(res_wheelchair[0].items[0].arrival, "20150928T0901"_dt);

    res_wheelchair = compute_wheelchair(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res_wheelchair.size(), 1);
    BOOST_CHECK_EQUAL(res_wheelchair[0].items[0].arrival, "20150928T0910"_dt);
}

BOOST_AUTO_TEST_CASE(train_delayed_vj_cleaned_up) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0810"_pts).delay(9_min), RTStopTime("stop2", "20150928T0910"_pts).delay(9_min)});
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true, true);
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
}

BOOST_AUTO_TEST_CASE(two_different_delays_on_same_vj) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t)("stop3", "10:01"_t);
    });

    transit_realtime::TripUpdate trip_update_1 = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0810"_pts), RTStopTime("stop2", "20150928T0910"_pts).delay(9_min),
         RTStopTime("stop3", "20150928T1001"_pts)});

    transit_realtime::TripUpdate trip_update_2 = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0810"_pts), RTStopTime("stop2", "20150928T0910"_pts).delay(9_min),
         RTStopTime("stop3", "20150928T1030"_pts).delay(29_min)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_1, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to), "08:00"_t, 0,
                                  navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0910"_dt);
    }

    // we add a second time the realtime message
    navitia::handle_realtime(feed_id_1, timestamp, trip_update_2, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to), "08:00"_t, 0,
                                  navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "23:00"_t)("stop2", "23:55"_t);
    });

    transit_realtime::TripUpdate trip_update =
        ntest::make_trip_update_message("vj:1", "20150928",
                                        {RTStopTime("stop1", "20150928T2330"_pts).delay(30_min),
                                         RTStopTime("stop2", "20150929T0025"_pts).delay(30_min)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true, true);

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
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T2355"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0025"_dt);
}

BOOST_AUTO_TEST_CASE(add_two_delay_disruption) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "23:00"_t)("stop2", "23:55"_t);
        b.vj("B", "000001", "", true, "vj:2")("stop3", "22:00"_t)("stop4", "22:30"_t);
    });

    transit_realtime::TripUpdate trip_update_A =
        ntest::make_trip_update_message("vj:1", "20150928",
                                        {RTStopTime("stop1", "20150928T2330"_pts).delay(30_min),
                                         RTStopTime("stop2", "20150929T0025"_pts).delay(30_min)});

    transit_realtime::TripUpdate trip_update_B =
        ntest::make_trip_update_message("vj:2", "20150928",
                                        {RTStopTime("stop3", "20150928T2230"_pts).delay(30_min),
                                         RTStopTime("stop4", "20150928T2300"_pts).delay(30_min)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj_A = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_EQUAL(vj_A->base_validity_pattern(), vj_A->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_A, *b.data, true, true);

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
    navitia::handle_realtime(feed_id_1, timestamp, trip_update_B, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    auto vj_B = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_NE(vj_B->base_validity_pattern(), vj_B->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
        return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    });
    transit_realtime::TripUpdate trip_update_A =
        ntest::make_trip_update_message("vj:1", "20150928",
                                        {RTStopTime("stop1", "20150928T0810"_pts).delay(10_min),
                                         RTStopTime("stop2", "20150928T0910"_pts).delay(10_min)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj_A = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_EQUAL(vj_A->base_validity_pattern(), vj_A->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_A, *b.data, true, true);

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
                             .on(nt::Type_e::MetaVehicleJourney, "vj:1", *b.data->pt_data)
                             .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                             .get_disruption();

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    const auto* vj = pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1");
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::apply_disruption(disrup, *b.data->pt_data, *b.data->meta);

    b.finalize_disruption_batch();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to), "08:00"_t, 0,
                                  navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0900"_dt);

        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 0);  // <-- No VJ is availble
    }

    navitia::delete_disruption(std::string(disrup.uri), *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // no cleanup for the moment, but it would be nice
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // but the vp should be equals again
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());
    b.finalize_disruption_batch();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to), "08:00"_t, 0,
                                  navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    navitia::type::VehicleJourney* vj = nullptr;
    ed::builder b("20150928", [&](ed::builder& b) {
        vj = b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t).make();
    });

    transit_realtime::TripUpdate wrong_st_order = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {// stop1 is after stop2, it's not valid
         RTStopTime("stop1", "20150928T1000"_pts).delay(2_h), RTStopTime("stop2", "20150928T0910"_pts).delay(10_min)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    navitia::handle_realtime(feed_id, timestamp, wrong_st_order, *b.data, true, true);

    // there should be no disruption added
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    // we test with a wrongly formated stoptime
    transit_realtime::TripUpdate dep_before_arr = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0810"_pts).delay(10_min),
         // departure is before arrival, it's not valid too
         RTStopTime("stop2", "20150928T0910"_pts, "20150928T0900"_pts).arrival_delay(9_min)});

    navitia::handle_realtime(feed_id, timestamp, dep_before_arr, *b.data, true, true);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    // we test with a first stop time before the day of the disrupted vj
    transit_realtime::TripUpdate wrong_first_st = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150926T0800"_pts, "20150927T0200"_pts).arrival_delay(10_min).departure_delay(18_h),
         RTStopTime("stop2", "20150927T0300"_pts).delay(18_h)});

    navitia::handle_realtime(feed_id, timestamp, wrong_first_st, *b.data, true, true);

    // there should be no disruption added
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150929T0610"_pts).delay(9_min), RTStopTime("stop2", "20150929T0710"_pts).delay(9_min)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1DayD0_bis", timestamp, trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150929T1710"_pts).delay(9_h), RTStopTime("stop2", "20150930T0110"_pts).delay(16_h)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayPassMidnightD0", timestamp, trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1DayPassMidnightD0_bis", timestamp, trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), hour, day,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate first_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150929T0710"_pts).delay(23_h), RTStopTime("stop2", "20150929T0810"_pts).delay(23_h)});
    transit_realtime::TripUpdate second_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0901"_pts).delay(24_h), RTStopTime("stop2", "20150928T1001"_pts).delay(24_h)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1HourD0", timestamp, second_trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T1001"_dt);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "06:00"_t, 1,
                         navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, 2_min, 2_min, true);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150929T0901"_dt);
}

BOOST_AUTO_TEST_CASE(add_delays_and_back_to_normal) {
    ed::builder b("20190101", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    });

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    transit_realtime::TripUpdate delayed = ntest::make_trip_update_message(
        "vj:1", "20190101",
        {RTStopTime("stop1", "20190101T0900"_pts).delay(1_h), RTStopTime("stop2", "20190101T1000"_pts).delay(1_h)},
        transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS);

    navitia::handle_realtime("feed-1", timestamp, delayed, *b.data, true, true);
    b.finalize_disruption_batch();

    auto res = compute("20190101T073000", "stop1", "stop2");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 2);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).departure_date_time(), "20190101T090000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).stop_point().uri(), "stop1");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).departure_date_time(), "20190101T100000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).stop_point().uri(), "stop2");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    // impact
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).uri(), "feed-1");
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_SIGNIFICANT_DELAYS);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops_size(), 2);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::DELAYED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::DELAYED);

    transit_realtime::TripUpdate back_to_normal = ntest::make_trip_update_message(
        "vj:1", "20190101", {RTStopTime("stop1", "20190101T0800"_pts), RTStopTime("stop2", "20190101T0900"_pts)},
        transit_realtime::Alert_Effect::Alert_Effect_UNKNOWN_EFFECT);

    navitia::handle_realtime("feed-1", timestamp, back_to_normal, *b.data, true, true);
    b.finalize_disruption_batch();

    res = compute("20190101T073000", "stop1", "stop2");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 2);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).departure_date_time(), "20190101T080000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).stop_point().uri(), "stop1");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).departure_date_time(), "20190101T090000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).stop_point().uri(), "stop2");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    // Unknown effect impact is only with disruption API
    BOOST_CHECK_EQUAL(res.impacts_size(), 0);
}

/* testing a vj delayed to the day after (1 day late)
 * then a second disruption on the same day is pushed, bringing back the vj to normal
 * testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +1 day
 * D0: back to normal
 */
BOOST_AUTO_TEST_CASE(train_delayed_day_after_then_back_to_normal) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate first_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150929T0710"_pts).delay(23_h), RTStopTime("stop2", "20150929T0810"_pts).delay(23_h)},
        transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS);
    transit_realtime::TripUpdate second_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928", {RTStopTime("stop1", "20150928T0801"_pts), RTStopTime("stop2", "20150928T0901"_pts)},
        transit_realtime::Alert_Effect::Alert_Effect_UNKNOWN_EFFECT);

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("backToNormalD0", timestamp, second_trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "06:00"_t, 1,
                         navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate first_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150929T0710"_pts).delay(23_h), RTStopTime("stop2", "20150929T0810"_pts).delay(23_h)});
    transit_realtime::TripUpdate second_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150929",
        {RTStopTime("stop1", "20150929T0901"_pts).delay(25_h), RTStopTime("stop2", "20150929T1001"_pts).delay(25_h)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1HourD1", timestamp, second_trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 5);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), hour, day,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate first_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150929T0610"_pts).delay(22_h), RTStopTime("stop2", "20150929T0710"_pts).delay(22_h)});
    transit_realtime::TripUpdate second_trip_update = make_cancellation_message("vj:1", "20150928");

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD0", timestamp, second_trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), hour, day,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate first_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150929T0610"_pts).delay(22_h), RTStopTime("stop2", "20150929T0710"_pts).delay(22_h)});
    transit_realtime::TripUpdate second_trip_update = make_cancellation_message("vj:1", "20150929");

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1dayD0", timestamp, first_trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD1", timestamp, second_trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), hour, day,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate first_trip_update = make_cancellation_message("vj:1", "20150928");
    transit_realtime::TripUpdate second_trip_update = make_cancellation_message("vj:1", "20150929");

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("cancelD0", timestamp, first_trip_update, *b.data, true, true);

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
    navitia::handle_realtime("cancelD1", timestamp, second_trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    auto res = compute(nt::RTLevel::Base);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

    res = compute(nt::RTLevel::RealTime);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    res = raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), "09:02"_t, 1,
                         navitia::DateTimeUtils::inf, nt::RTLevel::RealTime, 2_min, 2_min, true);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150930T0901"_dt);
}

/* testing all is accepted and correctly handled
 * Disruptions sent:
 * D0: +10 hours
 * D0: cancel
 */
BOOST_AUTO_TEST_CASE(train_delayed_10_hours_then_canceled) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });

    transit_realtime::TripUpdate first_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T1801"_pts).delay(10_h), RTStopTime("stop2", "20150928T1901"_pts).delay(10_h)});
    transit_realtime::TripUpdate second_trip_update = make_cancellation_message("vj:1", "20150928");

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay10HoursD0", timestamp, first_trip_update, *b.data, true, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD0", timestamp, second_trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));

    auto compute = [&](nt::RTLevel level, int hour, int day) {
        return raptor.compute(pt_data->stop_areas_map.at("stop1"), pt_data->stop_areas_map.at("stop2"), hour, day,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000111", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);
    });
    // Add code on vj:1
    auto* vj_1 = b.get<navitia::type::VehicleJourney>("vehicle_journey:vj:1");
    b.data->pt_data->codes.add(vj_1, "source", "source_vj:1");
    b.data->pt_data->codes.add(vj_1, "gtfs", "gtfs_vj:1");

    transit_realtime::TripUpdate first_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0910"_pts).delay(1_h), RTStopTime("stop2", "20150928T1010"_pts).delay(1_h)});
    transit_realtime::TripUpdate second_trip_update = ntest::make_trip_update_message(
        "vj:1", "20150929",
        {RTStopTime("stop1", "20150929T1010"_pts).delay(2_h), RTStopTime("stop2", "20150929T1110"_pts).delay(2_h)});
    transit_realtime::TripUpdate third_trip_update = make_cancellation_message("vj:1", "20150930");

    const auto& pt_data = b.data->pt_data;
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(vj->get_impacts().size(), 0);

    // Search by code should also find the only vehicle_journey vj:1
    auto indexes = navitia::ptref::make_query(nt::Type_e::VehicleJourney,
                                              R"(vehicle_journey.has_code(source, source_vj:1))", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    navitia::handle_realtime("delay1hourD0", timestamp, first_trip_update, *b.data, true, true);

    // get vj realtime for d0 and check it's on day 0
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_rt_vj().size(), 1);
    const auto vj_rt_d0 = vj->meta_vj->get_rt_vj()[0].get();
    BOOST_CHECK(vj_rt_d0->get_validity_pattern_at(vj_rt_d0->realtime_level)->check(0));

    // Search by code should also find two vehicle_journeys
    indexes = navitia::ptref::make_query(nt::Type_e::VehicleJourney, R"(vehicle_journey.has_code(source, source_vj:1))",
                                         *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    BOOST_REQUIRE_EQUAL(vj->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_REQUIRE_EQUAL(vj_rt_d0->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d0->get_impacts()[0]->uri, "delay1hourD0");

    navitia::handle_realtime("delay2hourD1", timestamp, second_trip_update, *b.data, true, true);

    // get vj realtime for d1 and check it's on day 1
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 3);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_rt_vj().size(), 2);
    const auto vj_rt_d1 = vj->meta_vj->get_rt_vj()[1].get();
    BOOST_CHECK(vj_rt_d1->get_validity_pattern_at(vj_rt_d1->realtime_level)->check(1));

    // Search by code should also find three vehicle_journeys
    indexes = navitia::ptref::make_query(nt::Type_e::VehicleJourney, R"(vehicle_journey.has_code(source, source_vj:1))",
                                         *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 3);

    BOOST_REQUIRE_EQUAL(vj->get_impacts().size(), 2);
    BOOST_CHECK_EQUAL(vj->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_CHECK_EQUAL(vj->get_impacts()[1]->uri, "delay2hourD1");
    BOOST_REQUIRE_EQUAL(vj_rt_d0->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d0->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_REQUIRE_EQUAL(vj_rt_d1->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d1->get_impacts()[0]->uri, "delay2hourD1");

    navitia::handle_realtime("cancelD3", timestamp, third_trip_update, *b.data, true, true);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 3);
    // Search by code should also find three vehicle_journeys
    indexes = navitia::ptref::make_query(nt::Type_e::VehicleJourney, R"(vehicle_journey.has_code(source, source_vj:1))",
                                         *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 3);
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
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj_with_network("nt", "A", "000111", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj_with_network("nt", "A", "000111", "", true, "vj:2")("stop1", "08:10"_t)("stop2", "09:10"_t);
        b.vj_with_network("nt", "A", "000111", "", true, "vj:3")("stop1", "08:20"_t)("stop2", "09:20"_t);
    });

    transit_realtime::TripUpdate trip_update_vj2 = ntest::make_trip_update_message(
        "vj:2", "20150928",
        {RTStopTime("stop1", "20150928T0910"_pts).delay(1_h), RTStopTime("stop2", "20150929T1010"_pts).delay(1_h)});
    transit_realtime::TripUpdate trip_update_vj3 = make_cancellation_message("vj:3", "20150928");
    navitia::handle_realtime("trip_update_vj2", timestamp, trip_update_vj2, *b.data, true, true);
    navitia::handle_realtime("trip_update_vj3", timestamp, trip_update_vj3, *b.data, true, true);

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::posix_time::from_iso_string("20150928T0830"), null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *b.data, 1, 10, 0, "", {}, boost::none, boost::none);
    const auto resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 1);
    BOOST_REQUIRE_EQUAL(resp.traffic_reports(0).vehicle_journeys_size(), 1);
    BOOST_CHECK_EQUAL(resp.traffic_reports(0).vehicle_journeys(0).uri(), "vehicle_journey:vj:3");
}

BOOST_AUTO_TEST_CASE(traffic_reports_vehicle_journeys_without_since_until) {
    /*
    vj:1                                20150930
    vj:2                                                20151002
    vj:3             20150928

    */
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj_with_network("nt", "A", "111111", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj_with_network("nt", "A", "111111", "", true, "vj:2")("stop1", "08:10"_t)("stop2", "09:10"_t);
        b.vj_with_network("nt", "A", "111111", "", true, "vj:3")("stop1", "08:20"_t)("stop2", "09:20"_t);
    });

    transit_realtime::TripUpdate trip_update_vj1 = make_cancellation_message("vj:1", "20150930");
    navitia::handle_realtime("trip_update_vj1", timestamp, trip_update_vj1, *b.data, true, true);

    transit_realtime::TripUpdate trip_update_vj2 = make_cancellation_message("vj:2", "20151002");
    navitia::handle_realtime("trip_update_vj2", timestamp, trip_update_vj2, *b.data, true, true);

    transit_realtime::TripUpdate trip_update_vj3 = make_cancellation_message("vj:3", "20150928");
    navitia::handle_realtime("trip_update_vj3", timestamp, trip_update_vj3, *b.data, true, true);

    auto* data_ptr = b.data.get();

    navitia::PbCreator pb_creator(data_ptr, boost::posix_time::from_iso_string("20150928T0830"), null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *b.data, 1, 10, 0, "", {}, boost::none, boost::none);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"trip_update_vj1", "trip_update_vj2", "trip_update_vj3"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

BOOST_AUTO_TEST_CASE(traffic_reports_vehicle_journeys_without_since) {
    /*
    vj:1                                20150930
    vj:2                                                20151002
    vj:3             20150928

    */
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj_with_network("nt", "A", "111111", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj_with_network("nt", "A", "111111", "", true, "vj:2")("stop1", "08:10"_t)("stop2", "09:10"_t);
        b.vj_with_network("nt", "A", "111111", "", true, "vj:3")("stop1", "08:20"_t)("stop2", "09:20"_t);
    });

    transit_realtime::TripUpdate trip_update_vj1 = make_cancellation_message("vj:1", "20150930");
    navitia::handle_realtime("trip_update_vj1", timestamp, trip_update_vj1, *b.data, true, true);

    transit_realtime::TripUpdate trip_update_vj2 = make_cancellation_message("vj:2", "20151002");
    navitia::handle_realtime("trip_update_vj2", timestamp, trip_update_vj2, *b.data, true, true);

    transit_realtime::TripUpdate trip_update_vj3 = make_cancellation_message("vj:3", "20150928");
    navitia::handle_realtime("trip_update_vj3", timestamp, trip_update_vj3, *b.data, true, true);

    auto* data_ptr = b.data.get();

    navitia::PbCreator pb_creator(data_ptr, boost::posix_time::from_iso_string("20150928T0830"), null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *b.data, 1, 10, 0, "", {}, boost::none, "20150929T0830"_dt);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"trip_update_vj3"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

BOOST_AUTO_TEST_CASE(traffic_reports_vehicle_journeys_without_until) {
    /*
    vj:1                                20150930
    vj:2                                                20151002
    vj:3             20150928

    */
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj_with_network("nt", "A", "111111", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj_with_network("nt", "A", "111111", "", true, "vj:2")("stop1", "08:10"_t)("stop2", "09:10"_t);
        b.vj_with_network("nt", "A", "111111", "", true, "vj:3")("stop1", "08:20"_t)("stop2", "09:20"_t);
    });

    transit_realtime::TripUpdate trip_update_vj1 = make_cancellation_message("vj:1", "20150930");
    navitia::handle_realtime("trip_update_vj1", timestamp, trip_update_vj1, *b.data, true, true);

    transit_realtime::TripUpdate trip_update_vj2 = make_cancellation_message("vj:2", "20151002");
    navitia::handle_realtime("trip_update_vj2", timestamp, trip_update_vj2, *b.data, true, true);

    transit_realtime::TripUpdate trip_update_vj3 = make_cancellation_message("vj:3", "20150928");
    navitia::handle_realtime("trip_update_vj3", timestamp, trip_update_vj3, *b.data, true, true);

    auto* data_ptr = b.data.get();

    navitia::PbCreator pb_creator(data_ptr, boost::posix_time::from_iso_string("20150928T0830"), null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *b.data, 1, 10, 0, "", {}, "20150930T0830"_dt, boost::none);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"trip_update_vj1", "trip_update_vj2"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

BOOST_AUTO_TEST_CASE(traffic_reports_vehicle_journeys_with_since_until) {
    /*
    vj:1                                20150930
    vj:2                                                20151002
    vj:3             20150928

    */
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj_with_network("nt", "A", "111111", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj_with_network("nt", "A", "111111", "", true, "vj:2")("stop1", "08:10"_t)("stop2", "09:10"_t);
        b.vj_with_network("nt", "A", "111111", "", true, "vj:3")("stop1", "08:20"_t)("stop2", "09:20"_t);
    });

    transit_realtime::TripUpdate trip_update_vj1 = make_cancellation_message("vj:1", "20150930");
    navitia::handle_realtime("trip_update_vj1", timestamp, trip_update_vj1, *b.data, true, true);

    transit_realtime::TripUpdate trip_update_vj2 = make_cancellation_message("vj:2", "20151002");
    navitia::handle_realtime("trip_update_vj2", timestamp, trip_update_vj2, *b.data, true, true);

    transit_realtime::TripUpdate trip_update_vj3 = make_cancellation_message("vj:3", "20150928");
    navitia::handle_realtime("trip_update_vj3", timestamp, trip_update_vj3, *b.data, true, true);

    auto* data_ptr = b.data.get();

    navitia::PbCreator pb_creator(data_ptr, boost::posix_time::from_iso_string("20150928T0830"), null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *b.data, 1, 10, 0, "", {}, "20150930T0830"_dt, "20150930T1230"_dt);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"trip_update_vj1"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

BOOST_AUTO_TEST_CASE(traffic_reports_vehicle_journeys_no_base) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj_with_network("nt", "A", "000110", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    });

    transit_realtime::TripUpdate trip_update =
        ntest::make_trip_update_message("vj:1", "20150928",
                                        {RTStopTime("stop1", "20150928T0910"_pts).delay(69_min),
                                         RTStopTime("stop2", "20150929T1010"_pts).delay(69_min)});
    navitia::handle_realtime("trip_update", timestamp, trip_update, *b.data, true, true);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::posix_time::from_iso_string("20150928T0830"), null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *b.data, 1, 10, 0, "", {}, boost::none, boost::none);
    const auto resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 0);
}

/* Test unknown stop point
 * Since the stop point is unknown, the trip update should be ignored
 *
 * */
BOOST_AUTO_TEST_CASE(unknown_stop_point) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t)("stop3", "10:01"_t);
    });

    transit_realtime::TripUpdate bad_trip_update =
        ntest::make_trip_update_message("vj:1", "20150928",
                                        {RTStopTime("stop1", "20150928T0810"_pts).delay(9_min),
                                         RTStopTime("stop_unknown_toto", "20150928T0910"_pts).delay(9_min),  // <--- bad
                                         RTStopTime("stop3", "20150928T1001"_pts).delay(9_min)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, bad_trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.finalize_disruption_batch();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to), "08:00"_t, 0,
                                  navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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

/* Delay messages are applied in order
 * In this test, we are applying three
 * When one of delay message is removed/updated, remaining delay message should still be
 * applied in the original order
 * */
BOOST_AUTO_TEST_CASE(ordered_delay_message_test) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t)("stop3", "10:01"_t);
    });

    auto trip_update_1 = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0801"_pts), RTStopTime("stop2", "20150928T0910"_pts).delay(9_min),
         RTStopTime("stop3", "20150928T1001"_pts)});
    auto trip_update_2 = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0810"_pts), RTStopTime("stop2", "20150928T0920"_pts).delay(19_min),
         RTStopTime("stop3", "20150928T1001"_pts)});
    auto trip_update_3 = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("stop1", "20150928T0810"_pts), RTStopTime("stop2", "20150928T0925"_pts).delay(26_min),
         RTStopTime("stop3", "20150928T1001"_pts)});

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("feed_42", "20150101T1337"_dt, trip_update_1, *b.data, true, true);
    navitia::handle_realtime("feed_43", "20150101T1338"_dt, trip_update_2, *b.data, true, true);
    navitia::handle_realtime("feed_44", "20150101T1339"_dt, trip_update_3, *b.data, true, true);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);

    b.finalize_disruption_batch();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to), "08:00"_t, 0,
                                  navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

        // Nothing has changed, the result is the same than Base
        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0925"_dt);
    }
    // Now we remove the first disruption
    navitia::delete_disruption("feed_42", *pt_data, *b.data->meta);

    b.make();
    {
        navitia::routing::RAPTOR raptor(*(b.data));

        auto compute = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
            return raptor.compute(pt_data->stop_areas_map.at(from), pt_data->stop_areas_map.at(to), "08:00"_t, 0,
                                  navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
        };

        auto res = compute(nt::RTLevel::Base, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0901"_dt);

        // Nothing has changed, the result is the same than Base
        res = compute(nt::RTLevel::RealTime, "stop1", "stop2");
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0925"_dt);
    }
}

BOOST_AUTO_TEST_CASE(delays_with_boarding_alighting_times) {
    ed::builder b("20170101", [](ed::builder& b) {
        b.vj("line:A", "1111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t,
                                                    std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
            "stop_point:20", "08:20"_t, "08:21"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)(
            "stop_point:30", "08:30"_t, "08:31"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)(
            "stop_point:40", "08:40"_t, "08:41"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);
    });

    auto trip_update_1 = ntest::make_trip_update_message(
        "vj:1", "20170102",
        {RTStopTime("stop_point:10", "20170102T081000"_pts, "20170102T081100"_pts),
         RTStopTime("stop_point:20", "20170102T082000"_pts, "20170102T082100"_pts),
         RTStopTime("stop_point:30", "20170102T084000"_pts, "20170102T084100"_pts).delay(10_min),
         RTStopTime("stop_point:40", "20170102T085000"_pts, "20170102T085100"_pts).delay(10_min)});
    navitia::handle_realtime("feed", "20170101T1337"_dt, trip_update_1, *b.data, true, true);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "1111101");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "1111111");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().departure_time, "08:11"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().boarding_time, "08:06"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "stop_point:20");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).departure_time, "08:21"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).boarding_time, "08:21"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).stop_point->uri, "stop_point:30");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).departure_time, "08:31"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).boarding_time, "08:31"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().arrival_time, "08:40"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().alighting_time, "08:45"_t);

    // Check the realtime vj
    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj:1");
    const auto* rt_vj = meta_vj->get_rt_vj()[0].get();

    BOOST_CHECK_EQUAL(rt_vj->name, "vj:1");
    BOOST_CHECK_END_VP(rt_vj->rt_validity_pattern(), "0000010");
    BOOST_CHECK_END_VP(rt_vj->base_validity_pattern(), "0000000");
    // The realtime vj should have all 4 stop_times and kept the boarding_times
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.front().departure_time, "08:11"_t);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.front().boarding_time, "08:06"_t);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.at(1).stop_point->uri, "stop_point:20");
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.at(1).departure_time, "08:21"_t);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.at(1).boarding_time, "08:21"_t);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.at(2).stop_point->uri, "stop_point:30");
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.at(2).departure_time, "08:41"_t);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.at(2).boarding_time, "08:41"_t);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.back().stop_point->uri, "stop_point:40");
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.back().arrival_time, "08:50"_t);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.back().alighting_time, "08:55"_t);
}

BOOST_AUTO_TEST_CASE(delays_on_lollipop_with_boarding_alighting_times) {
    ed::builder b("20170101", [](ed::builder& b) {
        b.vj("line:A", "1111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t,
                                                    std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
            "stop_point:20", "08:20"_t, "08:21"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)(
            "stop_point:10", "08:30"_t, "08:31"_t, std::numeric_limits<uint16_t>::max(), true, true, 300, 0);
    });

    auto trip_update_1 = ntest::make_trip_update_message(
        "vj:1", "20170102",
        {
            RTStopTime("stop_point:10", "20170102T081000"_pts, "20170102T081100"_pts),
            RTStopTime("stop_point:20", "20170102T082000"_pts, "20170102T082100"_pts),
            RTStopTime("stop_point:10", "20170102T084000"_pts, "20170102T084100"_pts).delay(10_min),
        });
    navitia::handle_realtime("feed", "20170101T1337"_dt, trip_update_1, *b.data, true, true);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "1111101");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "1111111");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);

    // Check the realtime vj
    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj:1");
    const auto* rt_vj = meta_vj->get_rt_vj()[0].get();

    BOOST_CHECK_END_VP(rt_vj->rt_validity_pattern(), "0000010");
    BOOST_CHECK_END_VP(rt_vj->base_validity_pattern(), "0000000");

    // The realtime vj should have all 3 stop_times
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(0).stop_point->uri, "stop_point:10");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(0).departure_time, "08:11"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(0).boarding_time,
                      "08:06"_t);  // Boarding time is 5 min (300s) before departure

    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).stop_point->uri, "stop_point:20");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).departure_time, "08:21"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).boarding_time, "08:21"_t);

    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).stop_point->uri, "stop_point:10");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).arrival_time, "08:40"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).alighting_time,
                      "08:45"_t);  // Alighting time is 5 min (300s) after arrival
}

BOOST_AUTO_TEST_CASE(simple_skipped_stop) {
    ed::builder b("20170101",
                  [](ed::builder& b) { b.vj("l1").name("vj:1")("A", "08:10"_t)("B", "08:20"_t)("C", "08:30"_t); });

    auto trip_update_1 = ntest::make_trip_update_message("vj:1", "20170101",
                                                         {
                                                             RTStopTime("A", "20170101T081000"_pts),
                                                             RTStopTime("B", "20170101T082000"_pts).skipped(),
                                                             RTStopTime("C", "20170101T083000"_pts),
                                                         });
    navitia::handle_realtime("feed", "20170101T0337"_dt, trip_update_1, *b.data, true, true);

    navitia::routing::RAPTOR raptor(*(b.data));

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);

    // Check the realtime vj
    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj:1");
    const auto* rt_vj = meta_vj->get_rt_vj()[0].get();
    // The realtime vj should have all 3 stop_times but lose the ability to pickup/dropoff on B
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.size(), 3);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().stop_point->uri, "A");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().departure_time, "08:10"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().boarding_time, "08:10"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().skipped_stop(), false);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).stop_point->uri, "B");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).departure_time, "08:20"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).boarding_time, "08:20"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).pick_up_allowed(), false);  // disabled
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).drop_off_allowed(), false);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).skipped_stop(), false);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).stop_point->uri, "C");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).arrival_time, "08:30"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).alighting_time, "08:30"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).skipped_stop(), false);

    auto get_journeys = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
        return raptor.compute(b.get<nt::StopArea>(from), b.get<nt::StopArea>(to), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    BOOST_CHECK(!get_journeys(nt::RTLevel::Base, "A", "B").empty());
    // impossible to do a journey between A and B
    BOOST_CHECK(get_journeys(nt::RTLevel::RealTime, "A", "B").empty());

    BOOST_CHECK(!get_journeys(nt::RTLevel::Base, "B", "C").empty());
    // impossible to do a journey between B and C
    BOOST_CHECK(get_journeys(nt::RTLevel::RealTime, "B", "C").empty());
}

/**
 * A ------------ B ------------ C ------------ D
 *
 * We first do not stop at B and C, then we send another disruption to stop at C but with a delay
 */
BOOST_AUTO_TEST_CASE(skipped_stop_then_delay) {
    ed::builder b("20170101", [](ed::builder& b) {
        b.vj("l1").name("vj:1")("A", "08:10"_t)("B", "08:20"_t)("C", "08:30"_t)("D", "08:40"_t);
    });

    auto trip_update_1 = ntest::make_trip_update_message("vj:1", "20170101",
                                                         {
                                                             RTStopTime("A", "20170101T081000"_pts),
                                                             RTStopTime("B", "20170101T082000"_pts).departure_skipped(),
                                                             RTStopTime("C", "20170101T083000"_pts).skipped(),
                                                             RTStopTime("D", "20170101T084000"_pts),
                                                         });
    navitia::handle_realtime("feed", "20170101T0337"_dt, trip_update_1, *b.data, true, true);

    auto trip_update_2 = ntest::make_trip_update_message("vj:1", "20170101",
                                                         {
                                                             RTStopTime("A", "20170101T081000"_pts),
                                                             RTStopTime("B", "20170101T082000"_pts).departure_skipped(),
                                                             RTStopTime("C", "20170101T083500"_pts).delay(5_min),
                                                             RTStopTime("D", "20170101T084000"_pts),
                                                         });
    navitia::handle_realtime("feed", "20170101T0337"_dt, trip_update_2, *b.data, true, true);
    b.data->build_raptor();
    navitia::routing::RAPTOR raptor(*(b.data));

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "1111110");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "1111111");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);

    // Check the realtime vj
    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj:1");
    // two disruptions applied, there should be only one realtime vj
    BOOST_REQUIRE_EQUAL(meta_vj->get_rt_vj().size(), 1);

    auto rt_vj = meta_vj->get_rt_vj()[0].get();

    BOOST_CHECK_END_VP(rt_vj->rt_validity_pattern(), "0000001");
    BOOST_CHECK_END_VP(rt_vj->base_validity_pattern(), "0000000");
    // The realtime vj should have all 3 stop_times but lose the ability to pickup/dropoff on B
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.size(), 4);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().stop_point->uri, "A");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().departure_time, "08:10"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().boarding_time, "08:10"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.front().skipped_stop(), false);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).stop_point->uri, "B");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).departure_time, "08:20"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).boarding_time, "08:20"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).pick_up_allowed(), false);  // disabled
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(1).skipped_stop(), false);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).stop_point->uri, "C");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).arrival_time, "08:35"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).alighting_time, "08:35"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(2).skipped_stop(), false);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(3).stop_point->uri, "D");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(3).arrival_time, "08:40"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(3).alighting_time, "08:40"_t);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(3).pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(3).drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.at(3).skipped_stop(), false);

    auto get_journeys = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
        return raptor.compute(b.get<nt::StopArea>(from), b.get<nt::StopArea>(to), "08:00"_t, 0,
                              navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
    };

    BOOST_CHECK(!get_journeys(nt::RTLevel::Base, "A", "B").empty());
    // possible to do a journey between A and B
    BOOST_CHECK(!get_journeys(nt::RTLevel::RealTime, "A", "B").empty());

    BOOST_CHECK(!get_journeys(nt::RTLevel::Base, "B", "C").empty());
    // impossible to do a journey between B and C
    BOOST_CHECK(get_journeys(nt::RTLevel::RealTime, "B", "C").empty());

    auto journeys = get_journeys(nt::RTLevel::Base, "A", "C");
    BOOST_REQUIRE_EQUAL(journeys.size(), 1);
    BOOST_CHECK_EQUAL(journeys[0].items.back().arrival, "20170101T083000"_dt);

    // but A->C is ok, just delayed
    journeys = get_journeys(nt::RTLevel::RealTime, "A", "C");
    BOOST_REQUIRE_EQUAL(journeys.size(), 1);
    BOOST_CHECK_EQUAL(journeys[0].items.back().arrival, "20170101T083500"_dt);
}

/*
 * Test that we display delays on journeys only if the traveler is
 * really impacted by it.
 *
 * delay: +5      0      0     +5      0
 *         A ---- B ---- C ---- D ---- E
 *
 * A to B -> print disruption
 * B to C -> no disruption
 * C to D -> print disruption
 * C to E -> print disruption
 */
BOOST_AUTO_TEST_CASE(train_delayed_and_on_time) {
    ed::builder b("20150928", [](ed::builder& b) {
        b.vj("1").name("vj:1")("A", "08:00"_t)("B", "09:00"_t)("C", "10:00"_t)("D", "11:00"_t)("E", "12:00"_t);
    });

    transit_realtime::TripUpdate trip_update = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("A", "20150928T0805"_pts).delay(5_min), RTStopTime("B", "20150928T0900"_pts).delay(0_min),
         RTStopTime("C", "20150928T1000"_pts).delay(0_min), RTStopTime("D", "20150928T1105"_pts).delay(5_min),
         RTStopTime("E", "20150928T1200"_pts).delay(0_min)},
        transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS);

    navitia::handle_realtime("bob", timestamp, trip_update, *b.data, true, true);

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* from, const char* to, nt::RTLevel level) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20150928T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20150928T073000")}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, level, 2_min);
        return pb_creator.get_response();
    };

    auto res = compute("A", "B", nt::RTLevel::RealTime);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);

    res = compute("A", "B", nt::RTLevel::Base);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);

    res = compute("B", "C", nt::RTLevel::RealTime);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 0);

    res = compute("B", "C", nt::RTLevel::Base);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 0);

    res = compute("C", "D", nt::RTLevel::RealTime);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);

    res = compute("C", "D", nt::RTLevel::Base);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);

    res = compute("C", "E", nt::RTLevel::RealTime);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);

    res = compute("C", "E", nt::RTLevel::Base);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);
}

BOOST_AUTO_TEST_CASE(train_delayed_3_times_different_id) {
    ed::builder b("20150928", [](ed::builder& b) { b.vj("1").name("vj:1")("A", "08:00"_t)("B", "09:00"_t); });

    transit_realtime::TripUpdate trip_update1 = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("A", "20150928T0801"_pts).delay(1_min), RTStopTime("B", "20150928T0900"_pts).delay(0_min)});

    navitia::handle_realtime("bob1", timestamp, trip_update1, *b.data, true, true);

    transit_realtime::TripUpdate trip_update2 = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("A", "20150928T0805"_pts).delay(5_min), RTStopTime("B", "20150928T0900"_pts).delay(0_min)});

    navitia::handle_realtime("bob2", timestamp, trip_update2, *b.data, true, true);

    transit_realtime::TripUpdate trip_update3 = ntest::make_trip_update_message(
        "vj:1", "20150928",
        {RTStopTime("A", "20150928T0802"_pts).delay(2_min), RTStopTime("B", "20150928T0900"_pts).delay(0_min)});

    navitia::handle_realtime("bob3", timestamp, trip_update2, *b.data, true, true);

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* from, const char* to, nt::RTLevel level) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20150928T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20150928T073000")}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, level, 2_min);
        return pb_creator.get_response();
    };

    auto res = compute("A", "B", nt::RTLevel::RealTime);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 3);
}

BOOST_AUTO_TEST_CASE(teleportation_train_2_delays_check_disruptions) {
    ed::builder b("20171101",
                  [](ed::builder& b) { b.vj("1").name("vj:1")("A", "08:00"_t)("B", "08:00"_t)("C", "09:00"_t); });

    transit_realtime::TripUpdate trip_update1 = ntest::make_trip_update_message(
        "vj:1", "20171101",
        {RTStopTime("A", "20171101T0801"_pts).delay(1_min), RTStopTime("B", "20171101T0801"_pts).delay(1_min)});

    navitia::handle_realtime("late-01", timestamp, trip_update1, *b.data, true, true);

    transit_realtime::TripUpdate trip_update2 = ntest::make_trip_update_message(
        "vj:1", "20171102",
        {RTStopTime("A", "20171102T0802"_pts).delay(2_min), RTStopTime("B", "20171102T0802"_pts).delay(2_min)});

    navitia::handle_realtime("late-02", timestamp, trip_update2, *b.data, true, true);

    b.finalize_disruption_batch();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id("A");
        navitia::type::Type_e destination_type = b.data->get_type_of_id("B");
        navitia::type::EntryPoint origin(origin_type, "A");
        navitia::type::EntryPoint destination(destination_type, "B");

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    auto res = compute("20171101T073000");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_REQUIRE_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).uri(), "late-01");

    res = compute("20171102T073000");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_REQUIRE_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).uri(), "late-02");

    res = compute("20171103T073000");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 0);
}

BOOST_AUTO_TEST_CASE(add_new_stop_time_in_the_trip) {
    ed::builder b("20171101", [](ed::builder& b) {
        b.sa("A", 0, 0, true, true);
        b.sa("B", 0, 0, true, true);
        b.sa("B_bis", 0, 0, true, true);
        b.sa("C", 0, 0, true, true);
        b.sa("D", 0, 0, true, true);
        b.vj("1").name("vj:1")("stop_point:A", "08:00"_t)("stop_point:B", "08:30"_t)("stop_point:C", "09:00"_t);
    });

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    auto res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).arrival_date_time(), "20171101T0900"_pts);

    transit_realtime::TripUpdate just_new_stop =
        ntest::make_trip_update_message("vj:1", "20171101",
                                        {
                                            RTStopTime("stop_point:A", "20171101T0800"_pts),
                                            RTStopTime("stop_point:B", "20171101T0830"_pts),
                                            RTStopTime("stop_point:B_bis", "20171101T0845"_pts).added(),
                                            RTStopTime("stop_point:C", "20171101T0900"_pts),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_MODIFIED_SERVICE);

    navitia::handle_realtime("add_new_stop_time_in_the_trip", timestamp, just_new_stop, *b.data, true, true);

    b.finalize_disruption_batch();

    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_MODIFIED_SERVICE);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).arrival_date_time(), "20171101T0900"_pts);

    transit_realtime::TripUpdate delay_and_new_stop =
        ntest::make_trip_update_message("vj:1", "20171101",
                                        {
                                            RTStopTime("stop_point:A", "20171101T0805"_pts).delay(5_min),
                                            RTStopTime("stop_point:B", "20171101T0830"_pts),
                                            RTStopTime("stop_point:B_bis", "20171101T0845"_pts).added(),
                                            RTStopTime("stop_point:C", "20171101T0905"_pts).delay(5_min),
                                        });

    // Use trip_update effect (MODIFIED_SERVICE)
    delay_and_new_stop.SetExtension(kirin::effect, transit_realtime::Alert_Effect::Alert_Effect_MODIFIED_SERVICE);
    navitia::handle_realtime("add_new_stop_time_in_the_trip", timestamp, delay_and_new_stop, *b.data, true, true);

    b.finalize_disruption_batch();

    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_MODIFIED_SERVICE);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().name(), "trip modified");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).arrival_date_time(), "20171101T0905"_pts);

    transit_realtime::TripUpdate new_stop_at_the_end = ntest::make_trip_update_message(
        "vj:1", "20171101",
        {RTStopTime("stop_point:A", "20171101T0800"_pts), RTStopTime("stop_point:B", "20171101T0830"_pts),
         RTStopTime("stop_point:C", "20171101T0905"_pts), RTStopTime("stop_point:D", "20171101T1000"_pts).added()});

    // Use trip_update effect = SIGNIFICANT_DELAYS just for the test
    new_stop_at_the_end.SetExtension(kirin::effect, transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS);
    navitia::handle_realtime("add_new_stop_time_in_the_trip", timestamp, new_stop_at_the_end, *b.data, true, true);

    b.finalize_disruption_batch();

    res = compute("20171101T073000", "stop_point:A", "stop_point:D");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_SIGNIFICANT_DELAYS);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().name(), "trip delayed");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).arrival_date_time(), "20171101T1000"_pts);
}

BOOST_AUTO_TEST_CASE(add_modify_and_delete_new_stop_time_in_the_trip) {
    // Init data for a vj = vj:1 -> A -> B -> C
    ed::builder b("20171101", [](ed::builder& b) {
        b.sa("A", 0, 0, true, true);
        b.sa("B", 0, 0, true, true);
        b.sa("B_bis", 0, 0, true, true);
        b.sa("C", 0, 0, true, true);
        b.sa("D", 0, 0, true, true);
        b.vj("1").name("vj:1")("stop_point:A", "08:00"_t)("stop_point:B", "08:30"_t)("stop_point:C", "09:00"_t);
    });

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    auto res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).arrival_date_time(), "20171101T0900"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);

    // We should not have any journey in public_transport from B_bis to C
    res = compute("20171101T073000", "stop_point:B_bis", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);

    // Add a new stop_time in vj = vj:1 betwen stops B and C with B and C unchanged
    transit_realtime::TripUpdate just_new_stop =
        ntest::make_trip_update_message("vj:1", "20171101",
                                        {
                                            RTStopTime("stop_point:A", "20171101T0800"_pts),
                                            RTStopTime("stop_point:B", "20171101T0830"_pts),
                                            RTStopTime("stop_point:B_bis", "20171101T0845"_pts).added(),
                                            RTStopTime("stop_point:C", "20171101T0900"_pts),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_MODIFIED_SERVICE);

    navitia::handle_realtime("feed-1", timestamp, just_new_stop, *b.data, true, true);

    b.finalize_disruption_batch();

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "1111110");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "1111111");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);

    // Check the realtime vj with a newly added stop_time
    // Il should be initialize with skipped_stop = false
    // Check the realtime vj
    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj:1");
    // two disruptions applied, there should be only one realtime vj
    BOOST_REQUIRE_EQUAL(meta_vj->get_rt_vj().size(), 1);

    const auto* rt_vj = meta_vj->get_rt_vj()[0].get();
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].pick_up_allowed(), true);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].drop_off_allowed(), true);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].skipped_stop(), false);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.size(), 4);

    // The new stop_time added should be in stop_date_times
    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_MODIFIED_SERVICE);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).arrival_date_time(), "20171101T0900"_pts);

    // We should have A journey in public_transport from B_bis to C
    res = compute("20171101T073000", "stop_point:B_bis", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 2);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_MODIFIED_SERVICE);

    // Delete the recently added stop_time in vj = vj:1 B_bis
    transit_realtime::TripUpdate delete_new_stop =
        ntest::make_trip_update_message("vj:1", "20171101",
                                        {
                                            RTStopTime("stop_point:A", "20171101T0800"_pts),
                                            RTStopTime("stop_point:B", "20171101T0830"_pts),
                                            RTStopTime("stop_point:B_bis", "20171101T0845"_pts).skipped(),
                                            RTStopTime("stop_point:C", "20171101T0900"_pts),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_REDUCED_SERVICE);

    navitia::handle_realtime("feed-2", timestamp, delete_new_stop, *b.data, true, true);

    b.finalize_disruption_batch();

    // Check the realtime vj after the recently added stop_time is deleted
    meta_vj = b.get_meta_vj("vj:1");
    // two disruptions applied, there should be only one realtime vj
    BOOST_REQUIRE_EQUAL(meta_vj->get_rt_vj().size(), 1);

    rt_vj = meta_vj->get_rt_vj()[0].get();
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].pick_up_allowed(), false);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].drop_off_allowed(), false);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].skipped_stop(), false);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.size(), 4);

    // The new stop_time added should be in stop_date_times
    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.impacts_size(), 2);
    for (const auto& impact : res.impacts()) {
        if (impact.uri() == "feed-2") {
            BOOST_CHECK_EQUAL(impact.severity().effect(), pbnavitia::Severity_Effect_REDUCED_SERVICE);
            BOOST_CHECK_EQUAL(impact.impacted_objects(0).impacted_stops(2).is_detour(), false);
            BOOST_CHECK_EQUAL(impact.impacted_objects(0).impacted_stops(2).departure_status(),
                              pbnavitia::StopTimeUpdateStatus::DELETED);
        }
    }

    // We should not have any journey in public_transport from B_bis to C
    res = compute("20171101T073000", "stop_point:B_bis", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);

    // Delete the recently deleted stop_time in vj = vj:1 B_bis
    // We should be able to add impact on the stop_time even if it is already deleted.
    transit_realtime::TripUpdate redelete_stop =
        ntest::make_trip_update_message("vj:1", "20171101",
                                        {
                                            RTStopTime("stop_point:A", "20171101T0800"_pts),
                                            RTStopTime("stop_point:B", "20171101T0830"_pts),
                                            RTStopTime("stop_point:B_bis", "20171101T0845"_pts).skipped(),
                                            RTStopTime("stop_point:C", "20171101T0900"_pts),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_REDUCED_SERVICE);

    navitia::handle_realtime("feed-3", timestamp, redelete_stop, *b.data, true, true);
    b.finalize_disruption_batch();

    // Check the realtime vj after the recently added stop_time is deleted twice
    meta_vj = b.get_meta_vj("vj:1");
    // two disruptions applied, there should be only one realtime vj
    BOOST_REQUIRE_EQUAL(meta_vj->get_rt_vj().size(), 1);

    rt_vj = meta_vj->get_rt_vj()[0].get();
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].pick_up_allowed(), false);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].drop_off_allowed(), false);
    BOOST_REQUIRE_EQUAL(rt_vj->stop_time_list[2].skipped_stop(), false);

    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.impacts_size(), 3);
    for (const auto& impact : res.impacts()) {
        if (impact.uri() == "feed-3") {
            BOOST_CHECK_EQUAL(impact.severity().effect(), pbnavitia::Severity_Effect_REDUCED_SERVICE);
            BOOST_CHECK_EQUAL(impact.impacted_objects(0).impacted_stops(2).is_detour(), false);
            BOOST_CHECK_EQUAL(impact.impacted_objects(0).impacted_stops(2).departure_status(),
                              pbnavitia::StopTimeUpdateStatus::DELETED);
        }
    }

    // We should not have any journey in public_transport from B_bis to C
    res = compute("20171101T073000", "stop_point:B_bis", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
}

BOOST_AUTO_TEST_CASE(add_new_and_delete_existingstop_time_in_the_trip_for_detour) {
    // Init data for a vj = vj:1 -> A -> B -> C
    ed::builder b("20171101", [](ed::builder& b) {
        b.sa("A", 0, 0, true, true);
        b.sa("B", 0, 0, true, true);
        b.sa("B_bis", 0, 0, true, true);
        b.sa("C", 0, 0, true, true);
        b.sa("D", 0, 0, true, true);
        b.vj("1").name("vj:1")("stop_point:A", "08:00"_t)("stop_point:B", "08:30"_t)("stop_point:C", "09:00"_t);
    });

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    auto res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).arrival_date_time(), "20171101T0900"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);

    // Delete the stop_time at B (delete_for_detour) followed by a new stop_time
    // added at B_bis (added_for_detour) and C unchanged.
    transit_realtime::TripUpdate just_new_stop =
        ntest::make_trip_update_message("vj:1", "20171101",
                                        {
                                            RTStopTime("stop_point:A", "20171101T0800"_pts),
                                            RTStopTime("stop_point:B", "20171101T0830"_pts).deleted_for_detour(),
                                            RTStopTime("stop_point:B_bis", "20171101T0845"_pts).added_for_detour(),
                                            RTStopTime("stop_point:C", "20171101T0900"_pts),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_DETOUR);
    navitia::handle_realtime("add_new_and_delete_existingstop_time_in_the_trip_for_detour", timestamp, just_new_stop,
                             *b.data, true, true);

    b.finalize_disruption_batch();

    // The delete stop_time should not be in stop_date_times where as the added stop_time should be in stop_date_times
    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_DETOUR);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).stop_point().uri(), "stop_point:B_bis");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).arrival_date_time(), "20171101T0900"_pts);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::UNCHANGED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).is_detour(), true);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::DELETED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).is_detour(), true);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::UNCHANGED);

    // We should have A journey in public_transport from B_bis to C
    res = compute("20171101T073000", "stop_point:B_bis", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 2);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_DETOUR);
}

BOOST_AUTO_TEST_CASE(add_new_with_earlier_arrival_and_delete_existingstop_time_in_the_trip_for_detour) {
    // Init data for a vj = vj:1 -> A -> B -> C
    ed::builder b("20171101", [](ed::builder& b) {
        b.sa("A", 0, 0, true, true);
        b.sa("B", 0, 0, true, true);
        b.sa("B_bis", 0, 0, true, true);
        b.sa("C", 0, 0, true, true);
        b.sa("D", 0, 0, true, true);
        b.vj("1").name("vj:1")("stop_point:A", "08:00"_t)("stop_point:B", "08:30"_t)("stop_point:C", "09:00"_t);
    });

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    auto res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).arrival_date_time(), "20171101T0900"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);

    // Delete the stop_time at B (delete_for_detour) followed by a new stop_time added
    // at B_bis (added_for_detour) and C unchanged.
    // the arrival time of newly added stop_time is earlier than deleted, and this
    // feature works well.
    transit_realtime::TripUpdate just_new_stop =
        ntest::make_trip_update_message("vj:1", "20171101",
                                        {
                                            RTStopTime("stop_point:A", "20171101T0800"_pts),
                                            RTStopTime("stop_point:B", "20171101T0830"_pts).deleted_for_detour(),
                                            RTStopTime("stop_point:B_bis", "20171101T0825"_pts).added_for_detour(),
                                            RTStopTime("stop_point:C", "20171101T0900"_pts),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_DETOUR);

    navitia::handle_realtime("add_new_and_delete_existingstop_time_in_the_trip_for_detour", timestamp, just_new_stop,
                             *b.data, true, true);

    b.finalize_disruption_batch();

    // Same verifications as in the test above
    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_DETOUR);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).stop_point().uri(), "stop_point:B_bis");
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::UNCHANGED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).is_detour(), true);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::DELETED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).is_detour(), true);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::UNCHANGED);
}

BOOST_AUTO_TEST_CASE(should_get_base_stoptime_with_realtime_added_stop_time) {
    ed::builder b("20171101", [](ed::builder& b) {
        b.sa("A");
        b.sa("B");
        b.sa("C");
        b.sa("D");
        b.vj("L1").name("vj:1")("stop_point:A", "08:10"_t)("stop_point:C", "08:30"_t)("stop_point:D", "08:40"_t);
    });

    tr::TripUpdate add_stop =
        ntest::make_trip_update_message("vj:1", "20171101",
                                        {
                                            RTStopTime("stop_point:A", "20171101T0810"_pts),
                                            RTStopTime("stop_point:B", "20171101T0820"_pts).added(),
                                            RTStopTime("stop_point:C", "20171101T0830"_pts),
                                            RTStopTime("stop_point:D", "20171101T0840"_pts),
                                        });

    navitia::handle_realtime("add_new_stop", timestamp, add_stop, *b.data, true, true);
    b.finalize_disruption_batch();

    auto res = compute_iti(b, "20171101T080000", "A", "D", nt::RTLevel::RealTime);

    BOOST_REQUIRE_EQUAL(res.impacts_size(), 1);
    BOOST_REQUIRE_EQUAL(res.impacts(0).impacted_objects_size(), 1);

    const auto& stop_time_updates = res.impacts(0).impacted_objects(0).impacted_stops();
    BOOST_REQUIRE_EQUAL(stop_time_updates.size(), 4);

    const auto& stop_time_A = stop_time_updates.Get(0);
    BOOST_CHECK_EQUAL(stop_time_A.stop_point().uri(), "stop_point:A");
    BOOST_CHECK_EQUAL(stop_time_A.base_stop_time().arrival_time(), "08:10"_t);
    BOOST_CHECK_EQUAL(stop_time_A.base_stop_time().departure_time(), "08:10"_t);
    BOOST_CHECK_EQUAL(stop_time_A.amended_stop_time().arrival_time(), "08:10"_t);
    BOOST_CHECK_EQUAL(stop_time_A.amended_stop_time().departure_time(), "08:10"_t);

    const auto& stop_time_B = stop_time_updates.Get(1);
    BOOST_CHECK_EQUAL(stop_time_B.stop_point().uri(), "stop_point:B");
    BOOST_CHECK_MESSAGE(stop_time_B.base_stop_time().arrival_time() == "00:00"_t,
                        "Base stop time doesn't exist for added stop time");
    BOOST_CHECK_MESSAGE(stop_time_B.base_stop_time().departure_time() == "00:00"_t,
                        "Base stop time doesn't exist for added stop time");
    BOOST_CHECK_EQUAL(stop_time_B.amended_stop_time().arrival_time(), "08:20"_t);
    BOOST_CHECK_EQUAL(stop_time_B.amended_stop_time().departure_time(), "08:20"_t);

    const auto& stop_time_C = stop_time_updates.Get(2);
    BOOST_CHECK_EQUAL(stop_time_C.stop_point().uri(), "stop_point:C");
    BOOST_CHECK_EQUAL(stop_time_C.base_stop_time().arrival_time(), "08:30"_t);
    BOOST_CHECK_EQUAL(stop_time_C.base_stop_time().departure_time(), "08:30"_t);
    BOOST_CHECK_EQUAL(stop_time_C.amended_stop_time().arrival_time(), "08:30"_t);
    BOOST_CHECK_EQUAL(stop_time_C.amended_stop_time().departure_time(), "08:30"_t);

    const auto& stop_time_D = stop_time_updates.Get(3);
    BOOST_CHECK_EQUAL(stop_time_D.stop_point().uri(), "stop_point:D");
    BOOST_CHECK_EQUAL(stop_time_D.base_stop_time().arrival_time(), "08:40"_t);
    BOOST_CHECK_EQUAL(stop_time_D.base_stop_time().departure_time(), "08:40"_t);
    BOOST_CHECK_EQUAL(stop_time_D.amended_stop_time().arrival_time(), "08:40"_t);
    BOOST_CHECK_EQUAL(stop_time_D.amended_stop_time().departure_time(), "08:40"_t);
}

BOOST_AUTO_TEST_CASE(should_get_correct_base_stop_time_with_lollipop) {
    /*
    We create a network with a lollipop and apply delay on every stop points.
    We expect from the "real time" VJ that its base stop times are matching the original VJ.

        (08:10) A   A' (08:50)
                ▼   ▲
        (08:20) B   B' (08:40)
                \   /
                 \ /
        (08:30)   C
    */
    const navitia::type::VehicleJourney* vj = nullptr;
    ed::builder b("20171101", [&](ed::builder& b) {
        vj = b.vj("L1")
                 .name("vj:1")("A", "08:10"_t)("B", "08:20"_t)("C", "08:30"_t)("B", "08:40"_t)("A", "08:50"_t)
                 .make();
    });

    navitia::handle_realtime("add_new_stop", timestamp,
                             ntest::make_trip_update_message("vj:1", "20171101",
                                                             {
                                                                 RTStopTime("A", "20171101T0810"_pts).delay(1_min),
                                                                 RTStopTime("B", "20171101T0820"_pts).delay(1_min),
                                                                 RTStopTime("C", "20171101T0830"_pts).delay(1_min),
                                                                 RTStopTime("B", "20171101T0840"_pts).delay(1_min),
                                                                 RTStopTime("A", "20171101T0850"_pts).delay(1_min),
                                                             }),
                             *b.data, true, true);
    b.finalize_disruption_batch();

    const auto& realtime_vj = vj->meta_vj->get_rt_vj()[0];

    const auto& stoptime_rltm_A = realtime_vj->stop_time_list[0];
    const auto& stoptime_rltm_B = realtime_vj->stop_time_list[1];
    const auto& stoptime_rltm_C = realtime_vj->stop_time_list[2];
    const auto& stoptime_rltm_B_bis = realtime_vj->stop_time_list[3];
    const auto& stoptime_rltm_A_bis = realtime_vj->stop_time_list[4];

    BOOST_CHECK_EQUAL(stoptime_rltm_A.get_base_stop_time(), &vj->stop_time_list[0]);
    BOOST_CHECK_EQUAL(stoptime_rltm_B.get_base_stop_time(), &vj->stop_time_list[1]);
    BOOST_CHECK_EQUAL(stoptime_rltm_C.get_base_stop_time(), &vj->stop_time_list[2]);
    BOOST_CHECK_EQUAL(stoptime_rltm_B_bis.get_base_stop_time(), &vj->stop_time_list[3]);
    BOOST_CHECK_EQUAL(stoptime_rltm_A_bis.get_base_stop_time(), &vj->stop_time_list[4]);
}

BOOST_AUTO_TEST_CASE(should_get_correct_base_stop_time_with_lollipop_II) {
    /*
    We create a network with a lollipop and apply delay on every stop points.
    We expect from the "real time" VJ that its base stop times are matching the original VJ.

        (08:10) A
                ▼
                |
        (08:20) B ▶ ─────╮
                         C (08:30)
        (08:40) B ◀ ─────╯
                |
                ▼
        (08:50) D
    */
    nt::VehicleJourney* vj = nullptr;
    ed::builder b("20171101", [&](ed::builder& b) {
        vj = b.vj("L1")
                 .name("vj:1")("A", "08:10"_t)("B", "08:20"_t)("C", "08:30"_t)("B", "08:40"_t)("D", "08:50"_t)
                 .make();
    });

    navitia::handle_realtime("add_new_stop", timestamp,
                             ntest::make_trip_update_message("vj:1", "20171101",
                                                             {
                                                                 RTStopTime("A", "20171101T0810"_pts).delay(1_min),
                                                                 RTStopTime("B", "20171101T0820"_pts).delay(1_min),
                                                                 RTStopTime("C", "20171101T0830"_pts).delay(1_min),
                                                                 RTStopTime("B", "20171101T0840"_pts).delay(1_min),
                                                                 RTStopTime("D", "20171101T0850"_pts).delay(1_min),
                                                             }),
                             *b.data, true, true);
    b.finalize_disruption_batch();

    const auto& realtime_vj = vj->meta_vj->get_rt_vj()[0];

    const auto& stoptime_rltm_A = realtime_vj->stop_time_list[0];
    const auto& stoptime_rltm_B = realtime_vj->stop_time_list[1];
    const auto& stoptime_rltm_C = realtime_vj->stop_time_list[2];
    const auto& stoptime_rltm_B_bis = realtime_vj->stop_time_list[3];
    const auto& stoptime_rltm_D = realtime_vj->stop_time_list[4];

    BOOST_CHECK_EQUAL(stoptime_rltm_A.get_base_stop_time(), &vj->stop_time_list[0]);
    BOOST_CHECK_EQUAL(stoptime_rltm_B.get_base_stop_time(), &vj->stop_time_list[1]);
    BOOST_CHECK_EQUAL(stoptime_rltm_C.get_base_stop_time(), &vj->stop_time_list[2]);
    BOOST_CHECK_EQUAL(stoptime_rltm_B_bis.get_base_stop_time(), &vj->stop_time_list[3]);
    BOOST_CHECK_EQUAL(stoptime_rltm_D.get_base_stop_time(), &vj->stop_time_list[4]);
}

struct AddTripDataset {
    AddTripDataset()
        : comp_name("comp_name"),
          comp_uri("comp_uri"),
          phy_mode_name("name physical_mode_uri"),
          phy_mode_uri("physical_mode_uri"),
          phy_mode_name_2("name physical_mode_uri_2"),
          phy_mode_uri_2("physical_mode_uri_2"),
          contributor_name("contributor_name"),
          contributor_uri("contributor_uri"),
          dataset_name("dataset_name"),
          dataset_uri("dataset_uri"),
          b("20190101", [&](ed::builder& b) {
              b.sa("A", 0, 0, true, true);
              b.sa("B", 0, 0, true, true);
              b.sa("C", 0, 0, true, true);
              b.sa("D", 0, 0, true, true);
              b.sa("E", 0, 0, true, true);
              b.sa("F", 0, 0, true, true);
              b.sa("G", 0, 0, true, true);
              b.sa("H", 0, 0, true, true);
              b.sa("I", 0, 0, true, true);
              b.sa("J", 0, 0, true, true);

              b.vj("1").name("vj:1").physical_mode("physical_mode_uri")("stop_point:A", "08:00"_t)(
                  "stop_point:B", "08:30"_t)("stop_point:C", "09:00"_t)("stop_point:D", "09:30"_t);

              // Add company
              auto* cmp1 = new navitia::type::Company();
              cmp1->line_list.push_back(b.lines["1"]);
              cmp1->idx = b.data->pt_data->companies.size();
              cmp1->name = comp_name;
              cmp1->uri = comp_uri;
              b.data->pt_data->companies.push_back(cmp1);
              b.data->pt_data->companies_map[comp_uri] = cmp1;
              b.lines["1"]->company_list.push_back(cmp1);

              // Add physical mode
              auto* phy_mode = new navitia::type::PhysicalMode();
              phy_mode->idx = b.data->pt_data->physical_modes.size();
              phy_mode->name = phy_mode_name;
              phy_mode->uri = phy_mode_uri;
              phy_mode->vehicle_journey_list.push_back(b.data->pt_data->vehicle_journeys[0]);
              b.data->pt_data->physical_modes.push_back(phy_mode);
              b.data->pt_data->physical_modes_map[phy_mode_uri] = phy_mode;
              b.lines["1"]->physical_mode_list.push_back(phy_mode);

              // Add second physical mode
              phy_mode = new navitia::type::PhysicalMode();
              phy_mode->idx = b.data->pt_data->physical_modes.size();
              phy_mode->name = phy_mode_name_2;
              phy_mode->uri = phy_mode_uri_2;
              phy_mode->vehicle_journey_list.push_back(b.data->pt_data->vehicle_journeys[0]);
              b.data->pt_data->physical_modes.push_back(phy_mode);
              b.data->pt_data->physical_modes_map[phy_mode_uri_2] = phy_mode;

              // Add contributor
              auto* contributor = new navitia::type::Contributor();
              contributor->idx = b.data->pt_data->contributors.size();
              contributor->name = contributor_name;
              contributor->uri = contributor_uri;
              b.data->pt_data->contributors.push_back(contributor);
              b.data->pt_data->contributors_map[contributor->uri] = contributor;

              // Add dataset
              auto* dataset = new navitia::type::Dataset();
              dataset->idx = b.data->pt_data->datasets.size();
              dataset->name = dataset_name;
              dataset->uri = dataset_uri;
              dataset->contributor = contributor;
              b.data->pt_data->datasets.push_back(dataset);
              b.data->pt_data->datasets_map[dataset->uri] = dataset;
              b.data->pt_data->contributors_map[contributor->uri]->dataset_list.insert(dataset);
          }) {}

    std::string comp_name;
    std::string comp_uri;
    std::string phy_mode_name;
    std::string phy_mode_uri;
    std::string phy_mode_name_2;
    std::string phy_mode_uri_2;
    std::string contributor_name;
    std::string contributor_uri;
    std::string dataset_name;
    std::string dataset_uri;
    ed::builder b;
};

BOOST_FIXTURE_TEST_CASE(add_and_update_trip_to_verify_default_route_line_commercial_mode_network, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;
    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    const std::string network_id = "network:additional_service";
    auto it_network = pt_data.networks_map.find(network_id);
    BOOST_CHECK(it_network == pt_data.networks_map.end());
    const std::string cm_id = "commercial_mode:additional_service";
    auto it_cm = pt_data.commercial_modes_map.find(cm_id);
    BOOST_CHECK(it_cm == pt_data.commercial_modes_map.end());
    const std::string line_id = "line:A_F";
    auto it_line = pt_data.lines_map.find(line_id);
    BOOST_CHECK(it_line == pt_data.lines_map.end());
    const std::string route_id = "route:A_F";
    auto it_route = pt_data.routes_map.find(route_id);
    BOOST_CHECK(it_route == pt_data.routes_map.end());

    transit_realtime::TripUpdate new_trip =
        ntest::make_trip_update_message("vj_new_trip", "20190101",
                                        {
                                            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
                                            RTStopTime("stop_point:F", "20190101T0900"_pts).added(),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri,
                                        phy_mode_uri, "", "", "trip_headsign");

    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    it_network = pt_data.networks_map.find(network_id);
    BOOST_REQUIRE(it_network != pt_data.networks_map.end());
    const nt::Network* network = it_network->second;
    it_cm = pt_data.commercial_modes_map.find(cm_id);
    BOOST_REQUIRE(it_cm != pt_data.commercial_modes_map.end());
    const nt::CommercialMode* cm = it_cm->second;
    it_line = pt_data.lines_map.find(line_id);
    BOOST_REQUIRE(it_line != pt_data.lines_map.end());
    const nt::Line* line = it_line->second;
    it_route = pt_data.routes_map.find(route_id);
    BOOST_REQUIRE(it_route != pt_data.routes_map.end());
    const nt::Route* route = it_route->second;
    const std::string sa_id = "F";
    auto it_sa = pt_data.stop_areas_map.find(sa_id);
    const nt::StopArea* sa = it_sa->second;

    // Check destination and direction_type of the route
    BOOST_CHECK_EQUAL(route->direction_type, "forward");
    BOOST_CHECK_EQUAL(route->destination, sa);

    // check links
    BOOST_CHECK_EQUAL(network->line_list.front(), line);
    BOOST_CHECK_EQUAL(line->network, network);
    BOOST_CHECK_EQUAL(cm->line_list.front(), line);
    BOOST_CHECK_EQUAL(line->commercial_mode, cm);
    BOOST_CHECK_EQUAL(line->route_list.front(), route);
    BOOST_CHECK_EQUAL(route->line, line);

    // check indexes
    BOOST_CHECK_EQUAL(pt_data.networks[network->idx], network);
    BOOST_CHECK_EQUAL(pt_data.commercial_modes[cm->idx], cm);
    BOOST_CHECK_EQUAL(pt_data.lines[line->idx], line);
    BOOST_CHECK_EQUAL(pt_data.routes[route->idx], route);

    // check uris
    BOOST_CHECK_EQUAL(network->uri, network_id);
    BOOST_CHECK_EQUAL(cm->uri, cm_id);
    BOOST_CHECK_EQUAL(line->uri, line_id);
    BOOST_CHECK_EQUAL(route->uri, route_id);

    // check names
    BOOST_CHECK_EQUAL(network->name, "additional service");
    BOOST_CHECK_EQUAL(cm->name, "additional service");
    BOOST_CHECK_EQUAL(line->name, "A - F");
    BOOST_CHECK_EQUAL(route->name, "A - F");

    // Verify that a journey from stop_point:A to stop_point:F exists
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:F");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);

    // Verify that headsign exists in display_informations
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).pt_display_informations().headsign(), "trip_headsign");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).pt_display_informations().name(), "A - F");

    // Update the trip recently added with destination F replaced by J
    new_trip = ntest::make_trip_update_message("vj_new_trip", "20190101",
                                               {
                                                   RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
                                                   RTStopTime("stop_point:F", "20190101T0900"_pts).skipped(),
                                                   RTStopTime("stop_point:J", "20190101T0900"_pts).added(),
                                               },
                                               transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE,
                                               comp_uri, phy_mode_uri);

    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    it_network = pt_data.networks_map.find(network_id);
    BOOST_REQUIRE(it_network != pt_data.networks_map.end());
    network = it_network->second;
    it_cm = pt_data.commercial_modes_map.find(cm_id);
    BOOST_REQUIRE(it_cm != pt_data.commercial_modes_map.end());
    cm = it_cm->second;
    it_line = pt_data.lines_map.find(line_id);
    BOOST_REQUIRE(it_line != pt_data.lines_map.end());
    line = it_line->second;
    it_route = pt_data.routes_map.find(route_id);
    BOOST_REQUIRE(it_route != pt_data.routes_map.end());
    route = it_route->second;

    // check uris
    BOOST_CHECK_EQUAL(network->uri, network_id);
    BOOST_CHECK_EQUAL(cm->uri, cm_id);
    BOOST_CHECK_EQUAL(line->uri, line_id);
    BOOST_CHECK_EQUAL(route->uri, route_id);

    // check names
    BOOST_CHECK_EQUAL(network->name, "additional service");
    BOOST_CHECK_EQUAL(cm->name, "additional service");
    BOOST_CHECK_EQUAL(line->name, "A - F");
    BOOST_CHECK_EQUAL(route->name, "A - F");

    // Check destination and direction_type of the route
    BOOST_CHECK_EQUAL(route->direction_type, "forward");
    BOOST_CHECK_EQUAL(route->destination, sa);

    // Verify that a journey from stop_point:A to stop_point:J exists
    res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
}

BOOST_FIXTURE_TEST_CASE(add_and_update_trip_to_verify_filled_route_line_commercial_mode_network, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;
    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    // before disruption, no "default-rt" PT objects
    const std::string default_network_id = "network:additional_service";
    auto it_default_network = pt_data.networks_map.find(default_network_id);
    BOOST_CHECK(it_default_network == pt_data.networks_map.end());
    const std::string default_cm_id = "commercial_mode:additional_service";
    auto it_default_cm = pt_data.commercial_modes_map.find(default_cm_id);
    BOOST_CHECK(it_default_cm == pt_data.commercial_modes_map.end());
    const std::string default_line_id = "line:A_F";
    auto it_default_line = pt_data.lines_map.find(default_line_id);
    BOOST_CHECK(it_default_line == pt_data.lines_map.end());
    const std::string default_route_id = "route:A_F";
    auto it_default_route = pt_data.routes_map.find(default_route_id);
    BOOST_CHECK(it_default_route == pt_data.routes_map.end());

    // before disruption, PT objects to be used are already present and linked (except route)
    auto it_dataset = pt_data.datasets_map.find(dataset_uri);
    BOOST_CHECK(it_dataset != pt_data.datasets_map.end());
    const nt::Dataset* dataset = it_dataset->second;
    const std::string network_id = "base_network";
    auto it_network = pt_data.networks_map.find(network_id);
    BOOST_CHECK(it_network != pt_data.networks_map.end());
    const nt::Network* network = it_network->second;
    const std::string cm_id = "0x0";
    auto it_cm = pt_data.commercial_modes_map.find(cm_id);
    BOOST_CHECK(it_cm != pt_data.commercial_modes_map.end());
    const nt::CommercialMode* cm = it_cm->second;
    const std::string line_id = "1";
    auto it_line = pt_data.lines_map.find(line_id);
    BOOST_CHECK(it_line != pt_data.lines_map.end());
    const nt::Line* line = it_line->second;
    const std::string route_id = "route:1:additional_service";
    auto it_route = pt_data.routes_map.find(route_id);
    BOOST_CHECK(it_route == pt_data.routes_map.end());
    const auto nb_vj_in_dataset_beginning = dataset->vehiclejourney_list.size();

    // check links
    BOOST_CHECK_EQUAL(network->line_list.front(), line);
    BOOST_CHECK_EQUAL(line->network, network);
    BOOST_CHECK(navitia::contains(cm->line_list, line));
    BOOST_CHECK_EQUAL(line->commercial_mode, cm);

    // check indexes
    BOOST_CHECK_EQUAL(pt_data.networks[network->idx], network);
    BOOST_CHECK_EQUAL(pt_data.commercial_modes[cm->idx], cm);
    BOOST_CHECK_EQUAL(pt_data.lines[line->idx], line);

    transit_realtime::TripUpdate new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
            RTStopTime("stop_point:F", "20190101T0900"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri, phy_mode_uri, "", "",
        "trip_headsign", "trip_short_name", dataset_uri, network_id, cm_id, line_id, route_id);

    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    // after disruption, no "default-rt" PT objects (as existing one are used)
    it_default_network = pt_data.networks_map.find(default_network_id);
    BOOST_REQUIRE(it_default_network == pt_data.networks_map.end());
    it_default_cm = pt_data.commercial_modes_map.find(default_cm_id);
    BOOST_REQUIRE(it_default_cm == pt_data.commercial_modes_map.end());
    it_default_line = pt_data.lines_map.find(default_line_id);
    BOOST_REQUIRE(it_default_line == pt_data.lines_map.end());
    it_default_route = pt_data.routes_map.find(default_route_id);
    BOOST_REQUIRE(it_default_route == pt_data.routes_map.end());

    it_route = pt_data.routes_map.find(route_id);
    BOOST_REQUIRE(it_route != pt_data.routes_map.end());
    const nt::Route* route = it_route->second;

    const std::string sa_id = "F";
    auto it_sa = pt_data.stop_areas_map.find(sa_id);
    const nt::StopArea* sa = it_sa->second;

    // Check destination and direction_type of the route
    BOOST_CHECK_EQUAL(route->direction_type, "outbound");
    BOOST_CHECK_EQUAL(route->destination, sa);

    // check links
    BOOST_CHECK_EQUAL(network->line_list.front(), line);
    BOOST_CHECK_EQUAL(line->network, network);
    BOOST_CHECK(navitia::contains(cm->line_list, line));
    BOOST_CHECK_EQUAL(line->commercial_mode, cm);
    BOOST_CHECK(navitia::contains(line->route_list, route));
    BOOST_CHECK_EQUAL(route->line, line);
    nt::VehicleJourney* rt_vj = pt_data.vehicle_journeys.back();
    BOOST_CHECK_EQUAL(rt_vj->dataset, dataset);
    BOOST_CHECK_EQUAL(nb_vj_in_dataset_beginning + 1, dataset->vehiclejourney_list.size());
    BOOST_CHECK(navitia::contains(dataset->vehiclejourney_list, rt_vj));
    BOOST_CHECK_EQUAL(rt_vj->route, route);

    // check indexes
    BOOST_CHECK_EQUAL(pt_data.networks[network->idx], network);
    BOOST_CHECK_EQUAL(pt_data.commercial_modes[cm->idx], cm);
    BOOST_CHECK_EQUAL(pt_data.lines[line->idx], line);
    BOOST_CHECK_EQUAL(pt_data.routes[route->idx], route);

    // check uris
    BOOST_CHECK_EQUAL(network->uri, network_id);
    BOOST_CHECK_EQUAL(cm->uri, cm_id);
    BOOST_CHECK_EQUAL(line->uri, line_id);
    BOOST_CHECK_EQUAL(route->uri, route_id);

    // check names
    BOOST_CHECK_EQUAL(network->name, network_id);
    BOOST_CHECK_EQUAL(cm->name, "Tramway");
    BOOST_CHECK_EQUAL(line->name, line_id);
    BOOST_CHECK_EQUAL(route->name, "Additional service");

    // Verify that a journey from stop_point:A to stop_point:F exists
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:F");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);

    // Verify that headsign exists in display_informations
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).pt_display_informations().headsign(), "trip_headsign");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).pt_display_informations().trip_short_name(), "trip_short_name");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).pt_display_informations().name(), line_id);

    // Update the trip recently added with destination F replaced by J
    new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
            RTStopTime("stop_point:F", "20190101T0900"_pts).skipped(),
            RTStopTime("stop_point:J", "20190101T0900"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri, phy_mode_uri, "", "",
        "new_trip_headsign", "new_trip_short_name", dataset_uri, network_id, cm_id, line_id, route_id);

    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    // after disruption update, still no "default-rt" PT objects (as existing one are used)
    it_default_network = pt_data.networks_map.find(default_network_id);
    BOOST_REQUIRE(it_default_network == pt_data.networks_map.end());
    it_default_cm = pt_data.commercial_modes_map.find(default_cm_id);
    BOOST_REQUIRE(it_default_cm == pt_data.commercial_modes_map.end());
    it_default_line = pt_data.lines_map.find(default_line_id);
    BOOST_REQUIRE(it_default_line == pt_data.lines_map.end());
    it_default_route = pt_data.routes_map.find(default_route_id);
    BOOST_REQUIRE(it_default_route == pt_data.routes_map.end());

    it_route = pt_data.routes_map.find(route_id);
    BOOST_REQUIRE(it_route != pt_data.routes_map.end());
    route = it_route->second;

    rt_vj = pt_data.vehicle_journeys.back();
    BOOST_CHECK_EQUAL(rt_vj->dataset, dataset);
    BOOST_CHECK_EQUAL(nb_vj_in_dataset_beginning + 1, dataset->vehiclejourney_list.size());
    BOOST_CHECK(navitia::contains(dataset->vehiclejourney_list, rt_vj));
    BOOST_CHECK_EQUAL(rt_vj->route, route);

    // Check destination and direction_type of the route
    BOOST_CHECK_EQUAL(route->direction_type, "outbound");
    BOOST_CHECK_EQUAL(route->destination, sa);

    // check uris
    BOOST_CHECK_EQUAL(network->uri, network_id);
    BOOST_CHECK_EQUAL(cm->uri, cm_id);
    BOOST_CHECK_EQUAL(line->uri, line_id);
    BOOST_CHECK_EQUAL(route->uri, route_id);

    // check names
    BOOST_CHECK_EQUAL(network->name, network_id);
    BOOST_CHECK_EQUAL(cm->name, "Tramway");
    BOOST_CHECK_EQUAL(line->name, line_id);
    BOOST_CHECK_EQUAL(route->name, "Additional service");

    // Verify that a journey from stop_point:A to stop_point:J exists
    res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);

    // Verify that headsign exists in display_informations
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).pt_display_informations().headsign(), "new_trip_headsign");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).pt_display_informations().trip_short_name(), "new_trip_short_name");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).pt_display_informations().name(), line_id);
}

BOOST_FIXTURE_TEST_CASE(add_new_trip_and_update, AddTripDataset) {
    using year = navitia::type::ValidityPattern::year_bitset;
    auto& pt_data = *b.data->pt_data;
    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    auto res = compute("20190101T073000", "stop_point:A", "stop_point:D");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).departure_date_time(), "20190101T080000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).stop_point().uri(), "stop_point:A");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).departure_date_time(), "20190101T083000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).stop_point().uri(), "stop_point:B");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).departure_date_time(), "20190101T090000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).stop_point().uri(), "stop_point:C");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).departure_date_time(), "20190101T093000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).stop_point().uri(), "stop_point:D");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(res.impacts_size(), 0);

    // For the moment, trip from A to G doesn't exist
    res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);

    transit_realtime::TripUpdate new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
            RTStopTime("stop_point:E", "20190101T0830"_pts).added(),
            RTStopTime("stop_point:F", "20190101T0900"_pts).added(),
            RTStopTime("stop_point:G", "20190101T0930"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri, phy_mode_uri);

    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    // Check meta vj table
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj:1"), true);
    auto* mvj = pt_data.meta_vjs.get_mut("vj:1");
    BOOST_CHECK_EQUAL(mvj->get_label(), "vj:1");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 1);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), true);
    mvj = pt_data.meta_vjs.get_mut("vj_new_trip");
    BOOST_CHECK_EQUAL(mvj->get_label(), "vj_new_trip");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 1);

    // Check vj table
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 2);

    auto* vj = pt_data.vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_EQUAL(vj->uri, "vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(vj->idx, 0);
    BOOST_CHECK_EQUAL(vj->meta_vj->get_label(), "vj:1");
    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 4);
    BOOST_CHECK_EQUAL(vj->base_validity_pattern()->days, year("11111111"));
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern()->days, year("11111111"));
    BOOST_CHECK_EQUAL(vj->rt_validity_pattern()->days, year("11111111"));

    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj_new_trip");
    BOOST_REQUIRE_EQUAL(meta_vj->get_rt_vj().size(), 1);

    const auto* rt_vj = meta_vj->get_rt_vj()[0].get();
    BOOST_CHECK_EQUAL(rt_vj->company->uri, comp_uri);
    BOOST_CHECK_EQUAL(rt_vj->company->name, comp_name);
    BOOST_CHECK_EQUAL(rt_vj->physical_mode->uri, phy_mode_uri);
    BOOST_CHECK_EQUAL(rt_vj->physical_mode->name, phy_mode_name);
    BOOST_CHECK_EQUAL(rt_vj->dataset->uri, dataset_uri);
    BOOST_CHECK_EQUAL(rt_vj->dataset->name, dataset_name);
    BOOST_CHECK_EQUAL(rt_vj->dataset->contributor->uri, contributor_uri);
    BOOST_CHECK_EQUAL(rt_vj->dataset->contributor->name, contributor_name);
    BOOST_ASSERT(boost::algorithm::contains(rt_vj->uri, "RealTime"));
    BOOST_CHECK_EQUAL(rt_vj->idx, 1);
    BOOST_CHECK_EQUAL(rt_vj->name, "");
    BOOST_CHECK_EQUAL(rt_vj->headsign, "");
    BOOST_CHECK_EQUAL(rt_vj->meta_vj->get_label(), "vj_new_trip");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.size(), 4);
    BOOST_CHECK_EQUAL(rt_vj->base_validity_pattern()->days, year("00000000"));
    BOOST_CHECK_EQUAL(rt_vj->adapted_validity_pattern()->days, year("00000000"));
    BOOST_CHECK_EQUAL(rt_vj->rt_validity_pattern()->days, year("00000001"));

    // verify that filters &since and &until work well with vehicle_jouneys added by realtime (added trip)
    // Note: For backward compatibility parameter &data_freshness with base_schedule is added and works
    // only with &since and &until
    auto indexes =
        navitia::ptref::make_query(nt::Type_e::VehicleJourney, "", {}, nt::OdtLevel_e::all, {"20190101T0400"_dt},
                                   {"20190101T1000"_dt}, navitia::type::RTLevel::Base, *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);
    indexes = navitia::ptref::make_query(nt::Type_e::VehicleJourney, "", {}, nt::OdtLevel_e::all, {"20190101T0400"_dt},
                                         {"20190101T1000"_dt}, navitia::type::RTLevel::RealTime, *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    // New trip added
    res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).departure_date_time(), "20190101T080000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).stop_point().uri(), "stop_point:A");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).departure_date_time(), "20190101T083000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).stop_point().uri(), "stop_point:E");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).departure_date_time(), "20190101T090000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).stop_point().uri(), "stop_point:F");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).departure_date_time(), "20190101T093000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).stop_point().uri(), "stop_point:G");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    // impact
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).uri(), "feed-1");
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_ADDITIONAL_SERVICE);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);

    // Recall `and completely over-ride` the same new trip `: this is how Kirin updates a trip-add`
    // The old "new trip" is replace by the new, but the meta VJ is the same (don't create).
    // The old impact is deleted too
    // To show that it is the new GTFS-RT, schedules has changed
    new_trip = ntest::make_trip_update_message("vj_new_trip", "20190101",
                                               {
                                                   RTStopTime("stop_point:A", "20190101T0900"_pts).added(),
                                                   RTStopTime("stop_point:E", "20190101T0930"_pts).added(),
                                                   RTStopTime("stop_point:F", "20190101T1000"_pts).added(),
                                                   RTStopTime("stop_point:G", "20190101T1030"_pts).added(),
                                               },
                                               transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE,
                                               comp_uri, phy_mode_uri);
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    // Check meta vj (there is no creation)
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj:1"), true);
    mvj = pt_data.meta_vjs.get_mut("vj:1");
    BOOST_CHECK_EQUAL(mvj->get_label(), "vj:1");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 1);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), true);
    mvj = pt_data.meta_vjs.get_mut("vj_new_trip");
    BOOST_CHECK_EQUAL(mvj->get_label(), "vj_new_trip");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 1);

    // Check vj table
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 2);

    // New trip added
    res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).departure_date_time(), "20190101T090000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).stop_point().uri(), "stop_point:A");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).departure_date_time(), "20190101T093000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).stop_point().uri(), "stop_point:E");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).departure_date_time(), "20190101T100000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).stop_point().uri(), "stop_point:F");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).departure_date_time(), "20190101T103000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).stop_point().uri(), "stop_point:G");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    // impact
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).uri(), "feed-1");
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_ADDITIONAL_SERVICE);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);

    // Add new trip with new id
    transit_realtime::TripUpdate new_trip_2 =
        ntest::make_trip_update_message("vj_new_trip_2", "20190101",
                                        {
                                            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
                                            RTStopTime("stop_point:H", "20190101T0830"_pts).added(),
                                            RTStopTime("stop_point:I", "20190101T0900"_pts).added(),
                                            RTStopTime("stop_point:J", "20190101T0930"_pts).added(),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri,
                                        phy_mode_uri, "", "", "trip_headsign");
    navitia::handle_realtime("feed-2", timestamp, new_trip_2, *b.data, true, true);
    b.finalize_disruption_batch();

    // Check if meta vj exist
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 3);
    mvj = pt_data.meta_vjs.get_mut("vj_new_trip_2");
    BOOST_CHECK_EQUAL(mvj->get_label(), "vj_new_trip_2");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 1);

    // Check vj table
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 3);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 3);

    // New VJ
    meta_vj = b.get_meta_vj("vj_new_trip_2");
    BOOST_REQUIRE_EQUAL(meta_vj->get_rt_vj().size(), 1);

    rt_vj = meta_vj->get_rt_vj()[0].get();
    BOOST_CHECK_EQUAL(rt_vj->company->uri, comp_uri);
    BOOST_CHECK_EQUAL(rt_vj->company->name, comp_name);
    BOOST_CHECK_EQUAL(rt_vj->physical_mode->uri, phy_mode_uri);
    BOOST_CHECK_EQUAL(rt_vj->physical_mode->name, phy_mode_name);
    BOOST_CHECK_EQUAL(rt_vj->dataset->uri, dataset_uri);
    BOOST_CHECK_EQUAL(rt_vj->dataset->name, dataset_name);
    BOOST_CHECK_EQUAL(rt_vj->dataset->contributor->uri, contributor_uri);
    BOOST_CHECK_EQUAL(rt_vj->dataset->contributor->name, contributor_name);
    BOOST_ASSERT(boost::algorithm::contains(rt_vj->uri, "RealTime"));
    BOOST_CHECK_EQUAL(rt_vj->idx, 2);
    BOOST_CHECK_EQUAL(rt_vj->name, "trip_headsign");
    BOOST_CHECK_EQUAL(rt_vj->headsign, "trip_headsign");
    BOOST_CHECK_EQUAL(rt_vj->meta_vj->get_label(), "vj_new_trip_2");
    BOOST_CHECK_EQUAL(rt_vj->stop_time_list.size(), 4);
    BOOST_CHECK_EQUAL(rt_vj->base_validity_pattern()->days, year("00000000"));
    BOOST_CHECK_EQUAL(rt_vj->adapted_validity_pattern()->days, year("00000000"));
    BOOST_CHECK_EQUAL(rt_vj->rt_validity_pattern()->days, year("00000001"));
    auto st = rt_vj->stop_time_list.front();
    BOOST_CHECK_EQUAL(pt_data.headsign_handler.get_headsign(st), "trip_headsign");

    // New trip added
    res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).departure_date_time(), "20190101T080000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(0).stop_point().uri(), "stop_point:A");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).departure_date_time(), "20190101T083000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(1).stop_point().uri(), "stop_point:H");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).departure_date_time(), "20190101T090000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).stop_point().uri(), "stop_point:I");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).departure_date_time(), "20190101T093000"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).stop_point().uri(), "stop_point:J");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    // impact
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).uri(), "feed-2");
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_ADDITIONAL_SERVICE);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).departure_status(),
                      pbnavitia::StopTimeUpdateStatus::ADDED);
}

BOOST_FIXTURE_TEST_CASE(flags_block_new_trip, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    transit_realtime::TripUpdate new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
            RTStopTime("stop_point:E", "20190101T0830"_pts).added(),
            RTStopTime("stop_point:F", "20190101T0900"_pts).added(),
            RTStopTime("stop_point:G", "20190101T0930"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri, phy_mode_uri);

    // call function with is_realtime_add_enabled=false and is_realtime_add_trip_enabled=false
    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, false, false);
    b.finalize_disruption_batch();
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);

    // call function with is_realtime_add_enabled=false
    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, false, true);
    b.finalize_disruption_batch();
    res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);

    // call function with is_realtime_add_trip_enabled=false
    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, false);
    b.finalize_disruption_batch();
    res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);
}

BOOST_FIXTURE_TEST_CASE(company_id_doesnt_exist_in_new_trip, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    // If the company id doesn't exist inside the data with ADDED type, we reject the new trip
    transit_realtime::TripUpdate new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
            RTStopTime("stop_point:H", "20190101T0830"_pts).added(),
            RTStopTime("stop_point:I", "20190101T0900"_pts).added(),
            RTStopTime("stop_point:J", "20190101T0930"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, "company_id_that_doesnt_exist", phy_mode_uri);

    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);
}

BOOST_FIXTURE_TEST_CASE(line_id_doesnt_exist_in_new_trip, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    // If the line id doesn't exist inside the data with ADDED type, we reject the new trip
    transit_realtime::TripUpdate new_trip =
        ntest::make_trip_update_message("vj_new_trip", "20190101",
                                        {
                                            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
                                            RTStopTime("stop_point:H", "20190101T0830"_pts).added(),
                                            RTStopTime("stop_point:I", "20190101T0900"_pts).added(),
                                            RTStopTime("stop_point:J", "20190101T0930"_pts).added(),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri,
                                        phy_mode_uri, "", "", "trip_headsign", "trip_short_name", dataset_uri,
                                        "base_network", "0x0", "wrong_line_id", "route:1:additional_service");

    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);
}

BOOST_FIXTURE_TEST_CASE(dataset_id_doesnt_exist_in_new_trip, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    // If the dataset id doesn't exist inside the data with ADDED type, we reject the new trip
    transit_realtime::TripUpdate new_trip =
        ntest::make_trip_update_message("vj_new_trip", "20190101",
                                        {
                                            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
                                            RTStopTime("stop_point:H", "20190101T0830"_pts).added(),
                                            RTStopTime("stop_point:I", "20190101T0900"_pts).added(),
                                            RTStopTime("stop_point:J", "20190101T0930"_pts).added(),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri,
                                        phy_mode_uri, "", "", "trip_headsign", "trip_short_name", "wrong_dataset_id",
                                        "base_network", "0x0", "1", "route:1:additional_service");

    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);
}

BOOST_FIXTURE_TEST_CASE(network_id_doesnt_exist_in_new_trip, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    // If the network id doesn't exist inside the data with ADDED type, we reject the new trip
    transit_realtime::TripUpdate new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
            RTStopTime("stop_point:H", "20190101T0830"_pts).added(),
            RTStopTime("stop_point:I", "20190101T0900"_pts).added(),
            RTStopTime("stop_point:J", "20190101T0930"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri, phy_mode_uri, "", "",
        "trip_headsign", "trip_short_name", dataset_uri, "wrong_network_id", "0x0", "1", "route:1:additional_service");

    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);
}

BOOST_FIXTURE_TEST_CASE(commercial_mode_id_doesnt_exist_in_new_trip, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    // If the commercial_mode id doesn't exist inside the data with ADDED type, we reject the new trip
    transit_realtime::TripUpdate new_trip =
        ntest::make_trip_update_message("vj_new_trip", "20190101",
                                        {
                                            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
                                            RTStopTime("stop_point:H", "20190101T0830"_pts).added(),
                                            RTStopTime("stop_point:I", "20190101T0900"_pts).added(),
                                            RTStopTime("stop_point:J", "20190101T0930"_pts).added(),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri,
                                        phy_mode_uri, "", "", "trip_headsign", "trip_short_name", dataset_uri,
                                        "base_network", "wrong_commercial_mode_id", "1", "route:1:additional_service");

    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);
}

BOOST_FIXTURE_TEST_CASE(physical_mode_id_doesnt_exist_in_new_trip, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    // If the physical mode id doesn't exist inside the data with ADDED type, we reject the new trip
    transit_realtime::TripUpdate new_trip =
        ntest::make_trip_update_message("vj_new_trip", "20190101",
                                        {
                                            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
                                            RTStopTime("stop_point:H", "20190101T0830"_pts).added(),
                                            RTStopTime("stop_point:I", "20190101T0900"_pts).added(),
                                            RTStopTime("stop_point:J", "20190101T0930"_pts).added(),
                                        },
                                        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri,
                                        "physical_mode_id_that_doesnt_exist");

    // the new trip update is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), false);
}

BOOST_FIXTURE_TEST_CASE(trip_id_that_doesnt_exist_must_be_rejected_in_classical_mode, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    // If trip id doesn't exist and the GTFS-RT is not an "Add new trip", we reject
    transit_realtime::TripUpdate new_trip =
        ntest::make_trip_update_message("vj_id_doesnt_exist", "20190101",
                                        {RTStopTime("stop_point:A", "20190101T0809"_pts).delay(9_min),
                                         RTStopTime("stop_point:H", "20190101T0839"_pts).delay(9_min),
                                         RTStopTime("stop_point:H", "20190101T0909"_pts).delay(9_min),
                                         RTStopTime("stop_point:H", "20190101T0939"_pts).delay(9_min)},
                                        transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS);

    // the new trip update with bad id is blocked directly
    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_CHECK(!pt_data.meta_vjs.exists("vj_id_doesnt_exist"));
}

BOOST_FIXTURE_TEST_CASE(cancelled_trip, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;
    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    transit_realtime::TripUpdate new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
            RTStopTime("stop_point:E", "20190101T0830"_pts).added(),
            RTStopTime("stop_point:F", "20190101T0900"_pts).added(),
            RTStopTime("stop_point:G", "20190101T0930"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri, phy_mode_uri);

    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    auto res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);

    // Check meta vj table
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj:1"), true);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), true);

    // Check vj table
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 2);

    new_trip = ntest::make_trip_update_message("vj_new_trip_2", "20190101",
                                               {
                                                   RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
                                                   RTStopTime("stop_point:H", "20190101T0830"_pts).added(),
                                                   RTStopTime("stop_point:I", "20190101T0900"_pts).added(),
                                                   RTStopTime("stop_point:J", "20190101T0930"_pts).added(),
                                               },
                                               transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE,
                                               comp_uri, phy_mode_uri);

    navitia::handle_realtime("feed-2", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    res = compute("20190101T073000", "stop_point:A", "stop_point:J");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);

    // Check meta vj table
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 3);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj:1"), true);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), true);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip_2"), true);

    // Check vj table
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 3);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 3);

    transit_realtime::TripUpdate remove_new_trip = ntest::make_trip_update_message(
        "vj_new_trip", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0900"_pts),
            RTStopTime("stop_point:E", "20190101T0930"_pts),
            RTStopTime("stop_point:F", "20190101T1000"_pts),
            RTStopTime("stop_point:G", "20190101T1030"_pts),
        },
        transit_realtime::Alert_Effect::Alert_Effect_NO_SERVICE, comp_uri, phy_mode_uri);

    navitia::handle_realtime("feed-1", timestamp, remove_new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    // trip from A to G is now removed
    res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);

    // Check meta vj table
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 3);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj:1"), true);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip"), true);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj_new_trip_2"), true);

    // Check vj table
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 2);

    // Send the same GTFS-RT. Have to reject it because id doesn't exist.
    remove_new_trip = ntest::make_trip_update_message("vj_new_trip", "20190101",
                                                      {
                                                          RTStopTime("stop_point:A", "20190101T0900"_pts),
                                                          RTStopTime("stop_point:E", "20190101T0930"_pts),
                                                          RTStopTime("stop_point:F", "20190101T1000"_pts),
                                                          RTStopTime("stop_point:G", "20190101T1030"_pts),
                                                      },
                                                      transit_realtime::Alert_Effect::Alert_Effect_NO_SERVICE, comp_uri,
                                                      phy_mode_uri);

    navitia::handle_realtime("feed-1", timestamp, remove_new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    // Check meta vj table
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 3);

    // Check vj table
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 2);
}

BOOST_FIXTURE_TEST_CASE(cant_cancel_trip_that_doesnt_exist, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    BOOST_CHECK(!pt_data.meta_vjs.exists("vj_id_doesnt_exist"));
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 1);

    // Can't cancel trip that doesn't exist
    transit_realtime::TripUpdate remove_trip = ntest::make_trip_update_message(
        "vj_id_doesnt_exist", "20190101",
        {
            RTStopTime("stop_point:X", "20190101T0900"_pts),
            RTStopTime("stop_point:X", "20190101T0930"_pts),
            RTStopTime("stop_point:X", "20190101T1000"_pts),
            RTStopTime("stop_point:X", "20190101T1030"_pts),
        },
        transit_realtime::Alert_Effect::Alert_Effect_NO_SERVICE, comp_uri, phy_mode_uri);

    navitia::handle_realtime("feed-1", timestamp, remove_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    BOOST_CHECK(!pt_data.meta_vjs.exists("vj_id_doesnt_exist"));
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys_map.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 1);
}

BOOST_FIXTURE_TEST_CASE(physical_mode_id_only_impact_additional_service, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;

    // Base VJ
    auto* vj = pt_data.vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_EQUAL(vj->physical_mode->uri, phy_mode_uri);
    BOOST_CHECK_EQUAL(vj->physical_mode->name, phy_mode_name);

    transit_realtime::TripUpdate update_trip = ntest::make_trip_update_message(
        "vj:1", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).delay(0_min),
            RTStopTime("stop_point:B", "20190101T0830"_pts).delay(0_min),
            RTStopTime("stop_point:C", "20190101T0900"_pts).delay(5_min),
            RTStopTime("stop_point:D", "20190101T0930"_pts).delay(5_min),
        },
        transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS, "", phy_mode_uri_2);

    navitia::handle_realtime("feed-1", timestamp, update_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    // physical mode =  base VJ physical mode
    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj:1");
    BOOST_REQUIRE_EQUAL(meta_vj->get_rt_vj().size(), 1);

    const auto* rt_vj = meta_vj->get_rt_vj()[0].get();
    BOOST_CHECK_EQUAL(rt_vj->physical_mode->uri, phy_mode_uri);
    BOOST_CHECK_EQUAL(rt_vj->physical_mode->name, phy_mode_name);

    update_trip = ntest::make_trip_update_message("vj:1", "20190101",
                                                  {
                                                      RTStopTime("stop_point:A", "20190101T0800"_pts).delay(0_min),
                                                      RTStopTime("stop_point:B", "20190101T0830"_pts).delay(0_min),
                                                      RTStopTime("stop_point:C", "20190101T0900"_pts).delay(5_min),
                                                      RTStopTime("stop_point:D", "20190101T0930"_pts).delay(5_min),
                                                  },
                                                  transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS, "",
                                                  "physical_mode_id_that_doesnt_exist");

    navitia::handle_realtime("feed-1", timestamp, update_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    // physical mode =  base VJ physical mode
    // physical mode =  base VJ physical mode
    meta_vj = b.get_meta_vj("vj:1");
    BOOST_REQUIRE_EQUAL(meta_vj->get_rt_vj().size(), 1);

    rt_vj = meta_vj->get_rt_vj()[0].get();
    BOOST_CHECK_EQUAL(rt_vj->physical_mode->uri, phy_mode_uri);
    BOOST_CHECK_EQUAL(rt_vj->physical_mode->name, phy_mode_name);
}

BOOST_FIXTURE_TEST_CASE(cannot_add_new_trip_if_id_corresponds_to_a_base_VJ_the_same_day, AddTripDataset) {
    auto& pt_data = *b.data->pt_data;
    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const std::string& datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20190101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp(datetime)}, true,
                      navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::RealTime, 2_min);
        return pb_creator.get_response();
    };

    // Add new trip with same id of the vj:1 base vj
    transit_realtime::TripUpdate new_trip = ntest::make_trip_update_message(
        "vj:1", "20190101",
        {
            RTStopTime("stop_point:A", "20190101T0800"_pts).added(),
            RTStopTime("stop_point:E", "20190101T0830"_pts).added(),
            RTStopTime("stop_point:F", "20190101T0900"_pts).added(),
            RTStopTime("stop_point:G", "20190101T0930"_pts).added(),
        },
        transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE, comp_uri, phy_mode_uri);

    navitia::handle_realtime("feed-1", timestamp, new_trip, *b.data, true, true);
    b.finalize_disruption_batch();

    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data.meta_vjs.exists("vj:1"), true);
    auto res = compute("20190101T073000", "stop_point:A", "stop_point:G");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.impacts_size(), 0);
}
