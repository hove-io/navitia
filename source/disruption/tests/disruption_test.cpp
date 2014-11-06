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
#include "tests/utils_test.h"

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
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace pt = boost::posix_time;
using navitia::type::new_disruption::Impact;
using navitia::type::new_disruption::PtObj;
using navitia::type::new_disruption::Disruption;

enum class ChaosType {
    //we copy the chaos type because we don't want chaos dependencies here
    Network,
    StopArea,
    Line,
    Route,
    LineSection
};

struct disruption_creator {
    std::string uri;
    std::string object_uri;
    ChaosType object_type;
    pt::time_period application_period {pt::time_from_string("2013-12-19 12:32:00"),
                pt::time_from_string("2013-12-21 12:32:00")};
    pt::time_period publication_period {pt::time_from_string("2013-12-19 12:32:00"),
                pt::time_from_string("2013-12-21 12:32:00")};
    std::bitset<7> active_days = std::bitset<7>("1111111");
    pt::time_duration application_daily_start_hour = pt::duration_from_string("00:00");
    pt::time_duration application_daily_end_hour = pt::duration_from_string("24:00");

    std::vector<pt::time_period> get_application_periods() const {
        return navitia::expand_calendar(application_period.begin(), application_period.last(),
                                     application_daily_start_hour, application_daily_end_hour, active_days);
    }
};

class Params {
public:
    std::vector<std::string> forbidden;
    ed::builder b;
    size_t period;

    void add_disruption(disruption_creator disrupt, nt::PT_Data& pt_data) {
        nt::new_disruption::DisruptionHolder& holder = pt_data.disruption_holder;

        auto disruption = std::make_unique<Disruption>();
        disruption->uri = disrupt.uri;
        disruption->publication_period = disrupt.publication_period;

        auto impact = boost::make_shared<Impact>();
        impact->uri = disrupt.uri;
        impact->application_periods = disrupt.get_application_periods();

        switch (disrupt.object_type) {
        case ChaosType::Network:
            impact->informed_entities.push_back(make_pt_obj(nt::Type_e::Network, disrupt.object_uri, pt_data, impact));
            break;
        case ChaosType::StopArea:
            impact->informed_entities.push_back(make_pt_obj(nt::Type_e::StopArea, disrupt.object_uri, pt_data, impact));
            break;
        case ChaosType::Line:
            impact->informed_entities.push_back(make_pt_obj(nt::Type_e::Line, disrupt.object_uri, pt_data, impact));
            break;
        case ChaosType::Route:
            impact->informed_entities.push_back(make_pt_obj(nt::Type_e::Route, disrupt.object_uri, pt_data, impact));
            break;
        case ChaosType::LineSection:
            throw navitia::exception("LineSection not handled yet");
            break;
        }

        disruption->add_impact(impact);

        holder.disruptions.push_back(std::move(disruption));
    }

