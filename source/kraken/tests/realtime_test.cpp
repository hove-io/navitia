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
#include "routing/raptor_api.h"
#include "kraken/apply_disruption.h"
#include "disruption/traffic_reports_api.h"
#include "type/pb_converter.h"

struct logger_initialized {
    logger_initialized()   { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );

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

static transit_realtime::TripUpdate
make_cancellation_message(const std::string& vj_uri, const std::string& date) {
    transit_realtime::TripUpdate trip_update;
    auto trip = trip_update.mutable_trip();
    trip->set_trip_id(vj_uri);
    trip->set_start_date(date);
    trip->set_schedule_relationship(transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED);
    trip->SetExtension(kirin::contributor, "cow.owner");
    trip_update.SetExtension(kirin::trip_message, "cow on the tracks");
    return trip_update;
}

static pbnavitia::Response compute_iti(
    const ed::builder& b,
    const char* datetime, 
    const std::string& from, 
    const std::string& to,
    const navitia::type::RTLevel level) 
{
    navitia::type::EntryPoint origin(b.data->get_type_of_id(from), from);
    navitia::type::EntryPoint destination(b.data->get_type_of_id(to), to);

    navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    make_response(pb_creator, raptor, origin, destination,
            { ntest::to_posix_timestamp(datetime) },
            true, navitia::type::AccessibiliteParams(), {}, {},
            sn_worker, level, 2_min);

    return  pb_creator.get_response();
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

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true);

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

    //here we verify contributor:
    auto disruption = b.data->pt_data->disruption_holder.get_disruption(impact->uri);
    BOOST_REQUIRE_EQUAL(disruption->contributor, "cow.owner");

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true);

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

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true);

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

    b.make();
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

    b.make();
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

    // to test company
    navitia::type::Company* cmp1 = new navitia::type::Company();
    cmp1->line_list.push_back(b.lines["A"]);
    cmp1->idx = b.data->pt_data->companies.size();
    cmp1->name = "Comp1";
    cmp1->uri = comp_id_1;
    b.data->pt_data->companies.push_back(cmp1);
    b.lines["A"]->company_list.push_back(cmp1);

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0810"_pts).delay(9_min),
                    RTStopTime("stop2", "20150928T0910"_pts).delay(9_min)
            },
            comp_id_1);
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("bob", timestamp, trip_update, *b.data, true);

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
    navitia::handle_realtime("bob", timestamp, trip_update, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    BOOST_REQUIRE_EQUAL(impacts.size(), 1);  //we should still have only one impact

    // we add a third time the same message but with a different id, it should not change anything
    // but for the number of impacts in the meta vj
    navitia::handle_realtime("bobette", timestamp, trip_update, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 2);

    b.make();

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

BOOST_AUTO_TEST_CASE(train_delayed_vj_cleaned_up) {

    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t);

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0810"_pts).delay(9_min),
                    RTStopTime("stop2", "20150928T0910"_pts).delay(9_min)
            });
    b.data->build_uri();
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true);
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
}

