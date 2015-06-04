/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

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
#define BOOST_TEST_MODULE fill_disruption_from_chaos_test
#include <boost/test/unit_test.hpp>
#include "routing/raptor_api.h"
#include "ed/build_helper.h"
#include "routing/tests/routing_api_test_data.h"
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "type/data.h"
#include "kraken/fill_disruption_from_chaos.h"
#include "type/chaos.pb.h"
#include "type/gtfs-realtime.pb.h"
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace ntest = navitia::test;
namespace bt = boost::posix_time;
namespace ba = boost::algorithm;


struct stop_area_finder{
    std::string sa_uri;
    stop_area_finder(const std::string& sa_uri): sa_uri(sa_uri){}

    bool operator()(const nt::JourneyPatternPoint* jpp){
        return jpp->stop_point->stop_area->uri == sa_uri;
    }

};

std::string dump_vj(const nt::VehicleJourney& vj){
    std::stringstream ss;
    ss << vj.uri << std::endl;
    for(const auto& st: vj.stop_time_list){
    ss << "\t" << st.journey_pattern_point->stop_point->uri << ": " << st.arrival_time << " - " << st.departure_time << std::endl;
    }
    ss << "\tvalidity_pattern: " << ba::trim_left_copy_if(vj.validity_pattern->days.to_string(), boost::is_any_of("0")) << std::endl;
    ss << "\tadapted_validity_pattern: " << ba::trim_left_copy_if(vj.adapted_validity_pattern->days.to_string(), boost::is_any_of("0")) << std::endl;
    return ss.str();

}

BOOST_AUTO_TEST_CASE(add_impact_on_line) {
    ed::builder b("20120614");
    b.vj("A", "000111", "", true, "vj:1")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop2", 8*3600 + 20 * 60, 8*3600 + 21*60);
    b.vj("A", "000111", "", true, "vj:2")("stop_area:stop1", 9*3600 +10*60, 9*3600 + 11 * 60)("stop_area:stop2", 9*3600 + 20 * 60, 9*3600 + 21*60);
    b.vj("A", "001101", "", true, "vj:3")("stop_area:stop1", 16*3600 +10*60, 16*3600 + 11 * 60)("stop_area:stop2", 16*3600 + 20 * 60, 16*3600 + 21*60);
    navitia::type::Data data;
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

    //we delete the line A for three day, the first two vj start too early for being impacted on the first day
    //the third vj start too late for being impacted the last day


    chaos::Disruption disruption;
    disruption.set_id("test01");
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


    navitia::add_disruption(disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->journey_patterns.size(), 1);
    auto* vj = b.data->pt_data->vehicle_journeys_map["vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000001"), vj->adapted_validity_pattern->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "000111"), vj->validity_pattern->days);

    vj = b.data->pt_data->vehicle_journeys_map["vj:2"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000001"), vj->adapted_validity_pattern->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "000111"), vj->validity_pattern->days);

    vj = b.data->pt_data->vehicle_journeys_map["vj:3"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "001100"), vj->adapted_validity_pattern->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "001101"), vj->validity_pattern->days);

    //we delete the disruption, all must be as before!
    navitia::delete_disruption("test01", *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->journey_patterns.size(), 1);
    vj = b.data->pt_data->vehicle_journeys_map["vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000111"), vj->adapted_validity_pattern->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "000111"), vj->validity_pattern->days);

    vj = b.data->pt_data->vehicle_journeys_map["vj:2"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000111"), vj->adapted_validity_pattern->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "000111"), vj->validity_pattern->days);

    vj = b.data->pt_data->vehicle_journeys_map["vj:3"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "001101"), vj->adapted_validity_pattern->days);
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "001101"), vj->validity_pattern->days);
}

