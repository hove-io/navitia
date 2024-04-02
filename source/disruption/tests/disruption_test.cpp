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
#define BOOST_TEST_MODULE test_disruption

#include "disruption/traffic_reports_api.h"
#include "disruption/line_reports_api.h"
#include "routing/raptor.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "type/chaos.pb.h"
#include "type/datetime.h"
#include "type/pb_converter.h"
#include "kraken/apply_disruption.h"
#include "ptreferential/ptreferential.h"

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/*

publicationDate    |------------------------------------------------------------------------------------------------|
ApplicationDate                            |--------------------------------------------|

Test1       * 20140101T0900
Test2                   * 20140103T0900
Test3                                           *20140113T0900
Test4                                                                                               *20140203T0900
Test5 *20140212T0900


start_publication_date  = 2014-01-02 08:32:00
end_publication_date    = 2014-02-10 12:32:00

start_application_date  = 2014-01-12 08:32:00
end_application_date    = 2014-02-02 18:32:00

start_application_daily_hour = 08h40
end_application_daily_hour = 18h00
active_days = 11111111
*/
struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

using namespace navitia;

using boost::posix_time::ptime;
using boost::posix_time::time_period;
using navitia::type::disruption::Disruption;
using navitia::type::disruption::Impact;
using navitia::type::disruption::PtObj;
using navitia::type::disruption::Severity;

namespace {

void disrupt(ed::builder& b,
             const std::string& pertub_name,
             nt::Type_e pt_obj_type,
             const std::string& pt_obj_name,
             time_period publish_period,
             time_period application_period,
             const std::string& pertub_tag = "",
             const nt::disruption::Effect effect = nt::disruption::Effect::NO_SERVICE) {
    auto& disruption = b.disrupt(nt::RTLevel::Adapted, pertub_name)
                           .tag_if_not_empty(pertub_tag)
                           .impact()
                           .severity(effect)
                           .on(pt_obj_type, pt_obj_name, *b.data->pt_data)
                           .application_periods(application_period)
                           .publish(publish_period)
                           .get_disruption();

    navitia::apply_disruption(disruption, *b.data->pt_data, *b.data->meta);
}
}  // namespace

class Params {
public:
    std::vector<std::string> forbidden;
    ed::builder b;
    u_int64_t end_date;

    Params()
        : b("20120614",
            [](ed::builder& b) {
                b.vj_with_network("network:R", "line:A", "11111111", "", true, "")(
                    "stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 8 * 3600 + 20 * 60,
                                                                               8 * 3600 + 21 * 60);
                b.vj_with_network("network:R", "line:S", "11111111", "", true, "")(
                    "stop_area:stop5", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop6", 8 * 3600 + 20 * 60,
                                                                               8 * 3600 + 21 * 60);
                b.vj_with_network("network:K", "line:B", "11111111", "", true, "")(
                    "stop_area:stop3", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop4", 8 * 3600 + 20 * 60,
                                                                               8 * 3600 + 21 * 60);
                b.vj_with_network("network:M", "line:M", "11111111", "", true, "")(
                    "stop_area:stop22", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop22", 8 * 3600 + 20 * 60,
                                                                                8 * 3600 + 21 * 60);
                b.vj_with_network("network:Test", "line:test", "11111111", "", true, "")(
                    "stop_area:stop22", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop22", 8 * 3600 + 20 * 60,
                                                                                8 * 3600 + 21 * 60);
            }),
          end_date("20130614T000000"_pts) {
        for (navitia::type::Line* line : b.data->pt_data->lines) {
            line->network->line_list.push_back(line);
        }

        using btp = boost::posix_time::time_period;
        nt::disruption::ApplicationPattern application_pattern;
        application_pattern.application_period = boost::gregorian::date_period("20131220"_d, "20131221"_d);
        application_pattern.add_time_slot("12:32"_t, "12:33"_t);
        application_pattern.week_pattern[navitia::Friday] = true;
        application_pattern.week_pattern[navitia::Saturday] = true;
        b.impact(nt::RTLevel::Adapted)
            .uri("mess1")
            .publish(btp("20131219T123200"_dt, "20131221T123201"_dt))
            .application_periods(btp("20131220T123200"_dt, "20131221T123201"_dt))
            .application_patterns(application_pattern)
            .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
            .on(nt::Type_e::Line, "line:A", *b.data->pt_data);

        application_pattern = nt::disruption::ApplicationPattern();
        application_pattern.application_period = boost::gregorian::date_period("20131220"_d, "20131221"_d);
        application_pattern.add_time_slot("08:32"_t, "12:32"_t);
        application_pattern.week_pattern[navitia::Friday] = true;
        application_pattern.week_pattern[navitia::Saturday] = true;

        b.impact(nt::RTLevel::Adapted)
            .uri("mess0")
            .publish(btp("20131221T083200"_dt, "20131221T123201"_dt))
            .application_periods(btp("20131221T083200"_dt, "20131221T123201"_dt))
            .application_patterns(application_pattern)
            .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
            .on(nt::Type_e::Line, "line:S", *b.data->pt_data);

        application_pattern = nt::disruption::ApplicationPattern();
        application_pattern.application_period = boost::gregorian::date_period("20131223"_d, "20131225"_d);
        application_pattern.add_time_slot("12:32"_t, "12:33"_t);
        application_pattern.week_pattern[navitia::Monday] = true;
        application_pattern.week_pattern[navitia::Tuesday] = true;
        application_pattern.week_pattern[navitia::Wednesday] = true;

        b.impact(nt::RTLevel::Adapted)
            .uri("mess2")
            .application_periods(btp("20131223T123200"_dt, "20131225T123201"_dt))
            .publish(btp("20131224T123200"_dt, "20131226T123201"_dt))
            .application_patterns(application_pattern)
            .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
            .on(nt::Type_e::Network, "network:K", *b.data->pt_data);
    }
};