BOOST_AUTO_TEST_CASE(two_different_delays_on_same_vj) {

    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t)("stop3", "10:01"_t);

    transit_realtime::TripUpdate trip_update_1 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0810"_pts),
                    RTStopTime("stop2", "20150928T0910"_pts).delay(9_min),
                    RTStopTime("stop3", "20150928T1001"_pts)
            });

    transit_realtime::TripUpdate trip_update_2 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0810"_pts),
                    RTStopTime("stop2", "20150928T0910"_pts).delay(9_min),
                    RTStopTime("stop3", "20150928T1030"_pts).delay(29_min)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_1, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    // We should have two vp
    // a vp for the current vj, and an empty vp
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.make();
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
    navitia::handle_realtime(feed_id_1, timestamp, trip_update_2, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.make();
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
                    RTStopTime("stop1", "20150928T2330"_pts).delay(30_min),
                    RTStopTime("stop2", "20150929T0025"_pts).delay(30_min)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true);

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
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    b.make();

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
                    RTStopTime("stop1", "20150928T2330"_pts).delay(30_min),
                    RTStopTime("stop2", "20150929T0025"_pts).delay(30_min)
            });

    transit_realtime::TripUpdate trip_update_B = ntest::make_delay_message("vj:2",
            "20150928",
            {
                    RTStopTime("stop3", "20150928T2230"_pts).delay(30_min),
                    RTStopTime("stop4", "20150928T2300"_pts).delay(30_min)
            });

    b.data->build_uri();

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj_A = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_EQUAL(vj_A->base_validity_pattern(), vj_A->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_A, *b.data, true);

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
    navitia::handle_realtime(feed_id_1, timestamp, trip_update_B, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    auto vj_B = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_NE(vj_B->base_validity_pattern(), vj_B->rt_validity_pattern());


    b.make();

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
                    RTStopTime("stop1", "20150928T0810"_pts).delay(10_min),
                    RTStopTime("stop2", "20150928T0910"_pts).delay(10_min)
            });
    b.data->build_uri();

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj_A = pt_data->vehicle_journeys.at(0);
    BOOST_CHECK_EQUAL(vj_A->base_validity_pattern(), vj_A->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, trip_update_A, *b.data, true);

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
    const auto* vj = pt_data->vehicle_journeys_map.at("vj:1");
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::apply_disruption(disrup, *b.data->pt_data, *b.data->meta);

    b.make();
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

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    //no cleanup for the moment, but it would be nice
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // but the vp should be equals again
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());
    b.make();
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
    auto vj = b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t).make();
    b.data->build_uri();

    transit_realtime::TripUpdate wrong_st_order = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    //stop1 is after stop2, it's not valid
                    RTStopTime("stop1", "20150928T1000"_pts).delay(2_h),
                    RTStopTime("stop2", "20150928T0910"_pts).delay(10_min)
            });

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    navitia::handle_realtime(feed_id, timestamp, wrong_st_order, *b.data, true);

    //there should be no disruption added
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    // we test with a wrongly formated stoptime
    transit_realtime::TripUpdate dep_before_arr = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0810"_pts).delay(10_min),
                    //departure is before arrival, it's not valid too
                    RTStopTime("stop2", "20150928T0910"_pts, "20150928T0900"_pts).arrival_delay(9_min)
            });

    navitia::handle_realtime(feed_id, timestamp, dep_before_arr, *b.data, true);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(pt_data->disruption_holder.nb_disruptions(), 0);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_impacts().size(), 0);

    //we test with a first stop time before the day of the disrupted vj
    transit_realtime::TripUpdate wrong_first_st = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150926T0800"_pts, "20150927T0200"_pts).arrival_delay(10_min)
                                                                                      .departure_delay(18_h),
                    RTStopTime("stop2", "20150927T0300"_pts).delay(18_h)
            });

    navitia::handle_realtime(feed_id, timestamp, wrong_first_st, *b.data, true);

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
                    RTStopTime("stop1", "20150929T0610"_pts).delay(9_min),
                    RTStopTime("stop2", "20150929T0710"_pts).delay(9_min)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, trip_update, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1DayD0_bis", timestamp, trip_update, *b.data, true);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    b.make();

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
                    RTStopTime("stop1", "20150929T1710"_pts).delay(9_h),
                    RTStopTime("stop2", "20150930T0110"_pts).delay(16_h)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayPassMidnightD0", timestamp, trip_update, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1DayPassMidnightD0_bis", timestamp, trip_update, *b.data, true);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 3);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    b.make();

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
                    RTStopTime("stop1", "20150929T0710"_pts).delay(23_h),
                    RTStopTime("stop2", "20150929T0810"_pts).delay(23_h)
            });
    transit_realtime::TripUpdate second_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0901"_pts).delay(24_h),
                    RTStopTime("stop2", "20150928T1001"_pts).delay(24_h)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1HourD0", timestamp, second_trip_update, *b.data, true);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    b.make();

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
                    RTStopTime("stop1", "20150929T0710"_pts).delay(23_h),
                    RTStopTime("stop2", "20150929T0810"_pts).delay(23_h)
            });
    transit_realtime::TripUpdate second_trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0801"_pts),
                    RTStopTime("stop2", "20150928T0901"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("backToNormalD0", timestamp, second_trip_update, *b.data, true);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    b.make();

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
                    RTStopTime("stop1", "20150929T0710"_pts).delay(23_h),
                    RTStopTime("stop2", "20150929T0810"_pts).delay(23_h)
            });
    transit_realtime::TripUpdate second_trip_update = ntest::make_delay_message("vj:1",
            "20150929",
            {
                    RTStopTime("stop1", "20150929T0901"_pts).delay(25_h),
                    RTStopTime("stop2", "20150929T1001"_pts).delay(25_h)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("delay1HourD1", timestamp, second_trip_update, *b.data, true);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 5);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    b.make();

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
                    RTStopTime("stop1", "20150929T0610"_pts).delay(22_h),
                    RTStopTime("stop2", "20150929T0710"_pts).delay(22_h)
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

    navitia::handle_realtime("delay1DayD0", timestamp, first_trip_update, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD0", timestamp, second_trip_update, *b.data, true);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.make();

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
                    RTStopTime("stop1", "20150929T0610"_pts).delay(22_h),
                    RTStopTime("stop2", "20150929T0710"_pts).delay(22_h)
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

    navitia::handle_realtime("delay1dayD0", timestamp, first_trip_update, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD1", timestamp, second_trip_update, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    b.make();

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

    navitia::handle_realtime("cancelD0", timestamp, first_trip_update, *b.data, true);

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
    navitia::handle_realtime("cancelD1", timestamp, second_trip_update, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 2);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.make();

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
                    RTStopTime("stop1", "20150928T1801"_pts).delay(10_h),
                    RTStopTime("stop2", "20150928T1901"_pts).delay(10_h)
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

    navitia::handle_realtime("delay10HoursD0", timestamp, first_trip_update, *b.data, true);

    // We should have 2 vj
    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 4);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime("cancelD0", timestamp, second_trip_update, *b.data, true);


    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_GE(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());


    b.make();

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
                    RTStopTime("stop1", "20150928T0910"_pts).delay(1_h),
                    RTStopTime("stop2", "20150928T1010"_pts).delay(1_h)
            });
    transit_realtime::TripUpdate second_trip_update = ntest::make_delay_message("vj:1",
            "20150929",
            {
                    RTStopTime("stop1", "20150929T1010"_pts).delay(2_h),
                    RTStopTime("stop2", "20150929T1110"_pts).delay(2_h)
            });
    transit_realtime::TripUpdate third_trip_update = make_cancellation_message("vj:1", "20150930");

    b.data->build_uri();

    const auto& pt_data = b.data->pt_data;
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(vj->get_impacts().size(), 0);

    navitia::handle_realtime("delay1hourD0", timestamp, first_trip_update, *b.data, true);

    // get vj realtime for d0 and check it's on day 0
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->meta_vj->get_rt_vj().size(), 1);
    const auto vj_rt_d0 = vj->meta_vj->get_rt_vj()[0].get();
    BOOST_CHECK(vj_rt_d0->get_validity_pattern_at(vj_rt_d0->realtime_level)->check(0));

    BOOST_REQUIRE_EQUAL(vj->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj->get_impacts()[0]->uri, "delay1hourD0");
    BOOST_REQUIRE_EQUAL(vj_rt_d0->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(vj_rt_d0->get_impacts()[0]->uri, "delay1hourD0");

    navitia::handle_realtime("delay2hourD1", timestamp, second_trip_update, *b.data, true);

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

    navitia::handle_realtime("cancelD3", timestamp, third_trip_update, *b.data, true);

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
    b.make();

    transit_realtime::TripUpdate trip_update_vj2 = ntest::make_delay_message(
        "vj:2",
        "20150928",
        {
            RTStopTime("stop1", "20150928T0910"_pts).delay(1_h),
            RTStopTime("stop2", "20150929T1010"_pts).delay(1_h)
        });
    transit_realtime::TripUpdate trip_update_vj3 = make_cancellation_message("vj:3", "20150928");
    navitia::handle_realtime("trip_update_vj2", timestamp, trip_update_vj2, *b.data, true);
    navitia::handle_realtime("trip_update_vj3", timestamp, trip_update_vj3, *b.data, true);

    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::posix_time::from_iso_string("20150928T0830"), null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *b.data,
                                         1, 10, 0, "", {});
    const auto resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 1);
    BOOST_REQUIRE_EQUAL(resp.traffic_reports(0).vehicle_journeys_size(), 1);
    BOOST_CHECK_EQUAL(resp.traffic_reports(0).vehicle_journeys(0).uri(), "vj:3");
}

