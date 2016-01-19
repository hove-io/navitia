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

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace ntest = navitia::test;
namespace ba = boost::algorithm;

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
void check_vj(const nt::VehicleJourney* vj, const std::vector<nt::StopTime>& sts) {
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
    b.sa("S2")("S2");

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

    BOOST_CHECK(ba::ends_with(vj2->adapted_validity_pattern()->days.to_string(), "110"));
    BOOST_CHECK(ba::ends_with(vj2_adapted->base_validity_pattern()->days.to_string(), "000"));
    BOOST_CHECK(ba::ends_with(vj2_adapted->adapted_validity_pattern()->days.to_string(), "001"));
    check_vj(vj2_adapted, {{"10:00"_t, "10:00"_t, s1}});
    check_vj(get_adapted(vj3, 1), {{"10:00"_t, "10:00"_t, s1}});
    check_vj(get_adapted(vj4, 1), {{"08:00"_t, "08:00"_t, s1},
                                {"10:30"_t, "10:30"_t, s3}});
    check_vj(get_adapted(vj5, 1), {{"08:00"_t, "08:00"_t, s1},
                                {"13:00"_t, "13:00"_t, s2}});
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

    navitia::type::Data data;
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

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

    b.vj("A", "000111")
            ("A1",              "08:10"_t, "08:11"_t)
            ("stop_area:stop1", "09:10"_t, "09:11"_t)
            ("stop_area:stop2", "10:20"_t, "10:21"_t)
            ("A2",              "11:20"_t, "11:21"_t);

    b.vj("B", "000111")
            ("B1",              "08:10"_t, "08:11"_t)
            ("stop_area:stop1", "09:10"_t, "09:11"_t)
            ("stop_area:stop3", "10:20"_t, "10:21"_t)
            ("B2",              "11:20"_t, "11:21"_t);

    navitia::type::Data data;
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

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
            BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000001"),
vj->adapted_validity_pattern()->days);
            BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000111"),
                    vj->base_validity_pattern()->days);
            break;
        case nt::RTLevel::Adapted:
            has_adapted_vj = true;
            if (vj->uri == "vehicle_journey 0:Adapted:0:stop_area_closed" ||
                    vj->uri == "vehicle_journey 1:Adapted:0:stop_area_closed") {
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000000"),
                        vj->adapted_validity_pattern()->days);
            } else {
                BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000110"),
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

    navitia::type::Data data;
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

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
}

/*
 *
 *
 *  Day 14              15              16              17              18              19              20
 *      -----------------------------------------------------------------------------------------------------
 *   VJ     8H----9H         8H----9H        8H----9H        8H----9H        8H----9H        8H----9H
 * Impact  |-----------------------------------------------|           |---|                     |------|
 *        7:00                                            6:00       23:00                      8:30   9:30
 * */
BOOST_AUTO_TEST_CASE(add_impact_with_sevral_application_period) {

    ed::builder b("20120614");
    b.vj("A", "111111", "", true, "vj:1")
            ("stop1", "08:00"_t, "08:01"_t)
            ("stop2", "08:15"_t, "08:16"_t)
            ("stop3", "08:45"_t, "08:46"_t) // <- Only stop3 will be impacted
            ("stop4", "09:00"_t, "09:01"_t);

    navitia::type::Data data;
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

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

}

BOOST_AUTO_TEST_CASE(remove_stop_point_impact) {

    ed::builder b("20120614");
    b.vj("A", "111111", "", true, "vj:1")
            ("stop1", "08:00"_t, "08:01"_t)
            ("stop2", "08:15"_t, "08:16"_t)
            ("stop3", "08:45"_t, "08:46"_t) // <- Only stop3 will be impacted
            ("stop4", "09:00"_t, "09:01"_t);

    navitia::type::Data data;
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

    auto&& disruption = b.impact(nt::RTLevel::Adapted, "stop3_closed")
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

    // Base VJ
    vj  = b.data->pt_data->vehicle_journeys_map["vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "111111"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "111111"),
            vj->adapted_validity_pattern()->days);

    // Adapted VJ
    vj  = b.data->pt_data->vehicle_journeys_map["vj:1:Adapted:0:stop3_closed"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000000"),
            vj->base_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000000"),
            vj->adapted_validity_pattern()->days);
}
