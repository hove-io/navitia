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
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "type/data.h"
#include "ed/build_helper.h"
#include "kraken/apply_disruption.h"
#include "kraken/make_disruption_from_chaos.h"
#include "kraken/realtime.h"
#include "type/kirin.pb.h"
#include "routing/raptor.h"
#include "type/pb_converter.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace ntest = navitia::test;
namespace ba = boost::algorithm;
namespace bg = boost::gregorian;

using btp = boost::posix_time::time_period;

struct SimpleDataset {
    SimpleDataset() : b("20150928") {
        b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj("A", "000001", "", true, "vj:A-2")("stop1", "08:00"_t)("stop2", "09:00"_t);
    }

    ed::builder b;
    void apply(const nt::disruption::Disruption& dis) {
        return navitia::apply_disruption(dis, *b.data->pt_data, *b.data->meta);
    }
};

// Check if new indexes are unique and continuous
static void test_vjs_indexes(const std::vector<navitia::type::VehicleJourney*>& vjs) {
    int prec = -1;
    // Check if new indexes are unique and continuous
    std::for_each(vjs.begin(), vjs.end(), [&prec](const navitia::type::VehicleJourney* vj) {
        BOOST_REQUIRE_EQUAL((static_cast<int>(vj->idx) - prec), 1);
        prec = static_cast<int>(vj->idx);
    });
}

BOOST_FIXTURE_TEST_CASE(simple_train_cancellation, SimpleDataset) {
    const auto& disrup = b.impact(nt::RTLevel::RealTime)
                             .severity(nt::disruption::Effect::NO_SERVICE)
                             .on(nt::Type_e::MetaVehicleJourney, "vj:A-1", *b.data->pt_data)
                             .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                             .get_disruption();

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    const auto& vj = pt_data->vehicle_journeys_map.at("vehicle_journey:vj:A-1");
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    apply(disrup);

    // we should not have created any objects save for one validity_pattern
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

    // the rt vp must be empty
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
    // no cleanup for the moment, but it would be nice
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
    // but the vp should be equals again
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());
}

/*
 * small helper to check VJs
 */
static void check_vj(const nt::VehicleJourney* vj, const std::vector<nt::StopTime>& sts) {
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), sts.size());
    for (size_t idx = 0; idx < sts.size(); ++idx) {
        BOOST_CHECK_EQUAL(vj->stop_time_list[idx].departure_time, sts[idx].departure_time);
        BOOST_CHECK_EQUAL(vj->stop_time_list[idx].arrival_time, sts[idx].arrival_time);
        BOOST_CHECK_EQUAL(vj->stop_time_list[idx].stop_point, sts[idx].stop_point);
    }
}

static void check_vjs_without_disruptions(const std::vector<nt::VehicleJourney*>& vehicle_journeys,
                                          const std::string& original_VP) {
    for (const auto* vj : vehicle_journeys) {
        switch (vj->realtime_level) {
            case nt::RTLevel::Base:
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), original_VP),
                                    vj->adapted_validity_pattern()->days);
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), original_VP),
                                    vj->base_validity_pattern()->days);
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), original_VP),
                                    vj->base_validity_pattern()->days);
                break;
            case nt::RTLevel::Adapted:
            case nt::RTLevel::RealTime:
                BOOST_FAIL("we should not have any adapted or realtime vj");
                break;
        }
    }
}

static std::vector<navitia::routing::Path> compute(navitia::type::Data& data,
                                                   nt::RTLevel level,
                                                   const std::string& from,
                                                   const std::string& to,
                                                   navitia::DateTime datetime,
                                                   int day) {
    data.build_raptor();
    navitia::routing::RAPTOR raptor{data};
    return raptor.compute(data.pt_data->stop_areas_map.at(from), data.pt_data->stop_areas_map.at(to), datetime, day,
                          navitia::DateTimeUtils::inf, level, 2_min, 2_min, true);
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
 * no vj1_adapted
 *
 * vj2:           10h           10h30          11h
 * vj2_adapted:   10h           X              X
 *
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
    nt::VehicleJourney *vj1, *vj2, *vj3, *vj4, *vj5;
    ed::builder b("20160101", [&](ed::builder& b) {
        b.sa("S1")("S1");
        b.sa("S2")("S2");
        b.sa("S3")("S3");

        vj1 = b.vj("A").name("vj1")("S1", "08:00"_t)("S2", "09:00"_t)("S3", "10:00"_t).make();
        vj2 = b.vj("A").name("vj2")("S1", "10:00"_t)("S2", "10:30"_t)("S3", "11:00"_t).make();
        vj3 = b.vj("A").name("vj3")("S1", "10:00"_t)("S3", "11:00"_t).make();
        vj4 = b.vj("A").name("vj4")("S1", "08:00"_t)("S2", "10:00"_t)("S3", "10:30"_t).make();
        vj5 = b.vj("A").name("vj5")("S1", "08:00"_t)("S2", "13:00"_t)("S3", "13:30"_t).make();
    });

    auto* s1 = b.data->pt_data->stop_points_map["S1"];
    auto* s2 = b.data->pt_data->stop_points_map["S2"];
    auto* s3 = b.data->pt_data->stop_points_map["S3"];

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 5);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "S2_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "S2", *b.data->pt_data)
                                  .application_periods(btp("20160101T100000"_dt, "20160101T1120001"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 7);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "S3_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "S3", *b.data->pt_data)
                                  .application_periods(btp("20160101T110000"_dt, "20160101T1400001"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 9);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    auto get_adapted = [](const nt::VehicleJourney* vj, int nb_adapted_vj) {
        BOOST_REQUIRE_EQUAL(vj->meta_vj->get_adapted_vj().size(), nb_adapted_vj);
        return vj->meta_vj->get_adapted_vj().back().get();
    };

    // no adapted for vj1
    BOOST_REQUIRE_EQUAL(vj1->meta_vj->get_adapted_vj().size(), 0);

    const auto* vj2_adapted = get_adapted(vj2, 1);

    BOOST_CHECK(ba::ends_with(vj2->base_validity_pattern()->days.to_string(), "111"));
    BOOST_CHECK(ba::ends_with(vj2->adapted_validity_pattern()->days.to_string(), "110"));
    BOOST_CHECK(ba::ends_with(vj2_adapted->base_validity_pattern()->days.to_string(), "000"));
    BOOST_CHECK(ba::ends_with(vj2_adapted->adapted_validity_pattern()->days.to_string(), "001"));
    check_vj(vj2_adapted, {{"10:00"_t, "10:00"_t, s1}});
    check_vj(get_adapted(vj3, 1), {{"10:00"_t, "10:00"_t, s1}});
    check_vj(get_adapted(vj4, 1), {{"08:00"_t, "08:00"_t, s1}, {"10:30"_t, "10:30"_t, s3}});
    check_vj(get_adapted(vj5, 1), {{"08:00"_t, "08:00"_t, s1}, {"13:00"_t, "13:00"_t, s2}});

    navitia::delete_disruption("S2_closed", *b.data->pt_data, *b.data->meta);
    navitia::delete_disruption("S3_closed", *b.data->pt_data, *b.data->meta);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "111111");
}

BOOST_AUTO_TEST_CASE(add_impact_on_stop_area) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.vj("A", "000111")("stop_area:stop1", "08:10"_t, "08:11"_t)("stop_area:stop2", "08:20"_t, "08:21"_t)(
            "stop_area:stop3", "08:30"_t, "08:31"_t);
        b.vj("A", "000111")("stop_area:stop1", "09:10"_t, "09:11"_t)("stop_area:stop2", "09:20"_t, "09:21"_t)(
            "stop_area:stop3", "09:30"_t, "09:31"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    // we delete the stop_area 1, two vjs are impacted, we create a new journey pattern without this stop_area
    // and two new vj are enable on this journey pattern, of course the theorical vj are disabled

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area:stop1_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopArea, "stop_area:stop1", *b.data->pt_data)
                                  .application_periods(btp("20120614T173200"_dt, "20120618T123200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);
    bool has_adapted_vj = false;
    for (const auto* vj : b.data->pt_data->vehicle_journeys) {
        switch (vj->realtime_level) {
            case nt::RTLevel::Base:
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000001"),
                                    vj->adapted_validity_pattern()->days);
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000111"),
                                    vj->base_validity_pattern()->days);
                break;
            case nt::RTLevel::Adapted:
                has_adapted_vj = true;
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000110"),
                                    vj->adapted_validity_pattern()->days);
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
                                    vj->base_validity_pattern()->days);
                break;
            case nt::RTLevel::RealTime:
                // TODO
                throw navitia::exception("realtime check unhandled case");
        }
    }
    BOOST_REQUIRE(has_adapted_vj);
}

/*           v A1
 *           v
 *           v
 *         |-----------|
 * B1 > > >|sp1> > >sp3|> > > > B2
 *         | v         |      (VJ_B)
 *         | v         |
 *         |sp2        |
 *         |-----------|
 *           v         stop_area
 *           v
 *           v A2
 *          (VJ_A)
 * */
BOOST_AUTO_TEST_CASE(add_impact_on_stop_area_with_several_stop_point) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.sa("stop_area", 0, 0, false, true)("stop_area:stop1")("stop_area:stop2")("stop_area:stop3");

        b.vj("A", "111111")
            .name("VjA")("A1", "08:10"_t, "08:11"_t)("stop_area:stop1", "09:10"_t, "09:11"_t)(
                "stop_area:stop2", "10:20"_t, "10:21"_t)("A2", "11:20"_t, "11:21"_t);

        b.vj("B", "111111")
            .name("VjB")("B1", "08:10"_t, "08:11"_t)("stop_area:stop1", "09:10"_t, "09:11"_t)(
                "stop_area:stop3", "10:20"_t, "10:21"_t)("B2", "11:20"_t, "11:21"_t);
    });
    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    // we delete the stop_area 1, two vj are impacted, we create a new journey pattern without this stop_area
    // and two new vj are enable on this journey pattern, of course the theorical vj are disabled

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopArea, "stop_area", *b.data->pt_data)
                                  .application_periods(btp("20120614T173200"_dt, "20120618T123200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 2);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);

    bool has_adapted_vj = false;
    for (const auto* vj : b.data->pt_data->vehicle_journeys) {
        switch (vj->realtime_level) {
            case nt::RTLevel::Base:
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "100001"),
                                    vj->adapted_validity_pattern()->days);
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
                                    vj->base_validity_pattern()->days);
                break;
            case nt::RTLevel::Adapted:
                has_adapted_vj = true;
                if (vj->uri == "VjA:Adapted:0:stop_area_closed" || vj->uri == "VjB:Adapted:0:stop_area_closed") {
                    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000000"),
                                        vj->adapted_validity_pattern()->days);
                } else {
                    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "011110"),
                                        vj->adapted_validity_pattern()->days);
                }
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
                                    vj->base_validity_pattern()->days);
                break;

            case nt::RTLevel::RealTime:
                // TODO
                throw navitia::exception("realtime check unhandled case");
        }
    }
    BOOST_REQUIRE(has_adapted_vj);

    auto res = compute(*b.data, nt::RTLevel::Adapted, "A1", "A2", "08:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items[0].stop_times.size(), 4);

    res = compute(*b.data, nt::RTLevel::Adapted, "A1", "A2", "08:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items[0].stop_times.size(), 2);
    auto vp = res[0].items[0].stop_times[0]->vehicle_journey->adapted_validity_pattern();
    BOOST_CHECK_MESSAGE(ba::ends_with(vp->days.to_string(), "011110"), vp->days.to_string());
}

/*
 *
 *
 *  Day 14              15              16              17              18              19              20
 *      -----------------------------------------------------------------------------------------------------
 *   VJ              23H----2H       23H----2H       23H----2H       23H----2H       23H----2H       23H----2H
 * Impact                |-----------------------------------------------|
 * */
BOOST_AUTO_TEST_CASE(add_stop_area_impact_on_vj_pass_midnight) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.sa("stop_area", 0, 0, false, true)("stop_area:stop1")("stop_area:stop2")("stop_area:stop3");

        b.vj("A", "0111111")("A1", "23:10"_t, "23:11"_t)("stop_area:stop1", "24:10"_t, "24:11"_t)(
            "stop_area:stop2", "25:20"_t, "25:21"_t)("A2", "26:20"_t, "26:21"_t);
    });
    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    // we delete the stop_area 1, on vj passing midnight is impacted,

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopArea, "stop_area", *b.data->pt_data)
                                  .application_periods(btp("20120615T000000"_dt, "20120618T000000"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Base VJ
    auto vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:A:0"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "0111111"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "0111000"),
                        vj->adapted_validity_pattern()->days);

    // Adapted VJ
    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj 0:Adapted:1:stop_area_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "0000000"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "0000111"),
                        vj->adapted_validity_pattern()->days);

    auto res = compute(*b.data, nt::RTLevel::Adapted, "A1", "A2", "23:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items[0].stop_times.size(), 2);
    auto vp = res[0].items[0].stop_times[0]->vehicle_journey->adapted_validity_pattern();
    BOOST_CHECK_MESSAGE(ba::ends_with(vp->days.to_string(), "0000111"), vp->days.to_string());

    BOOST_REQUIRE(!compute(*b.data, nt::RTLevel::Base, "A1", "stop_area", "08:00"_t, 1).empty());
    BOOST_REQUIRE(compute(*b.data, nt::RTLevel::Adapted, "A1", "stop_area", "08:00"_t, 1).empty());

    navitia::delete_disruption("stop_area_closed", *b.data->pt_data, *b.data->meta);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "0111111");

    BOOST_REQUIRE(!compute(*b.data, nt::RTLevel::Base, "A1", "stop_area", "08:00"_t, 1).empty());
    BOOST_REQUIRE(!compute(*b.data, nt::RTLevel::Adapted, "A1", "stop_area", "08:00"_t, 1).empty());
}

/*
 * This test make sure the filtering on the impact_vp handle correctly the last day in case there is an impact
 * starting for example at 9am on the first day, but ending at 8:59 on the last.
 */