BOOST_AUTO_TEST_CASE(traffic_reports_vehicle_journeys_no_base) {
    ed::builder b("20150928");
    b.vj_with_network("nt", "A", "000110", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    b.make();

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message(
        "vj:1",
        "20150928",
        {
            RTStopTime("stop1", "20150928T0910"_pts).delay(69_min),
            RTStopTime("stop2", "20150929T1010"_pts).delay(69_min)
        });
    navitia::handle_realtime("trip_update", timestamp, trip_update, *b.data, true);
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::posix_time::from_iso_string("20150928T0830"), null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *b.data,
                                         1, 10, 0, "", {});
    const auto resp = pb_creator.get_response();
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
                    RTStopTime("stop1", "20150928T0810"_pts).delay(9_min),
                    RTStopTime("stop_unknown_toto", "20150928T0910"_pts).delay(9_min), // <--- bad
                    RTStopTime("stop3", "20150928T1001"_pts).delay(9_min)
            });

    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime(feed_id, timestamp, bad_trip_update, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    b.make();
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

/* Delay messages are applied in order
 * In this test, we are applying three
 * When one of delay message is removed/updated, remaining delay message should still be
 * applied in the original order
 * */
BOOST_AUTO_TEST_CASE(ordered_delay_message_test) {

    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:01"_t)("stop2", "09:01"_t)("stop3", "10:01"_t);

    auto trip_update_1 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0801"_pts),
                    RTStopTime("stop2", "20150928T0910"_pts).delay(9_min),
                    RTStopTime("stop3", "20150928T1001"_pts)
            });
    auto trip_update_2 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0810"_pts),
                    RTStopTime("stop2", "20150928T0920"_pts).delay(19_min),
                    RTStopTime("stop3", "20150928T1001"_pts)
            });
    auto trip_update_3 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("stop1", "20150928T0810"_pts),
                    RTStopTime("stop2", "20150928T0925"_pts).delay(26_min),
                    RTStopTime("stop3", "20150928T1001"_pts)
            });
    b.data->build_uri();


    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    auto vj = pt_data->vehicle_journeys.front();
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    navitia::handle_realtime("feed_42", "20150101T1337"_dt, trip_update_1, *b.data, true);
    navitia::handle_realtime("feed_43", "20150101T1338"_dt, trip_update_2, *b.data, true);
    navitia::handle_realtime("feed_44", "20150101T1339"_dt, trip_update_3, *b.data, true);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);

    b.make();
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
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0925"_dt);
    }
    // Now we remove the first disruption
    navitia::delete_disruption("feed_42", *pt_data, *b.data->meta);

    b.make();
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
        BOOST_CHECK_EQUAL(res[0].items[0].arrival, "20150928T0925"_dt);
    }
}


BOOST_AUTO_TEST_CASE(delays_with_boarding_alighting_times) {
    ed::builder b("20170101");

    b.vj("line:A", "1111111", "", true, "vj:1")
            ("stop_point:10", "08:10"_t, "08:11"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)
            ("stop_point:20", "08:20"_t, "08:21"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)
            ("stop_point:30", "08:30"_t, "08:31"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)
            ("stop_point:40", "08:40"_t, "08:41"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

    b.make();

    auto trip_update_1 = ntest::make_delay_message("vj:1",
            "20170102",
            {
                    RTStopTime("stop_point:10", "20170102T081000"_pts, "20170102T081100"_pts),
                    RTStopTime("stop_point:20", "20170102T082000"_pts, "20170102T082100"_pts),
                    RTStopTime("stop_point:30", "20170102T084000"_pts, "20170102T084100"_pts).delay(10_min),
                    RTStopTime("stop_point:40", "20170102T085000"_pts, "20170102T085100"_pts).delay(10_min)
            });
    navitia::handle_realtime("feed", "20170101T1337"_dt, trip_update_1, *b.data, true);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vj:1");
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
    vj = b.get<nt::VehicleJourney>("vj:1:modified:0:feed");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "0000010");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "0000000");
    // The realtime vj should have all 4 stop_times and kept the boarding_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().departure_time, "08:11"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().boarding_time, "08:06"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "stop_point:20");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).departure_time, "08:21"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).boarding_time, "08:21"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).stop_point->uri, "stop_point:30");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).departure_time, "08:41"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).boarding_time, "08:41"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().arrival_time, "08:50"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().alighting_time, "08:55"_t);
}

