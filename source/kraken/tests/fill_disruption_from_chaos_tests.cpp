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
#define BOOST_TEST_MODULE fill_disruption_from_chaos_test
#include <boost/test/unit_test.hpp>
#include "routing/raptor_api.h"
#include "ed/build_helper.h"
#include "routing/tests/routing_api_test_data.h"
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "type/data.h"
#include "kraken/make_disruption_from_chaos.h"
#include "kraken/apply_disruption.h"
#include "type/chaos.pb.h"
#include "type/gtfs-realtime.pb.h"
#include "type/meta_vehicle_journey.h"
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "utils/logger.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace ntest = navitia::test;
namespace bt = boost::posix_time;
namespace ba = boost::algorithm;

/*
struct stop_area_finder{
    std::string sa_uri;
    stop_area_finder(const std::string& sa_uri): sa_uri(sa_uri){}

    bool operator()(const nt::JourneyPatternPoint* jpp){
        return jpp->stop_point->stop_area->uri == sa_uri;
    }
};
*/

static std::string dump_vj(const nt::VehicleJourney& vj) {
    std::stringstream ss;
    ss << vj.uri << std::endl;
    for (const auto& st : vj.stop_time_list) {
        ss << "\t" << st.stop_point->uri << ": " << st.arrival_time << " - " << st.departure_time << std::endl;
    }
    ss << "\tvalidity_pattern: "
       << ba::trim_left_copy_if(vj.base_validity_pattern()->days.to_string(), boost::is_any_of("0")) << std::endl;
    ss << "\tadapted_validity_pattern: "
       << ba::trim_left_copy_if(vj.adapted_validity_pattern()->days.to_string(), boost::is_any_of("0")) << std::endl;
    return ss.str();
}

BOOST_AUTO_TEST_CASE(add_impact_on_line) {
    ed::builder b("20120614");
    b.vj("A", "000111", "", true, "vj:1")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)(
        "stop_area:stop2", 8 * 3600 + 20 * 60, 8 * 3600 + 21 * 60);
    b.vj("A", "000111", "", true, "vj:2")("stop_area:stop1", 9 * 3600 + 10 * 60, 9 * 3600 + 11 * 60)(
        "stop_area:stop2", 9 * 3600 + 20 * 60, 9 * 3600 + 21 * 60);
    b.vj("A", "001101", "", true, "vj:3")("stop_area:stop1", 16 * 3600 + 10 * 60, 16 * 3600 + 11 * 60)(
        "stop_area:stop2", 16 * 3600 + 20 * 60, 16 * 3600 + 21 * 60);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::days(7));

    // we delete the line A for three day, the first two vj start too early for being impacted on the first day
    // the third vj start too late for being impacted the last day

    chaos::Disruption disruption;
    disruption.set_id("test01");
    // Add a period with valid start date without end date
    auto period = disruption.mutable_publication_period();
    period->set_start(ntest::to_posix_timestamp("20120613T153200"));
    auto* impact = disruption.add_impacts();
    impact->set_id("impact_id");
    auto* severity = impact->mutable_severity();
    severity->set_id("severity");
    severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
    auto* object = impact->add_informed_entities();
    object->set_pt_object_type(chaos::PtObject_Type_line);
    object->set_uri("A");
    auto* app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120614T153200"));
    app_period->set_end(ntest::to_posix_timestamp("20120616T123200"));

    auto* ch_application_pattern = impact->add_application_patterns();
    ch_application_pattern->set_start_date(ntest::to_int_date("20120614T153200"));
    ch_application_pattern->set_end_date(ntest::to_int_date("20120616T123200"));

    auto week = ch_application_pattern->mutable_week_pattern();
    week->set_monday(true);
    week->set_tuesday(true);
    week->set_wednesday(true);
    week->set_thursday(true);
    week->set_friday(true);
    week->set_saturday(true);
    week->set_sunday(true);

    auto* ch_time_slot = ch_application_pattern->add_time_slots();
    ch_time_slot->set_begin("15:32"_t);
    ch_time_slot->set_end("12:32"_t);

    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000001"),
                        vj->adapted_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000111"),
                        vj->base_validity_pattern()->days);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:2"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000001"),
                        vj->adapted_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000111"),
                        vj->base_validity_pattern()->days);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:3"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "001100"),
                        vj->adapted_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "001101"),
                        vj->base_validity_pattern()->days);

    auto impacts = b.data->pt_data->disruption_holder.get_weak_impacts();
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    auto impact_sptr = impacts[0].lock();
    BOOST_REQUIRE_EQUAL(impact_sptr->application_patterns.size(), 1);
    auto application_pattern = *(impact_sptr->application_patterns.begin());
    BOOST_REQUIRE_EQUAL(
        application_pattern.application_period,
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::date(2012, 6, 16)));
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 1);

    auto time_slot = *(application_pattern.time_slots.begin());
    BOOST_REQUIRE_EQUAL(time_slot.begin, "15:32"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "12:32"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern.to_string(), "1111111");

    // we delete the disruption, all must be as before!
    navitia::delete_disruption("test01", *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000111"),
                        vj->adapted_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000111"),
                        vj->base_validity_pattern()->days);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:2"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000111"),
                        vj->adapted_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000111"),
                        vj->base_validity_pattern()->days);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:3"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "001101"),
                        vj->adapted_validity_pattern()->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "001101"),
                        vj->base_validity_pattern()->days);
}