BOOST_AUTO_TEST_CASE(add_impact_on_stop_area) {
    ed::builder b("20120614");
    b.vj("A", "000111")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop2", 8*3600 + 20 * 60, 8*3600 + 21*60);
    b.vj("A", "000111")("stop_area:stop1", 9*3600 +10*60, 9*3600 + 11 * 60)("stop_area:stop2", 9*3600 + 20 * 60, 9*3600 + 21*60);
    navitia::type::Data data;
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

    //we delete the stop_area 1, two vj are impacted, we create a new journey pattern without this stop_area
    //and two new vj are enable on this journey pattern, of course the theorical vj are disabled

    chaos::Disruption disruption;
    disruption.set_id("test01");
    auto* impact = disruption.add_impacts();
    impact->set_id("impact_id");
    auto* severity = impact->mutable_severity();
    severity->set_id("severity");
    severity->set_effect(transit_realtime::Alert_Effect_NO_SERVICE);
    auto* object = impact->add_informed_entities();
    object->set_pt_object_type(chaos::PtObject_Type_stop_area);
    object->set_uri("stop_area:stop1");
    auto* app_period = impact->add_application_periods();
    app_period->set_start(ntest::to_posix_timestamp("20120614T173200"));
    app_period->set_end(ntest::to_posix_timestamp("20120618T123200"));


    navitia::add_disruption(disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->journey_patterns.size(), 2);
    bool has_adapted_vj = false;
    for(const auto* vj: b.data->pt_data->vehicle_journeys){
        if(vj->is_adapted){
            has_adapted_vj = true;
            BOOST_CHECK(boost::find_if(vj->journey_pattern->journey_pattern_point_list,
                        stop_area_finder("stop_area:stop1")) == vj->journey_pattern->journey_pattern_point_list.end());
            BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000110"), vj->adapted_validity_pattern->days);
        }else{
            BOOST_CHECK(boost::find_if(vj->journey_pattern->journey_pattern_point_list,
                        stop_area_finder("stop_area:stop1")) != vj->journey_pattern->journey_pattern_point_list.end());
            BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000001"), vj->adapted_validity_pattern->days);
            BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "000111"), vj->validity_pattern->days);
        }
    }
    BOOST_REQUIRE(has_adapted_vj);
}

BOOST_AUTO_TEST_CASE(add_impact_and_update_on_stop_area) {
    ed::builder b("20120614");
    b.vj("A", "000111", "", true, "vj:1")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60)
            ("stop_area:stop3", 8*3600 + 30 * 60 ,8*3600 + 31*60)("stop_area:stop4", 8*3600 + 40 * 60 ,8*3600 + 41*60);
    b.vj("A", "000111", "", true, "vj:2")("stop_area:stop1", 9*3600 +10*60, 9*3600 + 11 * 60)("stop_area:stop2", 9*3600 + 20 * 60 ,9*3600 + 21*60)
            ("stop_area:stop3", 9*3600 + 30 * 60 ,9*3600 + 31*60)("stop_area:stop4", 9*3600 + 40 * 60 ,9*3600 + 41*60);
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

    //mostly like the previous test, but we disable the stop_area 1 and 2
    //some useless journey pattern and vj are created but won't circulate
    //then we update the disruption (without any change) this way we check all the process (previously there was some segfault)

    chaos::Disruption disruption;
    {
        disruption.set_id("test01");
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
    }

    auto check = [](const nt::Data& data){
        BOOST_REQUIRE_EQUAL(data.pt_data->lines.size(), 1);
        BOOST_CHECK_EQUAL(data.pt_data->vehicle_journeys.size(), 6);//two of them don't circulate :(
        BOOST_CHECK_EQUAL(data.pt_data->journey_patterns.size(), 4);//some of them aren't used
        bool has_adapted_vj = false;
        for(const auto* vj: data.pt_data->vehicle_journeys){
            if(vj->adapted_validity_pattern->days.none() && vj->validity_pattern->days.none()){
                //some vj don't circulate we don't want to check them
                continue;
            }
            if(vj->is_adapted){
                has_adapted_vj = true;
                BOOST_CHECK_MESSAGE(boost::find_if(vj->journey_pattern->journey_pattern_point_list,
                            stop_area_finder("stop_area:stop1")) == vj->journey_pattern->journey_pattern_point_list.end(), dump_vj(*vj));
                BOOST_CHECK_MESSAGE(boost::find_if(vj->journey_pattern->journey_pattern_point_list,
                            stop_area_finder("stop_area:stop2")) == vj->journey_pattern->journey_pattern_point_list.end(), dump_vj(*vj));
            }else{
                BOOST_CHECK(boost::find_if(vj->journey_pattern->journey_pattern_point_list,
                            stop_area_finder("stop_area:stop1")) != vj->journey_pattern->journey_pattern_point_list.end());
            }

        }
        BOOST_REQUIRE(has_adapted_vj);

        auto* vj = data.pt_data->vehicle_journeys_map["vj:1"];
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000001"), vj->adapted_validity_pattern->days);
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "000111"), vj->validity_pattern->days);

        vj = data.pt_data->vehicle_journeys_map["vj:2"];
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000001"), vj->adapted_validity_pattern->days);
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->validity_pattern->days.to_string(), "000111"), vj->validity_pattern->days);

        //useless vj, need to be deleted...
        vj = data.pt_data->vehicle_journeys_map["vj:1:adapted-2"];
        BOOST_CHECK(vj->adapted_validity_pattern->days.none());
        BOOST_CHECK(vj->adapted_validity_pattern->days.none());
        //
        //useless vj, need to be deleted...
        vj = data.pt_data->vehicle_journeys_map["vj:2:adapted-3"];
        BOOST_CHECK(vj->adapted_validity_pattern->days.none());
        BOOST_CHECK(vj->adapted_validity_pattern->days.none());

        vj = data.pt_data->vehicle_journeys_map["vj:1:adapted-2:adapted-4"];
        BOOST_CHECK(vj->validity_pattern->days.none());
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000110"), vj->adapted_validity_pattern->days);

        vj = data.pt_data->vehicle_journeys_map["vj:2:adapted-3:adapted-5"];
        BOOST_CHECK(vj->validity_pattern->days.none());
        BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "000110"), vj->adapted_validity_pattern->days);
    };

    navitia::add_disruption(disruption, *b.data->pt_data, *b.data->meta);
    check(*b.data);

    navitia::add_disruption(disruption, *b.data->pt_data, *b.data->meta);
    check(*b.data);

    navitia::delete_disruption(disruption.id(), *b.data->pt_data, *b.data->meta);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(b.data->pt_data->journey_patterns.size(), 1);
    for(const auto* vj: b.data->pt_data->vehicle_journeys){
        if(vj->is_adapted){
            BOOST_FAIL("no adapted vj expected");
        }
        BOOST_CHECK(vj->validity_pattern->days == vj->adapted_validity_pattern->days);
    }
    for(const auto* sp: b.data->pt_data->stop_points){
        if(sp->journey_pattern_point_list.size() > 1){
            BOOST_FAIL("some adapted jpp are not removed for " << sp->uri);
        }

    }

}