BOOST_AUTO_TEST_CASE(delays_on_lollipop_with_boarding_alighting_times) {
    ed::builder b("20170101");

    b.vj("line:A", "1111111", "", true, "vj:1")
            ("stop_point:10", "08:10"_t, "08:11"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)
            ("stop_point:20", "08:20"_t, "08:21"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)
            ("stop_point:10", "08:30"_t, "08:31"_t, std::numeric_limits<uint16_t>::max(), true, true, 300, 0);

    b.make();

    auto trip_update_1 = ntest::make_delay_message("vj:1",
            "20170102",
            {
                    RTStopTime("stop_point:10", "20170102T081000"_pts, "20170102T081100"_pts),
                    RTStopTime("stop_point:20", "20170102T082000"_pts, "20170102T082100"_pts),
                    RTStopTime("stop_point:10", "20170102T084000"_pts, "20170102T084100"_pts).delay(10_min),
            });
    navitia::handle_realtime("feed", "20170101T1337"_dt, trip_update_1, *b.data, true);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vj:1");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "1111101");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "1111111");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);

    // Check the realtime vj
    vj = b.get<nt::VehicleJourney>("vj:1:modified:0:feed");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "0000010");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "0000000");

    // The realtime vj should have all 3 stop_times 
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(0).stop_point->uri, "stop_point:10");
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(0).departure_time, "08:11"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(0).boarding_time, "08:06"_t); // Boarding time is 5 min (300s) before departure 
    
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "stop_point:20");
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).departure_time, "08:21"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).boarding_time, "08:21"_t);

    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).stop_point->uri, "stop_point:10");
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).arrival_time, "08:40"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).alighting_time, "08:45"_t); // Alighting time is 5 min (300s) after arrival
}

BOOST_AUTO_TEST_CASE(simple_skipped_stop) {
    ed::builder b("20170101");

    b.vj("l1").uri("vj:1")
            ("A", "08:10"_t)
            ("B", "08:20"_t)
            ("C", "08:30"_t);
    b.make();

    auto trip_update_1 = ntest::make_delay_message("vj:1",
            "20170101",
            {
                    RTStopTime("A", "20170101T081000"_pts),
                    RTStopTime("B", "20170101T082000"_pts).skipped(),
                    RTStopTime("C", "20170101T083000"_pts),
            });
    navitia::handle_realtime("feed", "20170101T0337"_dt, trip_update_1, *b.data, true);

    navitia::routing::RAPTOR raptor(*(b.data));

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vj:1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);

    // Check the realtime vj
    vj = b.get<nt::VehicleJourney>("vj:1:modified:0:feed");
    // The realtime vj should have all 3 stop_times but lose the ability to pickup/dropoff on B
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().stop_point->uri, "A");
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().departure_time, "08:10"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().boarding_time, "08:10"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "B");
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).departure_time, "08:20"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).boarding_time, "08:20"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).pick_up_allowed(), false); // disabled
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).drop_off_allowed(), false);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).stop_point->uri, "C");
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).arrival_time, "08:30"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).alighting_time, "08:30"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).drop_off_allowed(), true);

    auto get_journeys = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
        return raptor.compute(b.get<nt::StopArea>(from), b.get<nt::StopArea>(to),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    BOOST_CHECK(! get_journeys(nt::RTLevel::Base, "A", "B").empty());
    // impossible to do a journey between A and B
    BOOST_CHECK(get_journeys(nt::RTLevel::RealTime, "A", "B").empty());

    BOOST_CHECK(! get_journeys(nt::RTLevel::Base, "B", "C").empty());
    // impossible to do a journey between B and C
    BOOST_CHECK(get_journeys(nt::RTLevel::RealTime, "B", "C").empty());
}

/**
 * A ------------ B ------------ C ------------ D
 *
 * We first do not stop at B and C, then we send another disruption to stop at C but with a delay
 */