BOOST_FIXTURE_TEST_CASE(network_filter1, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20131219T155000"_dt;
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *(b.data), 1, 10, 0, "network.uri=network:R", forbidden_uris,
                                         boost::none, boost::none);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 1);

    auto disruptions = resp.traffic_reports(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 1);
    BOOST_CHECK_EQUAL(disruptions.network().uri(), "network:R");

    pbnavitia::Line line = disruptions.lines(0);
    BOOST_REQUIRE_EQUAL(line.uri(), "line:A");

    BOOST_REQUIRE_EQUAL(line.impact_uris_size(), 1);
    const auto* impact = navitia::test::get_impact(line.impact_uris(0), resp);
    BOOST_REQUIRE(impact);
    BOOST_CHECK_EQUAL(impact->uri(), "mess1");
    // we are in the publication period of the disruption but the application_period of the impact is in the future
    BOOST_CHECK_EQUAL(impact->status(), pbnavitia::ActiveStatus::future);

    BOOST_REQUIRE_EQUAL(impact->application_periods_size(), 1);
    BOOST_CHECK_EQUAL(impact->application_periods(0).begin(), navitia::test::to_posix_timestamp("20131220T123200"));
    BOOST_CHECK_EQUAL(impact->application_periods(0).end(), navitia::test::to_posix_timestamp("20131221T123201"));

    BOOST_REQUIRE_EQUAL(impact->application_patterns_size(), 1);
    auto application_pattern = impact->application_patterns(0);
    BOOST_CHECK_EQUAL(application_pattern.application_period().begin(),
                      navitia::test::to_posix_timestamp("20131220T000000"));
    BOOST_CHECK_EQUAL(application_pattern.application_period().end(),
                      navitia::test::to_posix_timestamp("20131221T000000"));

    BOOST_REQUIRE_EQUAL(application_pattern.time_slots_size(), 1);
    auto time_slot = application_pattern.time_slots(0);
    BOOST_REQUIRE_EQUAL(time_slot.begin(), "12:32"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end(), "12:33"_t);

    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().monday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().tuesday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().wednesday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().thursday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().friday(), true);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().saturday(), true);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().sunday(), false);
}

BOOST_FIXTURE_TEST_CASE(network_filter2, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20131224T125000"_dt;
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *(b.data), 1, 10, 0, "network.uri=network:K", forbidden_uris,
                                         boost::none, boost::none);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 1);

    auto disruptions = resp.traffic_reports(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruptions.network().uri(), "network:K");
    pbnavitia::Network network = disruptions.network();
    BOOST_REQUIRE_EQUAL(network.impact_uris_size(), 1);
    const auto* impact = navitia::test::get_impact(network.impact_uris(0), resp);
    BOOST_REQUIRE(impact);
    BOOST_REQUIRE_EQUAL(impact->uri(), "mess2");
    BOOST_CHECK_EQUAL(impact->status(), pbnavitia::ActiveStatus::active);
    BOOST_REQUIRE_EQUAL(impact->application_periods_size(), 1);
    BOOST_CHECK_EQUAL(impact->application_periods(0).begin(), navitia::test::to_posix_timestamp("20131223T123200"));
    BOOST_CHECK_EQUAL(impact->application_periods(0).end(), navitia::test::to_posix_timestamp("20131225T123201"));

    BOOST_REQUIRE_EQUAL(impact->application_patterns_size(), 1);
    auto application_pattern = impact->application_patterns(0);
    BOOST_CHECK_EQUAL(application_pattern.application_period().begin(),
                      navitia::test::to_posix_timestamp("20131223T000000"));
    BOOST_CHECK_EQUAL(application_pattern.application_period().end(),
                      navitia::test::to_posix_timestamp("20131225T000000"));

    BOOST_REQUIRE_EQUAL(application_pattern.time_slots_size(), 1);
    auto time_slot = application_pattern.time_slots(0);
    BOOST_REQUIRE_EQUAL(time_slot.begin(), "12:32"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end(), "12:33"_t);

    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().monday(), true);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().tuesday(), true);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().wednesday(), true);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().thursday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().friday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().saturday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().sunday(), false);
}