BOOST_AUTO_TEST_CASE(add_stop_point_impact_check_vp_filtering_last_day) {
    ed::builder b("20160404", [](ed::builder& b) {
        b.vj("line:A", "111100")
            .name("vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:20"_t, "08:21"_t)(
                "stop_point:30", "08:30"_t, "08:31"_t)("stop_point:40", "08:40"_t, "08:41"_t);
    });
    b.data->meta->production_date = bg::date_period(bg::date(2016, 4, 4), bg::days(7));

    // Close stop_point:20 from the 4h at 9am to the 6h at 8:59am
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_point:20_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop_point:20", *b.data->pt_data)
                                  .application_periods(btp("20160404T090000"_dt, "20160406T085900"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    // 2 vj, vj:1 needs to be impacted on the 6th
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111100"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:stop_point:20_closed");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);

    navitia::delete_disruption("stop_point:20_closed", *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111100"), base_vp);
}

/*
 *
 *
 *  Day 14              15              16              17              18              19              20
 *      -----------------------------------------------------------------------------------------------------
 *   VJ     8H----9H         8H----9H        8H----9H        8H----9H        8H----9H        8H----9H
 * Impact  |-----------------------------------------------|           |---|                     |------|
 *        7:00                                            6:00      23:00  6:00                 8:30   9:30
 * */
BOOST_AUTO_TEST_CASE(add_impact_with_sevral_application_period) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.vj("A").name("vj:1")("stop1", "08:00"_t)("stop2", "08:15"_t)("stop3",
                                                                       "08:45"_t)  // <- Only stop3 will be impacted
            ("stop4", "09:00"_t);
    });
    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop3_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop3", *b.data->pt_data)
                                  // 2012/6/14 7h -> 2012/6/17 6h
                                  .application_periods(btp("20120614T070000"_dt, "20120617T060000"_dt))
                                  // 2012/6/17 23h -> 2012/6/18 6h
                                  .application_periods(btp("20120617T230000"_dt, "20120618T060000"_dt))
                                  // 2012/6/19 4h -> 2012/6/19 8h30
                                  .application_periods(btp("20120619T083000"_dt, "20120619T093000"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // Base VJ
    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "011000"),
                        vj->adapted_validity_pattern()->days);

    // Adapted VJ
    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:Adapted:0:stop3_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "100111"),
                        vj->adapted_validity_pattern()->days);

    // 2012/6/15 8:00
    auto res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "08:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop4", "08:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "08:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 0);  // <-- The stop3 is not accessible at the given datetime.

    // 2012/6/18 5:00
    res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "05:00"_t, 4);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "05:00"_t, 4);
    BOOST_REQUIRE_EQUAL(res.size(), 1);  // <-- The result should be the base vj
    BOOST_REQUIRE_EQUAL(res[0].items[0].stop_times[0]->vehicle_journey->uri, "vehicle_journey:vj:1");

    navitia::delete_disruption("stop3_closed", *b.data->pt_data, *b.data->meta);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "1111111");

    BOOST_REQUIRE_EQUAL(compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "08:00"_t, 1).size(), 1);
    BOOST_REQUIRE_EQUAL(compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "05:00"_t, 4).size(), 1);
}

BOOST_AUTO_TEST_CASE(remove_stop_point_impact) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.vj("A").name("vj:1")("stop1", "08:00"_t)("stop2", "08:15"_t)("stop3",
                                                                       "08:45"_t)  // <- Only stop3 will be impacted
            ("stop4", "09:00"_t);
    });
    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    const auto& disruption = b.impact(nt::RTLevel::Adapted, "stop3_closed")
                                 .severity(nt::disruption::Effect::NO_SERVICE)
                                 .on(nt::Type_e::StopPoint, "stop3", *b.data->pt_data)
                                 // 2012/6/14 7h -> 2012/6/17 6h
                                 .application_periods(btp("20120614T070000"_dt, "20120617T060000"_dt))
                                 // 2012/6/17 23h -> 2012/6/18 6h
                                 .application_periods(btp("20120617T230000"_dt, "20120618T060000"_dt))
                                 // 2012/6/19 4h -> 2012/6/19 8h30
                                 .application_periods(btp("20120619T083000"_dt, "20120619T093000"_dt))
                                 .get_disruption();

    navitia::apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    navitia::apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    navitia::apply_disruption(disruption, *b.data->pt_data, *b.data->meta);

    // Base VJ
    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "011000"),
                        vj->adapted_validity_pattern()->days);

    // Adapted VJ
    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:Adapted:0:stop3_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "100111"),
                        vj->adapted_validity_pattern()->days);

    navitia::delete_disruption("stop3_closed", *b.data->pt_data, *b.data->meta);
    navitia::delete_disruption("stop3_closed", *b.data->pt_data, *b.data->meta);
    navitia::delete_disruption("stop3_closed", *b.data->pt_data, *b.data->meta);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "1111111");
}

BOOST_AUTO_TEST_CASE(remove_all_stop_point) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.vj("A").name("vj:1")("stop1", "08:00"_t)("stop2", "08:15"_t)("stop3", "08:45"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop1_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop1", *b.data->pt_data)
                                  // 2012/6/14 8h00 -> 2012/6/19 8h05
                                  .application_periods(btp("20120614T080000"_dt, "20120619T080500"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop2_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop2", *b.data->pt_data)
                                  // 2012/6/14 8h15 -> 2012/6/19 8h17
                                  .application_periods(btp("20120614T081500"_dt, "20120619T081700"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop3_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop3", *b.data->pt_data)
                                  // 2012/6/14 8h45 -> 2012/6/19 8h47
                                  .application_periods(btp("20120614T084500"_dt, "20120619T084700"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    // Base VJ
    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000000"),
                        vj->adapted_validity_pattern()->days);

    // Adapted VJ
    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:Adapted:1:stop1_closed:stop2_closed:stop3_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "111111"),
                        vj->adapted_validity_pattern()->days);

    // in Adapted Vj, all stop point is blocked....
    BOOST_REQUIRE(vj->stop_time_list.empty());

    auto res = compute(*b.data, nt::RTLevel::Adapted, "stop1", "stop3", "8:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    navitia::delete_disruption("stop1_closed", *b.data->pt_data, *b.data->meta);
    navitia::delete_disruption("stop2_closed", *b.data->pt_data, *b.data->meta);
    navitia::delete_disruption("stop3_closed", *b.data->pt_data, *b.data->meta);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "1111111");
}

BOOST_AUTO_TEST_CASE(stop_point_no_service_with_shift) {
    nt::VehicleJourney* vj1 = nullptr;
    ed::builder b("20120614", [&](ed::builder& b) {
        vj1 = b.vj("A").name("vj:1")("stop1", "23:00"_t)("stop2", "24:15"_t)("stop3", "24:45"_t).make();
    });

    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    auto trip_update =
        ntest::make_trip_update_message("vj:1", "20120616",
                                        {
                                            ntest::RTStopTime("stop1", "20120617T0005"_pts).delay(65_min),
                                            ntest::RTStopTime("stop2", "20120617T0105"_pts).delay(55_min),
                                            ntest::RTStopTime("stop3", "20120617T0205"_pts).delay(80_min),
                                        });
    navitia::handle_realtime("bob", "20120616T1337"_dt, trip_update, *b.data, true, true);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "111011"),
                        vj1->rt_validity_pattern()->days);

    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "001000"),
                        vj->rt_validity_pattern()->days);

    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "stop2_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop2", *b.data->pt_data)
                                  .application_periods(btp("20120617T000000"_dt, "20120617T080500"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "111011"),
                        vj1->rt_validity_pattern()->days);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:RealTime:1:bob:stop2_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
                        vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "001000"),
                        vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 1);
    auto* s1 = b.data->pt_data->stop_points_map["stop1"];
    auto* s3 = b.data->pt_data->stop_points_map["stop3"];

    check_vj(vj, {{"00:05"_t, "00:05"_t, s1}, {"02:05"_t, "02:05"_t, s3}});

    auto res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "8:00"_t, 3);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    navitia::delete_disruption("stop2_closed", *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "111011"),
                        vj1->rt_validity_pattern()->days);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "001000"),
                        vj->rt_validity_pattern()->days);

    navitia::delete_disruption("bob", *b.data->pt_data, *b.data->meta);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "1111111");
}

/*
 *  Day 14          15          16          17          18          19          20          21
 *      -----------------------------------------------------------------------------------------------
 *  Vj1:
 *                                        23H---1H
 * Delayed_VJ:
 *                                                    23H---1H
 * Disrupted_Delayed_VJ:
 *                                                       0H05-1H
 * */
BOOST_AUTO_TEST_CASE(test_shift_of_a_disrupted_delayed_train) {
    nt::VehicleJourney* vj1 = nullptr;
    ed::builder b("20120614", [&](ed::builder& b) {
        vj1 = b.vj("A", "00100").name("vj:1")("stop1", "23:00"_t)("stop2", "24:15"_t)("stop3", "25:00"_t).make();
    });
    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);

    auto trip_update =
        ntest::make_trip_update_message("vj:1", "20120616",
                                        {
                                            ntest::RTStopTime("stop1", "20120617T2300"_pts).delay(24_h),
                                            ntest::RTStopTime("stop2", "20120618T0005"_pts).delay(23_h + 50_min),
                                            ntest::RTStopTime("stop3", "20120618T0100"_pts).delay(24_h),
                                        });
    navitia::handle_realtime("bob", "20120616T1337"_dt, trip_update, *b.data, true, true);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "000000"),
                        vj1->rt_validity_pattern()->days);

    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "0001000"),
                        vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 1);

    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "stop1_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop1", *b.data->pt_data)
                                  .application_periods(btp("20120617T2200"_dt, "20120618T0000"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "000000"),
                        vj1->rt_validity_pattern()->days);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:RealTime:1:bob:stop1_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "010000"),
                        vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 2);

    auto* stop2 = b.data->pt_data->stop_points_map["stop2"];
    auto* stop3 = b.data->pt_data->stop_points_map["stop3"];
    check_vj(vj, {{"00:05"_t, "00:05"_t, stop2}, {"01:00"_t, "01:00"_t, stop3}});

    auto res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "23:00"_t, 2);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "23:00"_t, 2);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "23:00"_t, 3);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "23:00"_t, 3);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    res = compute(*b.data, nt::RTLevel::RealTime, "stop2", "stop3", "23:00"_t, 3);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items[0].stop_times.size(), 2);
    auto vp = res[0].items[0].stop_times[0]->vehicle_journey->rt_validity_pattern();
    BOOST_CHECK_MESSAGE(ba::ends_with(vp->days.to_string(), "010000"), vp->days.to_string());

    navitia::delete_disruption("stop1_closed", *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "0001000"),
                        vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 1);

    navitia::delete_disruption("bob", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->adapted_validity_pattern()->days.to_string(), "00100"),
                        vj1->adapted_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->base_validity_pattern()->days.to_string(), "00100"),
                        vj1->base_validity_pattern()->days);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "00100");

    res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "23:00"_t, 2);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "23:00"_t, 2);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
}

/*
 *  Day 14          15          16          17          18          19          20          21
 *      -----------------------------------------------------------------------------------------------
 *  Vj1:
 *                                        23H---1H
 * Disrupted_Delayed_VJ:
 *                                           0H15-1H
 * Delayed_VJ:
 *                                                         0H15-1H
 * */
BOOST_AUTO_TEST_CASE(disrupted_stop_point_then_delayed) {
    nt::VehicleJourney* vj1 = nullptr;
    ed::builder b("20120614", [&](ed::builder& b) {
        vj1 = b.vj("A", "00100").name("vj:1")("stop1", "23:00"_t)("stop2", "24:15"_t)("stop3", "25:00"_t).make();
    });

    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    // Stop1 is closed between 2012/06/16 22:00 and 2012/06/16 23:30
    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "stop1_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop1", *b.data->pt_data)
                                  .application_periods(btp("20120616T2200"_dt, "20120616T2330"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "000000"),
                        vj1->rt_validity_pattern()->days);

    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:RealTime:0:stop1_closed"];

    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "001000"),
                        vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 1);

    auto trip_update = ntest::make_trip_update_message("vj:1", "20120616",
                                                       {
                                                           ntest::RTStopTime("stop2", "20120618T0015"_pts).delay(48_h),
                                                           ntest::RTStopTime("stop3", "20120618T0100"_pts).delay(48_h),
                                                       });

    navitia::handle_realtime("bob", "20120616T1337"_dt, trip_update, *b.data, true, true);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "000000"),
                        vj1->rt_validity_pattern()->days);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:modified:1:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "0010000"),
                        vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 2);

    auto* stop2 = b.data->pt_data->stop_points_map["stop2"];
    auto* stop3 = b.data->pt_data->stop_points_map["stop3"];
    check_vj(vj, {{"00:15"_t, "00:15"_t, stop2}, {"01:00"_t, "01:00"_t, stop3}});

    auto res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "23:00"_t, 2);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "23:00"_t, 2);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "23:00"_t, 3);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    // Stop1 is closed
    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "23:00"_t, 3);
    BOOST_REQUIRE_EQUAL(res.size(), 0);

    res = compute(*b.data, nt::RTLevel::RealTime, "stop2", "stop3", "23:00"_t, 3);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items[0].stop_times.size(), 2);
    auto vp = res[0].items[0].stop_times[0]->vehicle_journey->rt_validity_pattern();
    BOOST_CHECK_MESSAGE(ba::ends_with(vp->days.to_string(), "010000"), vp->days.to_string());

    navitia::delete_disruption("stop1_closed", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "010000"),
                        vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 2);

    navitia::delete_disruption("bob", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "00100");

    res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "23:00"_t, 2);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "23:00"_t, 2);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
}

/*
 * In this test case, some commented tests could not be satisfied because of the current algo.
 * 3 three VJ should be created in this test case, but only one is now created...
 *
 * */
BOOST_AUTO_TEST_CASE(same_stop_point_on_vj) {
    nt::VehicleJourney* vj = nullptr;
    ed::builder b("20120614", [&](ed::builder& b) {
        vj = b.vj("A")
                 .name("vj:1")("stop1", "08:00"_t)("stop2", "09:00"_t)("stop3", "10:00"_t)("stop1", "11:00"_t)
                 .make();
    });

    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop1_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop1", *b.data->pt_data)
                                  .application_periods(btp("20120615T100000"_dt, "20120617T100000"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "110001"),
                        vj->adapted_validity_pattern()->days);

    const nt::VehicleJourney* vj15 = nullptr;
    const nt::VehicleJourney* vj16 = nullptr;
    const nt::VehicleJourney* vj17 = nullptr;
    for (const auto* cur_vj : b.data->pt_data->vehicle_journeys) {
        if (cur_vj->adapted_validity_pattern()->check(1)) {
            vj15 = cur_vj;
        }
        if (cur_vj->adapted_validity_pattern()->check(2)) {
            vj16 = cur_vj;
        }
        if (cur_vj->adapted_validity_pattern()->check(3)) {
            vj17 = cur_vj;
        }
    }

    BOOST_REQUIRE(vj15 && !vj15->stop_time_list.empty());
    // BOOST_CHECK_EQUAL(vj15->stop_time_list.size(), 3);
    // BOOST_CHECK_EQUAL(vj15->stop_time_list.front().stop_point->uri, "stop1");
    // BOOST_CHECK_EQUAL(vj15->stop_time_list.back().stop_point->uri, "stop3");

    BOOST_REQUIRE(vj16 && !vj16->stop_time_list.empty());
    BOOST_CHECK_EQUAL(vj16->stop_time_list.size(), 2);
    BOOST_CHECK_EQUAL(vj16->stop_time_list.front().stop_point->uri, "stop2");
    BOOST_CHECK_EQUAL(vj16->stop_time_list.back().stop_point->uri, "stop3");

    BOOST_REQUIRE(vj17 && !vj17->stop_time_list.empty());
    // BOOST_CHECK_EQUAL(vj17->stop_time_list.size(), 3);
    // BOOST_CHECK_EQUAL(vj17->stop_time_list.front().stop_point->uri, "stop2");
    // BOOST_CHECK_EQUAL(vj17->stop_time_list.back().stop_point->uri, "stop1");
}

/*
 *
 *         |---------------------------------------------|
 *         |
 * A1 > > >|sp1> >sp2> >sp3> >sp4> >sp5> >sp6> >sp7> >sp8|> > > > A2
 *         |      /                               \      |          (VJ_A)
 * B1 > > >|-----/                                 \-----|> > > > B2 > > > B3 > > > B4
 *         |                                             |                            (VJ_B)
 *         |---------------------------------------------|
 *                    stop_area
 *
 *
 *
 * */