BOOST_AUTO_TEST_CASE(skipped_stop_then_delay) {
    ed::builder b("20170101");

    b.vj("l1").uri("vj:1")
            ("A", "08:10"_t)
            ("B", "08:20"_t)
            ("C", "08:30"_t)
            ("D", "08:40"_t);
    b.make();

    auto trip_update_1 = ntest::make_delay_message("vj:1",
            "20170101",
            {
                    RTStopTime("A", "20170101T081000"_pts),
                    RTStopTime("B", "20170101T082000"_pts).departure_skipped(),
                    RTStopTime("C", "20170101T083000"_pts).skipped(),
                    RTStopTime("D", "20170101T084000"_pts),
            });
    navitia::handle_realtime("feed", "20170101T0337"_dt, trip_update_1, *b.data, true);

    auto trip_update_2 = ntest::make_delay_message("vj:1",
            "20170101",
            {
                    RTStopTime("A", "20170101T081000"_pts),
                    RTStopTime("B", "20170101T082000"_pts).departure_skipped(),
                    RTStopTime("C", "20170101T083500"_pts).delay(5_min),
                    RTStopTime("D", "20170101T084000"_pts),
            });
    navitia::handle_realtime("feed", "20170101T0337"_dt, trip_update_2, *b.data, true);
    b.data->build_raptor();
    navitia::routing::RAPTOR raptor(*(b.data));

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vj:1");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "1111110");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "1111111");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);

    // Check the realtime vj
    vj = b.get<nt::VehicleJourney>("vj:1:modified:0:feed");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "0000001");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "0000000");
    // The realtime vj should have all 3 stop_times but lose the ability to pickup/dropoff on B
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().stop_point->uri, "A");
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().departure_time, "08:10"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().boarding_time, "08:10"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front().drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "B");
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).departure_time, "08:20"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).boarding_time, "08:20"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).pick_up_allowed(), false); // disabled
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(1).drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).stop_point->uri, "C");
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).arrival_time, "08:35"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).alighting_time, "08:35"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(2).drop_off_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(3).stop_point->uri, "D");
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(3).arrival_time, "08:40"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(3).alighting_time, "08:40"_t);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(3).pick_up_allowed(), true);
    BOOST_CHECK_EQUAL(vj->stop_time_list.at(3).drop_off_allowed(), true);

    auto get_journeys = [&](nt::RTLevel level, const std::string& from, const std::string& to) {
        return raptor.compute(b.get<nt::StopArea>(from), b.get<nt::StopArea>(to),
                              "08:00"_t, 0, navitia::DateTimeUtils::inf, level, 2_min, true);
    };

    BOOST_CHECK(! get_journeys(nt::RTLevel::Base, "A", "B").empty());
    // possible to do a journey between A and B
    BOOST_CHECK(! get_journeys(nt::RTLevel::RealTime, "A", "B").empty());

    BOOST_CHECK(! get_journeys(nt::RTLevel::Base, "B", "C").empty());
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
    ed::builder b("20150928");
    b.vj("1").uri("vj:1")
        ("A", "08:00"_t)
        ("B", "09:00"_t)
        ("C", "10:00"_t)
        ("D", "11:00"_t)
        ("E", "12:00"_t);

    transit_realtime::TripUpdate trip_update = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("A", "20150928T0805"_pts).delay(5_min),
                    RTStopTime("B", "20150928T0900"_pts).delay(0_min),
                    RTStopTime("C", "20150928T1000"_pts).delay(0_min),
                    RTStopTime("D", "20150928T1105"_pts).delay(5_min),
                    RTStopTime("E", "20150928T1200"_pts).delay(0_min)
            });
    b.data->build_uri();

    navitia::handle_realtime("bob", timestamp, trip_update, *b.data, true);

    b.make();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* from, const char* to, nt::RTLevel level) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20150928T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination,
                      {ntest::to_posix_timestamp("20150928T073000")},
                      true, navitia::type::AccessibiliteParams(), {}, {},
                      sn_worker, level, 2_min);
        return  pb_creator.get_response();
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
    ed::builder b("20150928");
    b.vj("1").uri("vj:1")
        ("A", "08:00"_t)
        ("B", "09:00"_t);

    transit_realtime::TripUpdate trip_update1 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("A", "20150928T0801"_pts).delay(1_min),
                    RTStopTime("B", "20150928T0900"_pts).delay(0_min)
            });
    b.data->build_uri();

    navitia::handle_realtime("bob1", timestamp, trip_update1, *b.data, true);

    transit_realtime::TripUpdate trip_update2 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("A", "20150928T0805"_pts).delay(5_min),
                    RTStopTime("B", "20150928T0900"_pts).delay(0_min)
            });
    b.data->build_uri();

    navitia::handle_realtime("bob2", timestamp, trip_update2, *b.data, true);

    transit_realtime::TripUpdate trip_update3 = ntest::make_delay_message("vj:1",
            "20150928",
            {
                    RTStopTime("A", "20150928T0802"_pts).delay(2_min),
                    RTStopTime("B", "20150928T0900"_pts).delay(0_min)
            });
    b.data->build_uri();

    navitia::handle_realtime("bob3", timestamp, trip_update2, *b.data, true);

    b.make();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* from, const char* to, nt::RTLevel level) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20150928T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination,
                      {ntest::to_posix_timestamp("20150928T073000")},
                      true, navitia::type::AccessibiliteParams(), {}, {},
                      sn_worker, level, 2_min);
        return  pb_creator.get_response();
    };

    auto res = compute("A", "B", nt::RTLevel::RealTime);
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 3);
}