BOOST_AUTO_TEST_CASE(add_impact_and_update_on_stop_area) {
    ed::builder b("20120614");
    b.vj("A", "000111")
        .name("vj:1")("stop_area:stop1", "08:10"_t, "08:11"_t)("stop_area:stop2", "08:20"_t, "08:21"_t)(
            "stop_area:stop3", "08:30"_t, "08:31"_t)("stop_area:stop4", "08:40"_t, "08:41"_t);
    b.vj("A", "000111")
        .name("vj:2")("stop_area:stop1", "09:10"_t, "09:11"_t)("stop_area:stop2", "09:20"_t, "09:21"_t)(
            "stop_area:stop3", "09:30"_t, "09:31"_t)("stop_area:stop4", "09:40"_t, "09:41"_t);

    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::days(7));

    // mostly like the previous test, but we disable the stop_area 1 and 2
    // some useless journey pattern and vj are created but won't circulate
    // then we update the disruption (without any change) this way we check all the process (previously there was some
    // segfault)

    chaos::Disruption disruption;
    {
        disruption.set_id("test01");
        // Add a period with valid start and end date
        auto period = disruption.mutable_publication_period();
        period->set_start(ntest::to_posix_timestamp("20120613T153200"));
        period->set_end(ntest::to_posix_timestamp("20120615T153200"));
        auto* impact = disruption.add_impacts();
        impact->set_id("impact_id");
        auto* severity = impact->mutable_severity();
        severity->set_id("severity");
        severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
        auto* app_period = impact->add_application_periods();
        app_period->set_start(ntest::to_posix_timestamp("20120614T123200"));
        app_period->set_end(ntest::to_posix_timestamp("20120617T123200"));

        auto* object = impact->add_informed_entities();
        object->set_pt_object_type(chaos::PtObject_Type_stop_area);
        object->set_uri("stop_area:stop1");

        object = impact->add_informed_entities();
        object->set_pt_object_type(chaos::PtObject_Type_stop_area);
        object->set_uri("stop_area:stop2");

        auto* ch_application_pattern = impact->add_application_patterns();
        ch_application_pattern->set_start_date(ntest::to_int_date("20120614T123200"));
        ch_application_pattern->set_end_date(ntest::to_int_date("20120617T123200"));

        auto week = ch_application_pattern->mutable_week_pattern();
        week->set_monday(true);
        week->set_tuesday(true);
        week->set_wednesday(true);
        week->set_thursday(true);
        week->set_friday(true);
        week->set_saturday(true);
        week->set_sunday(true);

        auto* ch_time_slot = ch_application_pattern->add_time_slots();
        ch_time_slot->set_begin("12:32"_t);
        ch_time_slot->set_end("12:32"_t);
    }

    auto check = [](const nt::Data& data) {
        BOOST_REQUIRE_EQUAL(data.pt_data->lines.size(), 1);

        auto* vj = data.pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000001"),
                            vj->adapted_validity_pattern()->days);
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000111"),
                            vj->base_validity_pattern()->days);

        vj = data.pt_data->vehicle_journeys_map["vehicle_journey:vj:2"];
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "000001"),
                            vj->adapted_validity_pattern()->days);
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->base_validity_pattern()->days.to_string(), "000111"),
                            vj->base_validity_pattern()->days);
    };

    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);

    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj:1");
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);
    const auto* adapted_vj = meta_vj->get_adapted_vj()[0].get();

    BOOST_CHECK(adapted_vj->base_validity_pattern()->days.none());
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vj->adapted_validity_pattern()->days.to_string(), "000110"),
                        adapted_vj->adapted_validity_pattern()->days);

    meta_vj = b.get_meta_vj("vj:2");
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);
    adapted_vj = meta_vj->get_adapted_vj()[0].get();

    BOOST_CHECK(adapted_vj->base_validity_pattern()->days.none());
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vj->adapted_validity_pattern()->days.to_string(), "000110"),
                        adapted_vj->adapted_validity_pattern()->days);
    check(*b.data);

    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);

    meta_vj = b.get_meta_vj("vj:1");
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);
    adapted_vj = meta_vj->get_adapted_vj()[0].get();

    BOOST_CHECK(adapted_vj->base_validity_pattern()->days.none());
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vj->adapted_validity_pattern()->days.to_string(), "000110"),
                        adapted_vj->adapted_validity_pattern()->days);

    meta_vj = b.get_meta_vj("vj:2");
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);
    adapted_vj = meta_vj->get_adapted_vj()[0].get();

    BOOST_CHECK(adapted_vj->base_validity_pattern()->days.none());
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vj->adapted_validity_pattern()->days.to_string(), "000110"),
                        adapted_vj->adapted_validity_pattern()->days);

    check(*b.data);

    auto impacts = b.data->pt_data->disruption_holder.get_weak_impacts();
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    auto impact_sptr = impacts[0].lock();
    BOOST_REQUIRE_EQUAL(impact_sptr->application_patterns.size(), 1);
    auto application_pattern = *(impact_sptr->application_patterns.begin());
    BOOST_REQUIRE_EQUAL(
        application_pattern.application_period,
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::date(2012, 6, 17)));
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 1);

    auto time_slot = *(application_pattern.time_slots.begin());
    BOOST_REQUIRE_EQUAL(time_slot.begin, "12:32"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "12:32"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern.to_string(), "1111111");

    navitia::delete_disruption(disruption.id(), *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    for (const auto* vj : b.data->pt_data->vehicle_journeys) {
        if (vj->realtime_level == nt::RTLevel::Base) {
            BOOST_CHECK(vj->base_validity_pattern()->days == vj->adapted_validity_pattern()->days);
        }
        BOOST_CHECK(vj->realtime_level != nt::RTLevel::Adapted);  // all adapted vj must have been remove
    }
}

