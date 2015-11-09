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

namespace nt = navitia::type;
namespace pt = boost::posix_time;

static const std::string feed_id = "42";
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

/*
template<typename T>
struct TypeChecker;
*/
int64_t to_int64(const std::string& date) {
    return (pt::from_iso_string(date) - pt::from_time_t(0)).ticks();
}
static transit_realtime::TripUpdate
make_delay_message(const std::string& vj_uri,
        const std::string& date,
        const std::vector<std::pair<std::string, std::string>>& delayed_time_stops) {
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
        // TypeChecker<decltype(pt::duration_from_string(delayed_st.first).total_seconds())>();
        arrival->set_time(to_int64(delayed_st.first));
        departure->set_time(to_int64(delayed_st.second));
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


/*
BOOST_AUTO_TEST_CASE(train_delayed) {
    ed::builder b("20150928");
    b.vj("A", "000001", "", true, "vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t);

    transit_realtime::TripUpdate trip_update = make_delay_message("vj:1",
            "20150928",
            {
                    {"20150928T0810", "20150928T0910"},
                    {"20150928T0910", "20150928T0910"}
            });

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
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we add a second time the realtime message, it should not change anything
    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    // The base VP is different from realtime VP
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

}
*/