BOOST_AUTO_TEST_CASE(teleportation_train_2_delays_check_disruptions) {
    ed::builder b("20171101");
    b.vj("1").uri("vj:1")
        ("A", "08:00"_t)
        ("B", "08:00"_t)
        ("C", "09:00"_t);

    transit_realtime::TripUpdate trip_update1 = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("A", "20171101T0801"_pts).delay(1_min),
                    RTStopTime("B", "20171101T0801"_pts).delay(1_min)
            });
    b.data->build_uri();

    navitia::handle_realtime("late-01", timestamp, trip_update1, *b.data, true);

    transit_realtime::TripUpdate trip_update2 = ntest::make_delay_message("vj:1",
            "20171102",
            {
                    RTStopTime("A", "20171102T0802"_pts).delay(2_min),
                    RTStopTime("B", "20171102T0802"_pts).delay(2_min)
            });
    b.data->build_uri();

    navitia::handle_realtime("late-02", timestamp, trip_update2, *b.data, true);

    b.make();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id("A");
        navitia::type::Type_e destination_type = b.data->get_type_of_id("B");
        navitia::type::EntryPoint origin(origin_type, "A");
        navitia::type::EntryPoint destination(destination_type, "B");

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination,
                      {ntest::to_posix_timestamp(datetime)},
                      true, navitia::type::AccessibiliteParams(), {}, {},
                      sn_worker, nt::RTLevel::RealTime, 2_min);
        return  pb_creator.get_response();
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

    ed::builder b("20171101");

    b.sa("A", 0, 0, true, true);
    b.sa("B", 0, 0, true, true);
    b.sa("B_bis", 0, 0, true, true);
    b.sa("C", 0, 0, true, true);
    b.sa("D", 0, 0, true, true);

    b.vj("1").uri("vj:1")
                   ("stop_point:A", "08:00"_t)
                   ("stop_point:B", "08:30"_t)
                   ("stop_point:C", "09:00"_t);

    b.make();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination,
                {ntest::to_posix_timestamp(datetime)},
                true, navitia::type::AccessibiliteParams(), {}, {},
                sn_worker, nt::RTLevel::RealTime, 2_min);
        return  pb_creator.get_response();
    };

    auto res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).arrival_date_time(), "20171101T0900"_pts);

    transit_realtime::TripUpdate just_new_stop = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("stop_point:A", "20171101T0800"_pts),
                    RTStopTime("stop_point:B", "20171101T0830"_pts),
                    RTStopTime("stop_point:B_bis", "20171101T0845"_pts).added(),
                    RTStopTime("stop_point:C", "20171101T0900"_pts),
            });

    navitia::handle_realtime("add_new_stop_time_in_the_trip", timestamp, just_new_stop, *b.data, true);

    b.make();

    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_MODIFIED_SERVICE);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).arrival_date_time(), "20171101T0900"_pts);

    transit_realtime::TripUpdate delay_and_new_stop = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("stop_point:A", "20171101T0805"_pts).delay(5_min),
                    RTStopTime("stop_point:B", "20171101T0830"_pts),
                    RTStopTime("stop_point:B_bis", "20171101T0845"_pts).added(),
                    RTStopTime("stop_point:C", "20171101T0905"_pts).delay(5_min),
            });

    //Use trip_update effect (MODIFIED_SERVICE)
    delay_and_new_stop.SetExtension(kirin::effect, transit_realtime::Alert_Effect::Alert_Effect_MODIFIED_SERVICE);
    navitia::handle_realtime("add_new_stop_time_in_the_trip", timestamp, delay_and_new_stop, *b.data, true);

    b.make();

    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_MODIFIED_SERVICE);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().name(), "trip modified");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).arrival_date_time(), "20171101T0905"_pts);

    transit_realtime::TripUpdate new_stop_at_the_end = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("stop_point:A", "20171101T0800"_pts),
                    RTStopTime("stop_point:B", "20171101T0830"_pts),
                    RTStopTime("stop_point:C", "20171101T0905"_pts),
                    RTStopTime("stop_point:D", "20171101T1000"_pts).added()
            });

    //Use trip_update effect = SIGNIFICANT_DELAYS just for the test
    new_stop_at_the_end.SetExtension(kirin::effect, transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS);
    navitia::handle_realtime("add_new_stop_time_in_the_trip", timestamp, new_stop_at_the_end, *b.data, true);

    b.make();

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

    //Init data for a vj = vj:1 -> A -> B -> C
    ed::builder b("20171101");

    b.sa("A", 0, 0, true, true);
    b.sa("B", 0, 0, true, true);
    b.sa("B_bis", 0, 0, true, true);
    b.sa("C", 0, 0, true, true);
    b.sa("D", 0, 0, true, true);

    b.vj("1").uri("vj:1")
                   ("stop_point:A", "08:00"_t)
                   ("stop_point:B", "08:30"_t)
                   ("stop_point:C", "09:00"_t);

    b.make();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination,
                {ntest::to_posix_timestamp(datetime)},
                true, navitia::type::AccessibiliteParams(), {}, {},
                sn_worker, nt::RTLevel::RealTime, 2_min);
        return  pb_creator.get_response();
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
    transit_realtime::TripUpdate just_new_stop = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("stop_point:A", "20171101T0800"_pts),
                    RTStopTime("stop_point:B", "20171101T0830"_pts),
                    RTStopTime("stop_point:B_bis", "20171101T0845"_pts).added(),
                    RTStopTime("stop_point:C", "20171101T0900"_pts),
            });

    navitia::handle_realtime("feed-1", timestamp, just_new_stop, *b.data, true);

    b.make();

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vj:1");
    BOOST_CHECK_END_VP(vj->rt_validity_pattern(), "1111110");
    BOOST_CHECK_END_VP(vj->base_validity_pattern(), "1111111");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);

    // Check the realtime vj with a newly added stop_time
    vj = b.get<nt::VehicleJourney>("vj:1:modified:0:feed-1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].pick_up_allowed(), true);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].drop_off_allowed(), true);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);

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
    transit_realtime::TripUpdate delete_new_stop = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("stop_point:A", "20171101T0800"_pts),
                    RTStopTime("stop_point:B", "20171101T0830"_pts),
                    RTStopTime("stop_point:B_bis", "20171101T0845"_pts).skipped(),
                    RTStopTime("stop_point:C", "20171101T0900"_pts),
            });

    navitia::handle_realtime("feed-2", timestamp, delete_new_stop, *b.data, true);

    b.make();

    // Check the realtime vj after the recently added stop_time is deleted
    vj = b.get<nt::VehicleJourney>("vj:1:modified:1:feed-2");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].pick_up_allowed(), false);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].drop_off_allowed(), false);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);

    // The new stop_time added should be in stop_date_times
    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts_size(), 2);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_DETOUR);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);

    // We should not have any journey in public_transport from B_bis to C
    res = compute("20171101T073000", "stop_point:B_bis", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);

    // Delete the recently deleted stop_time in vj = vj:1 B_bis
    // We should be able to add impact on the stop_time even if it is already deleted.
    transit_realtime::TripUpdate redelete_stop = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("stop_point:A", "20171101T0800"_pts),
                    RTStopTime("stop_point:B", "20171101T0830"_pts),
                    RTStopTime("stop_point:B_bis", "20171101T0845"_pts).skipped(),
                    RTStopTime("stop_point:C", "20171101T0900"_pts),
            });

    navitia::handle_realtime("feed-3", timestamp, redelete_stop, *b.data, true);
    b.make();

    // Check the realtime vj after the recently added stop_time is deleted twice
    vj = b.get<nt::VehicleJourney>("vj:1:modified:1:feed-3");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].pick_up_allowed(), false);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].drop_off_allowed(), false);

    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.impacts_size(), 3);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_DETOUR);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).departure_status(), pbnavitia::StopTimeUpdateStatus::DELETED);

    // We should not have any journey in public_transport from B_bis to C
    res = compute("20171101T073000", "stop_point:B_bis", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::NO_SOLUTION);
    BOOST_CHECK_EQUAL(res.journeys_size(), 0);
}