BOOST_AUTO_TEST_CASE(add_impact_on_line_over_midnigt) {
    ed::builder b("20120614");
    b.vj("A", "010101", "", true, "vj:1")("stop_area:stop1", 23 * 3600 + 10 * 60, 23 * 3600 + 11 * 60)(
        "stop_area:stop2", 24 * 3600 + 20 * 60, 24 * 3600 + 21 * 60);
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::days(7));

    // same as before but with a vehicle_journey that span on two day

    chaos::Disruption disruption;
    disruption.set_id("test01");
    // Add a period with valid start and end date
    auto period = disruption.mutable_publication_period();
    period->set_start(ntest::to_posix_timestamp("20120613T153200"));
    period->set_end(ntest::to_posix_timestamp("20120615T153200"));
    auto* impact = disruption.add_impacts();
    impact->set_id("impact_id");
    auto* severity = impact->mutable_severity();
    severity->set_id("severity");
    severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
    auto* object = impact->add_informed_entities();
    object->set_pt_object_type(chaos::PtObject_Type_line);
    object->set_uri("A");
    auto* app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120616T173200"));
    app_period->set_end(ntest::to_posix_timestamp("20120618T123200"));

    auto* ch_application_pattern = impact->add_application_patterns();
    ch_application_pattern->set_start_date(ntest::to_int_date("20120616T173200"));
    ch_application_pattern->set_end_date(ntest::to_int_date("20120618T123200"));

    auto week = ch_application_pattern->mutable_week_pattern();
    week->set_monday(true);
    week->set_tuesday(false);
    week->set_wednesday(false);
    week->set_thursday(false);
    week->set_friday(false);
    week->set_saturday(true);
    week->set_sunday(true);

    auto* ch_time_slot = ch_application_pattern->add_time_slots();
    ch_time_slot->set_begin("17:32"_t);
    ch_time_slot->set_end("12:32"_t);

    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "010001"), dump_vj(*vj));

    auto impacts = b.data->pt_data->disruption_holder.get_weak_impacts();
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    auto impact_sptr = impacts[0].lock();
    BOOST_REQUIRE_EQUAL(impact_sptr->application_patterns.size(), 1);
    auto application_pattern = *(impact_sptr->application_patterns.begin());
    BOOST_REQUIRE_EQUAL(
        application_pattern.application_period,
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 16), boost::gregorian::date(2012, 6, 18)));
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 1);

    auto time_slot = *(application_pattern.time_slots.begin());
    BOOST_REQUIRE_EQUAL(time_slot.begin, "17:32"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "12:32"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern.to_string(), "1100001");
}

BOOST_AUTO_TEST_CASE(add_impact_on_line_over_midnigt_2) {
    ed::builder b("20120614");
    b.vj("A", "010111", "", true, "vj:1")("stop_area:stop1", 23 * 3600 + 10 * 60, 23 * 3600 + 11 * 60)(
        "stop_area:stop2", 24 * 3600 + 20 * 60, 24 * 3600 + 21 * 60);
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::days(7));

    // again a vehicle_journey than span on two day, but this time the disruption only start the second day,
    // and there is no vj with a starting circulation date for that day.
    // But the vehicle journey of the first day is impacted since is also circulate on the second day!

    chaos::Disruption disruption;
    disruption.set_id("test01");
    // Add a period with valid start and end date
    auto period = disruption.mutable_publication_period();
    period->set_start(ntest::to_posix_timestamp("20120613T153200"));
    period->set_end(ntest::to_posix_timestamp("20120615T153200"));
    auto* impact = disruption.add_impacts();
    impact->set_id("impact_id");
    auto* severity = impact->mutable_severity();
    severity->set_id("severity");
    severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
    auto* object = impact->add_informed_entities();
    object->set_pt_object_type(chaos::PtObject_Type_line);
    object->set_uri("A");
    auto* app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120615T000000"));
    app_period->set_end(ntest::to_posix_timestamp("20120615T120000"));

    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern()->days.to_string(), "010110"), dump_vj(*vj));
}

