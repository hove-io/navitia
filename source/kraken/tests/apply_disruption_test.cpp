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
#include "kraken/make_disruption_from_chaos.h"
#include "kraken/realtime.h"
#include "type/kirin.pb.h"
#include "routing/raptor.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace ntest = navitia::test;
namespace ba = boost::algorithm;
namespace bg = boost::gregorian;

using btp = boost::posix_time::time_period;

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
    auto check_empty_vp = [](const nt::VehicleJourney* vj) {
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
                vj->base_validity_pattern()->days);
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000000"),
                vj->adapted_validity_pattern()->days);
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "000000"),
                vj->base_validity_pattern()->days);
    };
    for (const auto* vj: vehicle_journeys) {
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
            check_empty_vp(vj);
            break;
        case nt::RTLevel::RealTime:
            check_empty_vp(vj);
            break;
        }
    }
}

static std::vector<navitia::routing::Path> compute(navitia::type::Data& data,
        nt::RTLevel level,
        const std::string& from, const std::string& to,
        navitia::DateTime datetime, int day) {
    data.build_raptor();
    navitia::routing::RAPTOR raptor{data};
    return raptor.compute(data.pt_data->stop_areas_map.at(from), data.pt_data->stop_areas_map.at(to),
            datetime, day, navitia::DateTimeUtils::inf, level, 2_min, true);
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
 * vj2_adapted:   10h           X              X         TODO: With the current algo, some VJs won't circulate at all
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
    ed::builder b("20160101");

    b.sa("S1")("S1");
    b.sa("S2")("S2");
    b.sa("S3")("S3");

    const auto* vj1 = b.vj("A").uri("vj1")("S1", "08:00"_t)("S2", "09:00"_t)("S3", "10:00"_t).make();
    const auto* vj2 = b.vj("A").uri("vj2")("S1", "10:00"_t)("S2", "10:30"_t)("S3", "11:00"_t).make();
    const auto* vj3 = b.vj("A").uri("vj3")("S1", "10:00"_t)                 ("S3", "11:00"_t).make();
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

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 7);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "S3_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "S3")
                              .application_periods(btp("20160101T110000"_dt, "20160101T1400001"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 10);// 10, there are two VJs don't circulate at all

    auto get_adapted = [&b](const nt::VehicleJourney* vj, int nb_adapted_vj) {
        BOOST_REQUIRE_EQUAL(vj->meta_vj->get_adapted_vj().size(), nb_adapted_vj);
        return vj->meta_vj->get_adapted_vj().back().get();
    };

    //no adapted for vj1
    BOOST_REQUIRE_EQUAL(vj1->meta_vj->get_adapted_vj().size(), 0);

    const auto* vj2_adapted = get_adapted(vj2, 2);

    BOOST_CHECK(ba::ends_with(vj2->base_validity_pattern()->days.to_string(), "111"));
    BOOST_CHECK(ba::ends_with(vj2->adapted_validity_pattern()->days.to_string(), "110"));
    BOOST_CHECK(ba::ends_with(vj2_adapted->base_validity_pattern()->days.to_string(), "000"));
    BOOST_CHECK(ba::ends_with(vj2_adapted->adapted_validity_pattern()->days.to_string(), "001"));
    check_vj(vj2_adapted, {{"10:00"_t, "10:00"_t, s1}});
    check_vj(get_adapted(vj3, 1), {{"10:00"_t, "10:00"_t, s1}});
    check_vj(get_adapted(vj4, 1), {{"08:00"_t, "08:00"_t, s1},
                                   {"10:30"_t, "10:30"_t, s3}});
    check_vj(get_adapted(vj5, 1), {{"08:00"_t, "08:00"_t, s1},
                                   {"13:00"_t, "13:00"_t, s2}});

    navitia::delete_disruption("S2_closed", *b.data->pt_data, *b.data->meta);
    navitia::delete_disruption("S3_closed", *b.data->pt_data, *b.data->meta);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "111111");
}