BOOST_AUTO_TEST_CASE(add_new_and_delete_existingstop_time_in_the_trip_for_detour) {

    //Init data for a vj = vj:1 -> A -> B -> C
    ed::builder b("20171101");

    b.sa("A", 0, 0, true, true);
    b.sa("B", 0, 0, true, true);
    b.sa("B_bis", 0, 0, true, true);
    b.sa("C", 0, 0, true, true);
    b.sa("D", 0, 0, true, true);

    b.vj("1").uri("vj:1")
            ("stop_point:A", "08:00"_t)
            ("stop_point:B", "08:30"_t)
            ("stop_point:C", "09:00"_t);

    b.make();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination,
                {ntest::to_posix_timestamp(datetime)},
                true, navitia::type::AccessibiliteParams(), {}, {},
                sn_worker, nt::RTLevel::RealTime, 2_min);
        return  pb_creator.get_response();
    };

    auto res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 3);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).arrival_date_time(), "20171101T0900"_pts);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);

    // Delete the stop_time at B (delete_for_detour) followed by a new stop_time
    // added at B_bis (added_for_detour) and C unchanged.
    transit_realtime::TripUpdate just_new_stop = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("stop_point:A", "20171101T0800"_pts),
                    RTStopTime("stop_point:B", "20171101T0830"_pts).deleted_for_detour(),
                    RTStopTime("stop_point:B_bis", "20171101T0845"_pts).added_for_detour(),
                    RTStopTime("stop_point:C", "20171101T0900"_pts),
            });

    navitia::handle_realtime("add_new_and_delete_existingstop_time_in_the_trip_for_detour", timestamp, just_new_stop, *b.data, true);

    b.make();

    // The new stop_time added should be in stop_date_times
    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_DETOUR);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).stop_point().uri(), "stop_point:B_bis");
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(3).arrival_date_time(), "20171101T0900"_pts);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).departure_status(), pbnavitia::StopTimeUpdateStatus::UNCHANGED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).is_detour(), true);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).departure_status(), pbnavitia::StopTimeUpdateStatus::DELETED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).is_detour(), true);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).departure_status(), pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).departure_status(), pbnavitia::StopTimeUpdateStatus::UNCHANGED);

    // We should have A journey in public_transport from B_bis to C
    res = compute("20171101T073000", "stop_point:B_bis", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).type(), pbnavitia::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 2);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_DETOUR);
}

BOOST_AUTO_TEST_CASE(add_new_with_earlier_arrival_and_delete_existingstop_time_in_the_trip_for_detour) {

    //Init data for a vj = vj:1 -> A -> B -> C
    ed::builder b("20171101");

    b.sa("A", 0, 0, true, true);
    b.sa("B", 0, 0, true, true);
    b.sa("B_bis", 0, 0, true, true);
    b.sa("C", 0, 0, true, true);
    b.sa("D", 0, 0, true, true);

    b.vj("1").uri("vj:1")
            ("stop_point:A", "08:00"_t)
            ("stop_point:B", "08:30"_t)
            ("stop_point:C", "09:00"_t);

    b.make();

    navitia::routing::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto compute = [&](const char* datetime, const std::string& from, const std::string& to) {
        navitia::type::Type_e origin_type = b.data->get_type_of_id(from);
        navitia::type::Type_e destination_type = b.data->get_type_of_id(to);
        navitia::type::EntryPoint origin(origin_type, from);
        navitia::type::EntryPoint destination(destination_type, to);

        navitia::PbCreator pb_creator(b.data.get(), "20171101T073000"_dt, null_time_period);
        make_response(pb_creator, raptor, origin, destination,
                {ntest::to_posix_timestamp(datetime)},
                true, navitia::type::AccessibiliteParams(), {}, {},
                sn_worker, nt::RTLevel::RealTime, 2_min);
        return  pb_creator.get_response();
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
    transit_realtime::TripUpdate just_new_stop = ntest::make_delay_message("vj:1",
            "20171101",
            {
                    RTStopTime("stop_point:A", "20171101T0800"_pts),
                    RTStopTime("stop_point:B", "20171101T0830"_pts).deleted_for_detour(),
                    RTStopTime("stop_point:B_bis", "20171101T0825"_pts).added_for_detour(),
                    RTStopTime("stop_point:C", "20171101T0900"_pts),
            });

    navitia::handle_realtime("add_new_and_delete_existingstop_time_in_the_trip_for_detour", timestamp, just_new_stop, *b.data, true);

    b.make();

    // Same verifications as in the test above
    res = compute("20171101T073000", "stop_point:A", "stop_point:C");
    BOOST_CHECK_EQUAL(res.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_CHECK_EQUAL(res.journeys_size(), 1);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).severity().effect(), pbnavitia::Severity_Effect_DETOUR);
    BOOST_CHECK_EQUAL(res.journeys(0).sections(0).stop_date_times(2).stop_point().uri(), "stop_point:B_bis");
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects_size(), 1);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops_size(), 4);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(0).departure_status(), pbnavitia::StopTimeUpdateStatus::UNCHANGED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).is_detour(), true);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(1).departure_status(), pbnavitia::StopTimeUpdateStatus::DELETED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).is_detour(), true);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(2).departure_status(), pbnavitia::StopTimeUpdateStatus::ADDED);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).is_detour(), false);
    BOOST_CHECK_EQUAL(res.impacts(0).impacted_objects(0).impacted_stops(3).departure_status(), pbnavitia::StopTimeUpdateStatus::UNCHANGED);
}