BOOST_AUTO_TEST_CASE(add_impact_on_line_section) {
    ed::builder b("20160404");
    b.vj("line:A", "111111")
        .name("vj:1")("stop1", "15:00"_t, "15:10"_t)("stop2", "16:00"_t, "16:10"_t)("stop3", "17:00"_t, "17:10"_t)(
            "stop4", "18:00"_t, "18:10"_t);
    b.vj("line:A", "011111")
        .name("vj:2")("stop1", "17:00"_t, "17:10"_t)("stop2", "18:00"_t, "18:10"_t)("stop3", "19:00"_t, "19:10"_t)(
            "stop4", "20:00"_t, "20:10"_t);
    b.vj("line:A", "100100")
        .name("vj:3")("stop1", "12:00"_t, "12:10"_t)("stop2", "13:00"_t, "13:10"_t)("stop3", "14:00"_t, "14:10"_t)(
            "stop4", "15:00"_t, "15:10"_t);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2016, 4, 4), boost::gregorian::days(7));

    // Add a disruption on a section between stop2 and stop3, from the 4th 6am to the 6th 5:30pm
    chaos::Disruption disruption;
    disruption.set_id("dis_ls1");
    // Add a period with valid start and end date
    auto period = disruption.mutable_publication_period();
    period->set_start(ntest::to_posix_timestamp("20160403T060000"));
    period->set_end(ntest::to_posix_timestamp("20160406T173000"));
    auto* impact = disruption.add_impacts();
    impact->set_id("impact_id");
    auto* severity = impact->mutable_severity();
    severity->set_id("severity");
    severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);

    auto* object = impact->add_informed_entities();
    object->set_pt_object_type(chaos::PtObject_Type_line_section);
    object->set_uri("ls_stop2_stop3");
    auto* ls = object->mutable_pt_line_section();

    auto* ls_line = ls->mutable_line();
    ls_line->set_pt_object_type(chaos::PtObject_Type_line);
    ls_line->set_uri("line:A");

    auto* start_stop = ls->mutable_start_point();
    start_stop->set_pt_object_type(chaos::PtObject_Type_stop_area);
    start_stop->set_uri("stop2");
    auto* end_stop = ls->mutable_end_point();
    end_stop->set_pt_object_type(chaos::PtObject_Type_stop_area);
    end_stop->set_uri("stop3");

    auto* app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20160404T060000"));
    app_period->set_end(ntest::to_posix_timestamp("20160406T173000"));

    // 3 new vj will be created to cancel stop_times on stop 2 and 3
    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 6);

    // Check the original vj
    auto* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    auto adapted_vp = vj->adapted_validity_pattern()->days;
    auto base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:2"];
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "011100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "011111"), base_vp);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:3"];
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "100000"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "100100"), base_vp);

    // Check the created vj, they shouldn't have any base validity pattern
    const navitia::type::MetaVehicleJourney* meta_vj = b.get_meta_vj("vj:1");
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);
    const auto* adapted_vj = meta_vj->get_adapted_vj()[0].get();

    adapted_vp = adapted_vj->adapted_validity_pattern()->days;
    base_vp = adapted_vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);

    meta_vj = b.get_meta_vj("vj:2");
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);
    adapted_vj = meta_vj->get_adapted_vj()[0].get();

    adapted_vp = adapted_vj->adapted_validity_pattern()->days;
    base_vp = adapted_vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000011"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);

    meta_vj = b.get_meta_vj("vj:3");
    BOOST_REQUIRE_EQUAL(meta_vj->get_adapted_vj().size(), 1);
    adapted_vj = meta_vj->get_adapted_vj()[0].get();

    adapted_vp = adapted_vj->adapted_validity_pattern()->days;
    base_vp = adapted_vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "000100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "000000"), base_vp);

    // we delete the disruption, everything should be back to normal
    navitia::delete_disruption("dis_ls1", *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:1"];
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "111111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "111111"), base_vp);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:2"];
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "011111"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "011111"), base_vp);

    vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj:3"];
    adapted_vp = vj->adapted_validity_pattern()->days;
    base_vp = vj->base_validity_pattern()->days;
    BOOST_CHECK_MESSAGE(ba::ends_with(adapted_vp.to_string(), "100100"), adapted_vp);
    BOOST_CHECK_MESSAGE(ba::ends_with(base_vp.to_string(), "100100"), base_vp);
}