BOOST_AUTO_TEST_CASE(add_impact_on_stop_area) {
    ed::builder b("20120614");
    b.vj("A", "000111")
            ("stop_area:stop1", "08:10"_t, "08:11"_t)
            ("stop_area:stop2", "08:20"_t, "08:21"_t)
            ("stop_area:stop3", "08:30"_t, "08:31"_t);
    b.vj("A", "000111")
            ("stop_area:stop1", "09:10"_t, "09:11"_t)
            ("stop_area:stop2", "09:20"_t, "09:21"_t)
            ("stop_area:stop3", "09:30"_t, "09:31"_t);

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    // we delete the stop_area 1, two vjs are impacted, we create a new journey pattern without this stop_area
    // and two new vj are enable on this journey pattern, of course the theorical vj are disabled

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area:stop1_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopArea, "stop_area:stop1")
                              .application_periods(btp("20120614T173200"_dt, "20120618T123200"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);
    bool has_adapted_vj = false;
    for (const auto* vj: b.data->pt_data->vehicle_journeys) {
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
            //TODO
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


    ed::builder b("20120614");
    b.sa("stop_area", 0, 0, false, true)
                ("stop_area:stop1")
                ("stop_area:stop2")
                ("stop_area:stop3");

    b.vj("A", "111111").uri("VjA")
            ("A1",              "08:10"_t, "08:11"_t)
            ("stop_area:stop1", "09:10"_t, "09:11"_t)
            ("stop_area:stop2", "10:20"_t, "10:21"_t)
            ("A2",              "11:20"_t, "11:21"_t);

    b.vj("B", "111111").uri("VjB")
            ("B1",              "08:10"_t, "08:11"_t)
            ("stop_area:stop1", "09:10"_t, "09:11"_t)
            ("stop_area:stop3", "10:20"_t, "10:21"_t)
            ("B2",              "11:20"_t, "11:21"_t);

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    //we delete the stop_area 1, two vj are impacted, we create a new journey pattern without this stop_area
    //and two new vj are enable on this journey pattern, of course the theorical vj are disabled

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopArea, "stop_area")
                              .application_periods(btp("20120614T173200"_dt, "20120618T123200"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 2);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 6); // we have two vj that don't circulate

    bool has_adapted_vj = false;
    for (const auto* vj: b.data->pt_data->vehicle_journeys) {
        switch (vj->realtime_level) {
        case nt::RTLevel::Base:
            BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "100001"),
vj->adapted_validity_pattern()->days);
            BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
                    vj->base_validity_pattern()->days);
            break;
        case nt::RTLevel::Adapted:
            has_adapted_vj = true;
            if (vj->uri == "VjA:Adapted:0:stop_area_closed" ||
                    vj->uri == "VjB:Adapted:0:stop_area_closed") {
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
            //TODO
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
    BOOST_CHECK_MESSAGE(ba::ends_with(vp->days.to_string(), "011110"),  vp->days.to_string());
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


    ed::builder b("20120614");
    b.sa("stop_area", 0, 0, false, true)
                ("stop_area:stop1")
                ("stop_area:stop2")
                ("stop_area:stop3");

    b.vj("A", "0111111")
            ("A1",              "23:10"_t, "23:11"_t)
            ("stop_area:stop1", "24:10"_t, "24:11"_t)
            ("stop_area:stop2", "25:20"_t, "25:21"_t)
            ("A2",              "26:20"_t, "26:21"_t);

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    //we delete the stop_area 1, on vj passing midnight is impacted,

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopArea, "stop_area")
                              .application_periods(btp("20120615T000000"_dt, "20120618T000000"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3); // we have 1 vj that don't circulate at all :(

    // Base VJ
    auto vj  = b.data->pt_data->vehicle_journeys_map["vj:A:0"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "0111111"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "0111000"),
            vj->adapted_validity_pattern()->days);

    // Adapted VJ
    vj  = b.data->pt_data->vehicle_journeys_map["vehicle_journey 0:Adapted:1:stop_area_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "0000000"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "0000111"),
            vj->adapted_validity_pattern()->days);


    auto res = compute(*b.data, nt::RTLevel::Adapted, "A1", "A2", "23:00"_t, 1);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res[0].items[0].stop_times.size(), 2);
    auto vp = res[0].items[0].stop_times[0]->vehicle_journey->adapted_validity_pattern();
    BOOST_CHECK_MESSAGE(ba::ends_with(vp->days.to_string(), "0000111"),  vp->days.to_string());

    BOOST_REQUIRE(! compute(*b.data, nt::RTLevel::Base, "A1", "stop_area", "08:00"_t, 1).empty());
    BOOST_REQUIRE(compute(*b.data, nt::RTLevel::Adapted, "A1", "stop_area", "08:00"_t, 1).empty());

    navitia::delete_disruption("stop_area_closed", *b.data->pt_data, *b.data->meta);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "0111111");


    BOOST_REQUIRE(! compute(*b.data, nt::RTLevel::Base, "A1", "stop_area", "08:00"_t, 1).empty());
    BOOST_REQUIRE(! compute(*b.data, nt::RTLevel::Adapted, "A1", "stop_area", "08:00"_t, 1).empty());

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

    ed::builder b("20120614");
    b.vj("A").uri("vj:1")
            ("stop1", "08:00"_t)
            ("stop2", "08:15"_t)
            ("stop3", "08:45"_t) // <- Only stop3 will be impacted
            ("stop4", "09:00"_t);

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop3_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop3")
                              // 2012/6/14 7h -> 2012/6/17 6h
                              .application_periods(btp("20120614T070000"_dt, "20120617T060000"_dt))
                              // 2012/6/17 23h -> 2012/6/18 6h
                              .application_periods(btp("20120617T230000"_dt, "20120618T060000"_dt))
                              // 2012/6/19 4h -> 2012/6/19 8h30
                              .application_periods(btp("20120619T083000"_dt, "20120619T093000"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // Base VJ
    auto* vj  = b.data->pt_data->vehicle_journeys_map["vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "011000"),
            vj->adapted_validity_pattern()->days);

    // Adapted VJ
    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:Adapted:0:stop3_closed"];
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
    BOOST_REQUIRE_EQUAL(res.size(), 0); // <-- The stop3 is not accessible at the given datetime.

    // 2012/6/18 5:00
    res = compute(*b.data, nt::RTLevel::Base, "stop1", "stop3", "05:00"_t, 4);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "05:00"_t, 4);
    BOOST_REQUIRE_EQUAL(res.size(), 1); // <-- The result should be the base vj
    BOOST_REQUIRE_EQUAL(res[0].items[0].stop_times[0]->vehicle_journey->uri, "vj:1");

    navitia::delete_disruption("stop3_closed", *b.data->pt_data, *b.data->meta);

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "1111111");

    BOOST_REQUIRE_EQUAL(compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "08:00"_t, 1).size(), 1);
    BOOST_REQUIRE_EQUAL(compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "05:00"_t, 4).size(), 1);

}