BOOST_FIXTURE_TEST_CASE(line_filter, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20131221T085000"_dt;
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *(b.data), 1, 10, 0, "line.uri=line:S", forbidden_uris,
                                         boost::none, boost::none);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 1);

    auto disruptions = resp.traffic_reports(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 1);
    BOOST_REQUIRE_EQUAL(disruptions.network().uri(), "network:R");

    pbnavitia::Line line = disruptions.lines(0);
    BOOST_REQUIRE_EQUAL(line.uri(), "line:S");

    BOOST_REQUIRE_EQUAL(line.impact_uris_size(), 1);
    const auto* impact = navitia::test::get_impact(line.impact_uris(0), resp);
    BOOST_REQUIRE(impact);
    BOOST_REQUIRE_EQUAL(impact->uri(), "mess0");
    BOOST_CHECK_EQUAL(impact->status(), pbnavitia::ActiveStatus::active);

    BOOST_REQUIRE_EQUAL(impact->application_periods_size(), 1);
    BOOST_CHECK_EQUAL(impact->application_periods(0).begin(), navitia::test::to_posix_timestamp("20131221T083200"));
    BOOST_CHECK_EQUAL(impact->application_periods(0).end(), navitia::test::to_posix_timestamp("20131221T123201"));

    BOOST_REQUIRE_EQUAL(impact->application_patterns_size(), 1);
    auto application_pattern = impact->application_patterns(0);
    BOOST_CHECK_EQUAL(application_pattern.application_period().begin(),
                      navitia::test::to_posix_timestamp("20131220T000000"));
    BOOST_CHECK_EQUAL(application_pattern.application_period().end(),
                      navitia::test::to_posix_timestamp("20131221T000000"));

    BOOST_REQUIRE_EQUAL(application_pattern.time_slots_size(), 1);
    auto time_slot = application_pattern.time_slots(0);
    BOOST_REQUIRE_EQUAL(time_slot.begin(), "08:32"_t);
    BOOST_REQUIRE_EQUAL(time_slot.end(), "12:32"_t);

    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().monday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().tuesday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().wednesday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().thursday(), false);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().friday(), true);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().saturday(), true);
    BOOST_REQUIRE_EQUAL(application_pattern.week_pattern().sunday(), false);
}

BOOST_FIXTURE_TEST_CASE(Test1, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20140101T0900"_dt;
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *(b.data), 1, 10, 0, "", forbidden_uris, boost::none, boost::none);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}

BOOST_FIXTURE_TEST_CASE(Test2, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20131226T0900"_dt;
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *(b.data), 1, 10, 0, "", forbidden_uris, boost::none, boost::none);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.traffic_reports_size(), 1);

    auto disruptions = resp.traffic_reports(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruptions.network().uri(), "network:K");

    BOOST_REQUIRE_EQUAL(disruptions.network().impact_uris_size(), 1);

    const auto* impact = navitia::test::get_impact(disruptions.network().impact_uris(0), resp);
    BOOST_REQUIRE(impact);
    BOOST_REQUIRE_EQUAL(impact->uri(), "mess2");
    BOOST_CHECK_EQUAL(impact->status(), pbnavitia::ActiveStatus::past);
}

BOOST_FIXTURE_TEST_CASE(Test4, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20130203T0900"_dt;
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *(b.data), 1, 10, 0, "", forbidden_uris, boost::none, boost::none);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}

BOOST_FIXTURE_TEST_CASE(Test5, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20130212T0900"_dt;
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    navitia::disruption::traffic_reports(pb_creator, *(b.data), 1, 10, 0, "", forbidden_uris, boost::none, boost::none);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}

struct DisruptedNetwork {
    ed::builder b;
    navitia::PbCreator pb_creator;
    const boost::posix_time::ptime since = "20180102T060000"_dt;
    const boost::posix_time::ptime until = "20180103T060000"_dt;

    DisruptedNetwork(boost::posix_time::ptime query_date = /*since*/ "20180102T060000"_dt)
        : b("20180101", [](ed::builder& b) {
              b.vj_with_network("network_1", "line_1").route("route_1")("sp1_1", "08:10"_t)("sp1_2", "08:20"_t);
              b.vj_with_network("network_2", "line_2").route("route_2")("sp2_1", "08:10"_t)("sp2_2", "08:20"_t);
              b.vj_with_network("network_3", "line_3")
                  .route("route_3")("sp3_1", "08:10"_t)("sp3_2", "08:20"_t)("sp3_3", "08:30"_t)("sp3_4", "08:40"_t);
          }) {
        auto period = time_period(since, until);
        auto publish_period = time_period(since - boost::posix_time::hours(48), until + boost::posix_time::hours(48));

        // Let's create a disruption for all the different pt_objects related to
        // the line 'line_1'
        disrupt(b, "disrup_line_1", nt::Type_e::Line, "line_1", publish_period, period, "TAG_LINE_1");
        disrupt(b, "disrup_network_1", nt::Type_e::Network, "network_1", publish_period, period, "TAG_NETWORK_1");
        disrupt(b, "disrup_route_1", nt::Type_e::Route, "route_1", publish_period, period, "TAG_ROUTE_1");
        disrupt(b, "disrup_sa_1", nt::Type_e::StopArea, "sp1_1", publish_period, period, "TAG_SA_1");
        disrupt(b, "disrup_sp_1", nt::Type_e::StopPoint, "sp1_1", publish_period, period, "TAG_SP_1");

        // Let's do the same for all pt_objets from 'line_2'
        disrupt(b, "disrup_line_2", nt::Type_e::Line, "line_2", publish_period, period, "TAG_LINE_2");
        disrupt(b, "disrup_network_2", nt::Type_e::Network, "network_2", publish_period, period, "TAG_NETWORK_2");
        disrupt(b, "disrup_route_2", nt::Type_e::Route, "route_2", publish_period, period, "TAG_ROUTE_2");
        disrupt(b, "disrup_sa_2", nt::Type_e::StopArea, "sp2_1", publish_period, period, "TAG_SA_2");
        disrupt(b, "disrup_sp_2", nt::Type_e::StopPoint, "sp2_1", publish_period, period, "TAG_SP_2");

        // now applying disruption on a 'Line Section'
        navitia::apply_disruption(b.disrupt(nt::RTLevel::Adapted, "disrup_line_section")
                                      .tag("TAG_LINE_SECTION")
                                      .impact()
                                      .severity(nt::disruption::Effect::NO_SERVICE)
                                      .application_periods(period)
                                      .publish(publish_period)
                                      .on_line_section("line_3", "sp3_2", "sp3_3", {"route_3"}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);

        pb_creator.init(b.data.get(), query_date, period);
    }
};
/* query date is 1 day before start of application periode and 1 day after start of publication date */
struct DisruptedNetworkWithQueryDatePast : DisruptedNetwork {
    DisruptedNetworkWithQueryDatePast() : DisruptedNetwork("20180101T060000"_dt) {}
};
/* query date is 1 day after end of application periode and 1 day before end of publication date */
struct DisruptedNetworkWithQueryDateFuture : DisruptedNetwork {
    DisruptedNetworkWithQueryDateFuture() : DisruptedNetwork("20180104T060000"_dt) {}
};

/*
 *   By default, line report should return all the different pt objects
 *   that has an applicable disruption
 */
BOOST_FIXTURE_TEST_CASE(line_report_should_return_all_disruptions, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, since, until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 11);
}