BOOST_AUTO_TEST_CASE(add_impact_on_line_over_midnigt) {
    ed::builder b("20120614");
    b.vj("A", "010101", "", true, "vj:1")("stop_area:stop1", 23*3600 +10*60, 23*3600 + 11*60)("stop_area:stop2", 24*3600 + 20*60, 24*3600 + 21*60);
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

    //same as before but with a vehicle_journey that span on two day

    chaos::Disruption disruption;
    disruption.set_id("test01");
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


    navitia::add_disruption(disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->journey_patterns.size(), 1);
    auto* vj = b.data->pt_data->vehicle_journeys_map["vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "010001"), dump_vj(*vj));
}

BOOST_AUTO_TEST_CASE(add_impact_on_line_over_midnigt_2) {
    ed::builder b("20120614");
    b.vj("A", "010111", "", true, "vj:1")("stop_area:stop1", 23*3600 +10*60, 23*3600 + 11*60)("stop_area:stop2", 24*3600 + 20*60, 24*3600 + 21*60);
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012,6,14), boost::gregorian::days(7));

    //again a vehicle_journey than span on two day, but this time the disruption only start the second day,
    //and there is no vj with a starting circulation date for that day.
    //But the vehicle journey of the first day is impacted since is also circulate on the second day!

    chaos::Disruption disruption;
    disruption.set_id("test01");
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


    navitia::add_disruption(disruption, *b.data->pt_data, *b.data->meta);

    BOOST_REQUIRE_EQUAL(b.data->pt_data->lines.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
    BOOST_REQUIRE_EQUAL(b.data->pt_data->journey_patterns.size(), 1);
    auto* vj = b.data->pt_data->vehicle_journeys_map["vj:1"];
    BOOST_CHECK_MESSAGE(ba::ends_with(vj->adapted_validity_pattern->days.to_string(), "010110"), dump_vj(*vj));
}