BOOST_AUTO_TEST_CASE(stop_point_deletion_test) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.sa("stop_area", 0, 0, false, true)("stop_area:stop1")("stop_area:stop2")("stop_area:stop3")(
            "stop_area:stop4")("stop_area:stop5")("stop_area:stop6")("stop_area:stop7")("stop_area:stop8");

        b.vj("A", "111111")
            .name("VjA")("A1", "08:10"_t)("stop_area:stop1", "09:10"_t)("stop_area:stop2", "10:20"_t)(
                "stop_area:stop3", "11:10"_t)("stop_area:stop4", "12:20"_t)("stop_area:stop5", "13:10"_t)(
                "stop_area:stop6", "14:20"_t)("stop_area:stop7", "15:10"_t)("stop_area:stop8", "16:20"_t)("A2",
                                                                                                          "17:20"_t);

        b.vj("B", "111111")
            .name("VjB")("B1", "08:10"_t)("stop_area:stop2", "10:20"_t)("stop_area:stop3", "11:10"_t)(
                "stop_area:stop4", "12:20"_t)("stop_area:stop5", "13:10"_t)("stop_area:stop6", "14:20"_t)(
                "stop_area:stop7", "15:10"_t)("B2", "17:20"_t)("B3", "18:20"_t)("B4", "19:20"_t);
    });

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_point_B2_closed_3")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "B2", *b.data->pt_data)
                                  .application_periods(btp("20120616T080000"_dt, "20120616T203200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_point_B3_closed_3")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "B3", *b.data->pt_data)
                                  .application_periods(btp("20120616T080000"_dt, "20120616T203200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed_1")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopArea, "stop_area", *b.data->pt_data)
                                  .application_periods(btp("20120618T080000"_dt, "20120618T173200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed_2")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopArea, "stop_area", *b.data->pt_data)
                                  .application_periods(btp("20120615T080000"_dt, "20120615T173200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed_3")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopArea, "stop_area", *b.data->pt_data)
                                  .application_periods(btp("20120614T080000"_dt, "20120614T173200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    const auto* mvj_A = b.data->pt_data->meta_vjs["VjA"];
    const auto* mvj_B = b.data->pt_data->meta_vjs["VjB"];
    BOOST_CHECK_EQUAL(mvj_A->modified_by.size(), 3);
    BOOST_CHECK_EQUAL(mvj_B->modified_by.size(), 5);

    navitia::delete_disruption("disruption_does't_exist", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(mvj_A->modified_by.size(), 3);
    BOOST_CHECK_EQUAL(mvj_B->modified_by.size(), 5);

    navitia::delete_disruption("stop_area_closed_1", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(mvj_A->modified_by.size(), 2);
    BOOST_CHECK_EQUAL(mvj_B->modified_by.size(), 4);

    navitia::delete_disruption("stop_area_closed_2", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(mvj_A->modified_by.size(), 1);
    BOOST_CHECK_EQUAL(mvj_B->modified_by.size(), 3);

    navitia::delete_disruption("stop_area_closed_3", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(mvj_A->modified_by.size(), 0);
    BOOST_CHECK_EQUAL(mvj_B->modified_by.size(), 2);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);
}

BOOST_AUTO_TEST_CASE(add_simple_impact_on_line_section) {
    ed::builder b("20160404", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)(
            "stop_point:20", "08:20"_t, "08:21"_t)("stop_point:30", "08:30"_t, "08:31"_t)("stop_point:40", "08:40"_t,
                                                                                          "08:41"_t);
        b.vj("line:A", "101010", "", true, "vj:2")("stop_point:10", "09:10"_t, "09:11"_t)(
            "stop_point:20", "09:20"_t, "09:21"_t)("stop_point:30", "09:30"_t, "09:31"_t)("stop_point:40", "09:40"_t,
                                                                                          "09:41"_t);
        b.vj("line:A", "001100", "", true, "vj:3")("stop_point:10", "10:10"_t, "10:11"_t)(
            "stop_point:20", "10:20"_t, "10:21"_t)("stop_point:30", "10:30"_t, "10:31"_t)("stop_point:40", "10:40"_t,
                                                                                          "10:41"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2016, 4, 4), bg::days(7));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20160404T120000"_dt, "20160406T090000"_dt))
            .on_line_section("line:A", "stop_area:2", "stop_area:3", {"line:A:0"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    // Only 5 vj, vj:3 shouldn't be affected
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 5);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:2");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "101000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "101010"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:3");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "001100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "001100"), base_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_diverted");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000110"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().departure_time, "08:11"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().arrival_time, "08:40"_t);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:2:Adapted:0:line_section_on_line:A_diverted");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000010"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().departure_time, "09:11"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().arrival_time, "09:40"_t);

    // vj:3 shouldn't be affected by the disruption. On the 6th the disruption stops at 9am but its first
    // stop_time starts at 10am
    BOOST_CHECK(b.get<nt::VehicleJourney>("vehicle_journey:vj:3:Adapted:0:line_section_on_line:A_diverted") == nullptr);

    // Deleting the disruption
    navitia::delete_disruption("line_section_on_line:A_diverted", *b.data->pt_data, *b.data->meta);

    // Wqe are now back to the 'normal' state
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:2");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "101010"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "101010"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:3");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "001100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "001100"), base_vp);
}

BOOST_AUTO_TEST_CASE(multiple_impact_on_line_section) {
    ed::builder b("20160404", [](ed::builder& b) {
        b.vj("line:1").name("vj:1")("A", "08:10"_t)("B", "08:20"_t)("C", "08:30"_t)("D", "08:40"_t)("E", "08:50"_t)(
            "F", "08:60"_t)("G", "08:70"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2016, 4, 4), bg::days(7));

    auto get_stops = [](const nt::VehicleJourney* vj) {
        std::vector<std::string> res;
        for (const auto& st : vj->stop_time_list) {
            res.push_back(st.stop_point->uri);
        }
        return res;
    };
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "first_ls_B_C")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20160404T080000"_dt, "20160406T230000"_dt))
                                  .publish(btp("20160404T080000"_dt, "20160406T090000"_dt))
                                  .on_line_section("line:1", "B", "C", {"line:1:0"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // it should have created one vj
    BOOST_CHECK_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // we should be able to find the disuption on the stoppoints [B, C]
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("A")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("B")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("C")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("D")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("E")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("F")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("G")->get_impacts().size(), 0);

    // Check the original vjs
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 7);
    auto* meta_vj = vj->meta_vj;
    // this vj should be impacted by the line section, so the impact should be found in it's metavj
    BOOST_CHECK_EQUAL(meta_vj->get_impacts().size(), 1);

    // Check adapted vjs
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);

    const auto* adapted_vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:first_ls_B_C");
    BOOST_REQUIRE(adapted_vj);
    BOOST_CHECK(ba::ends_with(adapted_vj->adapted_validity_pattern()->days.to_string(), "000111"));
    BOOST_CHECK(ba::ends_with(adapted_vj->base_validity_pattern()->days.to_string(), "000000"));
    BOOST_CHECK_EQUAL(adapted_vj->meta_vj->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(adapted_vj->stop_time_list.size(), 5);

    BOOST_CHECK_EQUAL_RANGE(get_stops(adapted_vj), std::vector<std::string>({"A", "D", "E", "F", "G"}));

    // we now cut E->F
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "second_ls_E_F")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20160404T080000"_dt, "20160406T230000"_dt))
                                  .publish(btp("20160404T080000"_dt, "20160406T090000"_dt))
                                  .on_line_section("line:1", "E", "F", {"line:1:0"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // it will have created another vj and cleaned the previous one
    BOOST_CHECK_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // we should be able to find the disuption on the stoppoints [B, C] and [E, F]
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("A")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("B")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("C")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("D")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("E")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("F")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("G")->get_impacts().size(), 0);

    // Check the original vjs
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 7);
    // the 2 impacts should be there
    BOOST_CHECK_EQUAL(meta_vj->get_impacts().size(), 2);

    // Check adapted vjs, there should be only 1
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);

    const auto* second_adapted_vj =
        b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:first_ls_B_C:Adapted:1:second_ls_E_F");
    BOOST_REQUIRE(second_adapted_vj);
    BOOST_CHECK(ba::ends_with(second_adapted_vj->adapted_validity_pattern()->days.to_string(), "000111"));
    BOOST_CHECK(ba::ends_with(second_adapted_vj->base_validity_pattern()->days.to_string(), "000000"));

    BOOST_CHECK_EQUAL(second_adapted_vj->meta_vj->get_impacts().size(), 2);
    BOOST_CHECK_EQUAL_RANGE(get_stops(second_adapted_vj), std::vector<std::string>({"A", "D", "G"}));

    // we now cut D->G
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "third_ls_C_G")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20160404T080000"_dt, "20160406T230000"_dt))
                                  .publish(btp("20160404T080000"_dt, "20160406T090000"_dt))
                                  .on_line_section("line:1", "B", "G", {"line:1:0"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    // it should not have created another vj
    BOOST_CHECK_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // we should be able to find the disuption on the stoppoints [B, G]
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("A")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("B")->get_impacts().size(), 2);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("C")->get_impacts().size(), 2);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("D")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("E")->get_impacts().size(), 2);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("F")->get_impacts().size(), 2);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("G")->get_impacts().size(), 1);

    // Check the original vjs
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 7);
    // the 3 impacts should be there
    BOOST_CHECK_EQUAL(meta_vj->get_impacts().size(), 3);

    // Check adapted vjs, there should only be the last one
    const auto* third_adapted_vj = b.get<nt::VehicleJourney>(
        "vehicle_journey:vj:1:Adapted:0:first_ls_B_C:Adapted:1:second_ls_E_F:Adapted:1:third_ls_C_G");
    BOOST_REQUIRE(third_adapted_vj);
    BOOST_CHECK(ba::ends_with(third_adapted_vj->adapted_validity_pattern()->days.to_string(), "000111"));
    BOOST_CHECK(ba::ends_with(third_adapted_vj->base_validity_pattern()->days.to_string(), "000000"));
    BOOST_CHECK_EQUAL(third_adapted_vj->meta_vj->get_impacts().size(), 3);
    BOOST_CHECK_EQUAL_RANGE(get_stops(third_adapted_vj), std::vector<std::string>({"A"}));
}

BOOST_AUTO_TEST_CASE(add_impact_on_line_section_with_vj_pass_midnight) {
    ed::builder b("20160404", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");

        b.vj("line:A", "001001")
            .name("vj:1")("stop_point:10", "23:00"_t, "23:01"_t)("stop_point:20", "24:00"_t, "24:01"_t)(
                "stop_point:30", "25:00"_t, "25:01"_t)("stop_point:40", "26:00"_t, "26:01"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2016, 4, 4), bg::days(7));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20160405T003000"_dt, "20160406T090000"_dt))
            .on_line_section("line:A", "stop_area:2", "stop_area:3", {"line:A:0"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    // 2 vj
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "001000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "001001"), base_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_diverted");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().departure_time, "23:01"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().arrival_time, "26:00"_t);

    // Deleting the disruption
    navitia::delete_disruption("line_section_on_line:A_diverted", *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "001001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "001001"), base_vp);
}

BOOST_AUTO_TEST_CASE(add_impact_on_line_section_cancelling_vj) {
    ed::builder b("20160404", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "13:00"_t, "13:01"_t)(
            "stop_point:20", "14:00"_t, "14:01"_t)("stop_point:30", "15:00"_t, "15:01"_t)("stop_point:40", "16:00"_t,
                                                                                          "16:01"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2016, 4, 4), bg::days(7));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .on_line_section("line:A", "stop_area:1", "stop_area:4", {"line:A:0"}, *b.data->pt_data)
            .application_periods(btp("20160404T000000"_dt, "20160406T230000"_dt))
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    // 1 vj, since all stop_times are impacted we should just cancel the base vj
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);

    // Check the vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // Deleting the disruption
    navitia::delete_disruption("line_section_on_line:A_diverted", *b.data->pt_data, *b.data->meta);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
}

/*
 * In case we have a pretty complex line with repeated stops, since a line_section is only defined by
 * a line, two stop areas and an optional list of routes, we're impacting the shortest sections possible for each
 * route. Let's take a look at the line A below, with 4 routes, all departing from s1 and ending at s6.
 *
 * 1st route s1/s2/s3/s2/s3/s4/s5/s4/s5/s6 :
 *
 *                   loop once                          loop once
 *                .------<-------.                  .-------<-------.
 *                |              |                  |               |
 *                v              ^                  v               ^
 *                |              |                  |               |
 * s1 ---------- s2 -----------> s3 -------------> s4 -----------> s5 -----------> s6
 *
 *  2nd route s1/s5/s4/s3/s2/s5'/s6 :
 *
 *       .------->-------------->------------------>---------------.
 *       |                                                         |
 * s1 ---'      s2 <----------- s3 <------------- s4 <----------- s5  .----------> s6
 *              |                                               --s5'-'
 *              |                                              |
 *              '---------------->----------------->-----------'    s5 and s5' in the same stop_area
 *
 *  3rd route s1/s3/s4/s5/s4/s5/s6 :
 *                                                      loop once
 *                                                  .-------<-------.
 *                                                  |               |
 *         .------>---------->---.                  ^               ^
 *         |                     |                  |               |
 * s1------'                    s3 --------------> s4 -----------> s5 -----------> s6
 *
 *
 * 4th route :
 * s1 ---------- s2 -----------> s3 -------------> s4 -----------> s5 -----------> s6
 *
 */