    Params(): b("20120614"), period(365) {
        std::vector<std::string> forbidden;
        b.vj("network:R", "line:A", "11111111", "", true, "")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:R", "line:S", "11111111", "", true, "")("stop_area:stop5", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop6", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:K","line:B","11111111","",true, "")("stop_area:stop3", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop4", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:M","line:M","11111111","",true, "")("stop_area:stop22", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop22", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:Test","line:test","11111111","",true, "")("stop_area:stop22", 8*3600 +10*60, 8*3600 + 11 * 60)
                ("stop_area:stop22", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.generate_dummy_basis();
        b.data->pt_data->index();
        b.data->build_uri();
        for(navitia::type::Line *line : b.data->pt_data->lines){
            line->network->line_list.push_back(line);
        }
        disruption_creator disruption_wrapper;
        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess1";
        disruption_wrapper.object_uri = "line:A";
        disruption_wrapper.object_type = ChaosType::Line;
        //note we add one seconds, because the end is not is the period
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),
                                                                pt::time_from_string("2013-12-21 12:32:00") + pt::seconds(1));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),
                                                                pt::time_from_string("2013-12-21 12:32:00") + pt::seconds(1));
        add_disruption(disruption_wrapper, *b.data->pt_data);

        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess0";
        disruption_wrapper.object_uri = "line:S";
        disruption_wrapper.object_type = ChaosType::Line;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),
                                                                pt::time_from_string("2013-12-21 12:32:00") + pt::seconds(1));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),
                                                                pt::time_from_string("2013-12-21 12:32:00") + pt::seconds(1));
        add_disruption(disruption_wrapper, *b.data->pt_data);

        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess2";
        disruption_wrapper.object_uri = "line:B";
        disruption_wrapper.object_type = ChaosType::Line;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),
                                                                pt::time_from_string("2013-12-25 12:32:00") + pt::seconds(1));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),
                                                                pt::time_from_string("2013-12-25 12:32:00") + pt::seconds(1));
        add_disruption(disruption_wrapper, *b.data->pt_data);

        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess3";
        disruption_wrapper.object_uri = "network:M";
        disruption_wrapper.object_type = ChaosType::Network;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),
                                                                pt::time_from_string("2013-12-25 12:32:00") + pt::seconds(1));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),
                                                                pt::time_from_string("2013-12-25 12:32:00") + pt::seconds(1));
        add_disruption(disruption_wrapper, *b.data->pt_data);

        disruption_wrapper = disruption_creator();
        disruption_wrapper.uri = "mess4";
        disruption_wrapper.object_uri = "network:Test";
        disruption_wrapper.object_type = ChaosType::Network;
        disruption_wrapper.application_period = pt::time_period(pt::time_from_string("2014-01-12 08:32:00"),
                                                                pt::time_from_string("2014-02-02 18:32:00") + pt::seconds(1));
        disruption_wrapper.publication_period = pt::time_period(pt::time_from_string("2014-01-02 08:32:00"),
                                                                pt::time_from_string("2014-02-10 12:32:00") + pt::seconds(1));
        disruption_wrapper.application_daily_start_hour = pt::duration_from_string("08:40");
        disruption_wrapper.application_daily_end_hour = pt::duration_from_string("18:00");

        add_disruption(disruption_wrapper, *b.data->pt_data);
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

    pbnavitia::Disruptions disruptions = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 2);
    BOOST_CHECK_EQUAL(disruptions.network().uri(), "network:R");

    pbnavitia::Line line = disruptions.lines(0);
    BOOST_REQUIRE_EQUAL(line.uri(), "line:A");

    BOOST_REQUIRE_EQUAL(line.disruptions_size(), 1);
    auto disruption = line.disruptions(0);
    BOOST_CHECK_EQUAL(disruption.uri(), "mess1");

    BOOST_REQUIRE_EQUAL(disruption.application_periods_size(), 1);
    BOOST_CHECK_EQUAL(disruption.application_periods(0).begin(), navitia::test::to_posix_timestamp("20131219T123200"));
    BOOST_CHECK_EQUAL(disruption.application_periods(0).end(), navitia::test::to_posix_timestamp("20131221T123200"));

    line = disruptions.lines(1);
    BOOST_REQUIRE_EQUAL(line.uri(), "line:S");

    BOOST_REQUIRE_EQUAL(line.disruptions_size(), 1);
    disruption = line.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.uri(), "mess0");
    BOOST_REQUIRE_EQUAL(disruption.application_periods_size(), 1);
    BOOST_CHECK_EQUAL(disruption.application_periods(0).begin(), navitia::test::to_posix_timestamp("20131219T123200"));
    BOOST_CHECK_EQUAL(disruption.application_periods(0).end(), navitia::test::to_posix_timestamp("20131221T123200"));


    //old output, to be cleaned as soon as NMP uses disruptions
    {
        line = disruptions.lines(0);
        BOOST_REQUIRE_EQUAL(line.messages_size(), 1);
        pbnavitia::Message message = line.messages(0);
        BOOST_REQUIRE_EQUAL(message.uri(), "mess1");
        BOOST_REQUIRE_EQUAL(message.start_application_date(), "20131219T123200");
        BOOST_REQUIRE_EQUAL(message.end_application_date(), "20131221T123200");
        BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "000000");
        BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "235959");
        line = disruptions.lines(1);
        BOOST_REQUIRE_EQUAL(line.messages_size(), 1);
        message = line.messages(0);
        BOOST_REQUIRE_EQUAL(message.uri(), "mess0");
        BOOST_REQUIRE_EQUAL(message.start_application_date(), "20131219T123200");
        BOOST_REQUIRE_EQUAL(message.end_application_date(), "20131221T123200");
        BOOST_REQUIRE_EQUAL(message.start_application_daily_hour(), "000000");
        BOOST_REQUIRE_EQUAL(message.end_application_daily_hour(), "235959");
    }
}

BOOST_FIXTURE_TEST_CASE(network_filter2, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20131224T125000", period, 1, 10, 0, "network.uri=network:M", forbidden_uris);

    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruptions disruptions = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruptions.network().uri(), "network:M");
    pbnavitia::Network network = disruptions.network();
    BOOST_REQUIRE_EQUAL(network.disruptions_size(), 1);
    auto disruption = network.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.uri(), "mess3");
    BOOST_REQUIRE_EQUAL(disruption.application_periods_size(), 1);
    BOOST_CHECK_EQUAL(disruption.application_periods(0).begin(), navitia::test::to_posix_timestamp("20131223T123200"));
    BOOST_CHECK_EQUAL(disruption.application_periods(0).end(), navitia::test::to_posix_timestamp("20131225T123200"));
}