BOOST_FIXTURE_TEST_CASE(traffic_report_should_return_all_disruptions, DisruptedNetwork) {
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, boost::none, boost::none);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 11);
}

/*
 *   We now query a line_report on a line that has a disruption with
 *   a specific tag ( => 'TAG_1')
 *   In this case, we will return all the disruptions associated with
 *   that line that matches the specific tag.
 */
BOOST_FIXTURE_TEST_CASE(line_report_should_return_disruptions_from_tagged_disruption, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "disruption.tag(\"TAG_LINE_1 name\")", {}, {}, since,
                             until);

    auto& impacts = pb_creator.impacts;
    BOOST_CHECK_EQUAL(impacts.size(), 2);

    std::set<std::string> uris = navitia::test::get_impacts_uris(impacts);
    std::set<std::string> res = {"disrup_line_1", "disrup_network_1"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

BOOST_FIXTURE_TEST_CASE(line_report_should_return_disruptions_from_tagged_networkd, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "disruption.tag(\"TAG_NETWORK_1 name\")", {}, {}, since,
                             until);

    auto& impacts = pb_creator.impacts;
    BOOST_CHECK_EQUAL(impacts.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(traffic_report_should_return_disruptions_from_tagged_disruption, DisruptedNetwork) {
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "disruption.tag(\"TAG_NETWORK_1 name\")", {},
                                boost::none, boost::none);

    auto& impacts = pb_creator.impacts;
    BOOST_CHECK_EQUAL(impacts.size(), 1);

    std::set<std::string> uris = navitia::test::get_impacts_uris(impacts);
    std::set<std::string> res = {"disrup_network_1"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

BOOST_FIXTURE_TEST_CASE(traffic_report_should_return_disruptions_from_tagged_line, DisruptedNetwork) {
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "disruption.tag(\"TAG_LINE_1 name\")", {}, boost::none,
                                boost::none);

    auto& impacts = pb_creator.impacts;
    BOOST_CHECK_EQUAL(impacts.size(), 0);
}

/*
 *   When  querying a line_report a specific that does not belong to any line,
 *   we have nothing to work on, thus returning nothing;
 */
BOOST_FIXTURE_TEST_CASE(line_report_should_not_return_disruptions_from_non_tagged_line, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "disruption.tag(\"TAG_ROUTE_1 name\")", {}, {}, since,
                             until);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(traffic_report_should_not_return_disruptions_from_non_tagged_network, DisruptedNetwork) {
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "disruption.tag(\"TAG_ROUTE_1 name\")", {}, boost::none,
                                boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}

/*
 *   Query line_report with filter on impact status
 */
BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_no_filter, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, since, until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 11);
}

BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_past, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {nt::disruption::ActiveStatus::past}, since, until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_active, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {nt::disruption::ActiveStatus::active}, since,
                             until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 11);
}

BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_future, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {nt::disruption::ActiveStatus::future}, since,
                             until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_all, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {},
                             {nt::disruption::ActiveStatus::past, nt::disruption::ActiveStatus::active,
                              nt::disruption::ActiveStatus::future},
                             since, until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 11);
}

BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_multi_filter_1, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {},
                             {nt::disruption::ActiveStatus::past, nt::disruption::ActiveStatus::future}, since, until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_multi_filter_2, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {},
                             {nt::disruption::ActiveStatus::past, nt::disruption::ActiveStatus::active}, since, until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 11);
}

BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_before_impact, DisruptedNetworkWithQueryDatePast) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {nt::disruption::ActiveStatus::future},
                             pb_creator.now - boost::posix_time::seconds(3600), until);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 11);
}

BOOST_FIXTURE_TEST_CASE(line_report_filter_on_status_after_impact, DisruptedNetworkWithQueryDateFuture) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {nt::disruption::ActiveStatus::past}, since,
                             until + boost::posix_time::seconds(3600));
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 11);
}

/*
 *   We want to make sure disruption on a line section works with line_reports
 *   and trafic_report along with disruption tag
 */
BOOST_FIXTURE_TEST_CASE(line_report_on_a_tagged_line_section, DisruptedNetwork) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "disruption.tag(\"TAG_LINE_SECTION name\")", {}, {}, since,
                             until);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"disrup_line_section"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

