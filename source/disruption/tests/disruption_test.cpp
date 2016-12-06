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
#define BOOST_TEST_MODULE test_disruption
#include <boost/test/unit_test.hpp>
#include "disruption/traffic_reports_api.h"
#include "routing/raptor.h"
#include "ed/build_helper.h"
#include <boost/make_shared.hpp>
#include "tests/utils_test.h"
#include "type/chaos.pb.h"
#include "type/datetime.h"
#include "type/pb_converter.h"

/*

publicationDate    |------------------------------------------------------------------------------------------------|
ApplicationDate                            |--------------------------------------------|

Test1       * 20140101T0900
Test2                   * 20140103T0900
Test3                                           *20140113T0900
Test4                                                                                               *20140203T0900
Test5                                                                                                                   *20140212T0900


start_publication_date  = 2014-01-02 08:32:00
end_publication_date    = 2014-02-10 12:32:00

start_application_date  = 2014-01-12 08:32:00
end_application_date    = 2014-02-02 18:32:00

start_application_daily_hour = 08h40
end_application_daily_hour = 18h00
active_days = 11111111
*/
struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );

namespace pt = boost::posix_time;
using navitia::type::disruption::Impact;
using navitia::type::disruption::PtObj;
using navitia::type::disruption::Disruption;
using navitia::type::disruption::Severity;

class Params {
public:
    std::vector<std::string> forbidden;
    ed::builder b;
    u_int64_t end_date;

    Params(): b("20120614"), end_date("20130614T000000"_pts) {
        b.vj_with_network("network:R", "line:A", "11111111", "", true, "")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj_with_network("network:R", "line:S", "11111111", "", true, "")("stop_area:stop5", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop6", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj_with_network("network:K","line:B","11111111","",true, "")("stop_area:stop3", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop4", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj_with_network("network:M","line:M","11111111","",true, "")("stop_area:stop22", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop22", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj_with_network("network:Test","line:test","11111111","",true, "")("stop_area:stop22", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop22", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.generate_dummy_basis();
        b.finish();
        b.data->pt_data->index();
        b.data->build_uri();
        b.data->build_raptor();
        for(navitia::type::Line *line : b.data->pt_data->lines){
            line->network->line_list.push_back(line);
        }

        using btp = boost::posix_time::time_period;
        b.impact(nt::RTLevel::Adapted)
                .uri("mess1")
                .publish(btp("20131219T123200"_dt, "20131221T123201"_dt))
                .application_periods(btp("20131220T123200"_dt, "20131221T123201"_dt))
                .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
                .on(nt::Type_e::Line, "line:A");

        b.impact(nt::RTLevel::Adapted)
                .uri("mess0")
                .publish(btp("20131221T083200"_dt, "20131221T123201"_dt))
                .application_periods(btp("20131221T083200"_dt, "20131221T123201"_dt))
                .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
                .on(nt::Type_e::Line, "line:S");

        b.impact(nt::RTLevel::Adapted)
                .uri("mess2")
                .application_periods(btp("20131223T123200"_dt, "20131225T123201"_dt))
                .publish(btp("20131224T123200"_dt, "20131226T123201"_dt))
                .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
                .on(nt::Type_e::Network, "network:K");
    }
};

BOOST_FIXTURE_TEST_CASE(network_filter1, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20131219T155000"_dt;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    pbnavitia::Response resp = navitia::disruption::traffic_reports(pb_creator, *(b.data),
            dt, 1, 10, 0, "network.uri=network:R", forbidden_uris);

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
    //we are in the publication period of the disruption but the application_period of the impact is in the future
    BOOST_CHECK_EQUAL(impact->status(), pbnavitia::ActiveStatus::future);

    BOOST_REQUIRE_EQUAL(impact->application_periods_size(), 1);
    BOOST_CHECK_EQUAL(impact->application_periods(0).begin(), navitia::test::to_posix_timestamp("20131220T123200"));
    BOOST_CHECK_EQUAL(impact->application_periods(0).end(), navitia::test::to_posix_timestamp("20131221T123200"));
}

BOOST_FIXTURE_TEST_CASE(network_filter2, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20131224T125000"_dt;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    pbnavitia::Response resp = navitia::disruption::traffic_reports(pb_creator, *(b.data),
            dt, 1, 10, 0, "network.uri=network:K", forbidden_uris);

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
    BOOST_CHECK_EQUAL(impact->application_periods(0).end(), navitia::test::to_posix_timestamp("20131225T123200"));
}

BOOST_FIXTURE_TEST_CASE(line_filter, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20131221T085000"_dt;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    pbnavitia::Response resp = navitia::disruption::traffic_reports(pb_creator, *(b.data),
                                                                    dt, 1 ,10 ,0 , "line.uri=line:S", forbidden_uris);

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
    BOOST_CHECK_EQUAL(impact->application_periods(0).end(), navitia::test::to_posix_timestamp("20131221T123200"));
}

BOOST_FIXTURE_TEST_CASE(Test1, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20140101T0900"_dt;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    pbnavitia::Response resp = navitia::disruption::traffic_reports(pb_creator, *(b.data), dt, 1, 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}

BOOST_FIXTURE_TEST_CASE(Test2, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20131226T0900"_dt;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    pbnavitia::Response resp = navitia::disruption::traffic_reports(pb_creator, *(b.data), dt, 1, 10, 0, "", forbidden_uris);
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
    std::cout << "bob 4?" << std::endl;
    std::vector<std::string> forbidden_uris;
    auto dt = "20130203T0900"_dt;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    pbnavitia::Response resp = navitia::disruption::traffic_reports(pb_creator, *(b.data), dt, 1 , 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}

BOOST_FIXTURE_TEST_CASE(Test5, Params) {
    std::vector<std::string> forbidden_uris;
    auto dt = "20130212T0900"_dt;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, dt, null_time_period);
    pbnavitia::Response resp = navitia::disruption::traffic_reports(pb_creator, *(b.data), dt, 1, 10, 0, "", forbidden_uris);

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}