BOOST_AUTO_TEST_CASE(update_cause_severities_and_tag) {
    ed::builder b("20120614");
    b.vj("A")("stop1", "15:00"_t)("stop2", "16:00"_t);

    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::days(7));

    // we delete the line 1 with a given cause
    auto create_disruption = [&](const std::string& uri, const std::string& cause, const std::string& severity,
                                 const std::string& tag) {
        chaos::Disruption disruption;
        disruption.set_id(uri);
        auto* pb_cause = disruption.mutable_cause();
        pb_cause->set_id("cause_id");  // the cause, severity and tag will always have the same ids
        pb_cause->set_wording(cause);
        auto* pb_tag = disruption.add_tags();
        pb_tag->set_id("tag_id");
        pb_tag->set_name(tag);
        // Add a period with valid start and end date
        auto period = disruption.mutable_publication_period();
        period->set_start(ntest::to_posix_timestamp("20120613T153200"));
        period->set_end(ntest::to_posix_timestamp("20120617T153200"));
        auto* impact = disruption.add_impacts();
        impact->set_id(uri);
        auto* pb_severity = impact->mutable_severity();
        pb_severity->set_id("severity_id");
        pb_severity->set_wording(severity);
        pb_severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
        auto* object = impact->add_informed_entities();
        object->set_pt_object_type(chaos::PtObject_Type_line);
        object->set_uri("A");
        auto* app_period = impact->add_application_periods();
        app_period->set_start("20120614T153200"_pts);
        app_period->set_end("20120616T123200"_pts);

        return disruption;
    };

    navitia::make_and_apply_disruption(create_disruption("first_dis", "dead cow", "important", "a nice tag"),
                                       *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    const auto* nav_dis = b.data->pt_data->disruption_holder.get_disruption("first_dis");
    BOOST_REQUIRE(nav_dis);

    BOOST_REQUIRE(nav_dis->cause);
    BOOST_CHECK_EQUAL(nav_dis->cause->wording, "dead cow");
    BOOST_REQUIRE_EQUAL(nav_dis->tags.size(), 1);
    BOOST_CHECK_EQUAL(nav_dis->tags[0]->name, "a nice tag");
    BOOST_REQUIRE_EQUAL(nav_dis->get_impacts().size(), 1);
    BOOST_REQUIRE(nav_dis->get_impacts()[0]->severity);
    BOOST_CHECK_EQUAL(nav_dis->get_impacts()[0]->severity->wording, "important");

    // we send another disruption that update the cause, the severity and the tag, it should also update the
    // first disruption cause, severity and tag

    navitia::make_and_apply_disruption(create_disruption("dis2", "dead pig", "very important", "an even nicer tag"),
                                       *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 2);
    const auto* dis2 = b.data->pt_data->disruption_holder.get_disruption("dis2");
    BOOST_REQUIRE(dis2);

    BOOST_REQUIRE(dis2->cause);
    BOOST_CHECK_EQUAL(dis2->cause->wording, "dead pig");
    BOOST_REQUIRE_EQUAL(dis2->tags.size(), 1);
    BOOST_CHECK_EQUAL(dis2->tags[0]->name, "an even nicer tag");
    BOOST_REQUIRE_EQUAL(dis2->get_impacts().size(), 1);
    BOOST_REQUIRE(dis2->get_impacts()[0]->severity);
    BOOST_CHECK_EQUAL(dis2->get_impacts()[0]->severity->wording, "very important");

    // nav_dis should have been updated
    BOOST_REQUIRE(nav_dis->cause);
    BOOST_CHECK_EQUAL(nav_dis->cause->wording, "dead pig");
    BOOST_REQUIRE_EQUAL(nav_dis->tags.size(), 1);
    BOOST_CHECK_EQUAL(nav_dis->tags[0]->name, "an even nicer tag");
    BOOST_REQUIRE_EQUAL(nav_dis->get_impacts().size(), 1);
    BOOST_REQUIRE(nav_dis->get_impacts()[0]->severity);
    BOOST_CHECK_EQUAL(nav_dis->get_impacts()[0]->severity->wording, "very important");
}

BOOST_AUTO_TEST_CASE(update_properties) {
    ed::builder b("20171127");

    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::days(7));

    auto create_disruption = [&](const std::vector<std::tuple<std::string, std::string, std::string>>& properties) {
        chaos::Disruption disruption;
        disruption.set_id("disruption");
        auto* pb_cause = disruption.mutable_cause();
        pb_cause->set_id("cause_id");
        pb_cause->set_wording("foo");
        auto* pb_tag = disruption.add_tags();
        pb_tag->set_id("tag_id");
        pb_tag->set_name("tag");
        // Add a period with valid start and end date
        auto period = disruption.mutable_publication_period();
        period->set_start(ntest::to_posix_timestamp("20120613T153200"));
        period->set_end(ntest::to_posix_timestamp("20120616T153200"));
        auto* impact = disruption.add_impacts();
        impact->set_id("impact");
        auto* pb_severity = impact->mutable_severity();
        pb_severity->set_id("severity_id");
        pb_severity->set_wording("important");
        pb_severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
        auto* object = impact->add_informed_entities();
        object->set_pt_object_type(chaos::PtObject_Type_line);
        object->set_uri("A");
        auto* app_period = impact->add_application_periods();
        app_period->set_start("20120614T153200"_pts);
        app_period->set_end("20120616T123200"_pts);
        for (auto const& p : properties) {
            auto* property = disruption.add_properties();
            property->set_key(std::get<0>(p));
            property->set_type(std::get<1>(p));
            property->set_value(std::get<2>(p));
        }

        return disruption;
    };

    const std::tuple<std::string, std::string, std::string> property1("foo", "bar", "42");
    const std::tuple<std::string, std::string, std::string> property2("f", "oo", "bar42");
    std::vector<std::tuple<std::string, std::string, std::string>> properties = {property1, property2};

    navitia::make_and_apply_disruption(create_disruption(properties), *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    const auto* nav_dis = b.data->pt_data->disruption_holder.get_disruption("disruption");
    BOOST_REQUIRE(nav_dis);
    BOOST_CHECK_EQUAL(nav_dis->properties.size(), 2);
    nt::disruption::Property property = *nav_dis->properties.begin();

    BOOST_CHECK_EQUAL(property.key, "f");
    BOOST_CHECK_EQUAL(property.type, "oo");
    BOOST_CHECK_EQUAL(property.value, "bar42");

    property = *std::next(nav_dis->properties.begin(), 1);

    BOOST_CHECK_EQUAL(property.key, "foo");
    BOOST_CHECK_EQUAL(property.type, "bar");
    BOOST_CHECK_EQUAL(property.value, "42");

    const std::tuple<std::string, std::string, std::string> property3("f", "b", "4");
    properties = {property3};

    navitia::make_and_apply_disruption(create_disruption(properties), *b.data->pt_data, *b.data->meta);

    nav_dis = b.data->pt_data->disruption_holder.get_disruption("disruption");

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    BOOST_CHECK_EQUAL(nav_dis->properties.size(), 1);
    property = *nav_dis->properties.begin();

    BOOST_CHECK_EQUAL(property.key, "f");
    BOOST_CHECK_EQUAL(property.type, "b");
    BOOST_CHECK_EQUAL(property.value, "4");

    navitia::make_and_apply_disruption(create_disruption({}), *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);
    nav_dis = b.data->pt_data->disruption_holder.get_disruption("disruption");
    BOOST_CHECK_EQUAL(nav_dis->properties.size(), 0);
}