BOOST_AUTO_TEST_CASE(should_get_base_stoptime_with_realtime_added_stop_time) {
    ed::builder b("20171101");

    b.sa("A");
    b.sa("B");
    b.sa("C");
    b.sa("D");

    b.vj("L1").uri("vj:1")
        ("stop_point:A", "08:10"_t)
        ("stop_point:C", "08:30"_t)
        ("stop_point:D", "08:40"_t);
    b.make();
 
    tr::TripUpdate add_stop = ntest::make_delay_message("vj:1", "20171101", {
        RTStopTime("stop_point:A", "20171101T0810"_pts),
        RTStopTime("stop_point:B", "20171101T0820"_pts).added(),
        RTStopTime("stop_point:C", "20171101T0830"_pts),
        RTStopTime("stop_point:D", "20171101T0840"_pts),
    });

    navitia::handle_realtime("add_new_stop", timestamp, add_stop, *b.data, true);
    b.make();

    auto res = compute_iti(b, "20171101T080000", "A", "D", nt::RTLevel::RealTime);
    
    BOOST_REQUIRE_EQUAL(res.impacts_size(), 1);
    BOOST_REQUIRE_EQUAL(res.impacts(0).impacted_objects_size(), 1);

    const auto & stop_time_updates = res.impacts(0).impacted_objects(0).impacted_stops();
    BOOST_REQUIRE_EQUAL(stop_time_updates.size(), 4);

    const auto & stop_time_A = stop_time_updates.Get(0);
    BOOST_CHECK_EQUAL(stop_time_A.stop_point().uri(), "stop_point:A");
    BOOST_CHECK_EQUAL(stop_time_A.base_stop_time().arrival_time(), "08:10"_t);
    BOOST_CHECK_EQUAL(stop_time_A.base_stop_time().departure_time() ,"08:10"_t);
    BOOST_CHECK_EQUAL(stop_time_A.amended_stop_time().arrival_time(), "08:10"_t);
    BOOST_CHECK_EQUAL(stop_time_A.amended_stop_time().departure_time(), "08:10"_t);

    const auto & stop_time_B = stop_time_updates.Get(1);
    BOOST_CHECK_EQUAL(stop_time_B.stop_point().uri(), "stop_point:B");
    BOOST_CHECK_MESSAGE(stop_time_B.base_stop_time().arrival_time() == "00:00"_t, "Base stop time doesn't exist for added stop time");
    BOOST_CHECK_MESSAGE(stop_time_B.base_stop_time().departure_time() == "00:00"_t, "Base stop time doesn't exist for added stop time");
    BOOST_CHECK_EQUAL(stop_time_B.amended_stop_time().arrival_time(), "08:20"_t);
    BOOST_CHECK_EQUAL(stop_time_B.amended_stop_time().departure_time(), "08:20"_t);

    const auto & stop_time_C = stop_time_updates.Get(2);
    BOOST_CHECK_EQUAL(stop_time_C.stop_point().uri(), "stop_point:C");
    BOOST_CHECK_EQUAL(stop_time_C.base_stop_time().arrival_time(), "08:30"_t);
    BOOST_CHECK_EQUAL(stop_time_C.base_stop_time().departure_time() ,"08:30"_t);
    BOOST_CHECK_EQUAL(stop_time_C.amended_stop_time().arrival_time(), "08:30"_t);
    BOOST_CHECK_EQUAL(stop_time_C.amended_stop_time().departure_time(), "08:30"_t);

    const auto & stop_time_D = stop_time_updates.Get(3);
    BOOST_CHECK_EQUAL(stop_time_D.stop_point().uri(), "stop_point:D");
    BOOST_CHECK_EQUAL(stop_time_D.base_stop_time().arrival_time(), "08:40"_t);
    BOOST_CHECK_EQUAL(stop_time_D.base_stop_time().departure_time() ,"08:40"_t);
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
    ed::builder b("20171101");

    const auto* vj = b.vj("L1").uri("vj:1")
        ("A", "08:10"_t)
        ("B", "08:20"_t)
        ("C", "08:30"_t)
        ("B", "08:40"_t)
        ("A", "08:50"_t).make();
    b.make();

    navitia::handle_realtime("add_new_stop", timestamp, 
                             ntest::make_delay_message("vj:1", "20171101", {
                                RTStopTime("A", "20171101T0810"_pts).delay(1_min),
                                RTStopTime("B", "20171101T0820"_pts).delay(1_min),
                                RTStopTime("C", "20171101T0830"_pts).delay(1_min),
                                RTStopTime("B", "20171101T0840"_pts).delay(1_min),
                                RTStopTime("A", "20171101T0850"_pts).delay(1_min),
                             }), 
                             *b.data, true);
    b.make();

    const auto & realtime_vj = vj->meta_vj->get_rt_vj()[0];

    const auto & stoptime_rltm_A = realtime_vj->stop_time_list[0];
    const auto & stoptime_rltm_B = realtime_vj->stop_time_list[1];
    const auto & stoptime_rltm_C = realtime_vj->stop_time_list[2];
    const auto & stoptime_rltm_B_bis = realtime_vj->stop_time_list[3];
    const auto & stoptime_rltm_A_bis = realtime_vj->stop_time_list[4];

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
    ed::builder b("20171101");

    const auto* vj = b.vj("L1").uri("vj:1")
        ("A", "08:10"_t)
        ("B", "08:20"_t)
        ("C", "08:30"_t)
        ("B", "08:40"_t)
        ("D", "08:50"_t).make();
    b.make();

    navitia::handle_realtime("add_new_stop", timestamp, 
                             ntest::make_delay_message("vj:1", "20171101", {
                                RTStopTime("A", "20171101T0810"_pts).delay(1_min),
                                RTStopTime("B", "20171101T0820"_pts).delay(1_min),
                                RTStopTime("C", "20171101T0830"_pts).delay(1_min),
                                RTStopTime("B", "20171101T0840"_pts).delay(1_min),
                                RTStopTime("D", "20171101T0850"_pts).delay(1_min),
                             }), 
                             *b.data, true);
    b.make();

    const auto & realtime_vj = vj->meta_vj->get_rt_vj()[0];

    const auto & stoptime_rltm_A = realtime_vj->stop_time_list[0];
    const auto & stoptime_rltm_B = realtime_vj->stop_time_list[1];
    const auto & stoptime_rltm_C = realtime_vj->stop_time_list[2];
    const auto & stoptime_rltm_B_bis = realtime_vj->stop_time_list[3];
    const auto & stoptime_rltm_D = realtime_vj->stop_time_list[4];

    BOOST_CHECK_EQUAL(stoptime_rltm_A.get_base_stop_time(), &vj->stop_time_list[0]);
    BOOST_CHECK_EQUAL(stoptime_rltm_B.get_base_stop_time(), &vj->stop_time_list[1]);
    BOOST_CHECK_EQUAL(stoptime_rltm_C.get_base_stop_time(), &vj->stop_time_list[2]);
    BOOST_CHECK_EQUAL(stoptime_rltm_B_bis.get_base_stop_time(), &vj->stop_time_list[3]);
    BOOST_CHECK_EQUAL(stoptime_rltm_D.get_base_stop_time(), &vj->stop_time_list[4]);
}