BOOST_FIXTURE_TEST_CASE(traffic_report_on_a_tagged_line_section, DisruptedNetwork) {
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "disruption.tag(\"TAG_LINE_SECTION name\")", {},
                                boost::none, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"disrup_line_section"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

struct NetowrkTrafficReport {
    ed::builder b;
    navitia::PbCreator pb_creator;
    const boost::posix_time::ptime start_date = "20180402T060000"_dt;
    const boost::posix_time::ptime end_date = "20180428T060000"_dt;
    const time_period published_period = time_period(start_date, end_date);
    /*
     *
                                                                    published perdiod
       20180402T060000                                                                                20180428T060000
               |---------------------------------------------------------------------------------------------|
               |
               |
               |                                            application period
               |   20180405     20180409      20180412         20180416            20180419      20180423
impact1        |     |------------|
impact2        |     |                           |------------------|
impact3        |     |            |--------------|
impact4        |     |------------------------------------------------------------------|
impact5        |     |            |---------------------------------|
impact6        |     |                                                                               |-------|



since - until

test0 |---|
test1     |------|
test2            |---------|
test3                     |--|
test4                        |-----------|
test5                                    |---|
test6                                        |-----------|
test7                                                      |-------|
test8                                                                      |-------|
test9                                                                                        |---|
test10                                                                                         |-------|
test11                                                                                                             |--|

Results
test0:
test1:
test2: impact1 & impact4
test3: impact1 & impact4
test4: impact1 & impact3 & impact4 & impact5
test5: impact3 & impact4 & impact5
test6: impact2 & impact3 & impact4 & impact5
test7: impact2 & impact4 & impact5
test8: impact4
test9:
test10: impact6
test11:
    */
    NetowrkTrafficReport()
        : b("20180301", [](ed::builder& b) {
              b.vj_with_network("network_1", "line_1").route("route_1")("sp1_1", "08:10"_t)("sp1_2", "08:20"_t);
          }) {
        // Impact1
        disrupt(b, "disrup_impact_1", nt::Type_e::Network, "network_1", published_period,
                time_period("20180405T060000"_dt, "20180409T060000"_dt), "TAG_IMPACT_1");

        // Impact2
        disrupt(b, "disrup_impact_2", nt::Type_e::Network, "network_1", published_period,
                time_period("20180412T060000"_dt, "20180416T060000"_dt), "TAG_IMPACT_2");

        // Impact3
        disrupt(b, "disrup_impact_3", nt::Type_e::Network, "network_1", published_period,
                time_period("20180409T060000"_dt, "20180412T060000"_dt), "TAG_IMPACT_3");

        // Impact4
        disrupt(b, "disrup_impact_4", nt::Type_e::Network, "network_1", published_period,
                time_period("20180405T060000"_dt, "20180419T060000"_dt), "TAG_IMPACT_4");

        // Impact5
        disrupt(b, "disrup_impact_5", nt::Type_e::Network, "network_1", published_period,
                time_period("20180409T060000"_dt, "20180416T060000"_dt), "TAG_IMPACT_5");

        // Impact6
        disrupt(b, "disrup_impact_6", nt::Type_e::Network, "network_1", published_period,
                time_period("20180423T060000"_dt, "20180428T060000"_dt), "TAG_IMPACT_6");
    }
};

BOOST_FIXTURE_TEST_CASE(traffic_report_netowrk_since_until, NetowrkTrafficReport) {
    // Traffic report without since and until
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, boost::none, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
                                 "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Traffic report without since
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, boost::none, "20180429T060000"_dt);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
           "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Traffic report without until
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180401T060000"_dt, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
           "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test0
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180329T060000"_dt, "20180401T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test1
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180402T060000"_dt, "20180404T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test2
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180404T060000"_dt, "20180406T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test3
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180406T060000"_dt, "20180407T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test4
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180407T060000"_dt, "20180410T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 4);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test5
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180410T060000"_dt, "20180411T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 3);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test6
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180411T060000"_dt, "20180413T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 4);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_2", "disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test7
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180413T060000"_dt, "20180414T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 3);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_2", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test8
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180417T060000"_dt, "20180418T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);
    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test9
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180420T060000"_dt, "20180421T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test10
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180421T060000"_dt, "20180424T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test11
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180429T060000"_dt, "20180430T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}