BOOST_AUTO_TEST_CASE(add_line_section_impact_on_line_with_repeated_stops) {
    ed::builder b("20160404", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("s1");
        b.sa("stop_area:2", 0, 0, false, true)("s2");
        b.sa("stop_area:3", 0, 0, false, true)("s3");
        b.sa("stop_area:4", 0, 0, false, true)("s4");
        b.sa("stop_area:5", 0, 0, false, true)("s5")("s5'");
        b.sa("stop_area:6", 0, 0, false, true)("s6");

        b.vj("line:A", "111111", "", true, "vj:1")
            .route("route:A1")("s1", "08:00"_t, "08:00"_t)("s2", "08:01"_t, "08:01"_t)("s3", "08:02"_t, "08:02"_t)(
                "s2", "08:03"_t, "08:03"_t)("s3", "08:04"_t, "08:04"_t)("s4", "08:05"_t, "08:05"_t)(
                "s5", "08:06"_t, "08:06"_t)("s4", "08:07"_t, "08:07"_t)("s5", "08:08"_t, "08:08"_t)("s6", "08:09"_t,
                                                                                                    "08:09"_t);

        b.vj("line:A", "111111", "", true, "vj:2")
            .route("route:A2")("s1", "09:00"_t, "09:00"_t)("s5", "09:01"_t, "09:01"_t)("s4", "09:02"_t, "09:02"_t)(
                "s3", "09:03"_t, "09:03"_t)("s2", "09:04"_t, "09:04"_t)("s5'", "09:05"_t, "09:05"_t)("s6", "09:06"_t,
                                                                                                     "09:06"_t);

        b.vj("line:A", "111111", "", true, "vj:3")
            .route("route:A3")("s1", "10:00"_t, "10:00"_t)("s3", "10:01"_t, "10:01"_t)("s4", "10:03"_t, "10:03"_t)(
                "s5", "10:04"_t, "10:04"_t)("s4", "10:05"_t, "10:05"_t)("s5", "10:06"_t, "10:06"_t)("s6", "10:07"_t,
                                                                                                    "10:07"_t);

        b.vj("line:A", "111111", "", true, "vj:4")
            .route("route:A4")("s1", "11:00"_t, "10:00"_t)("s2", "11:01"_t, "10:01"_t)("s3", "11:02"_t, "10:02"_t)(
                "s4", "11:03"_t, "10:03"_t)("s5", "11:04"_t, "10:04"_t)("s6", "11:05"_t, "10:05"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2016, 4, 4), bg::days(7));

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line_section_sa1_sa5_routesA1-2-3")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20160404T000000"_dt, "20160409T240000"_dt))
                                  .on_line_section("line:A", "stop_area:2", "stop_area:5",
                                                   {"route:A1", "route:A2", "route:A3"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 4);
    // Only 6 vj, vj:3 shouldn't be affected by the section, vj:4 shouldn't be affected since the route isn't
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 6);

    // we should be able to find the disuption on the stoppoints [s2, s5]
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s1")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s2")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s3")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s4")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s5")->get_impacts().size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s6")->get_impacts().size(), 0);

    // Check the original vjs
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    // this vj should be impacted by the line section, so the impact should be found in it's metavj
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 1);
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:2");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 1);
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // vj3 shouldn't be impacted since there is no section stop_area:2 -> stop_area:5
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:3");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 0);
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // vj4 shouldn't be impacted too since it's route isn't
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:4");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 0);
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // Check adapted vjs
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_sa1_sa5_routesA1-2-3");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 1);
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // 6 stop_times since we don't impact the first iteration of each loop
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 6);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(0).stop_point->uri, "s1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "s2");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).stop_point->uri, "s3");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(3).stop_point->uri, "s4");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(4).stop_point->uri, "s5");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(5).stop_point->uri, "s6");

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:2:Adapted:0:line_section_sa1_sa5_routesA1-2-3");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 1);
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 5);
    std::vector<std::string> target_uris = {"s1", "s5", "s4", "s3", "s6"};
    for (size_t i = 0; i < vj->stop_time_list.size(); i++) {
        BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(i).stop_point->uri, target_uris.at(i));
    }

    // route:A3 shouldn't be impacted, the adapted vj shouldn't exist
    BOOST_CHECK(b.get<nt::VehicleJourney>("vehicle_journey:vj:3:Adapted:0:line_section_sa1_sa5_routesA1-2-3")
                == nullptr);
    // route:A4 wasn't in the list of impacted route
    BOOST_CHECK(b.get<nt::VehicleJourney>("vehicle_journey:vj:4:Adapted:0:line_section_sa1_sa5_routesA1-2-3")
                == nullptr);

    // Deleting the disruption
    navitia::delete_disruption("line_section_sa1_sa5_routesA1-2-3", *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    // Make sur there is no impacts anymore
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s1")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s2")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s3")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s4")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s5")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::StopPoint>("s6")->get_impacts().size(), 0);

    // Check the original vjs
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 0);  // there shouldn't be any impacts anymore
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:2");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 0);
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // vj3 shouldn't be impacted since there is no section stop_area:2 -> stop_area:5
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:3");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 0);
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:4");
    BOOST_CHECK_EQUAL(vj->meta_vj->get_impacts().size(), 0);
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
}

/*
 * We have a simple line, line A, from stop_area:1 to stop_area:5. The vps start on the 4th of april.
 * We have 3 vjs departing at 8:10am, 9:10am, 10:10am and arriving respectively at 8:50am, 9:50am, 10:50am.
 *
 * We have 2 disruptions :
 *  - The first one from the 4th at midnight to the 6th at 10:35am, on the section stop_area:4 -> stop_area:5,
 *  - The second one from the 5th at 9am to the 7th at 8:35am on the section stop_area:3 -> stop_area:4
 *
 * We're going to have 3 possible disrupted routes :
 *  - stop_point:10, stop_point:20, stop_point:30 when only the first disruption is applied,
 *  - stop_point:10, stop_point:20, stop_point:50 when only the second disruption is applied,
 *  - stop_point:10, stop_point:20 when both are applied
 *
 * This is what we should have for each vj.
 * vj1 :
 *  - Impacted by the first disruption from the 4th to the 6th,
 *  - Impacted by the second disruption on the 6th and 7th (the disruption starts after the service on the 5th)
 *  |
 *  '-> 3 adapted vj created, since disruptions overlap on the 6th
 *
 * vj2 :
 *  - Impacted by the first disruption on the 5th and 6th (vj2 doesn't run on the 4th),
 *  - Impacted by the second disruption on the 5th and 6th (the disruptions ends before the service on the 7th),
 *  |
 *  '-> 2 adapted vj created. The first disruption will create an adapted vj for its disruption, but since
 *      it's overlaping with the second on both days, the vp of this adapted vj will be empty after the second
 *      one is applied. The second adapted vj will be impacted by both disruptions and run on both days. We
 *      will not have an adapted vj for the second disruption only because the base vj doesn't run anymore on the
 *      5th and the 6th since it's already disrupted by the first disruption.
 *
 * vj3 :
 *  - Not impacted by the first disruption. It ends during the service of vj3 on the 6th, but before it enters
 *    into the section. And on the 4th and the 5th, vj3 doesn't run.
 *  - Impacted by the second disruption on the 6th, vj3 doesn't run on the 5th, and on the 7th it ends before
 *    vj3 runs.
 *  |
 *  '-> 1 adapted vj created.
 */
BOOST_AUTO_TEST_CASE(add_multiple_impact_on_line_section) {
    ed::builder b("20160404", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");
        b.sa("stop_area:5", 0, 0, false, true)("stop_point:50");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)(
            "stop_point:20", "08:20"_t, "08:21"_t)("stop_point:30", "08:30"_t, "08:31"_t)(
            "stop_point:40", "08:40"_t, "08:41"_t)("stop_point:50", "08:50"_t, "08:51"_t);
        b.vj("line:A", "101110", "", true, "vj:2")("stop_point:10", "09:10"_t, "09:11"_t)(
            "stop_point:20", "09:20"_t, "09:21"_t)("stop_point:30", "09:30"_t, "09:31"_t)(
            "stop_point:40", "09:40"_t, "09:41"_t)("stop_point:50", "09:50"_t, "09:51"_t);
        b.vj("line:A", "001100", "", true, "vj:3")("stop_point:10", "10:10"_t, "10:11"_t)(
            "stop_point:20", "10:20"_t, "10:21"_t)("stop_point:30", "10:30"_t, "10:31"_t)(
            "stop_point:40", "10:40"_t, "10:41"_t)("stop_point:50", "10:50"_t, "10:51"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2016, 4, 4), bg::days(7));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_stop:area4-5")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .on_line_section("line:A", "stop_area:4", "stop_area:5", {"line:A:0"}, *b.data->pt_data)
            .application_periods(btp("20160404T000000"_dt, "20160406T103500"_dt))
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    // Only the first 2 vj should be impacted since vj:3 starts on the 6th but pass at stop_area:4 at 10:40am
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 5);

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_stop:area3-4")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .on_line_section("line:A", "stop_area:3", "stop_area:4", {"line:A:0"}, *b.data->pt_data)
            .application_periods(btp("20160405T090000"_dt, "20160407T083500"_dt))
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 8);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "110000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:2");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "101000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "101110"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:3");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "001000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "001100"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_stop:area4-5");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000011"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:1:line_section_on_line:A_stop:area3-4");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "001000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:3:Adapted:0:line_section_on_line:A_stop:area3-4");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);

    vj = b.get<nt::VehicleJourney>(
        "vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_stop:area4-5:Adapted:2:line_section_on_line:A_stop:"
        "area3-4");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);

    vj = b.get<nt::VehicleJourney>(
        "vehicle_journey:vj:2:Adapted:0:line_section_on_line:A_stop:area4-5:Adapted:1:line_section_on_line:A_stop:"
        "area3-4");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000110"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
}

/* Update the message of an impact
 *
 * */
BOOST_AUTO_TEST_CASE(update_impact) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.vj("A").name("vj:1")("stop1", "08:00"_t)("stop2", "08:15"_t)("stop3",
                                                                       "08:45"_t)  // <- Only stop3 will be impacted
            ("stop4", "09:00"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2012, 6, 14), bg::days(7));

    auto original_disruption_text = "message disruption 1";
    auto disruption_message_1 = navitia::type::disruption::Message{original_disruption_text, "", "", "", {}, {}, {}};
    const auto& disruption_1 = b.impact(nt::RTLevel::Adapted, "stop3_closed")
                                   .severity(nt::disruption::Effect::NO_SERVICE)
                                   .msg(disruption_message_1)
                                   .on(nt::Type_e::StopPoint, "stop3", *b.data->pt_data)
                                   // 2012/6/14 7h -> 2012/6/17 6h
                                   .application_periods(btp("20120614T070000"_dt, "20120617T060000"_dt))
                                   // 2012/6/17 23h -> 2012/6/18 6h
                                   .application_periods(btp("20120617T230000"_dt, "20120618T060000"_dt))
                                   // 2012/6/19 4h -> 2012/6/19 8h30
                                   .application_periods(btp("20120619T083000"_dt, "20120619T093000"_dt))
                                   .get_disruption();

    const auto& disruption_2 = b.impact(nt::RTLevel::Adapted, "stop2_closed")
                                   .severity(nt::disruption::Effect::NO_SERVICE)
                                   .on(nt::Type_e::StopPoint, "stop2", *b.data->pt_data)
                                   // 2012/6/14 7h -> 2012/6/17 6h
                                   .application_periods(btp("20120614T070000"_dt, "20120617T060000"_dt))
                                   // 2012/6/17 23h -> 2012/6/18 6h
                                   .application_periods(btp("20120617T230000"_dt, "20120618T060000"_dt))
                                   // 2012/6/19 4h -> 2012/6/19 8h30
                                   .application_periods(btp("20120619T083000"_dt, "20120619T093000"_dt))
                                   .get_disruption();

    navitia::apply_disruption(disruption_1, *b.data->pt_data, *b.data->meta);
    navitia::apply_disruption(disruption_2, *b.data->pt_data, *b.data->meta);

    const auto* disruption = b.data->pt_data->disruption_holder.get_disruption("stop3_closed");
    BOOST_REQUIRE_EQUAL(disruption->get_impacts()[0]->messages[0].text, original_disruption_text);

    disruption_1.get_impacts()[0]->messages[0].text = "message disruption updated";

    // update the disruption_1 with the new message
    navitia::apply_disruption(disruption_1, *b.data->pt_data, *b.data->meta);
    navitia::apply_disruption(disruption_1, *b.data->pt_data, *b.data->meta);
    navitia::apply_disruption(disruption_1, *b.data->pt_data, *b.data->meta);

    disruption = b.data->pt_data->disruption_holder.get_disruption("stop3_closed");
    BOOST_REQUIRE_EQUAL(disruption->get_impacts()[0]->messages[0].text, "message disruption updated");

    // delete disruption
    navitia::delete_disruption("stop3_closed", *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
}

BOOST_AUTO_TEST_CASE(impact_with_boarding_alighting_times) {
    ed::builder b("20170101", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");

        b.vj("line:A", "1111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t,
                                                    std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
            "stop_point:20", "08:20"_t, "08:21"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)(
            "stop_point:30", "08:30"_t, "08:31"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 0)(
            "stop_point:40", "08:40"_t, "08:41"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);
    });
    b.data->meta->production_date = bg::date_period(bg::date(2017, 1, 1), bg::days(7));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .on_line_section("line:A", "stop_area:2", "stop_area:3", {"line:A:0"}, *b.data->pt_data)
            .application_periods(btp("20170101T120000"_dt, "20170131T000000"_dt))
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "0000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "1111111"), base_vp);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().departure_time, "08:11"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().boarding_time, "08:06"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "stop_point:20");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).stop_point->uri, "stop_point:30");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().arrival_time, "08:40"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().alighting_time, "08:45"_t);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_diverted");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "1111110"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "0000000"), base_vp);
    // The adapted vj should have only 2 stop_times and kept the boarding_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().departure_time, "08:11"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().boarding_time, "08:06"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().arrival_time, "08:40"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().alighting_time, "08:45"_t);
}

BOOST_AUTO_TEST_CASE(impact_lollipop_with_boarding_alighting_times) {
    ed::builder b("20170101", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");

        b.vj("line:A", "1111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t,
                                                    std::numeric_limits<uint16_t>::max(), false, true, 0, 300)(
            "stop_point:20", "08:20"_t, "08:21"_t, std::numeric_limits<uint16_t>::max(), true, true, 1200, 1200)(
            "stop_point:30", "08:30"_t, "08:31"_t, std::numeric_limits<uint16_t>::max(), true, true, 300, 300)(
            "stop_point:10", "08:40"_t, "08:41"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);
    });
    b.data->meta->production_date = bg::date_period(bg::date(2017, 1, 1), bg::days(7));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .on_line_section("line:A", "stop_area:3", "stop_area:3", {"line:A:0"}, *b.data->pt_data)
            .application_periods(btp("20170101T120000"_dt, "20170131T000000"_dt))
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "0000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "1111111"), base_vp);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(0).stop_point->idx, vj->stop_time_list.at(3).stop_point->idx);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(0).boarding_time, "08:06"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(0).alighting_time, "08:10"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).boarding_time, "08:01"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).alighting_time, "08:40"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).boarding_time, "08:26"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).alighting_time, "08:35"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(3).boarding_time, "08:41"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(3).alighting_time, "08:45"_t);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_diverted");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "1111110"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "0000000"), base_vp);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(0).boarding_time, "08:06"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(0).alighting_time, "08:10"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).boarding_time, "08:01"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).alighting_time, "08:40"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).boarding_time, "08:41"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(2).alighting_time, "08:45"_t);
}

/*
 * When applying a delay message on a line, it does nothing
 * (as we do not have enough information to delay each vehicle_journeys)
 */
BOOST_AUTO_TEST_CASE(test_delay_on_line_does_nothing) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.vj("A").name("vj:1")("stop1", "23:00"_t)("stop2", "24:15"_t)("stop3", "25:00"_t);
    });

    b.data->meta->production_date = bg::date_period("20120614"_d, 7_days);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);

    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "lineA_delayed")
                                  .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
                                  .on(nt::Type_e::Line, "A", *b.data->pt_data)
                                  .application_periods(btp("20120617T2200"_dt, "20120618T0000"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ
    // no message on the metavj or vj
    BOOST_CHECK_EQUAL(b.get<nt::VehicleJourney>("vehicle_journey:vj:1")->get_impacts().size(), 0);
    BOOST_CHECK_EQUAL(b.get<nt::VehicleJourney>("vehicle_journey:vj:1")->meta_vj->get_impacts().size(), 0);
}

/**
 *
 * vehicle_journeys are re-indexed after applying disruptions and their indexes must be unique and continuous
 */
BOOST_AUTO_TEST_CASE(test_indexes_after_applying_disruption) {
    using btp = boost::posix_time::time_period;

    ed::builder b("20170101", [](ed::builder& b) {
        b.vj("l1").name("vj:0")("A", "08:10"_t)("B", "08:20"_t);
        b.vj("l1").name("vj:1")("A", "09:10"_t)("B", "09:20"_t);
        b.vj("l1").name("vj:2")("A", "10:10"_t)("B", "10:20"_t)("C", "10:30"_t)("D", "10:40"_t);
        b.vj("l1").name("vj:3")("A", "11:10"_t)("B", "11:20"_t)("C", "11:30"_t)("D", "11:40"_t)("E", "11:50"_t)(
            "F", "11:55"_t);
        const auto it1 = b.sas.find("A");
        b.data->pt_data->routes.front()->destination = it1->second;
    });

    // Only appliable on vj:2
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Disruption_C")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "C", *b.data->pt_data)
                                  .application_periods(btp("20170101T100000"_dt, "20170101T110000"_dt))
                                  .publish(btp("20170101T100000"_dt, "20170101T110000"_dt))
                                  .msg("Disruption on stop_point C")
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 5);

    // Only appliable on vj:3
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Disruption_C1")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "C", *b.data->pt_data)
                                  .application_periods(btp("20170101T110000"_dt, "20170101T235900"_dt))
                                  .publish(btp("20170101T110000"_dt, "20170101T235900"_dt))
                                  .msg("Disruption on stop_point C")
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 6);

    // Only appliable on vj:2, and will trigger the cleaning up of vj and the re-indexing of vj
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Disruption_D")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "D", *b.data->pt_data)
                                  .application_periods(btp("20170101T100000"_dt, "20170101T110000"_dt))
                                  .publish(btp("20170101T100000"_dt, "20170101T110000"_dt))
                                  .msg("Disruption on stop_point D")
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    b.data->build_raptor();

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 6);

    test_vjs_indexes(b.data->pt_data->vehicle_journeys);

    navitia::PbCreator pb_creator(b.data.get(), bt::second_clock::universal_time(), null_time_period);

    pb_creator.pb_fill(b.data->pt_data->vehicle_journeys, 3);

    auto resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys_size(), 6);

    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys(0).uri(), "vehicle_journey:vj:0");
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys(1).uri(), "vehicle_journey:vj:1");
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys(2).uri(), "vehicle_journey:vj:2");
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys(3).uri(), "vehicle_journey:vj:3");
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys(4).uri(), "vehicle_journey:vj:3:Adapted:0:Disruption_C1");
    BOOST_REQUIRE_EQUAL(resp.vehicle_journeys(5).uri(), "vehicle_journey:vj:2:Adapted:1:Disruption_C:Disruption_D");
}