BOOST_FIXTURE_TEST_CASE(line_filter, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20131220T125000", period, 1 ,10 ,0 , "line.uri=line:S", forbidden_uris);

    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruptions disruptions = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 1);
    BOOST_REQUIRE_EQUAL(disruptions.network().uri(), "network:R");

    pbnavitia::Line line = disruptions.lines(0);
    BOOST_REQUIRE_EQUAL(line.uri(), "line:S");

    BOOST_REQUIRE_EQUAL(line.disruptions_size(), 1);
    auto disruption = line.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.uri(), "mess0");

    BOOST_REQUIRE_EQUAL(disruption.application_periods_size(), 1);
    BOOST_CHECK_EQUAL(disruption.application_periods(0).begin(), navitia::test::to_posix_timestamp("20131219T123200"));
    BOOST_CHECK_EQUAL(disruption.application_periods(0).end(), navitia::test::to_posix_timestamp("20131221T123200"));
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

    pbnavitia::Disruptions disruptions = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruptions.network().uri(), "network:Test");

    BOOST_REQUIRE_EQUAL(disruptions.network().disruptions_size(), 1);

    auto disruption = disruptions.network().disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.uri(), "mess4");

    auto nb_p = disruption.application_periods_size();
    BOOST_REQUIRE(nb_p > 1);
    //many date since it has been split by days
    //check first and last
    BOOST_CHECK_EQUAL(disruption.application_periods(0).begin(), navitia::test::to_posix_timestamp("20140112T084000"));
    BOOST_CHECK_EQUAL(disruption.application_periods(0).end(), navitia::test::to_posix_timestamp("20140112T180000"));

    BOOST_CHECK_EQUAL(disruption.application_periods(nb_p - 1).begin(), navitia::test::to_posix_timestamp("20140202T084000"));
    BOOST_CHECK_EQUAL(disruption.application_periods(nb_p - 1).end(), navitia::test::to_posix_timestamp("20140202T180000"));
}

BOOST_FIXTURE_TEST_CASE(Test3, Params) {
    std::vector<std::string> forbidden_uris;
    pbnavitia::Response resp = navitia::disruption::disruptions(*(b.data),
            "20140113T0900", period, 1, 10, 0, "", forbidden_uris);
    BOOST_REQUIRE_EQUAL(resp.disruptions_size(), 1);

    pbnavitia::Disruptions disruptions = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruptions.network().uri(), "network:Test");

    BOOST_REQUIRE_EQUAL(disruptions.network().disruptions_size(), 1);

    auto disruption = disruptions.network().disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.uri(), "mess4");
    auto nb_p = disruption.application_periods_size();
    BOOST_REQUIRE(nb_p > 1);
    //many date since it has been split by days
    //check first and last
    BOOST_CHECK_EQUAL(disruption.application_periods(0).begin(), navitia::test::to_posix_timestamp("20140112T084000"));
    BOOST_CHECK_EQUAL(disruption.application_periods(0).end(), navitia::test::to_posix_timestamp("20140112T180000"));

    BOOST_CHECK_EQUAL(disruption.application_periods(nb_p - 1).begin(), navitia::test::to_posix_timestamp("20140202T084000"));
    BOOST_CHECK_EQUAL(disruption.application_periods(nb_p - 1).end(), navitia::test::to_posix_timestamp("20140202T180000"));
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

    pbnavitia::Disruptions disruptions = resp.disruptions(0);
    BOOST_REQUIRE_EQUAL(disruptions.lines_size(), 0);
    BOOST_REQUIRE_EQUAL(disruptions.network().uri(), "network:Test");

    BOOST_REQUIRE_EQUAL(disruptions.network().disruptions_size(), 1);

    auto disruption = disruptions.network().disruptions(0);
    BOOST_REQUIRE_EQUAL(disruption.uri(), "mess4");
    auto nb_p = disruption.application_periods_size();
    BOOST_REQUIRE(nb_p > 1);
    //many date since it has been split by days
    //check first and last
    BOOST_CHECK_EQUAL(disruption.application_periods(0).begin(), navitia::test::to_posix_timestamp("20140112T084000"));
    BOOST_CHECK_EQUAL(disruption.application_periods(0).end(), navitia::test::to_posix_timestamp("20140112T180000"));

    BOOST_CHECK_EQUAL(disruption.application_periods(nb_p - 1).begin(), navitia::test::to_posix_timestamp("20140202T084000"));
    BOOST_CHECK_EQUAL(disruption.application_periods(nb_p - 1).end(), navitia::test::to_posix_timestamp("20140202T180000"));
}