// Traffic report: Disruption on line
struct LineTrafficReport {
    ed::builder b;
    navitia::PbCreator pb_creator;
    const boost::posix_time::ptime start_date = "20180402T060000"_dt;
    const boost::posix_time::ptime end_date = "20180428T060000"_dt;
    const time_period published_period = time_period(start_date, end_date);
    /*
     *
                                                                    published perdiod
       20180402T060000                                                                                20180428T060000
               |---------------------------------------------------------------------------------------------|
               |
               |
               |                                            application period
               |   20180405     20180409      20180412         20180416            20180419      20180423
impact1        |     |------------|
impact2        |     |                           |------------------|
impact3        |     |            |--------------|
impact4        |     |------------------------------------------------------------------|
impact5        |     |            |---------------------------------|
impact6        |     |                                                                               |-------|



since - until

test0 |---|
test1     |------|
test2            |---------|
test3                     |--|
test4                        |-----------|
test5                                    |---|
test6                                        |-----------|
test7                                                      |-------|
test8                                                                      |-------|
test9                                                                                        |---|
test10                                                                                         |-------|
test11                                                                                                             |--|

Results
test0:
test1:
test2: impact1 & impact4
test3: impact1 & impact4
test4: impact1 & impact3 & impact4 & impact5
test5: impact3 & impact4 & impact5
test6: impact2 & impact3 & impact4 & impact5
test7: impact2 & impact4 & impact5
test8: impact4
test9:
test10: impact6
test11:
    */
    LineTrafficReport()
        : b("20180301", [](ed::builder& b) {
              b.vj_with_network("network_1", "line_1").route("route_1")("sp1_1", "08:10"_t)("sp1_2", "08:20"_t);
          }) {
        // Impact1
        disrupt(b, "disrup_impact_1", nt::Type_e::Line, "line_1", published_period,
                time_period("20180405T060000"_dt, "20180409T060000"_dt), "TAG_IMPACT_1");

        // Impact2
        disrupt(b, "disrup_impact_2", nt::Type_e::Line, "line_1", published_period,
                time_period("20180412T060000"_dt, "20180416T060000"_dt), "TAG_IMPACT_2");

        // Impact3
        disrupt(b, "disrup_impact_3", nt::Type_e::Line, "line_1", published_period,
                time_period("20180409T060000"_dt, "20180412T060000"_dt), "TAG_IMPACT_3");

        // Impact4
        disrupt(b, "disrup_impact_4", nt::Type_e::Line, "line_1", published_period,
                time_period("20180405T060000"_dt, "20180419T060000"_dt), "TAG_IMPACT_4");

        // Impact5
        disrupt(b, "disrup_impact_5", nt::Type_e::Line, "line_1", published_period,
                time_period("20180409T060000"_dt, "20180416T060000"_dt), "TAG_IMPACT_5");

        // Impact6
        disrupt(b, "disrup_impact_6", nt::Type_e::Line, "line_1", published_period,
                time_period("20180423T060000"_dt, "20180428T060000"_dt), "TAG_IMPACT_6");
    }
};

BOOST_FIXTURE_TEST_CASE(traffic_report_line_since_until, LineTrafficReport) {
    // Traffic report without since and until
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, boost::none, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
                                 "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Traffic report without since
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, boost::none, "20180429T060000"_dt);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
           "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Traffic report without until
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180401T060000"_dt, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
           "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test0
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180329T060000"_dt, "20180401T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test1
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180402T060000"_dt, "20180404T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test2
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180404T060000"_dt, "20180406T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test3
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180406T060000"_dt, "20180407T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test4
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180407T060000"_dt, "20180410T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 4);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test5
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180410T060000"_dt, "20180411T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 3);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test6
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180411T060000"_dt, "20180413T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 4);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_2", "disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test7
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180413T060000"_dt, "20180414T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 3);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_2", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test8
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180417T060000"_dt, "20180418T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test9
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180420T060000"_dt, "20180421T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test10
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180421T060000"_dt, "20180424T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test11
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180429T060000"_dt, "20180430T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}

// Traffic report: Disruption on StopArea
struct StopAreaTrafficReport {
    ed::builder b;
    navitia::PbCreator pb_creator;
    const boost::posix_time::ptime start_date = "20180402T060000"_dt;
    const boost::posix_time::ptime end_date = "20180428T060000"_dt;
    const time_period published_period = time_period(start_date, end_date);
    /*
     *
                                                                    published perdiod
       20180402T060000                                                                                20180428T060000
               |---------------------------------------------------------------------------------------------|
               |
               |
               |                                            application period
               |   20180405     20180409      20180412         20180416            20180419      20180423
impact1        |     |------------|
impact2        |     |                           |------------------|
impact3        |     |            |--------------|
impact4        |     |------------------------------------------------------------------|
impact5        |     |            |---------------------------------|
impact6        |     |                                                                               |-------|



since - until

test0 |---|
test1     |------|
test2            |---------|
test3                     |--|
test4                        |-----------|
test5                                    |---|
test6                                        |-----------|
test7                                                      |-------|
test8                                                                      |-------|
test9                                                                                        |---|
test10                                                                                         |-------|
test11                                                                                                             |--|

Results
test0:
test1:
test2: impact1 & impact4
test3: impact1 & impact4
test4: impact1 & impact3 & impact4 & impact5
test5: impact3 & impact4 & impact5
test6: impact2 & impact3 & impact4 & impact5
test7: impact2 & impact4 & impact5
test8: impact4
test9:
test10: impact6
test11:
    */
    StopAreaTrafficReport()
        : b("20180301", [](ed::builder& b) {
              b.vj_with_network("network_1", "line_1").route("route_1")("sp1_1", "08:10"_t)("sp1_2", "08:20"_t);
          }) {
        // Impact1
        disrupt(b, "disrup_impact_1", nt::Type_e::StopArea, "sp1_1", published_period,
                time_period("20180405T060000"_dt, "20180409T060000"_dt), "TAG_IMPACT_1");

        // Impact2
        disrupt(b, "disrup_impact_2", nt::Type_e::StopArea, "sp1_1", published_period,
                time_period("20180412T060000"_dt, "20180416T060000"_dt), "TAG_IMPACT_2");

        // Impact3
        disrupt(b, "disrup_impact_3", nt::Type_e::StopArea, "sp1_1", published_period,
                time_period("20180409T060000"_dt, "20180412T060000"_dt), "TAG_IMPACT_3");

        // Impact4
        disrupt(b, "disrup_impact_4", nt::Type_e::StopArea, "sp1_1", published_period,
                time_period("20180405T060000"_dt, "20180419T060000"_dt), "TAG_IMPACT_4");

        // Impact5
        disrupt(b, "disrup_impact_5", nt::Type_e::StopArea, "sp1_1", published_period,
                time_period("20180409T060000"_dt, "20180416T060000"_dt), "TAG_IMPACT_5");

        // Impact6
        disrupt(b, "disrup_impact_6", nt::Type_e::StopArea, "sp1_1", published_period,
                time_period("20180423T060000"_dt, "20180428T060000"_dt), "TAG_IMPACT_6");
    }
};