BOOST_AUTO_TEST_CASE(significant_delay_on_stop_point_dont_remove_it) {
    ed::builder b("20170101", [](ed::builder& b) { b.vj("l1")("A", "08:10"_t)("B", "08:20"_t); });

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "B_delayed")
                                  .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
                                  .on(nt::Type_e::StopPoint, "B", *b.data->pt_data)
                                  .application_periods(btp("20170101T000000"_dt, "20170120T1120000"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    b.data->build_raptor();

    auto res = compute(*b.data, nt::RTLevel::Adapted, "A", "B", "08:00"_t, 0);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res[0].items[0].stop_times.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_realtime_disruption_on_line_then_stop_point) {
    nt::VehicleJourney *vj1, *vj2;
    ed::builder b("20120614", [&](ed::builder& b) {
        vj1 = b.vj("A").name("vj:1")("stopA1", "10:00"_t)("stopA2", "11:00"_t)("stopA3", "12:00"_t).make();
        vj2 = b.vj("B").name("vj:2")("stop1B", "10:00"_t)("stopB2", "11:00"_t)("stopB3", "12:00"_t).make();
    });

    b.data->meta->production_date = bg::date_period("20120614"_d, 7_days);

    /*
     * For VJ1
     *
     * Let's apply 2 disruption on 2 objects types (line then stop point)
     * Then, we make sure that the real time validity pattern has been disabled
     *
     */
    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "Line A : penguins on the line")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::Line, "A", *b.data->pt_data)
                                  .application_periods(btp("20120617T1000"_dt, "20120617T1200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "Fire at Montparnasse")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stopA2", *b.data->pt_data)
                                  .application_periods(btp("20120617T1000"_dt, "20120617T1200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "11110111"),
                        vj1->rt_validity_pattern()->days);

    /*
     * For VJ2
     *
     * Let's apply only 1 disruption on the same objects types (line then stop point)
     * Then, we make sure that the real time validity pattern has been disabled
     *
     */
    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "Penguins on fire at Montparnasse")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::Line, "B", *b.data->pt_data)
                                  .on(nt::Type_e::StopPoint, "stopB2", *b.data->pt_data)
                                  .application_periods(btp("20120617T1000"_dt, "20120617T1200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj2->rt_validity_pattern()->days.to_string(), "11110111"),
                        vj1->rt_validity_pattern()->days);
}

BOOST_AUTO_TEST_CASE(test_adapted_disruptions_on_stop_point_then_line) {
    nt::VehicleJourney* vj1 = nullptr;
    ed::builder b("20120614", [&](ed::builder& b) {
        vj1 = b.vj("A").name("vj:1")("stopA1", "10:00"_t)("stopA2", "11:00"_t)("stopA3", "12:00"_t).make();
    });

    b.data->meta->production_date = bg::date_period("20120614"_d, 7_days);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Penguins on fire at Montparnasse")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stopA2", *b.data->pt_data)
                                  .application_periods(btp("20120617T1000"_dt, "20120617T1200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->base_validity_pattern()->days.to_string(), "11111111"),
                        vj1->rt_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->adapted_validity_pattern()->days.to_string(), "11110111"),
                        vj1->adapted_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "11110111"),
                        vj1->rt_validity_pattern()->days);

    auto rt_vj1 =
        b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1:Adapted:0:Penguins on fire at Montparnasse"];

    BOOST_CHECK_MESSAGE(ba::ends_with(rt_vj1->base_validity_pattern()->days.to_string(), "00000000"),
                        rt_vj1->rt_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(rt_vj1->adapted_validity_pattern()->days.to_string(), "00001000"),
                        rt_vj1->rt_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(rt_vj1->rt_validity_pattern()->days.to_string(), "00001000"),
                        rt_vj1->rt_validity_pattern()->days);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "coffee spilled on the line")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::Line, "A", *b.data->pt_data)
                                  .application_periods(btp("20120617T1000"_dt, "20120617T1200"_dt))
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(rt_vj1->base_validity_pattern()->days.to_string(), "00000000"),
                        rt_vj1->rt_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(rt_vj1->adapted_validity_pattern()->days.to_string(), "00000000"),
                        rt_vj1->rt_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(rt_vj1->rt_validity_pattern()->days.to_string(), "00000000"),
                        rt_vj1->rt_validity_pattern()->days);
}

// We check that with a disruption with two impacts impacting the same vj the update is working.
// It was failing before since after deleting each impact, we were reapplying the disruption.
BOOST_AUTO_TEST_CASE(update_disruption_with_multiple_impact_on_same_vj) {
    ed::builder b("20190301", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");
        b.sa("stop_area:5", 0, 0, false, true)("stop_point:50");

        b.vj("line:A", "1111111", "", true, "vj:1")("stop_point:10", "10:00"_t)("stop_point:20", "11:00"_t)(
            "stop_point:30", "12:00"_t)("stop_point:40", "13:00"_t)("stop_point:50", "14:00"_t);
    });

    b.data->meta->production_date = bg::date_period("20190301"_d, 7_days);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 0);

    auto dis_builder = b.disrupt(nt::RTLevel::Adapted, "line_section_on_line:A_two_stops_in_two_impacts");
    dis_builder.impact()
        .severity(nt::disruption::Effect::NO_SERVICE)
        .application_periods(btp("20190301T000000"_dt, "20190305T000000"_dt))
        .on_line_section("line:A", "stop_area:2", "stop_area:2", {"line:A:0"}, *b.data->pt_data);

    dis_builder.impact()
        .severity(nt::disruption::Effect::NO_SERVICE)
        .application_periods(btp("20190301T000000"_dt, "20190305T000000"_dt))
        .on_line_section("line:A", "stop_area:4", "stop_area:4", {"line:A:0"}, *b.data->pt_data);

    navitia::apply_disruption(dis_builder.disruption, *b.data->pt_data, *b.data->meta);

    // There should be only one more vj
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    // And the base vj:1 should have 2 impacts
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 2);

    const auto* disruption =
        b.data->pt_data->disruption_holder.get_disruption("line_section_on_line:A_two_stops_in_two_impacts");
    BOOST_REQUIRE(disruption != nullptr);
    BOOST_REQUIRE_EQUAL(disruption->get_impacts().size(), 2);

    navitia::delete_disruption("line_section_on_line:A_two_stops_in_two_impacts", *b.data->pt_data, *b.data->meta);

    // Once again one vj after deletion. This was failing, we were reapplying the disruption and an adapted vj was kept
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
    // No more impacts
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 0);

    // Can't reuse the same disruption since the memory has been freed.
    auto dis_builder_update = b.disrupt(nt::RTLevel::Adapted, "line_section_on_line:A_two_stops_in_two_impacts");
    dis_builder_update.impact()
        .severity(nt::disruption::Effect::NO_SERVICE)
        .application_periods(btp("20190301T000000"_dt, "20190305T000000"_dt))
        .on_line_section("line:A", "stop_area:2", "stop_area:2", {"line:A:0"}, *b.data->pt_data);

    dis_builder_update.impact()
        .severity(nt::disruption::Effect::NO_SERVICE)
        .application_periods(btp("20190301T000000"_dt, "20190305T000000"_dt))
        .on_line_section("line:A", "stop_area:4", "stop_area:4", {"line:A:0"}, *b.data->pt_data);

    navitia::apply_disruption(dis_builder_update.disruption, *b.data->pt_data, *b.data->meta);

    // There should be only one more vj again
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    // This was failing too since the impact was not able to be applied again. We were getting 0.
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 2);

    disruption = b.data->pt_data->disruption_holder.get_disruption("line_section_on_line:A_two_stops_in_two_impacts");
    BOOST_REQUIRE(disruption != nullptr);
    BOOST_REQUIRE_EQUAL(disruption->get_impacts().size(), 2);

    navitia::delete_disruption("line_section_on_line:A_two_stops_in_two_impacts", *b.data->pt_data, *b.data->meta);

    // Once again one vj after deletion. This was failing, we were reapplying the disruption
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 0);
}

// Same kind of test but applying to two differents VJ
BOOST_AUTO_TEST_CASE(update_disruption_with_multiple_impact_on_different_vj) {
    ed::builder b("20190301", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");
        b.sa("stop_area:5", 0, 0, false, true)("stop_point:50");

        b.vj("line:A", "1111111", "", true, "vj:1")("stop_point:10", "10:00"_t)("stop_point:20", "11:00"_t)(
            "stop_point:30", "12:00"_t)("stop_point:40", "13:00"_t)("stop_point:50", "14:00"_t);

        b.vj("line:A", "1111111", "", true, "vj:2")
            .route("line:A:1")("stop_point:50", "10:00"_t)("stop_point:40", "11:00"_t)("stop_point:30", "12:00"_t)(
                "stop_point:20", "13:00"_t)("stop_point:10", "14:00"_t);
    });

    b.data->meta->production_date = bg::date_period("20190301"_d, 7_days);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:2")->get_impacts().size(), 0);

    auto dis_builder = b.disrupt(nt::RTLevel::Adapted, "line_section_on_line:A_section_in_each_way");
    dis_builder.impact()
        .severity(nt::disruption::Effect::NO_SERVICE)
        .application_periods(btp("20190301T000000"_dt, "20190305T000000"_dt))
        .on_line_section("line:A", "stop_area:2", "stop_area:4", {"line:A:0"}, *b.data->pt_data);

    dis_builder.impact()
        .severity(nt::disruption::Effect::NO_SERVICE)
        .application_periods(btp("20190301T000000"_dt, "20190305T000000"_dt))
        .on_line_section("line:A", "stop_area:4", "stop_area:2", {"line:A:1"}, *b.data->pt_data);

    navitia::apply_disruption(dis_builder.disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:2")->get_impacts().size(), 1);

    const auto* disruption =
        b.data->pt_data->disruption_holder.get_disruption("line_section_on_line:A_section_in_each_way");
    BOOST_REQUIRE(disruption != nullptr);
    BOOST_REQUIRE_EQUAL(disruption->get_impacts().size(), 2);

    navitia::delete_disruption("line_section_on_line:A_section_in_each_way", *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:2")->get_impacts().size(), 0);

    // Can't reuse the same disruption since the memory has been freed.
    auto dis_builder_update = b.disrupt(nt::RTLevel::Adapted, "line_section_on_line:A_section_in_each_way");
    dis_builder_update.impact()
        .severity(nt::disruption::Effect::NO_SERVICE)
        .application_periods(btp("20190301T000000"_dt, "20190305T000000"_dt))
        .on_line_section("line:A", "stop_area:2", "stop_area:4", {"line:A:0"}, *b.data->pt_data);

    dis_builder_update.impact()
        .severity(nt::disruption::Effect::NO_SERVICE)
        .application_periods(btp("20190301T000000"_dt, "20190305T000000"_dt))
        .on_line_section("line:A", "stop_area:4", "stop_area:2", {"line:A:1"}, *b.data->pt_data);

    navitia::apply_disruption(dis_builder_update.disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:2")->get_impacts().size(), 1);

    disruption = b.data->pt_data->disruption_holder.get_disruption("line_section_on_line:A_section_in_each_way");
    BOOST_REQUIRE(disruption != nullptr);
    BOOST_REQUIRE_EQUAL(disruption->get_impacts().size(), 2);

    navitia::delete_disruption("line_section_on_line:A_section_in_each_way", *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:1")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:2")->get_impacts().size(), 0);
}

/*
 * The real life use case associated with this test is when we have a line ending
 * with two stop_points in the same stop_area (mainly we have a drop off stop and
 * a different parking stop just after).
 *
 * Route :
 *
 * sp:10 ---> sp:20 ---> sp:30 ---> sp:31 ---> sp:40 ---> sp:50 ---> sp:51
 *
 * Each sp:<X><Y> are in the corresponding stop_area sa:<X>.
 *
 * If we a have a line_section impacting sa:3 to sa:5 we need to impact sp:30 to sp:51.
 * Even if we work on the shortest line_section the section is defined by stop_areas.
 */
BOOST_AUTO_TEST_CASE(add_impact_on_line_section_with_successive_stop_in_same_area) {
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30")("stop_point:31");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");
        b.sa("stop_area:5", 0, 0, false, true)("stop_point:50")("stop_point:51");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)(
            "stop_point:20", "08:20"_t, "08:21"_t)("stop_point:30", "08:30"_t, "08:31"_t)(
            "stop_point:31", "08:31"_t, "08:32"_t)("stop_point:40", "08:40"_t, "08:41"_t)(
            "stop_point:50", "08:50"_t, "08:51"_t)("stop_point:51", "08:51"_t, "08:52"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20190901T120000"_dt, "20190910T090000"_dt))
            .on_line_section("line:A", "stop_area:3", "stop_area:5", {"line:A:0"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_diverted");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:20");
}

/*
 * When we have a vj A/B/C/B/D :
 *
 *                D
 *                ^
 *                |
 *                |
 * A -----------> B -----------> C
 *                |              |
 *                |              |
 *                '--------------'
 *
 * If we impact B/C, since we want the smallest sections, we need to have an adapted vj A/B/D
 * We have B since the second passage through B should not be impacted
 */
BOOST_AUTO_TEST_CASE(add_impact_lollipop_vj_impact_stop_only_once) {
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)(
            "stop_point:20", "08:20"_t, "08:21"_t)("stop_point:30", "08:30"_t, "08:31"_t)(
            "stop_point:20", "08:40"_t, "08:41"_t)("stop_point:40", "08:50"_t, "08:51"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20190901T120000"_dt, "20190910T090000"_dt))
            .on_line_section("line:A", "stop_area:2", "stop_area:3", {"line:A:0"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_diverted");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have 3 stop_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "stop_point:20");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).departure_time, "08:41"_t);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
}

// Show a limitation of the current algorithm
// If we impact a non-base vehicle_journey we need to find the base stop_time associated
// to each stop_time of the vj to be able to know if its origin rank is impacted or not.
// Since get_base_stop_time is not perfect (we only check stop_point and departure_time)
// there is some (dumb) cases where we impact some stops we should not.
BOOST_AUTO_TEST_CASE(limitation_impact_repeated_stop_points_same_stop_time) {
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");
        b.sa("stop_area:3", 0, 0, false, true)("stop_point:30");
        b.sa("stop_area:4", 0, 0, false, true)("stop_point:40");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)(
            "stop_point:20", "08:10"_t, "08:11"_t)("stop_point:30", "08:10"_t, "08:11"_t)(
            "stop_point:20", "08:10"_t, "08:11"_t)("stop_point:40", "08:10"_t, "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    // Impact from 2 to 3 once
    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20190901T120000"_dt, "20190910T090000"_dt))
            .on_line_section("line:A", "stop_area:2", "stop_area:3", {"line:A:0"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_diverted");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have 3 stop_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.at(1).stop_point->uri, "stop_point:20");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");

    // And a second time on a slightly different calendar
    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "line_section_on_line:A_diverted_twice")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20190901T000000"_dt, "20190910T090000"_dt))
            .on_line_section("line:A", "stop_area:2", "stop_area:3", {"line:A:0"}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);

    // Check the original vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    for (auto* vj : b.data->pt_data->vehicle_journeys) {
        std::cerr << vj->uri << std::endl;
    }

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>(
        "vehicle_journey:vj:1:Adapted:0:line_section_on_line:A_diverted:Adapted:2:line_section_on_line:A_diverted_"
        "twice");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have 3 stop_times
    // But since the previous vj is adapted we must find the base stop time associated to the adapted
    // one to impact it.
    // However each stop_time have the same departure_time, so we are finding the base stop_time
    // of rank 1 for the stop_time initially of rank 3 (both are on stop_point:20).
    // We are lead to believe it is impacted even if it is not in reality.
    // Nothing we can do really since there is no hard link between the adapted stop_time and base one
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);  // Should be 3 [10, 20, 40]
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.front().stop_point->uri, "stop_point:10");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.back().stop_point->uri, "stop_point:40");
}