BOOST_AUTO_TEST_CASE(make_line_section_test) {
    ed::builder b("20171127");

    navitia::type::Data data;
    b.vj("A", "000111")
        .name("vj:1")
        .route("forward")("stop_area:stop1", "08:10"_t, "08:11"_t)("stop_area:stop2", "08:20"_t, "08:21"_t)(
            "stop_area:stop3", "08:30"_t, "08:31"_t)("stop_area:stop4", "08:40"_t, "08:41"_t);
    b.vj("A", "000111")
        .name("vj:2")
        .route("backward")("stop_area:stop1", "09:10"_t, "09:11"_t)("stop_area:stop2", "09:20"_t, "09:21"_t)(
            "stop_area:stop3", "09:30"_t, "09:31"_t)("stop_area:stop4", "09:40"_t, "09:41"_t);
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::days(7));

    chaos::PtObject object;
    object.set_pt_object_type(chaos::PtObject_Type_line_section);
    object.set_uri("ls");
    auto* ls = object.mutable_pt_line_section();

    auto* ls_line = ls->mutable_line();
    ls_line->set_pt_object_type(chaos::PtObject_Type_line);
    ls_line->set_uri("A");

    auto* start_stop = ls->mutable_start_point();
    start_stop->set_pt_object_type(chaos::PtObject_Type_stop_area);
    start_stop->set_uri("stop_area:stop1");
    auto* end_stop = ls->mutable_end_point();
    end_stop->set_pt_object_type(chaos::PtObject_Type_stop_area);
    end_stop->set_uri("stop_area:stop3");

    // basic line_section without routes
    auto line_section = navitia::make_line_section(object, *b.data->pt_data);
    BOOST_REQUIRE(line_section);
    BOOST_REQUIRE_EQUAL(line_section->line->uri, "A");
    BOOST_REQUIRE_EQUAL(line_section->start_point->uri, "stop_area:stop1");
    BOOST_REQUIRE_EQUAL(line_section->end_point->uri, "stop_area:stop3");
    BOOST_REQUIRE_EQUAL(line_section->routes.size(), 2);

    // line_section with invalid line
    ls_line->set_uri("B");
    line_section = navitia::make_line_section(object, *b.data->pt_data);
    BOOST_CHECK(!line_section);

    // line_section with invalid start stop_area
    ls_line->set_uri("A");
    start_stop->set_uri("stop_area:stop10");
    line_section = navitia::make_line_section(object, *b.data->pt_data);
    BOOST_CHECK(!line_section);

    // line_section with invalid end stop_area
    start_stop->set_uri("stop_area:stop1");
    end_stop->set_uri("stop_area:stop10");
    line_section = navitia::make_line_section(object, *b.data->pt_data);
    BOOST_CHECK(!line_section);

    // line_section filtered on one route
    end_stop->set_uri("stop_area:stop3");
    auto ls_route = ls->add_routes();
    ls_route->set_pt_object_type(chaos::PtObject_Type_route);
    ls_route->set_uri("backward");
    line_section = navitia::make_line_section(object, *b.data->pt_data);
    BOOST_REQUIRE(line_section);
    BOOST_REQUIRE_EQUAL(line_section->routes.size(), 1);
    BOOST_CHECK_EQUAL(line_section->routes[0]->uri, "backward");

    // line_section filtered on one route that doesn't exist
    ls_route->set_uri("unknown");
    line_section = navitia::make_line_section(object, *b.data->pt_data);
    BOOST_REQUIRE(!line_section);

    // line_section filtered by routes: one exist, the other doesn't
    ls_route = ls->add_routes();
    ls_route->set_pt_object_type(chaos::PtObject_Type_route);
    ls_route->set_uri("forward");
    line_section = navitia::make_line_section(object, *b.data->pt_data);
    BOOST_REQUIRE(line_section);
    BOOST_REQUIRE_EQUAL(line_section->routes.size(), 1);
    BOOST_CHECK_EQUAL(line_section->routes[0]->uri, "forward");
}

