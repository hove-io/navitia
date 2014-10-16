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
#include "disruption/disruption_api.h"
#include "routing/raptor.h"
#include "ed/build_helper.h"
#include <boost/make_shared.hpp>
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
namespace pt = boost::posix_time;
using navitia::type::new_disruption::Impact;
using navitia::type::new_disruption::PtObject;
using navitia::type::new_disruption::Disruption;

struct disruption_creator {
    std::string uri;
    std::string object_uri;
    navitia::type::Type_e object_type;
    pt::time_period application_period {pt::time_from_string("2013-12-19 12:32:00"),pt::time_from_string("2013-12-21 12:32:00")};
    pt::time_period publication_period {pt::time_from_string("2013-12-19 12:32:00"),pt::time_from_string("2013-12-21 12:32:00")};
    std::bitset<7> active_days = std::bitset<7>("1111111");
    pt::time_duration application_daily_start_hour = pt::duration_from_string("00:00");
    pt::time_duration application_daily_end_hour = pt::duration_from_string("23:59");
};

class Params {
public:
    std::vector<std::string> forbidden;
    ed::builder b;
    size_t period;

    template <typename T>
    void add_disruption(disruption_creator disrupt, T* obj) {
        auto disruption = std::make_unique<Disruption>();
        auto impact = std::make_shared<Impact>();
        //TODO
        obj->add_impact(impact);
    }

    Params(): b("20120614"), period(365) {
        std::vector<std::string> forbidden;
        b.vj("network:R", "line:A", "11111111", "", true, "")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:R", "line:S", "11111111", "", true, "")("stop_area:stop5", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop6", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:K","line:B","11111111","",true, "")("stop_area:stop3", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop4", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:M","line:M","11111111","",true, "")("stop_area:stop22", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop22", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:Test","line:test","11111111","",true, "")("stop_area:stop22", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop22", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.generate_dummy_basis();
        b.data->pt_data->index();
        for(navitia::type::Line *line : b.data->pt_data->lines){
            line->network->line_list.push_back(line);
        }
        navitia::type::Line* line =  b.data->pt_data->lines[0];
        disruption_creator disruption_wrapper;
        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess1";
        disruption_wrapper.object_uri="line:A";
        disruption_wrapper.object_type = navitia::type::Type_e::Line;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),pt::time_from_string("2013-12-21 12:32:00"));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),pt::time_from_string("2013-12-21 12:32:00"));
        add_disruption(disruption_wrapper, line);

        line = b.data->pt_data->lines[1];
        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess0";
        disruption_wrapper.object_uri="line:S";
        disruption_wrapper.object_type = navitia::type::Type_e::Line;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),pt::time_from_string("2013-12-21 12:32:00"));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),pt::time_from_string("2013-12-21 12:32:00"));
        add_disruption(disruption_wrapper, line);

        line = b.data->pt_data->lines[2];
        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess2";
        disruption_wrapper.object_uri="line:B";
        disruption_wrapper.object_type = navitia::type::Type_e::Line;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),pt::time_from_string("2013-12-25 12:32:00"));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),pt::time_from_string("2013-12-25 12:32:00"));
        add_disruption(disruption_wrapper, line);

        line = b.data->pt_data->lines[3];
        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess3";
        disruption_wrapper.object_uri="network:M";
        disruption_wrapper.object_type = navitia::type::Type_e::Network;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),pt::time_from_string("2013-12-25 12:32:00"));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),pt::time_from_string("2013-12-25 12:32:00"));
        add_disruption(disruption_wrapper, line->network);

        /// Cette ligne est pour effectuer les tests : Test1, ..., test5
        line = b.data->pt_data->lines[4];

        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess4";
        disruption_wrapper.object_uri="network:Test";
        disruption_wrapper.object_type = navitia::type::Type_e::Network;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2014-01-12 08:32:00"),pt::time_from_string("2014-02-02 18:32:00"));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2014-01-02 08:32:00"),pt::time_from_string("2014-02-10 12:32:00"));
        disruption_wrapper.application_daily_start_hour = pt::duration_from_string("08:40");
        disruption_wrapper.application_daily_end_hour = pt::duration_from_string("18:00");

        add_disruption(disruption_wrapper, line->network);
    }
};

BOOST_FIXTURE_TEST_CASE(error, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data), "AAA", period,
            1, 10, 0, "network.uri=network:R", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.error().id(), pbnavitia::Error::unable_to_parse);
}