BOOST_AUTO_TEST_CASE(impact_with_same_start_date_application_patterns) {
    /*
     *                              20190901                                        20190910    20190912
     application pattern 1              |----------------------------------------------|
     application pattern 2              |----------------------------------------------------------|
     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "09:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190901"_d, "20190912"_d);
    application_pattern2.add_time_slot("15:00"_t, "17:00"_t);
    application_pattern2.week_pattern[navitia::Friday] = true;
    application_pattern2.week_pattern[navitia::Saturday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T090000"_dt))
                                  .application_periods(btp("20190901T150000"_dt, "20190912T170000"_dt))
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ
    BOOST_REQUIRE_EQUAL(b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.size(), 2);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.begin(),
        "20190901"_d);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.end(),
        "20190910"_d);
}

BOOST_AUTO_TEST_CASE(impact_with_same_end_date_application_patterns) {
    /*
     *                          20190815   20190901                                        20190910
     application pattern 1                     |----------------------------------------------|
     application pattern 2          |---------------------------------------------------------|
     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "09:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190815"_d, "20190910"_d);
    application_pattern2.add_time_slot("15:00"_t, "17:00"_t);
    application_pattern2.week_pattern[navitia::Friday] = true;
    application_pattern2.week_pattern[navitia::Sunday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T090000"_dt))
                                  .application_periods(btp("20190815T150000"_dt, "20190910T170000"_dt))
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ
    BOOST_REQUIRE_EQUAL(b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.size(), 2);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.begin(),
        "20190815"_d);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.end(),
        "20190910"_d);
}

BOOST_AUTO_TEST_CASE(impact_with_same_start_date_and_end_date_application_patterns) {
    // Same begin application pattrens
    /*
     *                        20190901                                        20190910
     application pattern 1      |----------------------------------------------|
     application pattern 2      |----------------------------------------------|

                                    12:00       13:00       15:00       17:00
                    time_slot 1        |----------|
                    time_slot 2                                 |----------|
     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "13:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern2.add_time_slot("15:00"_t, "17:00"_t);
    application_pattern2.week_pattern[navitia::Friday] = true;
    application_pattern2.week_pattern[navitia::Saturday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T130000"_dt))
                                  .application_periods(btp("20190815T150000"_dt, "20190910T170000"_dt))
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ
    BOOST_REQUIRE_EQUAL(b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.size(), 2);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.begin(),
        "20190901"_d);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.end(),
        "20190910"_d);

    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->time_slots.begin()->begin,
        "12:00"_t);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->time_slots.begin()->end, "13:00"_t);
}

BOOST_AUTO_TEST_CASE(impact_with_same_start_date_and_end_date_and_start_time_application_patterns) {
    /*
     *                        20190901                                        20190910
     application pattern 1      |----------------------------------------------|
     application pattern 2      |----------------------------------------------|

                                    12:00       13:00            17:00
                    time_slot 1        |----------|
                    time_slot 2        |----------------------------|
     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "13:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern2.add_time_slot("12:00"_t, "17:00"_t);
    application_pattern2.week_pattern[navitia::Friday] = true;
    application_pattern2.week_pattern[navitia::Saturday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T130000"_dt))
                                  .application_periods(btp("20190801T120000"_dt, "20190910T170000"_dt))
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ
    BOOST_REQUIRE_EQUAL(b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.size(), 2);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.begin(),
        "20190901"_d);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.end(),
        "20190910"_d);

    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->time_slots.begin()->begin,
        "12:00"_t);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->time_slots.begin()->end, "13:00"_t);
}

BOOST_AUTO_TEST_CASE(impact_with_same_start_date_and_end_date_and_start_and_end_time_application_patterns) {
    /*
     *                        20190901                                        20190910
     application pattern 1      |----------------------------------------------|
     application pattern 2      |----------------------------------------------|

                                    12:00                         17:00
                    time_slot 1        |----------------------------|
                    time_slot 2        |----------------------------|
     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "17:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern2.add_time_slot("12:00"_t, "17:00"_t);
    application_pattern2.week_pattern[navitia::Sunday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T170000"_dt))
                                  .application_periods(btp("20190901T120000"_dt, "20190910T170000"_dt))
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ
    BOOST_REQUIRE_EQUAL(b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.size(), 2);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.begin(),
        "20190901"_d);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->application_period.end(),
        "20190910"_d);

    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->time_slots.begin()->begin,
        "12:00"_t);
    BOOST_REQUIRE_EQUAL(
        b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns.begin()->time_slots.begin()->end, "17:00"_t);
}

BOOST_AUTO_TEST_CASE(impact_with_2_timeslots) {
    /*
     *                        20190901                                        20190910
     application pattern 1      |----------------------------------------------|
                                    12:00         13:00
                    time_slot 1        |------------|
                                                          15:00          18:00
                    time_slot 2                             |--------------|

     application pattern 2      |----------------------------------------------|
                                    12:00         13:00
                    time_slot 1        |------------|
                                                          15:00          18:00
                    time_slot 2                             |--------------|

     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "13:00"_t);
    application_pattern1.add_time_slot("15:00"_t, "18:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern2.add_time_slot("12:00"_t, "13:00"_t);
    application_pattern2.add_time_slot("15:00"_t, "18:00"_t);
    application_pattern2.week_pattern[navitia::Sunday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T170000"_dt))
                                  .application_periods(btp("20190901T120000"_dt, "20190910T170000"_dt))
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ

    const auto first_impact = b.get<nt::Line>("line:A")->get_impacts()[0];
    BOOST_REQUIRE_EQUAL(first_impact->application_patterns.size(), 2);

    const auto first_application_pattern = first_impact->application_patterns.begin();
    BOOST_REQUIRE_EQUAL(first_application_pattern->application_period.begin(), "20190901"_d);
    BOOST_REQUIRE_EQUAL(first_application_pattern->application_period.end(), "20190910"_d);

    BOOST_REQUIRE_EQUAL(first_application_pattern->time_slots.begin()->begin, "12:00"_t);
    BOOST_REQUIRE_EQUAL(first_application_pattern->time_slots.begin()->end, "13:00"_t);

    BOOST_REQUIRE_EQUAL(first_application_pattern->week_pattern[navitia::Monday], false);
    BOOST_REQUIRE_EQUAL(first_application_pattern->week_pattern[navitia::Tuesday], false);
    BOOST_REQUIRE_EQUAL(first_application_pattern->week_pattern[navitia::Wednesday], false);
    BOOST_REQUIRE_EQUAL(first_application_pattern->week_pattern[navitia::Thursday], false);
    BOOST_REQUIRE_EQUAL(first_application_pattern->week_pattern[navitia::Friday], true);
    BOOST_REQUIRE_EQUAL(first_application_pattern->week_pattern[navitia::Saturday], true);
    BOOST_REQUIRE_EQUAL(first_application_pattern->week_pattern[navitia::Sunday], false);
}

BOOST_AUTO_TEST_CASE(impact_with_2_timeslots_compare_timeslots) {
    /*
     *                        20190901                                        20190910
     application pattern 1      |----------------------------------------------|
                                    12:00         13:00
                    time_slot 1        |------------|
                                                          15:00          18:00
                    time_slot 2                             |--------------|

     application pattern 2      |----------------------------------------------|
                                    12:00         13:00
                    time_slot 1        |------------|
                                                       14:00          16:00
                    time_slot 2                          |--------------|

     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "13:00"_t);
    application_pattern1.add_time_slot("15:00"_t, "18:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern2.add_time_slot("12:00"_t, "13:00"_t);
    application_pattern2.add_time_slot("14:00"_t, "16:00"_t);
    application_pattern2.week_pattern[navitia::Sunday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T170000"_dt))
                                  .application_periods(btp("20190901T120000"_dt, "20190910T170000"_dt))
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ

    const auto first_impact = b.get<nt::Line>("line:A")->get_impacts()[0];
    BOOST_REQUIRE_EQUAL(first_impact->application_patterns.size(), 2);

    const auto first_application_pattern = first_impact->application_patterns.begin();
    BOOST_REQUIRE_EQUAL(first_application_pattern->application_period.begin(), "20190901"_d);
    BOOST_REQUIRE_EQUAL(first_application_pattern->application_period.end(), "20190910"_d);

    BOOST_REQUIRE_EQUAL(first_application_pattern->time_slots.size(), 2);

    auto ts = next(first_application_pattern->time_slots.begin(), 0);
    BOOST_REQUIRE_EQUAL(ts->begin, "12:00"_t);
    BOOST_REQUIRE_EQUAL(ts->end, "13:00"_t);

    ts = next(first_application_pattern->time_slots.begin(), 1);
    BOOST_REQUIRE_EQUAL(ts->begin, "14:00"_t);
    BOOST_REQUIRE_EQUAL(ts->end, "16:00"_t);
}

BOOST_AUTO_TEST_CASE(impact_with_same_start_and_end_time_application_patterns) {
    /*
     *                        20190901                                        20190910
     application pattern 1      |----------------------------------------------|

                                    12:00              15:00      17:00      19:00
                    time_slot 1        |-----------------|
                    time_slot 2        |----------------------------|
                    time_slot 3        |---------------------------------------|
     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "19:00"_t);
    application_pattern1.add_time_slot("12:00"_t, "15:00"_t);
    application_pattern1.add_time_slot("12:00"_t, "17:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T190000"_dt))
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ
    auto application_patterns = b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns;
    BOOST_REQUIRE_EQUAL(application_patterns.size(), 1);
    BOOST_REQUIRE_EQUAL(application_patterns.begin()->application_period.begin(), "20190901"_d);
    BOOST_REQUIRE_EQUAL(application_patterns.begin()->application_period.end(), "20190910"_d);
    BOOST_REQUIRE_EQUAL(application_patterns.begin()->time_slots.size(), 3);

    auto time_slot = *application_patterns.begin()->time_slots.begin();
    BOOST_REQUIRE_EQUAL(time_slot.begin, "12:00"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "15:00"_t);

    time_slot = *std::next(application_patterns.begin()->time_slots.begin(), 1);
    BOOST_REQUIRE_EQUAL(time_slot.begin, "12:00"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "17:00"_t);

    time_slot = *std::next(application_patterns.begin()->time_slots.begin(), 2);
    BOOST_REQUIRE_EQUAL(time_slot.begin, "12:00"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "19:00"_t);
}

BOOST_AUTO_TEST_CASE(impact_with_start_and_end_time_application_patterns) {
    /*
     *                        20190901                                        20190910
     application pattern 1      |----------------------------------------------|

                                    12:00    14:00     15:00      17:00      19:00
                    time_slot 1        |-----------------|
                    time_slot 2                |--------------------|
                    time_slot 3        |---------------------------------------|
     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "15:00"_t);
    application_pattern1.add_time_slot("14:00"_t, "17:00"_t);
    application_pattern1.add_time_slot("12:00"_t, "19:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T190000"_dt))
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ
    auto application_patterns = b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns;
    BOOST_REQUIRE_EQUAL(application_patterns.size(), 1);
    BOOST_REQUIRE_EQUAL(application_patterns.begin()->application_period.begin(), "20190901"_d);
    BOOST_REQUIRE_EQUAL(application_patterns.begin()->application_period.end(), "20190910"_d);
    BOOST_REQUIRE_EQUAL(application_patterns.begin()->time_slots.size(), 3);

    auto time_slot = *application_patterns.begin()->time_slots.begin();
    BOOST_REQUIRE_EQUAL(time_slot.begin, "12:00"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "15:00"_t);

    time_slot = *std::next(application_patterns.begin()->time_slots.begin(), 1);
    BOOST_REQUIRE_EQUAL(time_slot.begin, "12:00"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "19:00"_t);

    time_slot = *std::next(application_patterns.begin()->time_slots.begin(), 2);
    BOOST_REQUIRE_EQUAL(time_slot.begin, "14:00"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "17:00"_t);
}

BOOST_AUTO_TEST_CASE(impact_with_application_patterns) {
    /*

                              20190901  20190902    20190903        20190905    20190910    20190920    20190922
     application pattern 1      |---------------------------------------|
     application pattern 2      |---------------------------------------|
     application pattern 3      |---------------------------------------------------|
     application pattern 4                               |--------------------------------------------------|
     application pattern 5                    |--------------------------------------------------|




     TimeSlots                  7h      8h      9h     10h        12h
     application pattern 1              |--------|
     application pattern 2      |----------------|
     application pattern 3              |---------------|
     application pattern 4                       |-----------------|
     application pattern 5              |--------------------------|


                        Result after sort

                application pattern 2
                application pattern 1
                application pattern 3
                application pattern 5
                application pattern 4

      */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190905"_d);
    application_pattern1.add_time_slot("08:00"_t, "09:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190901"_d, "20190905"_d);
    application_pattern2.add_time_slot("07:00"_t, "09:00"_t);
    application_pattern2.week_pattern[navitia::Monday] = true;

    nt::disruption::ApplicationPattern application_pattern3;
    application_pattern3.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern3.add_time_slot("08:00"_t, "10:00"_t);
    application_pattern3.week_pattern[navitia::Thursday] = true;

    nt::disruption::ApplicationPattern application_pattern4;
    application_pattern4.application_period = boost::gregorian::date_period("20190903"_d, "20190922"_d);
    application_pattern4.add_time_slot("09:00"_t, "12:00"_t);
    application_pattern4.week_pattern[navitia::Thursday] = true;

    nt::disruption::ApplicationPattern application_pattern5;
    application_pattern5.application_period = boost::gregorian::date_period("20190902"_d, "20190920"_d);
    application_pattern5.add_time_slot("08:00"_t, "12:00"_t);
    application_pattern5.week_pattern[navitia::Thursday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T070000"_dt, "20190922T120000"_dt))
                                  .application_patterns(application_pattern1)
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern3)
                                  .application_patterns(application_pattern4)
                                  .application_patterns(application_pattern5)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ

    auto application_patterns = b.get<nt::Line>("line:A")->get_impacts()[0]->application_patterns;
    BOOST_REQUIRE_EQUAL(application_patterns.size(), 5);

    // application pattern 2
    auto application_pattern = *application_patterns.begin();
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.begin(), "20190901"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.end(), "20190905"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 1);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->begin, "07:00"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->end, "09:00"_t);

    // application pattern 1
    application_pattern = *std::next(application_patterns.begin(), 1);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.begin(), "20190901"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.end(), "20190905"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 1);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->begin, "08:00"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->end, "09:00"_t);

    // application pattern 3
    application_pattern = *std::next(application_patterns.begin(), 2);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.begin(), "20190901"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.end(), "20190910"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 1);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->begin, "08:00"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->end, "10:00"_t);

    // application pattern 5
    application_pattern = *std::next(application_patterns.begin(), 3);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.begin(), "20190902"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.end(), "20190920"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 1);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->begin, "08:00"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->end, "12:00"_t);

    // application pattern 4
    application_pattern = *std::next(application_patterns.begin(), 4);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.begin(), "20190903"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.application_period.end(), "20190922"_d);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 1);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->begin, "09:00"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.begin()->end, "12:00"_t);
}