BOOST_FIXTURE_TEST_CASE(traffic_report_stop_area_since_until, StopAreaTrafficReport) {
    // Traffic report without since and until
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, boost::none, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
                                 "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Traffic report without since
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, boost::none, "20180429T060000"_dt);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
           "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Traffic report without until
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180401T060000"_dt, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_2", "disrup_impact_3",
           "disrup_impact_4", "disrup_impact_5", "disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test0
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180329T060000"_dt, "20180401T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test1
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180402T060000"_dt, "20180404T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test2
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180404T060000"_dt, "20180406T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test3
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180406T060000"_dt, "20180407T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test4
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180407T060000"_dt, "20180410T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 4);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_1", "disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test5
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180410T060000"_dt, "20180411T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 3);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test6
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180411T060000"_dt, "20180413T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 4);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_2", "disrup_impact_3", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test7
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180413T060000"_dt, "20180414T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 3);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_2", "disrup_impact_4", "disrup_impact_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test8
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180417T060000"_dt, "20180418T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test9
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180420T060000"_dt, "20180421T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test10
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180421T060000"_dt, "20180424T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"disrup_impact_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test11
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, "20180429T060000"_dt, "20180430T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}

struct DisruptedNetworkWithRailSection {
    ed::builder b;
    navitia::PbCreator pb_creator;
    const boost::posix_time::ptime since = "20180102T060000"_dt;
    const boost::posix_time::ptime until = "20180103T060000"_dt;