BOOST_AUTO_TEST_CASE(remove_stop_point_impact) {

    ed::builder b("20120614");
    b.vj("A").uri("vj:1")
              ("stop1", "08:00"_t)
              ("stop2", "08:15"_t)
              ("stop3", "08:45"_t) // <- Only stop3 will be impacted
              ("stop4", "09:00"_t);

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    const auto& disruption = b.impact(nt::RTLevel::Adapted, "stop3_closed")
                                              .severity(nt::disruption::Effect::NO_SERVICE)
                                              .on(nt::Type_e::StopPoint, "stop3")
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
    auto* vj  = b.data->pt_data->vehicle_journeys_map["vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "011000"),
            vj->adapted_validity_pattern()->days);

    // Adapted VJ
    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:Adapted:0:stop3_closed"];
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

    ed::builder b("20120614");
    b.vj("A").uri("vj:1")("stop1", "08:00"_t)("stop2", "08:15"_t)("stop3", "08:45"_t);

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop1_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop1")
                              // 2012/6/14 8h00 -> 2012/6/19 8h05
                              .application_periods(btp("20120614T080000"_dt, "20120619T080500"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop2_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop2")
                              // 2012/6/14 8h15 -> 2012/6/19 8h17
                              .application_periods(btp("20120614T081500"_dt, "20120619T081700"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop3_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop3")
                              // 2012/6/14 8h45 -> 2012/6/19 8h47
                              .application_periods(btp("20120614T084500"_dt, "20120619T084700"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // Base VJ
    auto* vj  = b.data->pt_data->vehicle_journeys_map["vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000000"),
            vj->adapted_validity_pattern()->days);


    // Adapted VJ
    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:Adapted:2:stop1_closed:stop2_closed:stop3_closed"];
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

    check_vjs_without_disruptions(b.data->pt_data->vehicle_journeys, "1111111");
}

BOOST_AUTO_TEST_CASE(stop_point_no_service_with_shift) {
    ed::builder b("20120614");
    auto* vj1 = b.vj("A").uri("vj:1")("stop1", "23:00"_t)("stop2", "24:15"_t)("stop3", "24:45"_t).make();
    
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    auto trip_update = ntest::make_delay_message("vj:1", "20120616", {
            std::make_tuple("stop1", "20120617T0005"_pts, "20120617T0005"_pts),
            std::make_tuple("stop2", "20120617T0105"_pts, "20120617T0105"_pts),
            std::make_tuple("stop3", "20120617T0205"_pts, "20120617T0205"_pts),
        });
    navitia::handle_realtime("bob", "20120616T1337"_dt, trip_update, *b.data);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "111011"),
            vj1->rt_validity_pattern()->days);

    auto* vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "001000"),
            vj->rt_validity_pattern()->days);

    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "stop2_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop2")
                              .application_periods(btp("20120617T000000"_dt, "20120617T080500"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "111011"),
            vj1->rt_validity_pattern()->days);


    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "000000"),
            vj->rt_validity_pattern()->days);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:RealTime:1:bob:stop2_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "001000"),
            vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 1);
    auto* s1 = b.data->pt_data->stop_points_map["stop1"];
    auto* s3 = b.data->pt_data->stop_points_map["stop3"];

    check_vj(vj, {{"00:05"_t, "00:05"_t, s1},
                  {"02:05"_t, "02:05"_t, s3}});

    auto res = compute(*b.data, nt::RTLevel::RealTime, "stop1", "stop3", "8:00"_t, 3);
    BOOST_REQUIRE_EQUAL(res.size(), 1);

    navitia::delete_disruption("stop2_closed", *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "111011"),
            vj1->rt_validity_pattern()->days);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "000000"),
            vj->rt_validity_pattern()->days);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:2:bob"];
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
    ed::builder b("20120614");
    auto* vj1 = b.vj("A", "00100").uri("vj:1")("stop1", "23:00"_t)("stop2", "24:15"_t)("stop3", "25:00"_t).make();

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    auto trip_update = ntest::make_delay_message("vj:1", "20120616", {
            std::make_tuple("stop1", "20120617T2300"_pts, "20120617T2300"_pts),
            std::make_tuple("stop2", "20120618T0005"_pts, "20120618T0005"_pts),
            std::make_tuple("stop3", "20120618T0100"_pts, "20120618T0100"_pts),
        });
    navitia::handle_realtime("bob", "20120616T1337"_dt, trip_update, *b.data);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "000000"),
            vj1->rt_validity_pattern()->days);

    auto* vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "0001000"),
            vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 1);

    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "stop1_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop1")
                              .application_periods(btp("20120617T2200"_dt, "20120618T0000"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "000000"),
            vj1->rt_validity_pattern()->days);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:0:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "0000000"),
            vj->rt_validity_pattern()->days);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:RealTime:1:bob:stop1_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "010000"),
            vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 2);

    auto* stop2 = b.data->pt_data->stop_points_map["stop2"];
    auto* stop3 = b.data->pt_data->stop_points_map["stop3"];
    check_vj(vj, {{"00:05"_t, "00:05"_t, stop2},
                  {"01:00"_t, "01:00"_t, stop3}});


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
    BOOST_CHECK_MESSAGE(ba::ends_with(vp->days.to_string(), "010000"),  vp->days.to_string());

    navitia::delete_disruption("stop1_closed", *b.data->pt_data, *b.data->meta);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:RealTime:1:bob:stop1_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "000000"),
            vj->rt_validity_pattern()->days);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:2:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "0001000"),
            vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 1);

    navitia::delete_disruption("bob", *b.data->pt_data, *b.data->meta);
    for (const auto* vj: b.data->pt_data->vehicle_journeys) {
          switch (vj->realtime_level) {
          case nt::RTLevel::Base:
              BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "00100"),
                      vj->adapted_validity_pattern()->days);
              BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "00100"),
                      vj->base_validity_pattern()->days);
              break;
          case nt::RTLevel::Adapted:
              throw navitia::exception("Adapted check unhandled case");
          case nt::RTLevel::RealTime:
              BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000000"),
                      vj->adapted_validity_pattern()->days);
              BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
                      vj->base_validity_pattern()->days);
              break;
          }
      }

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

    ed::builder b("20120614");
    auto* vj1 = b.vj("A", "00100").uri("vj:1")("stop1", "23:00"_t)("stop2", "24:15"_t)("stop3", "25:00"_t).make();

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    // Stop1 is closed between 2012/06/16 22:00 and 2012/06/16 23:30
    navitia::apply_disruption(b.impact(nt::RTLevel::RealTime, "stop1_closed")
                                  .severity(nt::disruption::Effect::NO_SERVICE)
                                  .on(nt::Type_e::StopPoint, "stop1")
                                  .application_periods(btp("20120616T2200"_dt, "20120616T2330"_dt))
                                  .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "000000"),
                vj1->rt_validity_pattern()->days);

    auto* vj  = b.data->pt_data->vehicle_journeys_map["vj:1:RealTime:0:stop1_closed"];

    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "001000"),
            vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 1);

    auto trip_update = ntest::make_delay_message("vj:1", "20120616", {
            std::make_tuple("stop2", "20120618T0015"_pts, "20120618T0015"_pts),
            std::make_tuple("stop3", "20120618T0100"_pts, "20120618T0100"_pts),
        });

    navitia::handle_realtime("bob", "20120616T1337"_dt, trip_update, *b.data);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj1->rt_validity_pattern()->days.to_string(), "000000"),
            vj1->rt_validity_pattern()->days);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:1:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "0010000"),
            vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 2);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:RealTime:0:stop1_closed"];

    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "0000000"),
            vj->rt_validity_pattern()->days);

    auto* stop2 = b.data->pt_data->stop_points_map["stop2"];
    auto* stop3 = b.data->pt_data->stop_points_map["stop3"];
    check_vj(vj, {{"00:15"_t, "00:15"_t, stop2},
                  {"01:00"_t, "01:00"_t, stop3}});

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
    BOOST_CHECK_MESSAGE(ba::ends_with(vp->days.to_string(), "010000"),  vp->days.to_string());

    navitia::delete_disruption("stop1_closed", *b.data->pt_data, *b.data->meta);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:1:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "000000"),
            vj->rt_validity_pattern()->days);

    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:modified:2:bob"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->rt_validity_pattern()->days.to_string(), "010000"),
            vj->rt_validity_pattern()->days);

    BOOST_REQUIRE_EQUAL(vj->shift, 2);

    navitia::delete_disruption("bob", *b.data->pt_data, *b.data->meta);

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
    ed::builder b("20120614");
    auto* vj = b.vj("A").uri("vj:1")
        ("stop1", "08:00"_t)
        ("stop2", "09:00"_t)
        ("stop3", "10:00"_t)
        ("stop1", "11:00"_t).make();

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = bg::date_period(bg::date(2012,6,14), bg::days(7));

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop1_closed")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop1")
                              .application_periods(btp("20120615T100000"_dt, "20120617T100000"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "110001"),
            vj->adapted_validity_pattern()->days);

    const nt::VehicleJourney* vj15 = nullptr;
    const nt::VehicleJourney* vj16 = nullptr;
    const nt::VehicleJourney* vj17 = nullptr;
    for (const auto* cur_vj: b.data->pt_data->vehicle_journeys) {
        if (cur_vj->adapted_validity_pattern()->check(1)) { vj15 = cur_vj; }
        if (cur_vj->adapted_validity_pattern()->check(2)) { vj16 = cur_vj; }
        if (cur_vj->adapted_validity_pattern()->check(3)) { vj17 = cur_vj; }
    }

    BOOST_REQUIRE(vj15 && ! vj15->stop_time_list.empty());
    // BOOST_CHECK_EQUAL(vj15->stop_time_list.size(), 3);
    // BOOST_CHECK_EQUAL(vj15->stop_time_list.front().stop_point->uri, "stop1");
    // BOOST_CHECK_EQUAL(vj15->stop_time_list.back().stop_point->uri, "stop3");

    BOOST_REQUIRE(vj16 && ! vj16->stop_time_list.empty());
    BOOST_CHECK_EQUAL(vj16->stop_time_list.size(), 2);
    BOOST_CHECK_EQUAL(vj16->stop_time_list.front().stop_point->uri, "stop2");
    BOOST_CHECK_EQUAL(vj16->stop_time_list.back().stop_point->uri, "stop3");

    BOOST_REQUIRE(vj17 && ! vj17->stop_time_list.empty());
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


    ed::builder b("20120614");
    b.sa("stop_area", 0, 0, false, true)
                ("stop_area:stop1")("stop_area:stop2")
                ("stop_area:stop3")("stop_area:stop4")
                ("stop_area:stop5")("stop_area:stop6")
                ("stop_area:stop7")("stop_area:stop8");

    b.vj("A", "111111").uri("VjA")
            ("A1",              "08:10"_t)
            ("stop_area:stop1", "09:10"_t)
            ("stop_area:stop2", "10:20"_t)
            ("stop_area:stop3", "11:10"_t)
            ("stop_area:stop4", "12:20"_t)
            ("stop_area:stop5", "13:10"_t)
            ("stop_area:stop6", "14:20"_t)
            ("stop_area:stop7", "15:10"_t)
            ("stop_area:stop8", "16:20"_t)
            ("A2",              "17:20"_t);

    b.vj("B", "111111").uri("VjB")
            ("B1",              "08:10"_t)
            ("stop_area:stop2", "10:20"_t)
            ("stop_area:stop3", "11:10"_t)
            ("stop_area:stop4", "12:20"_t)
            ("stop_area:stop5", "13:10"_t)
            ("stop_area:stop6", "14:20"_t)
            ("stop_area:stop7", "15:10"_t)
            ("B2",              "17:20"_t)
            ("B3",              "18:20"_t)
            ("B4",              "19:20"_t);


    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_point_B2_closed_3")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "B2")
                              .application_periods(btp("20120616T080000"_dt, "20120616T203200"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_point_B3_closed_3")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "B3")
                              .application_periods(btp("20120616T080000"_dt, "20120616T203200"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed_1")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopArea, "stop_area")
                              .application_periods(btp("20120618T080000"_dt, "20120618T173200"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed_2")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopArea, "stop_area")
                              .application_periods(btp("20120615T080000"_dt, "20120615T173200"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "stop_area_closed_3")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopArea, "stop_area")
                              .application_periods(btp("20120614T080000"_dt, "20120614T173200"_dt))
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);


    const auto* mvj_A = b.data->pt_data->meta_vjs["VjA"];
    const auto* mvj_B = b.data->pt_data->meta_vjs["VjB"];
    BOOST_CHECK_EQUAL(mvj_A->impacted_by.size(), 3);
    BOOST_CHECK_EQUAL(mvj_B->impacted_by.size(), 5);

    navitia::delete_disruption("disruption_does't_exist", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(mvj_A->impacted_by.size(), 3);
    BOOST_CHECK_EQUAL(mvj_B->impacted_by.size(), 5);


    navitia::delete_disruption("stop_area_closed_1", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(mvj_A->impacted_by.size(), 2);
    BOOST_CHECK_EQUAL(mvj_B->impacted_by.size(), 4);

    navitia::delete_disruption("stop_area_closed_2", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(mvj_A->impacted_by.size(), 1);
    BOOST_CHECK_EQUAL(mvj_B->impacted_by.size(), 3);

    navitia::delete_disruption("stop_area_closed_3", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(mvj_A->impacted_by.size(), 0);
    BOOST_CHECK_EQUAL(mvj_B->impacted_by.size(), 2);

}
