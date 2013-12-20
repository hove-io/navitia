#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_disruption
#include <boost/test/unit_test.hpp>
#include "disruption/disruption_api.h"
#include "routing/raptor.h"
#include "ed/build_helper.h"
#include <boost/make_shared.hpp>

namespace pt = boost::posix_time;
class Params{
public:
    std::vector<std::string> forbidden;
    ed::builder b;

    Params():b("20120614"){
        std::vector<std::string> forbidden;
        b.vj("network:R","line:A","11111111","",true, "")("stop_area:stop1", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop2", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:R","line:S","11111111","",true, "")("stop_area:stop5", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop6", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.vj("network:K","line:B","11111111","",true, "")("stop_area:stop3", 8*3600 +10*60, 8*3600 + 11 * 60)("stop_area:stop4", 8*3600 + 20 * 60 ,8*3600 + 21*60);
        b.generate_dummy_basis();
        b.data.pt_data.index();
        for(navitia::type::Line *line : b.data.pt_data.lines){
            line->network->line_list.push_back(line);
        }
        navitia::type::Line* line =  b.data.pt_data.lines[0];
        boost::shared_ptr<navitia::type::Message> message;
        message = boost::make_shared<navitia::type::Message>();
        message->uri = "mess1";
        message->object_uri="line:A";
        message->object_type = navitia::type::Type_e::Line;
        message->application_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),
                                                      pt::time_from_string("2013-12-21 12:32:00"));
        message->publication_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),
                                                      pt::time_from_string("2013-12-21 12:32:00"));
        message->active_days = std::bitset<8>("11111111");
        message->application_daily_start_hour = pt::duration_from_string("00:00");
        message->application_daily_end_hour = pt::duration_from_string("23:59");
        line->messages.push_back(message);

        line =  b.data.pt_data.lines[1];
        message = boost::make_shared<navitia::type::Message>();
        message->uri = "mess0";
        message->object_uri="line:S";
        message->object_type = navitia::type::Type_e::Line;
        message->application_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),
                                                      pt::time_from_string("2013-12-21 12:32:00"));
        message->publication_period = pt::time_period(pt::time_from_string("2013-12-19 12:32:00"),
                                                      pt::time_from_string("2013-12-21 12:32:00"));
        message->active_days = std::bitset<8>("11111111");;
        message->application_daily_start_hour = pt::duration_from_string("00:00");
        message->application_daily_end_hour = pt::duration_from_string("23:59");
        line->messages.push_back(message);

        line =  b.data.pt_data.lines[2];
        message = boost::make_shared<navitia::type::Message>();
        message->uri = "mess2";
        message->object_uri="line:B";
        message->object_type = navitia::type::Type_e::Line;
        message->application_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),
                                                      pt::time_from_string("2013-12-25 12:32:00"));
        message->publication_period = pt::time_period(pt::time_from_string("2013-12-23 12:32:00"),
                                                     pt::time_from_string("2013-12-25 12:32:00"));
        message->active_days = std::bitset<8>("11111111");;
        message->application_daily_start_hour = pt::duration_from_string("00:00");
        message->application_daily_end_hour = pt::duration_from_string("23:59");
        line->messages.push_back(message);

        b.data.build_raptor();
        b.data.build_uri();
        b.data.meta.production_date = boost::gregorian::date_period(boost::gregorian::date(2012,06,14), boost::gregorian::days(7));
        navitia::routing::RAPTOR raptor(b.data);
    }
};

BOOST_FIXTURE_TEST_CASE(error, Params) {

    pbnavitia::Response resp = navitia::disruption_api::disruptions(b.data,"AAA",1,10,0,"network.uri=network:R");
    BOOST_REQUIRE_EQUAL(resp.error().id(), pbnavitia::Error::unable_to_parse);
}

BOOST_FIXTURE_TEST_CASE(network_filter, Params) {
    pbnavitia::Response resp = navitia::disruption_api::disruptions(b.data,"20131220T125000",1,10,0,"network.uri=network:R");

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

BOOST_FIXTURE_TEST_CASE(line_filter, Params) {
    pbnavitia::Response resp = navitia::disruption_api::disruptions(b.data,"20131220T125000",1,10,0,"line.uri=line:S");

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
