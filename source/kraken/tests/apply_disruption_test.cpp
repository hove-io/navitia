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
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "type/data.h"
#include "ed/build_helper.h"
#include "kraken/apply_disruption.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace nt = navitia::type;
namespace pt = boost::posix_time;
using btp = boost::posix_time::time_period;
namespace ba = boost::algorithm;

struct SimpleDataset {
    SimpleDataset(): b("20150928") {
        b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj("A", "000001", "", true, "vj:A-2")("stop1", "08:00"_t)("stop2", "09:00"_t);
    }

    ed::builder b;
    void apply(const nt::disruption::Disruption& dis) {
        return navitia::apply_disruption(dis, *b.data->pt_data, *b.data->meta);
    }
};

BOOST_FIXTURE_TEST_CASE(simple_train_cancellation, SimpleDataset) {
    const auto& disrup = b.impact(nt::RTLevel::RealTime)
                     .severity(nt::disruption::Effect::NO_SERVICE)
                     .on(nt::Type_e::MetaVehicleJourney, "vj:A-1")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    const auto& vj = pt_data->vehicle_journeys_map.at("vj:A-1");
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    apply(disrup);

    // we should not have created any objects save for one validity_pattern
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    //the rt vp must be empty
    BOOST_CHECK_EQUAL(vj->rt_validity_pattern()->days, navitia::type::ValidityPattern::year_bitset());

    // we add a second time the realtime message, it should not change anything
    apply(disrup);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // we delete the message
    navitia::delete_disruption(std::string(disrup.uri), *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    //no cleanup for the moment, but it would be nice
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // but the vp should be equals again
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());
}

/*
 * small helper to check VJs
 */
void check_vj(const nt::VehicleJourney* vj, const std::vector<nt::StopTime> sts) {
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), sts.size());
    for (size_t idx = 0; idx < sts.size(); ++idx) {
        BOOST_CHECK_EQUAL(vj->stop_time_list[idx].departure_time, sts[idx].departure_time);
        BOOST_CHECK_EQUAL(vj->stop_time_list[idx].arrival_time, sts[idx].arrival_time);
        BOOST_CHECK_EQUAL(vj->stop_time_list[idx].stop_point, sts[idx].stop_point);
    }
}

/*
 *
 * test to close different stop point on different hours
 *
 *
 *                            closed         closed
 *                            [10-12]        [11-14]
 *                S1 ---------- S2 ------------S3
 *
 * vj1:           8h            9h             10h
 * no vj1_adated
 *
 * vj2:           10h           10h30          11h
 * vj2_adated:    10h           X              X         (Note for XL: the best would be to clean those vj, but as long as it does not code dump, it's not important not to clean them)
 *                                                            --> ie either do a routing test with vj2/vj3 or clean them
 * vj3:           10h                          11h
 * vj3_adapted:   10h                          X
 *
 * vj4:           8h            10h            10h30
 * vj4_adapted:   8h            X              10h30
 *
 * vj5:           8h            13h            13h30
 * vj5_adapted:   8h            13h            X
 *
 */
BOOST_AUTO_TEST_CASE(multiple_impact_on_stops_different_hours) {
    ed::builder b("20160101");

    b.sa("S1")("S1");
    b.sa("S2")("S2");
    b.sa("S2")("S2");

    const auto* vj1 = b.vj("A").uri("vj1")("S1", "08:00"_t)("S2", "09:00"_t)("S3", "10:00"_t).make();
    const auto* vj2 = b.vj("A").uri("vj2")("S1", "10:00"_t)("S2", "10:30"_t)("S3", "11:00"_t).make();
    const auto* vj3 = b.vj("A").uri("vj3")("S1", "10:00"_t)                 ("S3", "10:00"_t).make();
    const auto* vj4 = b.vj("A").uri("vj4")("S1", "08:00"_t)("S2", "10:00"_t)("S3", "10:30"_t).make();
    const auto* vj5 = b.vj("A").uri("vj5")("S1", "08:00"_t)("S2", "13:00"_t)("S3", "13:30"_t).make();

    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();

    auto* s1 = b.data->pt_data->stop_points_map["S1"];
    auto* s2 = b.data->pt_data->stop_points_map["S2"];
    auto* s3 = b.data->pt_data->stop_points_map["S3"];

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 5);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "S2_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "S2")
                              .application_periods(btp("20160101T100000"_dt, "20160101T1120001"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "S3_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "S3")
                              .application_periods(btp("20160101T110000"_dt, "20160101T1140001"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 10); //XL: or 8 if you clean vj2_adapted and vj3_adapted

    auto get_adapted = [&b](const nt::VehicleJourney* vj) {
        BOOST_REQUIRE_EQUAL(vj->meta_vj->get_adapted_vj().size(), 1); // for our test this is only 1 adapted
        return vj->meta_vj->get_adapted_vj().front().get();
    };

    //no adapted for vj1
    BOOST_REQUIRE_EQUAL(vj1->meta_vj->get_adapted_vj().size(), 0);

    //TODO XL, cf comment at the begining of the test for vj2 and vj3
    const auto* vj2_adapted = get_adapted(vj2);
    BOOST_CHECK(ba::ends_with(vj2->base_validity_pattern()->days.to_string(), "111"));
    BOOST_CHECK(ba::ends_with(vj2->adapted_validity_pattern()->days.to_string(), "110"));
    BOOST_CHECK(ba::ends_with(vj2_adapted->base_validity_pattern()->days.to_string(), "000"));
    BOOST_CHECK(ba::ends_with(vj2_adapted->adapted_validity_pattern()->days.to_string(), "001"));
    check_vj(vj2_adapted, {nt::StopTime{"08:00"_t, "08:00"_t, s1}});

    check_vj(get_adapted(vj3), {{"08:00"_t, "08:00"_t, s1}});

    check_vj(get_adapted(vj4), {{"08:00"_t, "08:00"_t, s1},
                                {"10:30"_t, "10:30"_t, s3}});

    check_vj(get_adapted(vj5), {{"08:00"_t, "08:00"_t, s1},
                                {"13:00"_t, "13:00"_t, s2}});
}