BOOST_AUTO_TEST_CASE(impact_with_timeslots_compare_timeslots_diff_size) {
    /*
     *                        20190901                                        20190910
     application pattern 1      |----------------------------------------------|
                                    12:00         13:00
                    time_slot 1        |------------|

     application pattern 2      |----------------------------------------------|
                                    12:00         13:00
                    time_slot 1        |------------|
                                                       14:00          16:00
                    time_slot 2                          |--------------|

     */
    ed::builder b("20190901", [](ed::builder& b) {
        b.sa("stop_area:1", 0, 0, false, true)("stop_point:10");
        b.sa("stop_area:2", 0, 0, false, true)("stop_point:20");

        b.vj("line:A", "111111", "", true, "vj:1")("stop_point:10", "08:10"_t, "08:11"_t)("stop_point:20", "08:10"_t,
                                                                                          "08:11"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2019, 9, 1), bg::days(7));

    using btp = boost::posix_time::time_period;
    nt::disruption::ApplicationPattern application_pattern1;
    application_pattern1.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern1.add_time_slot("12:00"_t, "13:00"_t);
    application_pattern1.week_pattern[navitia::Friday] = true;
    application_pattern1.week_pattern[navitia::Saturday] = true;

    nt::disruption::ApplicationPattern application_pattern2;
    application_pattern2.application_period = boost::gregorian::date_period("20190901"_d, "20190910"_d);
    application_pattern2.add_time_slot("12:00"_t, "13:00"_t);
    application_pattern2.add_time_slot("14:00"_t, "16:00"_t);
    application_pattern2.week_pattern[navitia::Sunday] = true;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "line A")
                                  .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                                  .application_periods(btp("20190901T120000"_dt, "20190910T170000"_dt))
                                  .application_periods(btp("20190901T120000"_dt, "20190910T170000"_dt))
                                  .application_patterns(application_pattern2)
                                  .application_patterns(application_pattern1)
                                  .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.get<nt::Line>("line:A")->get_impacts().size(), 1);  // the impact is linked to the line
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);  // we should not have created a VJ

    const auto first_impact = b.get<nt::Line>("line:A")->get_impacts()[0];
    BOOST_REQUIRE_EQUAL(first_impact->application_patterns.size(), 2);

    const auto first_application_pattern = first_impact->application_patterns.begin();
    BOOST_REQUIRE_EQUAL(first_application_pattern->application_period.begin(), "20190901"_d);
    BOOST_REQUIRE_EQUAL(first_application_pattern->application_period.end(), "20190910"_d);

    BOOST_REQUIRE_EQUAL(first_application_pattern->time_slots.size(), 1);

    auto ts = next(first_application_pattern->time_slots.begin(), 0);
    BOOST_REQUIRE_EQUAL(ts->begin, "12:00"_t);
    BOOST_REQUIRE_EQUAL(ts->end, "13:00"_t);
}

void check_rail_section_impact(const ed::builder& b) {
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 2);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-2");
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1:Adapted:0:rail_section_on_line1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times, for A, B and C stop point
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);
    // stopA
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].pick_up_allowed(), true);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].drop_off_allowed(), false);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].skipped_stop(), false);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].departure_time, "08:00"_t);
    // stopB
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].pick_up_allowed(), true);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].drop_off_allowed(), true);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].skipped_stop(), false);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].departure_time, "08:10"_t);
    // stopC
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].stop_point->uri, "stopC");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].pick_up_allowed(), false);  // pick-up is forbidden now
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].drop_off_allowed(), true);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].skipped_stop(), false);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].departure_time, "08:20"_t);

    // disruption
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 1);
    auto impact = b.get<nt::StopPoint>("stopC")->get_impacts()[0];
    BOOST_REQUIRE_EQUAL(impact->is_rail_section_of(*vj->route->line), true);
    BOOST_REQUIRE_EQUAL(impact->is_only_rail_section(), true);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopF")->get_impacts().size(), 1);

    // Deleting the disruption
    navitia::delete_disruption("rail_section_on_line1", *b.data->pt_data, *b.data->meta);

    // We are now back to the 'normal' state
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_REQUIRE_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_REQUIRE_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
}

BOOST_AUTO_TEST_CASE(classic_impact_with_rail_section) {
    /**
     We receive a disruption with rail_section

                        disruption
                   -------------------
                   |        |        |
     A ------ B ------ C ------ D ------ E ------ F
                   |        |        |
                   -------------------
                  The rail is broken between C and D.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : D
                  blocked stop areas : empty
                  routes             : empty

        NOTE : Select all routes, but only route-1 is impacted because C->D section only
        exists with it.
    */
    {
        // nt::disruption::Effect::NO_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        // new rail_section disruption
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                                      .severity(nt::disruption::Effect::NO_SERVICE)
                                      .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                      .on_rail_section("line:1", "stopAreaC", "stopAreaD", {}, {}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        // Check every relevant fields
        check_rail_section_impact(b);
    }

    {
        // nt::disruption::Effect::REDUCED_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        // new rail_section disruption
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                                      .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                      .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                      .on_rail_section("line:1", "stopAreaC", "stopAreaD", {}, {}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        // Check every relevant fields
        BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 2);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);

        // Check the original vj
        auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1");
        auto adapted_vp = vj->adapted_validity_pattern()->days;
        auto base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
        BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
        vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-2");
        base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

        // Check the adapted vj
        vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1:Adapted:0:rail_section_on_line1");
        adapted_vp = vj->adapted_validity_pattern()->days;
        base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 6);
        // stopA
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].pick_up_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].drop_off_allowed(), false);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].skipped_stop(), false);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].departure_time, "08:00"_t);
        // stopB
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].pick_up_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].drop_off_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].skipped_stop(), false);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].departure_time, "08:10"_t);
        // stopC
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].stop_point->uri, "stopC");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].pick_up_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].drop_off_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].skipped_stop(), false);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].departure_time, "08:20"_t);
        // stopD
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[3].stop_point->uri, "stopD");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[3].pick_up_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[3].drop_off_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[3].skipped_stop(), false);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[3].departure_time, "08:30"_t);
        // stopE
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[4].stop_point->uri, "stopE");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[4].pick_up_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[4].drop_off_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[4].skipped_stop(), false);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[4].departure_time, "08:40"_t);
        // stopF
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[5].stop_point->uri, "stopF");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[5].pick_up_allowed(), false);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[5].drop_off_allowed(), true);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[5].skipped_stop(), false);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[5].departure_time, "08:50"_t);
        // disruption
        BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 0);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 1);
        auto impact = b.get<nt::StopPoint>("stopC")->get_impacts()[0];
        BOOST_REQUIRE_EQUAL(impact->is_rail_section_of(*vj->route->line), true);
        BOOST_REQUIRE_EQUAL(impact->is_only_rail_section(), true);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 0);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopF")->get_impacts().size(), 0);

        // Deleting the disruption
        navitia::delete_disruption("rail_section_on_line1", *b.data->pt_data, *b.data->meta);

        // We are now back to the 'normal' state
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1");
        adapted_vp = vj->adapted_validity_pattern()->days;
        base_vp = vj->base_validity_pattern()->days;
        BOOST_REQUIRE_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
        BOOST_REQUIRE_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    }
    /**
     We receive a disruption with rail_section

                        disruption
                   -------------------
                   |        |        |
     A ------ B ------ C ------ D ------ E ------ F
                   |        |        |
                   -------------------
                  The rail is broken between C and D.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : D
                  blocked stop areas : empty
                  routes             : route:1
    */
    {
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        // new rail_section disruption
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                .on_rail_section("line:1", "stopAreaC", "stopAreaD", {}, {"route-1"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // Check every relevant fields
        check_rail_section_impact(b);
    }
    /**
     We receive a disruption with rail_section

                        disruption
                   -------------------
                   |        |        |
     A ------ B ------ C ------ D ------ E ------ F
                   |        |        |
                   -------------------
                  The rail is broken between C and D.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : D
                  blocked stop areas : empty
                  routes             : route:2

        NOTE : No disruptions are added because the route-2 don't have C->D section (This is D->C)
    */
    {
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        // new rail_section disruption
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                .on_rail_section("line:1", "stopAreaC", "stopAreaD", {}, {"route-2"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // Check every relevant fields
        BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 2);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }
    /**
     We receive a disruption with rail_section

                        disruption
                   ------------------------------------
                   |        |                 |       |
     A ------ B ------ C ------ D ------ E ------ F   |
                   |        |                 |       |
                   ------------------------------------
                  The rail is broken between C and F.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : F
                  blocked stop areas : empty
                  routes             : empty

        NOTE : Select all routes, but nothing is impacted since blocked stop areas is empty
    */
    {
        // NO_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        // new rail_section disruption
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                                      .severity(nt::disruption::Effect::NO_SERVICE)
                                      .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                      .on_rail_section("line:1", "stopAreaC", "stopAreaF", {}, {}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        // Check every relevant fields
        BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 2);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }

    {
        // NO_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        // new rail_section disruption
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                                      .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                      .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                      .on_rail_section("line:1", "stopAreaC", "stopAreaF", {}, {}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        // Check every relevant fields
        BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 2);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(classic_impact_with_long_rail_section) {
    /**
     We receive a disruption with rail_section

                        disruption
                   ------------------------------------
                   |        |                 |       |
     A ------ B ------ C ------ D ------ E ------ F   |
                   |        |                 |       |
                   ------------------------------------
                  The rail is broken between C and F.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : F
                  blocked stop areas : D, E
                  routes             : empty

        NOTE : Select all routes, but only route-1 is impacted because C->F section only
        exists with it.
    */

    ed::builder b("20210101", [](ed::builder& b) {
        b.sa("stopAreaA")("stopA");
        b.sa("stopAreaB")("stopB");
        b.sa("stopAreaC")("stopC");
        b.sa("stopAreaD")("stopD");
        b.sa("stopAreaE")("stopE");
        b.sa("stopAreaF")("stopF");
        b.vj("line:1", "111111", "", true, "vj:A-1")
            .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                "stopE", "08:40"_t)("stopF", "08:50"_t);
        b.vj("line:1", "111111", "", true, "vj:A-2")
            .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                "stopB", "08:40"_t)("stopA", "08:50"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

    using btp = boost::posix_time::time_period;

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

    // new rail_section disruption
    navitia::apply_disruption(
        b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
            .severity(nt::disruption::Effect::NO_SERVICE)
            .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
            .on_rail_section("line:1", "stopAreaC", "stopAreaF",
                             {std::make_pair("stopAreaE", 3), std::make_pair("stopAreaD", 2)}, {}, *b.data->pt_data)
            .get_disruption(),
        *b.data->pt_data, *b.data->meta);

    // Check every relevant fields
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 2);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-2");
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1:Adapted:0:rail_section_on_line1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times, for A, B and C stop point
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].stop_point->uri, "stopC");

    // disruption
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopF")->get_impacts().size(), 1);
}

BOOST_AUTO_TEST_CASE(classic_impact_rail_section_with_start_and_end_sa_in_blocked_sa) {
    /**
     We receive a disruption with rail_section

                        disruption
                   ------------------------------------
                   |        |                 |       |
     A ------ B ------ C ------ D ------ E ------ F   |
                   |        |                 |       |
                   ------------------------------------
                  The rail is broken between C and F.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : F
                  blocked stop areas : C, D, E, F
                  routes             : empty

        NOTE : Select all routes, but only route-1 is impacted because C->F section only
        exists with it.
    */
    {
        // NO_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        // new rail_section disruption
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                                      .severity(nt::disruption::Effect::NO_SERVICE)
                                      .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                      .on_rail_section("line:1", "stopAreaC", "stopAreaF",
                                                       {std::make_pair("stopAreaE", 5), std::make_pair("stopAreaD", 4),
                                                        std::make_pair("stopAreaF", 7), std::make_pair("stopAreaC", 1)},
                                                       {}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        // Check every relevant fields
        BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 2);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);

        // Check the original vj
        auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1");
        auto adapted_vp = vj->adapted_validity_pattern()->days;
        auto base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
        BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
        vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-2");
        base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

        // Check the adapted vj
        vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1:Adapted:0:rail_section_on_line1");
        adapted_vp = vj->adapted_validity_pattern()->days;
        base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
        // The adapted vj should have only 2 stop_times, for A, B and C stop point
        BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].stop_point->uri, "stopC");

        // disruption
        BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 0);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopF")->get_impacts().size(), 1);
    }

    {
        // REDUCED_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

        // new rail_section disruption
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                                      .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                      .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                      .on_rail_section("line:1", "stopAreaC", "stopAreaF",
                                                       {std::make_pair("stopAreaE", 5), std::make_pair("stopAreaD", 4),
                                                        std::make_pair("stopAreaF", 7), std::make_pair("stopAreaC", 1)},
                                                       {}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        // Check every relevant fields
        BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->routes.size(), 2);
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);

        // Check the original vj
        auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1");
        auto adapted_vp = vj->adapted_validity_pattern()->days;
        auto base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
        BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);
        vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-2");
        base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

        // Check the adapted vj
        vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:A-1:Adapted:0:rail_section_on_line1");
        adapted_vp = vj->adapted_validity_pattern()->days;
        base_vp = vj->base_validity_pattern()->days;
        BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
        BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
        // The adapted vj should have only 2 stop_times, for A and B stop point
        BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
        BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");

        // disruption
        BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 0);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 1);
        BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopF")->get_impacts().size(), 1);
    }
}