BOOST_FIXTURE_TEST_CASE(network_filter1, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20131220T125000",period, 1, 10, 0, "network.uri=network:R", forbidden_uris);

    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruption disruption = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.lines_size(), 2);
    BOOST_REQUIRE_EQUAL(disruption.network().uri(), "network:R");

    pbnavitia::Line line = disruption.lines(0);
    BOOST_REQUIRE_EQUAL(line.uri(), "line:A");

    BOOST_REQUIRE_EQUAL(line.messages_size(), 1);
    pbnavitia::Message message = line.messages(0);
    BOOST_REQUIRE_EQUAL(message.uri(), "mess1");
    BOOST_REQUIRE_EQUAL(message.start_application_date(), "20131219T123200");
    BOOST_REQUIRE_EQUAL(message.end_application_date(), "20131221T123200");
    BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "000000");
    BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "235900");

    line = disruption.lines(1);
    BOOST_REQUIRE_EQUAL(line.uri(), "line:S");

    BOOST_REQUIRE_EQUAL(line.messages_size(), 1);
    message = line.messages(0);
    BOOST_REQUIRE_EQUAL(message.uri(), "mess0");
    BOOST_REQUIRE_EQUAL(message.start_application_date(), "20131219T123200");
    BOOST_REQUIRE_EQUAL(message.end_application_date(), "20131221T123200");
    BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "000000");
    BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "235900");
}

BOOST_FIXTURE_TEST_CASE(network_filter2, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20131224T125000", period, 1, 10, 0, "network.uri=network:M", forbidden_uris);

    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruption disruption = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruption.network().uri(), "network:M");
    pbnavitia::Network network = disruption.network();
    BOOST_REQUIRE_EQUAL(network.messages_size(), 1);
    pbnavitia::Message message = network.messages(0);
    BOOST_REQUIRE_EQUAL(message.uri(), "mess3");
    BOOST_REQUIRE_EQUAL(message.start_application_date(), "20131223T123200");
    BOOST_REQUIRE_EQUAL(message.end_application_date(), "20131225T123200");
    BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "000000");
    BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "235900");
}

BOOST_FIXTURE_TEST_CASE(line_filter, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20131220T125000", period, 1 ,10 ,0 , "line.uri=line:S", forbidden_uris);

    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruption disruption = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.lines_size(), 1);
    BOOST_REQUIRE_EQUAL(disruption.network().uri(), "network:R");

    pbnavitia::Line line = disruption.lines(0);
    BOOST_REQUIRE_EQUAL(line.uri(), "line:S");

    BOOST_REQUIRE_EQUAL(line.messages_size(), 1);
    pbnavitia::Message message = line.messages(0);
    BOOST_REQUIRE_EQUAL(message.uri(), "mess0");
    BOOST_REQUIRE_EQUAL(message.start_application_date(), "20131219T123200");
    BOOST_REQUIRE_EQUAL(message.end_application_date(), "20131221T123200");
    BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "000000");
    BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "235900");
}

BOOST_FIXTURE_TEST_CASE(Test1, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20140101T0900", period, 1, 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}

BOOST_FIXTURE_TEST_CASE(Test2, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20140103T0900", period, 1, 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruption disruption = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruption.network().uri(), "network:Test");

    BOOST_REQUIRE_EQUAL(disruption.network().messages_size(), 1);

    pbnavitia::Message message = disruption.network().messages(0);
    BOOST_REQUIRE_EQUAL(message.uri(), "mess4");
    BOOST_REQUIRE_EQUAL(message.start_application_date(), "20140112T083200");
    BOOST_REQUIRE_EQUAL(message.end_application_date(), "20140202T183200");
    BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "084000");
    BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "180000");
}

BOOST_FIXTURE_TEST_CASE(Test3, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20140113T0900", period, 1, 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruption disruption = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruption.network().uri(), "network:Test");

    BOOST_REQUIRE_EQUAL(disruption.network().messages_size(), 1);

    pbnavitia::Message message = disruption.network().messages(0);
    BOOST_REQUIRE_EQUAL(message.uri(), "mess4");
    BOOST_REQUIRE_EQUAL(message.start_application_date(), "20140112T083200");
    BOOST_REQUIRE_EQUAL(message.end_application_date(), "20140202T183200");
    BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "084000");
    BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "180000");
}

BOOST_FIXTURE_TEST_CASE(Test4, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20140203T0900", period, 1 , 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}

BOOST_FIXTURE_TEST_CASE(Test5, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20140212T0900", period, 1, 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ResponseType::NO_SOLUTION);
}

BOOST_FIXTURE_TEST_CASE(Test7, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20140113T1801", period, 1, 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruption disruption = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruption.network().uri(), "network:Test");

    BOOST_REQUIRE_EQUAL(disruption.network().messages_size(), 1);

    pbnavitia::Message message = disruption.network().messages(0);
    BOOST_REQUIRE_EQUAL(message.uri(), "mess4");
    BOOST_REQUIRE_EQUAL(message.start_application_date(), "20140112T083200");
    BOOST_REQUIRE_EQUAL(message.end_application_date(), "20140202T183200");
    BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "084000");
    BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "180000");
}