    DisruptedNetworkWithRailSection()
        : b("20180101", [](ed::builder& b) {
              b.vj_with_network("network_1", "line_1").route("route_1")("sp1_1", "08:10"_t)("sp1_2", "08:20"_t);
              b.vj_with_network("network_2", "line_2")
                  .route("route_2")("sp2_1", "08:10"_t)("sp2_2", "08:20"_t)("sp2_3", "08:30"_t)("sp2_4", "08:40"_t)(
                      "sp2_5", "08:50"_t)("sp2_6", "08:55"_t);
              b.vj_with_network("network_3", "line_3")
                  .route("route_3")("sp3_1", "08:10"_t)("sp3_2", "08:20"_t)("sp3_3", "08:30"_t)("sp3_4", "08:40"_t);
          }) {
        auto period = time_period(since, until);

        // now applying disruption on a 'Rail Section'
        navitia::apply_disruption(b.disrupt(nt::RTLevel::Adapted, "disrup_rail_section")
                                      .tag("TAG_RAIL_SECTION")
                                      .impact()
                                      .severity(nt::disruption::Effect::NO_SERVICE)
                                      .application_periods(period)
                                      .publish(period)
                                      .on_rail_section("line_3", "sp3_2", "sp3_3", {}, {"route_3"}, *b.data->pt_data)
                                      .get_disruption(),
                                  *b.data->pt_data, *b.data->meta);
        // now applying disruption on a 'Rail Section' with REDUCED_SERVICE effect
        // route_2:
        // StopArea:        sp2_1       sp2_2       sp2_3       sp2_4       sp2_5       sp2_6
        //                    *----------*------------*-----------*-----------*-----------*
        // Blocked:                                   *************
        navitia::apply_disruption(
            b.disrupt(nt::RTLevel::Adapted, "disrup_rail_section_reduced_service")
                .tag("TAG_RAIL_SECTION_REDUCED_SERVICE")
                .impact()
                .severity(nt::disruption::Effect::REDUCED_SERVICE)
                .application_periods(period)
                .publish(period)
                .on_rail_section("line_2", "sp2_2", "sp2_5", {std::make_pair("sp2_3", 2), std::make_pair("sp2_4", 3)},
                                 {"route_2"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);
        pb_creator.init(b.data.get(), since, period);
    }
};

/**
 *   We want to make sure disruption on a rail section works with line_reports
 */
BOOST_FIXTURE_TEST_CASE(line_report_on_a_tagged_rail_section, DisruptedNetworkWithRailSection) {
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, since, until);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"disrup_rail_section", "disrup_rail_section_reduced_service"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}

BOOST_FIXTURE_TEST_CASE(traffic_report_on_a_tagged_rail_section, DisruptedNetworkWithRailSection) {
    disruption::traffic_reports(pb_creator, *b.data, 1, 25, 0, "", {}, boost::none, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"disrup_rail_section", "disrup_rail_section_reduced_service"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);
}


struct LineSectionLineReport {
    ed::builder b;
    navitia::PbCreator pb_creator;
    const boost::posix_time::ptime start_date = "20180402T060000"_dt;
    const boost::posix_time::ptime end_date = "20180428T060000"_dt;
    const time_period published_period = time_period(start_date, end_date);
    /*
     *
                                                                    published perdiod
       20180402T060000                                                                                20180428T060000
               |---------------------------------------------------------------------------------------------|
               |
               |
               |                                            application period
               |   20180405     20180409      20180412         20180416            20180419      20180423
impact1        |     |------------|
impact2        |     |                           |------------------|
impact3        |     |            |--------------|
impact4        |     |------------------------------------------------------------------|
impact5        |     |            |---------------------------------|
impact6        |     |                                                                               |-------|



since - until

test0 |---|
test1     |------|
test2            |---------|
test3                     |--|
test4                        |-----------|
test5                                    |---|
test6                                        |-----------|
test7                                                      |-------|
test8                                                                      |-------|
test9                                                                                        |---|
test10                                                                                         |-------|
test11                                                                                                             |--|

Results
test0:
test1:
test2: impact1 & impact4
test3: impact1 & impact4
test4: impact1 & impact3 & impact4 & impact5
test5: impact3 & impact4 & impact5
test6: impact2 & impact3 & impact4 & impact5
test7: impact2 & impact4 & impact5
test8: impact4
test9:
test10: impact6
test11:
    */
    LineSectionLineReport()
        : b("20180301", [](ed::builder& b) {
            b.sa("stop_area:1", 0, 0, false, true)("sp1_1");
            b.sa("stop_area:2", 0, 0, false, true)("sp1_2");
            b.sa("stop_area:3", 0, 0, false, true)("sp1_3");
            b.sa("stop_area:4", 0, 0, false, true)("sp1_4");
            b.sa("stop_area:5", 0, 0, false, true)("sp1_5");
            b.sa("stop_area:6", 0, 0, false, true)("sp1_6");
            b.vj_with_network("network_1", "line_1").route("route_1")
                    ("sp1_1", "08:10"_t)("sp1_2", "08:20"_t)
                    ("sp1_3", "08:30"_t)("sp1_4", "08:40"_t)
                    ("sp1_5", "08:44"_t)("sp1_6", "08:48"_t);
          }){

        // Impact1
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "line_section_1")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .publish(published_period)
                .application_periods(time_period("20180405T000000"_dt, "20180409T235900"_dt))
                .on_line_section("line_1", "stop_area:4", "stop_area:5",
                                 {"route_1"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // Impact2
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "line_section_2")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .publish(published_period)
                .application_periods(time_period("20180412T235900"_dt, "20180416T235900"_dt))
                .on_line_section("line_1", "stop_area:4", "stop_area:5",
                                 {"route_1"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // Impact3
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "line_section_3")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .publish(published_period)
                .application_periods(time_period("20180409T235900"_dt, "20180412T235900"_dt))
                .on_line_section("line_1", "stop_area:4", "stop_area:5",
                                 {"route_1"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // Impact4
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "line_section_4")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .publish(published_period)
                .application_periods(time_period("20180405T000000"_dt, "20180419T235900"_dt))
                .on_line_section("line_1", "stop_area:4", "stop_area:5",
                                 {"route_1"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // Impact5
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "line_section_5")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .publish(published_period)
                .application_periods(time_period("20180409T235900"_dt, "20180416T235900"_dt))
                .on_line_section("line_1", "stop_area:4", "stop_area:5",
                                 {"route_1"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);

        // Impact6
        navitia::apply_disruption(
            b.impact(nt::RTLevel::Adapted, "line_section_6")
                .severity(nt::disruption::Effect::NO_SERVICE)
                .publish(published_period)
                .msg("ms line_section_6", nt::disruption::ChannelType::sms)
                .application_periods(time_period("20180423T060000"_dt, "20180428T060000"_dt))
                .on_line_section("line_1", "stop_area:4", "stop_area:5",
                                 {"route_1"}, *b.data->pt_data)
                .get_disruption(),
            *b.data->pt_data, *b.data->meta);
    }
};

BOOST_FIXTURE_TEST_CASE(line_report_since_until, LineSectionLineReport) {
    // Line report without since and until

    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, boost::none, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    std::set<std::string> uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    std::set<std::string> res = {"line_section_1", "line_section_2", "line_section_3",
                                 "line_section_4", "line_section_5", "line_section_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Line report without since
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, boost::none, "20180429T060000"_dt);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Traffic report without until
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180401T060000"_dt, boost::none);

    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 6);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test0
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180329T060000"_dt, "20180401T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test1
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180402T060000"_dt, "20180404T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test2
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180404T060000"_dt, "20180406T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"line_section_1", "line_section_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test3
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180406T060000"_dt, "20180407T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 2);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"line_section_1", "line_section_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test4
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180407T060000"_dt, "20180410T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 4);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"line_section_1", "line_section_3", "line_section_4", "line_section_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test5
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180410T060000"_dt, "20180411T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 3);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"line_section_3", "line_section_4", "line_section_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test6
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180411T060000"_dt, "20180413T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 4);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"line_section_2", "line_section_3", "line_section_4", "line_section_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test7
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180413T060000"_dt, "20180414T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 3);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"line_section_2", "line_section_4", "line_section_5"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test8
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180417T060000"_dt, "20180418T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"line_section_4"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test9
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180420T060000"_dt, "20180421T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);

    // Test10
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180421T060000"_dt, "20180424T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 1);

    uris = navitia::test::get_impacts_uris(pb_creator.impacts);
    res = {"line_section_6"};
    BOOST_CHECK_EQUAL_RANGE(res, uris);

    // Test11
    pb_creator.init(b.data.get(), start_date, published_period);
    disruption::line_reports(pb_creator, *b.data, 1, 25, 0, "", {}, {}, "20180429T060000"_dt, "20180430T060000"_dt);
    BOOST_CHECK_EQUAL(pb_creator.impacts.size(), 0);
}