BOOST_AUTO_TEST_CASE(make_disruption_with_different_publication_periods) {
    ed::builder b("20120614");
    b.vj("A")("stop1", "15:00"_t)("stop2", "16:00"_t);

    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 14), boost::gregorian::days(7));

    auto create_disruption = [&](const std::string& uri, const std::string& cause, const std::string& severity,
                                 const std::string& tag) {
        chaos::Disruption disruption;
        disruption.set_id(uri);
        auto* pb_cause = disruption.mutable_cause();
        pb_cause->set_id("cause_id");  // the cause, severity and tag will always have the same ids
        pb_cause->set_wording(cause);
        auto* pb_tag = disruption.add_tags();
        pb_tag->set_id("tag_id");
        pb_tag->set_name(tag);
        auto* impact = disruption.add_impacts();
        impact->set_id(uri);
        auto* pb_severity = impact->mutable_severity();
        pb_severity->set_id("severity_id");
        pb_severity->set_wording(severity);
        pb_severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
        auto* object = impact->add_informed_entities();
        object->set_pt_object_type(chaos::PtObject_Type_line);
        object->set_uri("A");
        auto* app_period = impact->add_application_periods();
        app_period->set_start("20120614T153200"_pts);
        app_period->set_end("20120616T123200"_pts);

        return disruption;
    };
    // Create a disruption without publication_period
    auto disruption = create_disruption("first_dis", "dead cow", "important", "a nice tag");
    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 0);

    // Add a publication period with start date > production end date (2012,6,21)
    // This disruption is again rejected.
    auto period = disruption.mutable_publication_period();
    period->set_start(ntest::to_posix_timestamp("20120622T153200"));
    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 0);

    // Modify the publication start date with the value < production end date (2012,6,21)
    // The disruption will be added.
    period->set_start(ntest::to_posix_timestamp("20120618T153200"));
    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 1);

    // create a disruption and associate a period
    disruption = create_disruption("second_dis", "dead cow", "important", "a nice tag");

    // Add a period with valid start and end date
    period = disruption.mutable_publication_period();
    period->set_start(ntest::to_posix_timestamp("20120613T153200"));
    period->set_end(ntest::to_posix_timestamp("20120617T123200"));
    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 2);
    const auto* nav_dis = b.data->pt_data->disruption_holder.get_disruption("second_dis");
    BOOST_REQUIRE(nav_dis);
    BOOST_REQUIRE(nav_dis->cause);
    BOOST_CHECK_EQUAL(nav_dis->cause->wording, "dead cow");
    BOOST_REQUIRE_EQUAL(nav_dis->tags.size(), 1);
    BOOST_CHECK_EQUAL(nav_dis->tags[0]->name, "a nice tag");
    BOOST_REQUIRE_EQUAL(nav_dis->get_impacts().size(), 1);
    BOOST_REQUIRE(nav_dis->get_impacts()[0]->severity);
    BOOST_CHECK_EQUAL(nav_dis->get_impacts()[0]->severity->wording, "important");

    // Send another disruption with publication period having start > end
    // This disruption should be rejected.
    disruption = create_disruption("dis2", "dead pig", "very important", "an even nicer tag");

    // Add a period with valid start and end date having start > end date
    period = disruption.mutable_publication_period();
    period->set_start(ntest::to_posix_timestamp("20120617T123200"));
    period->set_end(ntest::to_posix_timestamp("20120613T153200"));

    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 2);

    // Modify publication period with valid value but does not intersect with production period
    // production_period : 2012-Jun-14 00:00:00 + 2012-Jun-21 00:00:00
    // publication_period: 2012-Jun-22 12:32:00 + 2012-Jun-25 15:32:00
    // The disruption will again be rejected.
    period->set_start(ntest::to_posix_timestamp("20120622T123200"));
    period->set_end(ntest::to_posix_timestamp("20120625T153200"));
    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 2);

    // Modify publication period with valid value and intersect with production period
    // publication_period: 2012-Jun-13 15:32:00 + 2012-Jun-17 12:32:00
    // The disruption will be added.
    period->set_start(ntest::to_posix_timestamp("20120613T153200"));
    period->set_end(ntest::to_posix_timestamp("20120617T123200"));
    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_EQUAL(b.data->pt_data->disruption_holder.nb_disruptions(), 3);
}

BOOST_AUTO_TEST_CASE(is_publishable_test) {
    // Initialize production_period as in data
    auto production_period = boost::posix_time::time_period("20180317T155000"_dt, "20180817T113000"_dt);

    // Initialize publication_period with the same format as in disruption
    // Initialize only start date with value < production_end. The test should be true
    auto publication_period = transit_realtime::TimeRange();
    publication_period.set_start(ntest::to_posix_timestamp("20180317T155000"));
    BOOST_CHECK_EQUAL(navitia::is_publishable(publication_period, production_period), true);

    // Modify publication_period start date with value > production_end. The test should be false
    publication_period.set_start(ntest::to_posix_timestamp("20180818T113000"));
    BOOST_CHECK_EQUAL(navitia::is_publishable(publication_period, production_period), false);

    // Modify publication_period start and end date so that two periods do not intersect
    // The test should be true
    publication_period.set_start(ntest::to_posix_timestamp("20180818T113000"));
    publication_period.set_end(ntest::to_posix_timestamp("20180820T113000"));
    BOOST_CHECK_EQUAL(navitia::is_publishable(publication_period, production_period), false);

    // Modify publication_period start and end date where start > end
    // The test should be true
    publication_period.set_start(ntest::to_posix_timestamp("20180518T113000"));
    publication_period.set_end(ntest::to_posix_timestamp("20180420T113000"));
    BOOST_CHECK_EQUAL(navitia::is_publishable(publication_period, production_period), false);

    // Modify publication_period start and end date with valid value -> two periods intersect
    // The test should be true
    publication_period.set_start(ntest::to_posix_timestamp("20180618T113000"));
    publication_period.set_end(ntest::to_posix_timestamp("20180623T113000"));
    BOOST_CHECK_EQUAL(navitia::is_publishable(publication_period, production_period), true);
}