BOOST_AUTO_TEST_CASE(rail_section_impact_with_wrong_blocked_sa) {
    /**
     We receive a disruption with rail_section

                        disruption
                   ------------------------------------
                   |        |                 |       |
     A ------ B ------ C ------ D ------ E ------ F   |
                   |        |                 |       |
                   ------------------------------------
                  The rail is broken between C and F.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : F
                  blocked stop areas : A, B    <----- Wrong SA !!
                  routes             : empty

        NOTE : There are no impacts because C->A->B->F section no exists
    */
    {
        // NO_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
        // new rail_section disruption
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                .on_rail_section("line:1", "stopAreaC", "stopAreaF",
                                 {std::make_pair("stopAreaA", 2), std::make_pair("stopAreaB", 3)}, {}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // No VJ is added
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }

    {
        // REDUCED_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
        // new rail_section disruption
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                .severity(nt::disruption::Effect::REDUCED_SERVICE)
                .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                .on_rail_section("line:1", "stopAreaC", "stopAreaF",
                                 {std::make_pair("stopAreaA", 2), std::make_pair("stopAreaB", 3)}, {}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // No VJ is added
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(rail_section_impact_with_unknown_blocked_sa) {
    /**
     We receive a disruption with rail_section

                        disruption
                   ------------------------------------
                   |        |                 |       |
     A ------ B ------ C ------ D ------ E ------ F   |
                   |        |                 |       |
                   ------------------------------------
                  The rail is broken between C and F.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : F
                  blocked stop areas : D, Z    <----- Unknown SA !!
                  routes             : empty

        NOTE : There are no impacts because C->D->Z->F section no exists
    */
    {
        // NO_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
        // new rail_section disruption
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                                      .severity(nt::disruption::Effect::NO_SERVICE)
                                      .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                      .on_rail_section("line:1", "stopAreaC", "stopAreaF",
                                                       {std::make_pair("stopAreaZ", 3),  // <---- Unknown SA !!
                                                        std::make_pair("stopAreaD", 2)},
                                                       {}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        // No VJ is added
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }
    {
        // REDUCED_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
        // new rail_section disruption
        navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                                      .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                      .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                      .on_rail_section("line:1", "stopAreaC", "stopAreaF",
                                                       {std::make_pair("stopAreaZ", 3),  // <---- Unknown SA !!
                                                        std::make_pair("stopAreaD", 2)},
                                                       {}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        // No VJ is added
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }
    /**
     We receive a disruption with rail_section

                        disruption
                   ------------------------------------
                   |        |                 |       |
     A ------ B ------ C ------ D ------ E ------ F   |
                   |        |                 |       |
                   ------------------------------------
                  The rail is broken between C and F.
                  Oh my gosh !
                  Trains are stopped at C and the pick-up is forbidden

                  -> chaos fields
                  line               : line:1
                  start_point        : C
                  end_point          : R    <----- Unknown SA !!
                  blocked stop areas : D, E
                  routes             : empty

        NOTE : There are no impacts because C->D->E->R section no exists
    */
    {
        // NO_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.sa("stopAreaR")("stopF");
            b.sa("stopAreaZ")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
        // new rail_section disruption
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                .on_rail_section("line:1", "stopAreaC",
                                 "stopAreaR",  // <----- Unknown SA !!!!
                                 {std::make_pair("stopAreaE", 3), std::make_pair("stopAreaD", 2)}, {}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // No VJ is added
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }
    {
        // REDUCED_SERVICE
        ed::builder b("20210101", [](ed::builder& b) {
            b.sa("stopAreaA")("stopA");
            b.sa("stopAreaB")("stopB");
            b.sa("stopAreaC")("stopC");
            b.sa("stopAreaD")("stopD");
            b.sa("stopAreaE")("stopE");
            b.sa("stopAreaF")("stopF");
            b.sa("stopAreaR")("stopF");
            b.sa("stopAreaZ")("stopF");
            b.vj("line:1", "111111", "", true, "vj:A-1")
                .route("route-1")("stopA", "08:00"_t)("stopB", "08:10"_t)("stopC", "08:20"_t)("stopD", "08:30"_t)(
                    "stopE", "08:40"_t)("stopF", "08:50"_t);
            b.vj("line:1", "111111", "", true, "vj:A-2")
                .route("route-2")("stopF", "08:00"_t)("stopE", "08:10"_t)("stopD", "08:20"_t)("stopC", "08:30"_t)(
                    "stopB", "08:40"_t)("stopA", "08:50"_t);
        });

        b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

        using btp = boost::posix_time::time_period;

        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
        // new rail_section disruption
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "rail_section_on_line1")
                .severity(nt::disruption::Effect::REDUCED_SERVICE)
                .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                .on_rail_section("line:1", "stopAreaC",
                                 "stopAreaR",  // <----- Unknown SA !!!!
                                 {std::make_pair("stopAreaE", 3), std::make_pair("stopAreaD", 2)}, {}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // No VJ is added
        BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    }
}

ed::builder create_complex_data_for_rail_section() {
    /*
     *
     *          |------- J ------ K ------ L --------
     *          |                                   |
     *          |                                   |
     * A x----- B ------ C ------ D ------ E ------ F ------ G ------ H -----x I
     *                   |        |                          |
     *                   |        |                          |
     *                   |        |                          |
     *                   |        |------- M ------ N ------ O
     *                   |
     *                   P ------ Q -----x R
     *
     * route 1 : A B C D E F G H I
     * route 2 : I H G F E D C B A
     * route 3 : A B J K L F G H I
     * route 4 : A B C D M N O G H I
     * route 5 : A B C P Q R
     * route 6 : R Q P C B A
     *
     */
    ed::builder b("20210101", [](ed::builder& b) {
        b.sa("stopAreaA")("stopA");
        b.sa("stopAreaB")("stopB");
        b.sa("stopAreaC")("stopC");
        b.sa("stopAreaD")("stopD");
        b.sa("stopAreaE")("stopE");
        b.sa("stopAreaF")("stopF");
        b.sa("stopAreaG")("stopG");
        b.sa("stopAreaH")("stopH");
        b.sa("stopAreaI")("stopI");
        b.sa("stopAreaJ")("stopJ");
        b.sa("stopAreaK")("stopK");
        b.sa("stopAreaL")("stopL");
        b.sa("stopAreaM")("stopM");
        b.sa("stopAreaN")("stopN");
        b.sa("stopAreaO")("stopO");
        b.sa("stopAreaP")("stopP");
        b.sa("stopAreaQ")("stopQ");
        b.sa("stopAreaR")("stopR");
        b.vj("line:1", "111111", "", true, "vj:1")
            .route("route1")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopD", "08:15"_t)(
                "stopE", "08:20"_t)("stopF", "08:25"_t)("stopG", "08:30"_t)("stopH", "08:35"_t)("stopI", "08:40"_t);
        b.vj("line:1", "111111", "", true, "vj:2")
            .route("route2")("stopI", "08:00"_t)("stopH", "08:05"_t)("stopG", "08:10"_t)("stopF", "08:15"_t)(
                "stopE", "08:20"_t)("stopD", "08:25"_t)("stopC", "08:30"_t)("stopB", "08:35"_t)("stopA", "08:40"_t);
        b.vj("line:1", "111111", "", true, "vj:3")
            .route("route3")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopJ", "08:10"_t)("stopK", "08:15"_t)(
                "stopL", "08:20"_t)("stopF", "08:25"_t)("stopG", "08:30"_t)("stopH", "08:35"_t)("stopI", "08:40"_t);
        b.vj("line:1", "111111", "", true, "vj:4")
            .route("route4")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopD", "08:15"_t)(
                "stopM", "08:20"_t)("stopN", "08:25"_t)("stopO", "08:30"_t)("stopG", "08:35"_t)("stopH", "08:40"_t)(
                "stopI", "08:40"_t);
        b.vj("line:1", "111111", "", true, "vj:5")
            .route("route5")("stopA", "08:00"_t)("stopB", "08:05"_t)("stopC", "08:10"_t)("stopP", "08:15"_t)(
                "stopQ", "08:20"_t)("stopR", "08:25"_t);
        b.vj("line:1", "111111", "", true, "vj:6")
            .route("route6")("stopR", "08:00"_t)("stopQ", "08:05"_t)("stopP", "08:10"_t)("stopC", "08:15"_t)(
                "stopB", "08:20"_t)("stopA", "08:25"_t);
    });

    b.data->meta->production_date = bg::date_period(bg::date(2021, 1, 1), bg::days(7));

    return b;
}

BOOST_AUTO_TEST_CASE(complex_impact_with_rail_section) {
    /**
     * We want to impact like this


              |------- J ------ K ------ L --------
              |                                   |
              |        X        X        X        |
     A x----- B ------ C ------ D ------ E ------ F ------ G ------ H -----x I
                       |        |                          |
                       |        |                          |
                       |        |                          |
                       |        |------- M ------ N ------ O
                       |                 X        X        X
                       |
                       P ------ Q -----x R
                       X        X      X

       X -> SA blocked : C-D-E-M-N-O-P-Q-R

       It can be the representation of a broken rail between B and C for all routes with B-C section

       We can't do this with a single disruption. We need 3 Disruptions :
       -> route1 - start_point B - End point E - blocked_stop_areas C D
       -> route4 - start_point B - End point O - blocked_stop_areas C D M N
       -> route5 - start_point B - End point R - blocked_stop_areas C P Q
    */
    ed::builder b = create_complex_data_for_rail_section();

    using btp = boost::posix_time::time_period;

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 6);

    // new rail_section disruption
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-1")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                  .on_rail_section("line:1", "stopAreaB", "stopAreaE",
                                                   {
                                                       std::make_pair("stopAreaC", 1),
                                                       std::make_pair("stopAreaD", 2),
                                                   },
                                                   {"route1"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 7);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:rail_section_on_line1-1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times, for A and B stop point
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");

    // disruption
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopF")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopG")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopH")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopI")->get_impacts().size(), 1);

    // new rail_section disruption
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-2")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                  .on_rail_section("line:1", "stopAreaB", "stopAreaO",
                                                   {std::make_pair("stopAreaC", 1), std::make_pair("stopAreaD", 2),
                                                    std::make_pair("stopAreaM", 3), std::make_pair("stopAreaN", 4)},
                                                   {"route4"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 8);

    // Check the original vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:4");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:4:Adapted:0:rail_section_on_line1-2");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times, for A and B stop point
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");

    // disruptions
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopM")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopN")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopO")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopG")->get_impacts().size(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopH")->get_impacts().size(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopI")->get_impacts().size(), 2);

    // new rail_section disruption
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-3")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                  .on_rail_section("line:1", "stopAreaB", "stopAreaR",
                                                   {std::make_pair("stopAreaC", 1), std::make_pair("stopAreaP", 2),
                                                    std::make_pair("stopAreaQ", 3)},
                                                   {"route5"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 9);

    // Check the original vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:5");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:5:Adapted:0:rail_section_on_line1-3");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times, for A and B stop point
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");

    // disruptions
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 3);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 3);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 3);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopP")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopQ")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopR")->get_impacts().size(), 1);
}

BOOST_AUTO_TEST_CASE(complex_impact_with_rail_section_reduced_service) {
    /**
     * We want to impact like this


              |------- J ------ K ------ L --------
              |                                   |
              |        X        X        X        |
     A x----- B ------ C ------ D ------ E ------ F ------ G ------ H -----x I
                       |        |                          |
                       |        |                          |
                       |        |                          |
                       |        |------- M ------ N ------ O
                       |                 X        X        X
                       |
                       P ------ Q -----x R
                       X        X      X

       X -> SA blocked : C-D-E-M-N-O-P-Q-R

       It can be the representation of a broken rail between B and C for all routes with B-C section

       We can't do this with a single disruption. We need 3 Disruptions :
       -> route1 - start_point B - End point E - blocked_stop_areas C D
       -> route4 - start_point B - End point O - blocked_stop_areas C D M N
       -> route5 - start_point B - End point R - blocked_stop_areas C P Q
    */
    ed::builder b = create_complex_data_for_rail_section();

    using btp = boost::posix_time::time_period;

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 6);

    // new rail_section disruption
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-1")
                                  .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                  .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                  .on_rail_section("line:1", "stopAreaB", "stopAreaE",
                                                   {
                                                       std::make_pair("stopAreaC", 1),
                                                       std::make_pair("stopAreaD", 2),
                                                   },
                                                   {"route1"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 7);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:rail_section_on_line1-1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 7 stop_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 7);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].stop_point->uri, "stopE");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[3].stop_point->uri, "stopF");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[4].stop_point->uri, "stopG");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[5].stop_point->uri, "stopH");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[6].stop_point->uri, "stopI");

    // disruption
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopF")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopG")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopH")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopI")->get_impacts().size(), 0);

    // new rail_section disruption
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-2")
                                  .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                  .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                  .on_rail_section("line:1", "stopAreaB", "stopAreaO",
                                                   {std::make_pair("stopAreaC", 1), std::make_pair("stopAreaD", 2),
                                                    std::make_pair("stopAreaM", 3), std::make_pair("stopAreaN", 4)},
                                                   {"route4"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 8);

    // Check the original vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:4");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:4:Adapted:0:rail_section_on_line1-2");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 6 stop_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 6);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].stop_point->uri, "stopO");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[3].stop_point->uri, "stopG");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[4].stop_point->uri, "stopH");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[5].stop_point->uri, "stopI");

    // disruptions
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 2);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopM")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopN")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopO")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopG")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopH")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopI")->get_impacts().size(), 0);

    // new rail_section disruption
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-3")
                                  .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                  .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                  .on_rail_section("line:1", "stopAreaB", "stopAreaR",
                                                   {std::make_pair("stopAreaC", 1), std::make_pair("stopAreaP", 2),
                                                    std::make_pair("stopAreaQ", 3)},
                                                   {"route5"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 9);

    // Check the original vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:5");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:5:Adapted:0:rail_section_on_line1-3");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 2 stop_times, for A and B stop point
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 3);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopA");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopB");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].stop_point->uri, "stopR");

    // disruptions
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 3);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 3);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 3);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopP")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopQ")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopR")->get_impacts().size(), 1);
}

BOOST_AUTO_TEST_CASE(complex_impact_with_rail_section_reduced_service_first_stop_blocked) {
    /**
     * We want to impact like this


              |------- J ------ K ------ L --------
              |                                   |
              |        X        X        X        |
     A x----- B ------ C ------ D ------ E ------ F ------ G ------ H -----x I
                       |        |                          |
                       |        |                          |
                       |        |                          |
                       |        |------- M ------ N ------ O
                       |                 X        X        X
                       |
                       P ------ Q -----x R
                       X        X      X

       -> route1 - start_point A - End point D - blocked_stop_areas A C
    */
    ed::builder b = create_complex_data_for_rail_section();

    using btp = boost::posix_time::time_period;

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 6);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 0);

    // new rail_section disruption
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "rail_section_on_line1-1")
                                  .severity(nt::disruption::Effect::REDUCED_SERVICE)
                                  .application_periods(btp("20210101T060000"_dt, "20210101T100000"_dt))
                                  .on_rail_section("line:1", "stopAreaA", "stopAreaD",
                                                   {
                                                       std::make_pair("stopAreaA", 0),
                                                       std::make_pair("stopAreaB", 1),
                                                       std::make_pair("stopAreaC", 2),
                                                   },
                                                   {"route1"}, *b.data->pt_data)
                                  .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 7);

    // Check the original vj
    auto* vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1");
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111110"), adapted_vp);

    // Check the adapted vj
    vj = b.get<nt::VehicleJourney>("vehicle_journey:vj:1:Adapted:0:rail_section_on_line1-1");
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000001"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);
    // The adapted vj should have only 7 stop_times
    BOOST_REQUIRE_EQUAL(vj->stop_time_list.size(), 6);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0].stop_point->uri, "stopD");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1].stop_point->uri, "stopE");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2].stop_point->uri, "stopF");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[3].stop_point->uri, "stopG");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[4].stop_point->uri, "stopH");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[5].stop_point->uri, "stopI");

    // disruption
    BOOST_REQUIRE_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopA")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopB")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopC")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopD")->get_impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopE")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopF")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopG")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopH")->get_impacts().size(), 0);
    BOOST_REQUIRE_EQUAL(b.get<nt::StopPoint>("stopI")->get_impacts().size(), 0);
}