BOOST_AUTO_TEST_CASE(add_impact_on_line_with_many_time_slots) {
    ed::builder b("20120614");
    b.vj("A", "000111", "", true, "vj:1")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)(
        "stop_area:stop2", 8 * 3600 + 20 * 60, 8 * 3600 + 21 * 60);
    b.vj("A", "000111", "", true, "vj:2")("stop_area:stop1", 9 * 3600 + 10 * 60, 9 * 3600 + 11 * 60)(
        "stop_area:stop2", 9 * 3600 + 20 * 60, 9 * 3600 + 21 * 60);
    b.vj("A", "001101", "", true, "vj:3")("stop_area:stop1", 16 * 3600 + 10 * 60, 16 * 3600 + 11 * 60)(
        "stop_area:stop2", 16 * 3600 + 20 * 60, 16 * 3600 + 21 * 60);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 03), boost::gregorian::days(30));

    // we delete the line A for three day, the first two vj start too early for being impacted on the first day
    // the third vj start too late for being impacted the last day

    chaos::Disruption disruption;
    disruption.set_id("test01");
    // Add a period with valid start date without end date
    auto period = disruption.mutable_publication_period();
    period->set_start(ntest::to_posix_timestamp("20120613T153200"));
    auto* impact = disruption.add_impacts();
    impact->set_id("impact_id");
    auto* severity = impact->mutable_severity();
    severity->set_id("severity");
    severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
    auto* object = impact->add_informed_entities();
    object->set_pt_object_type(chaos::PtObject_Type_line);
    object->set_uri("A");
    /*
        Appelication patterns
                        monday tuesday wednesday thursday friday saturday sunday
     10:32 -> 12:32        1      0        0        1        1     1         0
    */
    auto* app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120604T103200"));
    app_period->set_end(ntest::to_posix_timestamp("20120604T123200"));

    app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120607T103200"));
    app_period->set_end(ntest::to_posix_timestamp("20120607T123200"));

    app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120608T103200"));
    app_period->set_end(ntest::to_posix_timestamp("20120608T123200"));
    app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120609T103200"));
    app_period->set_end(ntest::to_posix_timestamp("20120609T123200"));

    /*
        Appelication patterns
                        monday tuesday wednesday thursday friday saturday sunday
     15:32 -> 17:32        1      0        0        1        1     1         0
    */
    app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120604T153200"));
    app_period->set_end(ntest::to_posix_timestamp("20120604T173200"));

    app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120607T153200"));
    app_period->set_end(ntest::to_posix_timestamp("20120607T173200"));

    app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120608T153200"));
    app_period->set_end(ntest::to_posix_timestamp("20120608T173200"));
    app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120609T153200"));
    app_period->set_end(ntest::to_posix_timestamp("20120609T173200"));

    auto* ch_application_pattern = impact->add_application_patterns();
    ch_application_pattern->set_start_date(ntest::to_int_date("20120604T000000"));
    ch_application_pattern->set_end_date(ntest::to_int_date("20120610T000000"));

    auto week = ch_application_pattern->mutable_week_pattern();
    week->set_monday(true);
    week->set_tuesday(false);
    week->set_wednesday(false);
    week->set_thursday(true);
    week->set_friday(true);
    week->set_saturday(true);
    week->set_sunday(false);

    auto* ch_time_slot = ch_application_pattern->add_time_slots();
    ch_time_slot->set_begin("10:32"_t);
    ch_time_slot->set_end("12:32"_t);

    ch_time_slot = ch_application_pattern->add_time_slots();
    ch_time_slot->set_begin("15:32"_t);
    ch_time_slot->set_end("17:32"_t);

    navitia::make_and_apply_disruption(disruption, *b.data->pt_data, *b.data->meta);

    auto impacts = b.data->pt_data->disruption_holder.get_weak_impacts();
    BOOST_REQUIRE_EQUAL(impacts.size(), 1);
    auto impact_sptr = impacts[0].lock();
    BOOST_REQUIRE_EQUAL(impact_sptr->application_patterns.size(), 1);
    auto application_pattern = *(impact_sptr->application_patterns.begin());
    BOOST_REQUIRE_EQUAL(
        application_pattern.application_period,
        boost::gregorian::date_period(boost::gregorian::date(2012, 6, 4), boost::gregorian::date(2012, 6, 10)));
    BOOST_REQUIRE_EQUAL(application_pattern.time_slots.size(), 2);

    auto time_slot = *(application_pattern.time_slots.begin());
    BOOST_REQUIRE_EQUAL(time_slot.begin, "10:32"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "12:32"_t);

    time_slot = *std::next(application_pattern.time_slots.begin(), 1);
    BOOST_REQUIRE_EQUAL(time_slot.begin, "15:32"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end, "17:32"_t);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern.to_string(), "0111001");

    BOOST_CHECK_EQUAL(application_pattern.week_pattern[navitia::Monday], true);
    BOOST_CHECK_EQUAL(application_pattern.week_pattern[navitia::Tuesday], false);
    BOOST_CHECK_EQUAL(application_pattern.week_pattern[navitia::Wednesday], false);
    BOOST_CHECK_EQUAL(application_pattern.week_pattern[navitia::Thursday], true);
    BOOST_CHECK_EQUAL(application_pattern.week_pattern[navitia::Friday], true);
    BOOST_CHECK_EQUAL(application_pattern.week_pattern[navitia::Saturday], true);
    BOOST_CHECK_EQUAL(application_pattern.week_pattern[navitia::Sunday], false);
}
