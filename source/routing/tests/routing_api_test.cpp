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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include "ed/data.h"
#include <boost/test/unit_test.hpp>
#include "routing/raptor_api.h"
#include "ed/build_helper.h"
#include "routing_api_test_data.h"
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "georef/street_network.h"
#include "type/data.h"
#include "type/rt_level.h"
#include <boost/range/algorithm/count.hpp>
#include "type/pb_converter.h"
#include "utils/rank.h"

struct logger_initialized {
    logger_initialized() {
        navitia::init_logger();
        auto raptor_logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("raptor"));
        raptor_logger.setLogLevel(log4cplus::FATAL_LOG_LEVEL);
        auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        logger.setLogLevel(log4cplus::FATAL_LOG_LEVEL);
        auto fare_logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("fare"));
        fare_logger.setLogLevel(log4cplus::FATAL_LOG_LEVEL);
    }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace nr = navitia::routing;
namespace ntest = navitia::test;
namespace bt = boost::posix_time;
namespace ng = navitia::georef;
namespace nt = navitia::type;
using navitia::type::disruption::ChannelType;

static void dump_response(pbnavitia::Response resp, std::string test_name, bool debug_info = false) {
    if (!debug_info)
        return;
    if (resp.journeys_size() == 0) {
        std::cout << "no journeys" << std::endl;
        return;
    }
    std::cout << test_name << ": " << std::endl;
    for (int idx_journey = 0; idx_journey < resp.journeys().size(); ++idx_journey) {
        std::cout << "  journey " << idx_journey << std::endl;
        const pbnavitia::Journey& journey = resp.journeys(idx_journey);
        for (int idx_section = 0; idx_section < journey.sections().size(); ++idx_section) {
            auto& section = journey.sections(idx_section);
            std::cout << "  * section type=" << int(section.type()) << " " << section.origin().name() << "@"
                      << section.begin_date_time() << " -> " << section.destination().name() << "@"
                      << section.end_date_time() << std::endl;
            std::cout << "    -- length: " << section.length() << std::endl;
            if (section.street_network().coordinates_size()) {
                std::cout << "    -- coordinates: " << std::endl;
                for (int i = 0; i < section.street_network().coordinates_size(); ++i)
                    std::cout << "      coord: "
                              << section.street_network().coordinates(i).lon()
                                     / navitia::type::GeographicalCoord::N_M_TO_DEG
                              << ", "
                              << section.street_network().coordinates(i).lat()
                                     / navitia::type::GeographicalCoord::N_M_TO_DEG
                              << std::endl;
            }
            if (section.street_network().path_items_size()) {
                std::cout << "    -- street network pathitem: " << std::endl;
                std::cout << "      -- mode: " << section.street_network().mode() << std::endl;
                for (int i = 0; i < section.street_network().path_items_size(); ++i)
                    std::cout << "     - " << section.street_network().path_items(i).name() << " with "
                              << section.street_network().path_items(i).length() << "m | "
                              << section.street_network().path_items(i).duration() << "s" << std::endl;
            }
        }
    }
}

namespace {

struct JourneyCompare {
    bool operator()(const pbnavitia::Journey& j1, const pbnavitia::Journey& j2) const {
        const auto& j1_time = j1.departure_date_time() + j1.duration();
        const auto& j2_time = j2.departure_date_time() + j2.duration();

        return j1_time < j2_time;
    }
};

struct JourneySectionCompare {
    bool operator()(const pbnavitia::Journey& j1, const pbnavitia::Journey& j2) const {
        int idx = 1;
        if (j1.sections_size() == 1)
            idx = 0;

        const auto& sec1_time = j1.sections(idx).begin_date_time() + j1.sections(idx).duration();
        const auto& sec2_time = j2.sections(idx).begin_date_time() + j2.sections(idx).duration();

        return sec1_time < sec2_time;
    }
};

template <class T>
std::vector<pbnavitia::Journey> sort_journeys_by(pbnavitia::Response resp, const T compare) {
    std::vector<pbnavitia::Journey> journeys;
    for (const auto& j : resp.journeys())
        journeys.push_back(j);

    std::sort(journeys.begin(), journeys.end(), compare);
    return journeys;
}

std::vector<pbnavitia::Journey> sort_journeys(pbnavitia::Response resp) {
    return sort_journeys_by(resp, JourneyCompare());
}

}  // namespace

BOOST_AUTO_TEST_CASE(simple_journey) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("A")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 8 * 3600 + 20 * 60,
                                                                         8 * 3600 + 21 * 60);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T021000")}, true,
                  navitia::type::AccessibiliteParams() /*false*/, forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    pbnavitia::Section section = journey.sections(1);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 0);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_area:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_area:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), ntest::to_posix_timestamp("20120614T081100"));
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), ntest::to_posix_timestamp("20120614T082000"));
    BOOST_CHECK_EQUAL(resp.has_next_request_date_time(), true);
}

/*
  0___________________
           4min       \
  1____________________2
           1h9min

  stop0 and stop 1 are only 300m away

  Testing that crow fly feature with no street network works
  for origin and destination, and with coordinates and stop areas.
 */
BOOST_AUTO_TEST_CASE(simple_journey_with_crow_fly) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.sa("stop_area:stop0", 10, 30.003);
    b.sa("stop_area:stop1", 10, 30);
    b.sa("stop_area:stop2", 120, 80);
    b.vj("A")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 9 * 3600 + 20 * 60,
                                                                         9 * 3600 + 21 * 60);
    b.vj("B")("stop_area:stop0", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 8 * 3600 + 15 * 60,
                                                                         8 * 3600 + 16 * 60);
    navitia::type::Data data;
    b.make();
    b.data->build_proximity_list();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    // - stop1 coordinates to stop area stop2 : using 0->2

    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(navitia::type::Type_e::Coord, "coord:10:30");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");
    destination.coordinates = b.data->pt_data->stop_areas_map["stop_area:stop2"]->coord;
    origin.streetnetwork_params.max_duration = 15_min;
    destination.streetnetwork_params.max_duration = 15_min;

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T021000")}, true,
                  navitia::type::AccessibiliteParams() /*false*/, forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    pbnavitia::Section section = journey.sections(1);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 0);
    auto stA = section.stop_date_times(0);
    auto stB = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(stA.stop_point().uri(), "stop_area:stop0");
    BOOST_CHECK_EQUAL(stB.stop_point().uri(), "stop_area:stop2");
    BOOST_CHECK_EQUAL(stA.departure_date_time(), ntest::to_posix_timestamp("20120614T081100"));
    BOOST_CHECK_EQUAL(stB.arrival_date_time(), ntest::to_posix_timestamp("20120614T081500"));
    BOOST_CHECK_EQUAL(journey.sections(2).duration(), 0);

    // - stop area stop1 to stop2 coordinates : using 0->2

    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    origin = {origin_type, "stop_area:stop1"};
    origin.coordinates = b.data->pt_data->stop_areas_map["stop_area:stop1"]->coord;
    destination = {navitia::type::Type_e::Coord, "coord:120:80.002"};
    origin.streetnetwork_params.max_duration = 15_min;
    destination.streetnetwork_params.max_duration = 15_min;

    pb_creator.init(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T021000")}, true,
                  navitia::type::AccessibiliteParams() /*false*/, forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min);
    resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 0);
    stA = section.stop_date_times(0);
    stB = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(stA.stop_point().uri(), "stop_area:stop0");
    BOOST_CHECK_EQUAL(stB.stop_point().uri(), "stop_area:stop2");
    BOOST_CHECK_EQUAL(stA.departure_date_time(), ntest::to_posix_timestamp("20120614T081100"));
    BOOST_CHECK_EQUAL(stB.arrival_date_time(), ntest::to_posix_timestamp("20120614T081500"));
    BOOST_CHECK_EQUAL(journey.sections(0).duration(), 420);
}

BOOST_AUTO_TEST_CASE(journey_stay_in) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true)("ehv", 60300, 60600)("ehb", 60780, 60780)("bet", 61080, 61080)(
        "btl", 61560, 61560)("vg", 61920, 61920)("ht", 62340, 62340);
    b.vj("4462", "1111111", "block1", true)("ht", 62760, 62760)("hto", 62940, 62940)("rs", 63180, 63180);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("bet");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("rs");
    navitia::type::EntryPoint origin(origin_type, "bet");
    navitia::type::EntryPoint destination(destination_type, "rs");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T165300")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min);

    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.nb_transfers(), 0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 1);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(3);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "bet");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "ht");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), navitia::test::to_posix_timestamp("20120614T165800"));
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T171900"));

    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::TRANSFER);
    BOOST_CHECK_EQUAL(section.transfer_type(), pbnavitia::TransferType::stay_in);
    BOOST_CHECK_EQUAL(section.duration(), 420);
    BOOST_CHECK_EQUAL(section.origin().uri(), "ht");
    BOOST_CHECK_EQUAL(section.destination().uri(), "ht");
}

BOOST_AUTO_TEST_CASE(journey_stay_in_teleport) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true)("ehv", 60300, 60600)("ehb", 60780, 60780)("bet", 61080, 61080)(
        "btl", 61560, 61560)("vg", 61920, 61920)("ht:4", 62340, 62340);
    b.vj("4462", "1111111", "block1", true)("ht:4a", 62760, 62760)("hto", 62940, 62940)("rs", 63180, 63180);
    b.connection("ht:4", "ht:4a", 120);
    b.connection("ht:4a", "ht:4", 120);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("bet");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("rs");
    navitia::type::EntryPoint origin(origin_type, "bet");
    navitia::type::EntryPoint destination(destination_type, "rs");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T165300")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.nb_transfers(), 0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 4);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(3);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "bet");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "ht:4");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), navitia::test::to_posix_timestamp("20120614T165800"));
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T171900"));

    section = journey.sections(1);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::TRANSFER);
    BOOST_REQUIRE_EQUAL(section.transfer_type(), pbnavitia::TransferType::stay_in);
    BOOST_REQUIRE_EQUAL(section.duration(), 420);
    BOOST_CHECK_EQUAL(section.origin().uri(), "ht:4");
    BOOST_CHECK_EQUAL(section.destination().uri(), "ht:4a");

    section = journey.sections(2);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 3);
    st1 = section.stop_date_times(0);
    st2 = section.stop_date_times(2);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "ht:4a");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "rs");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), navitia::test::to_posix_timestamp("20120614T172600"));
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T173300"));
}

BOOST_AUTO_TEST_CASE(journey_stay_in_shortteleport) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true)("ehv", 60300, 60600)("ehb", 60780, 60780)("bet", 61080, 61080)(
        "btl", 61560, 61560)("vg", 61920, 61920)("ht:4", 62340, 62340);
    b.vj("4462", "1111111", "block1", true)("ht:4a", 62400, 62400)("hto", 62940, 62940)("rs", 63180, 63180);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("bet");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("rs");
    navitia::type::EntryPoint origin(origin_type, "bet");
    navitia::type::EntryPoint destination(destination_type, "rs");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T165300")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    BOOST_REQUIRE_EQUAL(journey.nb_transfers(), 0);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 4);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(3);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "bet");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "ht:4");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), navitia::test::to_posix_timestamp("20120614T165800"));
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T171900"));
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 1);

    section = journey.sections(1);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::TRANSFER);
    BOOST_REQUIRE_EQUAL(section.transfer_type(), pbnavitia::TransferType::stay_in);
    BOOST_REQUIRE_EQUAL(section.duration(), 60);

    section = journey.sections(2);
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 3);
    auto st3 = section.stop_date_times(0);
    auto st4 = section.stop_date_times(2);
    BOOST_CHECK_EQUAL(st3.stop_point().uri(), "ht:4a");
    BOOST_CHECK_EQUAL(st4.stop_point().uri(), "rs");
    BOOST_CHECK_EQUAL(st3.departure_date_time(), navitia::test::to_posix_timestamp("20120614T172000"));
    BOOST_CHECK_EQUAL(st4.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T173300"));
}

BOOST_AUTO_TEST_CASE(journey_departure_from_a_stay_in) {
    /*
     * For this journey from 'start' to 'end', we begin with a service extension at the terminus of the first vj
     *
     * The results must be presented in 3 sections
     * - one from 'start' to 'start' tagged as a stay in
     * - one transfers
     * - one from 'ht:4a' to 'end'
     */
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true)("ehv", 60300, 60600)("ehb", 60780, 60780)("bet", 61080, 61080)(
        "btl", 61560, 61560)("vg", 61920, 61920)("start", 62340, 62340);
    b.vj("4462", "1111111", "block1", true)("ht:4a", 62400, 62400)("hto", 62940, 62940)("end", 63180, 63180);
    // Note: the vj '4462' is a service extension of '9658' because
    // they have the same block_id ('block1') and 4462 have been defined after 9658
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("start");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("end");
    navitia::type::EntryPoint origin(origin_type, "start");
    navitia::type::EntryPoint destination(destination_type, "end");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T165300")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    BOOST_REQUIRE_EQUAL(journey.nb_transfers(), 0);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 1);
    BOOST_CHECK_EQUAL(section.origin().uri(), "start");
    BOOST_CHECK_EQUAL(section.destination().uri(), "start");
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 1);

    section = journey.sections(1);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::TRANSFER);
    BOOST_REQUIRE_EQUAL(section.transfer_type(), pbnavitia::TransferType::stay_in);
    BOOST_REQUIRE_EQUAL(section.duration(), 60);

    section = journey.sections(2);
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 3);
    auto st3 = section.stop_date_times(0);
    auto st4 = section.stop_date_times(2);
    BOOST_CHECK_EQUAL(st3.stop_point().uri(), "ht:4a");
    BOOST_CHECK_EQUAL(st4.stop_point().uri(), "end");
    BOOST_CHECK_EQUAL(st3.departure_date_time(), navitia::test::to_posix_timestamp("20120614T172000"));
    BOOST_CHECK_EQUAL(st4.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T173300"));
}

BOOST_AUTO_TEST_CASE(journey_arrival_before_a_stay_in) {
    /*
     * For this journey from 'start' to 'end', we stop at the terminus and even if there is a service extension after we
     * do not take it
     *
     * The results must be presented in only one section (and not tagged as 'stay_in')
     */
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true)("ehv", 60300, 60600)("ehb", 60780, 60780)("start", 61080, 61080)(
        "btl", 61560, 61560)("vg", 61920, 61920)("end", 62340, 62340);
    b.vj("4462", "1111111", "block1", true)("ht:4a", 62400, 62400)("hto", 62940, 62940)("rs", 63180, 63180);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("start");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("end");
    navitia::type::EntryPoint origin(origin_type, "start");
    navitia::type::EntryPoint destination(destination_type, "end");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T165300")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    BOOST_REQUIRE_EQUAL(journey.nb_transfers(), 0);

    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(section.origin().uri(), "start");
    BOOST_CHECK_EQUAL(section.destination().uri(), "end");
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 0);
}

BOOST_AUTO_TEST_CASE(journey_arrival_in_a_stay_in) {
    /*
     * For this journey from 'start' to 'end', we stop just after a service extension
     *
     * The results must be presented in 3 sections
     * - one from 'start' to 'ht:4' tagged as a stay in
     * - one stay in transfer
     * - one section from 'end' to 'end'
     */
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true)("ehv", 60300, 60600)("ehb", 60780, 60780)("start", 61080, 61080)(
        "btl", 61560, 61560)("vg", 61920, 61920)("ht:4", 62340, 62350);
    b.vj("4462", "1111111", "block1", true)("end", 62400, 62500)("hto", 62940, 62940)("rs", 63180, 63180);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("start");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("end");
    navitia::type::EntryPoint origin(origin_type, "start");
    navitia::type::EntryPoint destination(destination_type, "end");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T165300")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    dump_response(resp, "arrival_before");
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    BOOST_CHECK_EQUAL(journey.nb_transfers(), 0);

    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(section.origin().uri(), "start");
    BOOST_CHECK_EQUAL(section.destination().uri(), "ht:4");
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 1);

    section = journey.sections(1);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::TRANSFER);
    BOOST_REQUIRE_EQUAL(section.transfer_type(), pbnavitia::TransferType::stay_in);
    BOOST_REQUIRE_EQUAL(section.duration(), 150);

    section = journey.sections(2);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 1);
    BOOST_CHECK_EQUAL(section.origin().uri(), "end");
    BOOST_CHECK_EQUAL(section.destination().uri(), "end");
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 0);
    BOOST_CHECK_EQUAL(section.duration(), 0);
}

BOOST_AUTO_TEST_CASE(journey_arrival_before_a_stay_in_without_teleport) {
    /*
     * For this journey from 'start' to 'end', we stop at the terminus and even if there is a service extension after we
     * do not take it the difference between the 'journey_arrival_before_a_stay_in' test is that the service extension
     * start from the same point and at the same time
     *
     * The results must be presented in only one section (and not tagged as 'stay_in')
     */
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true)("ehv", 60300, 60600)("ehb", 60780, 60780)("start", 61080, 61080)(
        "btl", 61560, 61560)("vg", 61920, 61920)("end", 62340, 62340);  // same point and same time
    b.vj("4462", "1111111", "block1", true)("end", 62340, 62340)        // same point and same time
        ("hto", 62940, 62940)("rs", 63180, 63180);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("start");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("end");
    navitia::type::EntryPoint origin(origin_type, "start");
    navitia::type::EntryPoint destination(destination_type, "end");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T165300")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    BOOST_REQUIRE_EQUAL(journey.nb_transfers(), 0);

    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 4);
    BOOST_CHECK_EQUAL(section.origin().uri(), "start");
    BOOST_CHECK_EQUAL(section.destination().uri(), "end");
    BOOST_CHECK_EQUAL(boost::count(section.additional_informations(), pbnavitia::STAY_IN), 0);
}

BOOST_AUTO_TEST_CASE(journey_stay_in_shortteleport_counterclockwise) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("9658", "1111111", "block1", true)("ehv", 60300, 60600)("ehb", 60780, 60780)("bet", 61080, 61080)(
        "btl", 61560, 61560)("vg", 61920, 61920)("ht:4", 62340, 62340);
    b.vj("4462", "1111111", "block1", true)("ht:4a", 62400, 62400)("hto", 62940, 62940)("rs", 63180, 63180);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("bet");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("rs");
    navitia::type::EntryPoint origin(origin_type, "bet");
    navitia::type::EntryPoint destination(destination_type, "rs");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T174000")}, false,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    BOOST_REQUIRE_EQUAL(journey.nb_transfers(), 0);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 4);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(3);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "bet");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "ht:4");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), navitia::test::to_posix_timestamp("20120614T165800"));
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T171900"));

    section = journey.sections(1);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::TRANSFER);
    BOOST_REQUIRE_EQUAL(section.transfer_type(), pbnavitia::TransferType::stay_in);
    BOOST_REQUIRE_EQUAL(section.duration(), 60);

    section = journey.sections(2);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 3);
    auto st3 = section.stop_date_times(0);
    auto st4 = section.stop_date_times(2);
    BOOST_CHECK_EQUAL(st3.stop_point().uri(), "ht:4a");
    BOOST_CHECK_EQUAL(st4.stop_point().uri(), "rs");
    BOOST_CHECK_EQUAL(st3.departure_date_time(), navitia::test::to_posix_timestamp("20120614T172000"));
    BOOST_CHECK_EQUAL(st4.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T173300"));
}

BOOST_AUTO_TEST_CASE(journey_array) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("A")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 8 * 3600 + 20 * 60,
                                                                         8 * 3600 + 21 * 60);
    b.vj("A")("stop_area:stop1", 9 * 3600 + 10 * 60, 9 * 3600 + 11 * 60)("stop_area:stop2", 9 * 3600 + 20 * 60,
                                                                         9 * 3600 + 21 * 60);
    navitia::type::Data data;
    b.make();
    b.data->geo_ref->init();
    b.data->build_proximity_list();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");

    ng::StreetNetwork sn_worker(*data.geo_ref);

    // we put the time not in the right order to check that they are correctly sorted
    std::vector<uint64_t> datetimes(
        {ntest::to_posix_timestamp("20120614T080000"), ntest::to_posix_timestamp("20120614T090000")});
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    nr::make_response(pb_creator, raptor, origin, destination, datetimes, true, navitia::type::AccessibiliteParams(),
                      forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 2);

    auto journeys = sort_journeys(resp);

    auto& journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    pbnavitia::Section section = journey.sections(1);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    auto st1 = section.stop_date_times(0);
    auto st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_area:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_area:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), navitia::test::to_posix_timestamp("20120614T081100"));
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T082000"));

    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_REQUIRE_EQUAL(section.stop_date_times_size(), 2);
    st1 = section.stop_date_times(0);
    st2 = section.stop_date_times(1);
    BOOST_CHECK_EQUAL(st1.stop_point().uri(), "stop_area:stop1");
    BOOST_CHECK_EQUAL(st2.stop_point().uri(), "stop_area:stop2");
    BOOST_CHECK_EQUAL(st1.departure_date_time(), navitia::test::to_posix_timestamp("20120614T091100"));
    BOOST_CHECK_EQUAL(st2.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T092000"));
}

template <typename speed_provider_trait>
struct streetnetworkmode_fixture : public routing_api_data<speed_provider_trait> {
    pbnavitia::Response make_response() {
        ng::StreetNetwork sn_worker(*this->b.data->geo_ref);
        nr::RAPTOR raptor(*this->b.data);
        auto* data_ptr = this->b.data.get();
        navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
        nr::make_response(pb_creator, raptor, this->origin, this->destination, this->datetimes, true,
                          navitia::type::AccessibiliteParams(), this->forbidden, {}, sn_worker, nt::RTLevel::Base,
                          2_min);
        return pb_creator.get_response();
    }
};

struct direct_path_routing_api_data : routing_api_data<test_speed_provider> {
    direct_path_routing_api_data()
        : routing_api_data<test_speed_provider>(100),
          // note: we need to set the A->B distance to 100 to have something coherent concerning the crow fly distance
          // between A and B note2: we set it to 200 in the routing_api_data not to have projection problems
          worker(*this->b.data->geo_ref) {
        origin.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
        origin.streetnetwork_params.offset = 0;
        origin.streetnetwork_params.max_duration =
            bt::pos_infin;  // not used, overloaded in find_neares_stop_points call
        origin.streetnetwork_params.speed_factor = 1;

        worker.init(origin);
    }
    ng::StreetNetwork worker;
};

BOOST_FIXTURE_TEST_CASE(direct_path_under_the_limit, direct_path_routing_api_data) {
    // we limit the search to the limit of A->distance minus 1, so we can't find A (just B)
    auto a_s_distance = B.distance_to(S) + distance_ab;
    auto a_s_dur = to_duration(a_s_distance, navitia::type::Mode_e::Walking);

    auto sp = worker.find_nearest_stop_points(a_s_dur - navitia::seconds(1), b.data->pt_data->stop_point_proximity_list,
                                              false);

    navitia::routing::map_stop_point_duration tested_map;
    tested_map[navitia::routing::SpIdx(*(b.data->pt_data->stop_points_map["stop_point:stopB"]))] =
        to_duration(B.distance_to(S), navitia::type::Mode_e::Walking);
    BOOST_CHECK_EQUAL_COLLECTIONS(sp.cbegin(), sp.cend(), tested_map.cbegin(), tested_map.cend());
}

BOOST_FIXTURE_TEST_CASE(direct_path_exactly_the_limit, direct_path_routing_api_data) {
    // we limit the search to the limit of A->distance so we can find A (and B)
    auto a_s_distance = B.distance_to(S) + distance_ab;
    auto a_s_dur = to_duration(a_s_distance, navitia::type::Mode_e::Walking);

    auto sp = worker.find_nearest_stop_points(a_s_dur, b.data->pt_data->stop_point_proximity_list, false);

    navitia::routing::map_stop_point_duration tested_map;
    tested_map[navitia::routing::SpIdx(*(b.data->pt_data->stop_points_map["stop_point:stopB"]))] =
        to_duration(B.distance_to(S), navitia::type::Mode_e::Walking);
    tested_map[navitia::routing::SpIdx(*(b.data->pt_data->stop_points_map["stop_point:stopA"]))] = a_s_dur;
    tested_map[navitia::routing::SpIdx(*(b.data->pt_data->stop_points_map["stop_point:uselessA"]))] = a_s_dur;
    BOOST_CHECK_EQUAL_COLLECTIONS(sp.cbegin(), sp.cend(), tested_map.cbegin(), tested_map.cend());
}

BOOST_FIXTURE_TEST_CASE(direct_path_over_the_limit, direct_path_routing_api_data) {
    // we limit the search to the limit of A->distance plus 1, so we can find A (and B)
    auto a_s_distance = B.distance_to(S) + distance_ab;
    auto a_s_dur = to_duration(a_s_distance, navitia::type::Mode_e::Walking);

    auto sp = worker.find_nearest_stop_points(a_s_dur + navitia::seconds(1), b.data->pt_data->stop_point_proximity_list,
                                              false);

    navitia::routing::map_stop_point_duration tested_map;
    tested_map[navitia::routing::SpIdx(*(b.data->pt_data->stop_points_map["stop_point:stopB"]))] =
        to_duration(B.distance_to(S), navitia::type::Mode_e::Walking);
    tested_map[navitia::routing::SpIdx(*(b.data->pt_data->stop_points_map["stop_point:stopA"]))] = a_s_dur;
    tested_map[navitia::routing::SpIdx(*(b.data->pt_data->stop_points_map["stop_point:uselessA"]))] = a_s_dur;
    BOOST_CHECK_EQUAL_COLLECTIONS(sp.cbegin(), sp.cend(), tested_map.cbegin(), tested_map.cend());
}

// Walking
BOOST_FIXTURE_TEST_CASE(walking_test, streetnetworkmode_fixture<test_speed_provider>) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.max_duration =
        navitia::seconds(15 * 60);  // navitia::seconds(200 / get_default_speed()[navitia::type::Mode_e::Walking]);
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.max_duration =
        navitia::seconds(15 * 60);  // navitia::seconds(50 / get_default_speed()[navitia::type::Mode_e::Walking]);

    pbnavitia::Response resp = make_response();

    dump_response(resp, "walking");

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 2);
    pbnavitia::Journey journey = resp.journeys(1);
    BOOST_CHECK_EQUAL(journey.departure_date_time(), navitia::test::to_posix_timestamp("20120614T080000"));
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), navitia::test::to_posix_timestamp("20120614T080510"));

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);

    BOOST_CHECK_EQUAL(section.street_network().coordinates_size(), 4);
    BOOST_CHECK(!section.id().empty());
    BOOST_CHECK_EQUAL(section.street_network().duration(), 310);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 3);
    BOOST_CHECK_EQUAL(section.street_network().path_items(0).name(), "rue bs");
    BOOST_CHECK_EQUAL(section.street_network().path_items(0).duration(), 20);
    BOOST_CHECK_EQUAL(section.street_network().path_items(1).name(), "rue ab");
    BOOST_CHECK_EQUAL(section.street_network().path_items(1).duration(), 200);
    BOOST_CHECK_EQUAL(section.street_network().path_items(2).name(), "rue ar");
    BOOST_CHECK_EQUAL(section.street_network().path_items(2).duration(), 90);

    // check that the shape is used, i.e. there is not only 2 points
    // from the stop times.
    journey = resp.journeys(0);
    BOOST_CHECK_EQUAL(journey.most_serious_disruption_effect(), "");  // no disruption should be found
    BOOST_CHECK_EQUAL(journey.duration(), 112);
    BOOST_CHECK_EQUAL(journey.durations().total(), 112);
    BOOST_CHECK_EQUAL(journey.durations().walking(), 110);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.shape().size(), 3);

    // check durations
    for (int idx_j = 0; idx_j < resp.journeys_size(); idx_j++) {
        const auto& j = resp.journeys(idx_j);
        auto journey_duration = j.duration();
        int total_dur(0);
        for (int idx_section = 0; idx_section < j.sections().size(); ++idx_section) {
            const auto& section = j.sections(idx_section);
            total_dur += section.duration();
        }

        BOOST_CHECK_EQUAL(journey_duration, total_dur);
    }
}

// biking
BOOST_FIXTURE_TEST_CASE(biking, streetnetworkmode_fixture<test_speed_provider>) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    origin.streetnetwork_params.max_duration = navitia::minutes(15);
    origin.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    destination.streetnetwork_params.max_duration = navitia::minutes(15);
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();
    dump_response(resp, "biking");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    auto section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ag");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 8);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_CHECK_EQUAL(section.street_network().duration(),
                      130);  // it's the biking distance / biking speed (but there can be rounding pb)
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 7);

    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(pathitem.direction(), 0);  // first direction is always 0°
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");  // after that we went strait so still 0°
    BOOST_CHECK_EQUAL(pathitem.direction(), 0);
    pathitem = section.street_network().path_items(2);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    BOOST_CHECK_EQUAL(pathitem.direction(), 90);  // then we turned right
    pathitem = section.street_network().path_items(3);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    BOOST_CHECK_EQUAL(pathitem.direction(), 90);  // then we turned right
    pathitem = section.street_network().path_items(4);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    BOOST_CHECK_EQUAL(pathitem.direction(), -90);  // then we turned left
    pathitem = section.street_network().path_items(5);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    pathitem = section.street_network().path_items(6);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");

    // Bike mode
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 1);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(0).has_co2_emission(), true);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(0).co2_emission().value(), 0.);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(0).co2_emission().unit(), "gEC");
    // Aggregator co2_emission
    BOOST_CHECK_EQUAL(resp.journeys(0).has_co2_emission(), true);
    BOOST_CHECK_EQUAL(resp.journeys(0).co2_emission().value(), 0.);
    BOOST_CHECK_EQUAL(resp.journeys(0).co2_emission().unit(), "gEC");
}

// biking
BOOST_FIXTURE_TEST_CASE(biking_walking, streetnetworkmode_fixture<test_speed_provider>) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    origin.streetnetwork_params.max_duration = navitia::minutes(15);
    origin.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Walking];
    destination.streetnetwork_params.max_duration = navitia::minutes(15);
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();
    dump_response(resp, "biking");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    auto section = journey.sections(0);
    // For a directpath we don't have next_request_date_time
    BOOST_CHECK_EQUAL(resp.has_next_request_date_time(), false);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ag");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 8);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_CHECK_EQUAL(section.street_network().duration(),
                      130);  // it's the biking distance / biking speed (but there can be rounding pb)
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 7);
    BOOST_CHECK_EQUAL(journey.durations().walking(), 0);
    BOOST_CHECK_EQUAL(journey.durations().bike(), 130);
    BOOST_CHECK_EQUAL(journey.durations().car(), 0);
    BOOST_CHECK_EQUAL(journey.durations().total(), 130);
    BOOST_CHECK_EQUAL(journey.distances().walking(), 0);
    BOOST_CHECK_EQUAL(journey.distances().bike(), 533);
    BOOST_CHECK_EQUAL(journey.distances().car(), 0);

    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(pathitem.direction(), 0);  // first direction is always 0°
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");  // after that we went strait so still 0°
    BOOST_CHECK_EQUAL(pathitem.direction(), 0);
    pathitem = section.street_network().path_items(2);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    BOOST_CHECK_EQUAL(pathitem.direction(), 90);  // then we turned right
    pathitem = section.street_network().path_items(3);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    BOOST_CHECK_EQUAL(pathitem.direction(), 90);  // then we turned right
    pathitem = section.street_network().path_items(4);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    BOOST_CHECK_EQUAL(pathitem.direction(), -90);  // then we turned left
    pathitem = section.street_network().path_items(5);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    pathitem = section.street_network().path_items(6);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
}

// Biking with a different speed
BOOST_FIXTURE_TEST_CASE(biking_with_different_speed, streetnetworkmode_fixture<test_speed_provider>) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    origin.streetnetwork_params.max_duration = bt::pos_infin;
    origin.streetnetwork_params.speed_factor = .5;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bike;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bike];
    destination.streetnetwork_params.max_duration = bt::pos_infin;
    destination.streetnetwork_params.speed_factor = .5;

    auto resp = make_response();
    dump_response(resp, "biking with different speed");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 2);
    auto journey = resp.journeys(1);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    auto section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ag");
    BOOST_REQUIRE_EQUAL(section.street_network().coordinates_size(), 8);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_CHECK_EQUAL(section.street_network().duration(), 130 * 2);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 7);

    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    // NOTE: we cannot check length here, it has to be checked in a separate test (below), since the used faked speed in
    // the test
    BOOST_CHECK_CLOSE(pathitem.duration(), S.distance_to(B) / double(get_default_speed()[nt::Mode_e::Bike] * .5f), 1);
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    pathitem = section.street_network().path_items(2);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    pathitem = section.street_network().path_items(3);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    pathitem = section.street_network().path_items(4);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    pathitem = section.street_network().path_items(5);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    pathitem = section.street_network().path_items(6);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    BOOST_CHECK_CLOSE(pathitem.duration(), A.distance_to(G) / double(get_default_speed()[nt::Mode_e::Bike] * .5f), 1);
}

// for an admin, when no main stop area is set
// we should output a fall back section, as it is used for computation
BOOST_FIXTURE_TEST_CASE(admin_explicit_fall_back, streetnetworkmode_fixture<test_speed_provider>) {
    origin.uri = "admin:74435";
    origin.type = b.data->get_type_of_id(origin.uri);
    origin.streetnetwork_params.max_duration = navitia::seconds(100);
    destination.streetnetwork_params.max_duration = navitia::seconds(100);

    auto resp = make_response();
    dump_response(resp, "admin_explicit_fall_back");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    auto section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_CHECK_GT(section.duration(), 0);
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);

    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(pathitem.direction(), 0);  // first direction is always 0°
}

// direct journey using only car as fallback mode
BOOST_FIXTURE_TEST_CASE(car_as_fallback_mode, streetnetworkmode_fixture<test_speed_provider>) {
    auto total_distance =
        S.distance_to(B) + B.distance_to(C) + C.distance_to(F) + F.distance_to(E) + E.distance_to(A) + 1;

    origin.streetnetwork_params.mode = navitia::type::Mode_e::Car;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Car];
    origin.streetnetwork_params.max_duration =
        navitia::seconds(total_distance / double(get_default_speed()[navitia::type::Mode_e::Car]));
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = navitia::type::Mode_e::Car;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Car];
    destination.streetnetwork_params.max_duration =
        navitia::seconds(total_distance / double(get_default_speed()[navitia::type::Mode_e::Car]));
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();

    dump_response(resp, "car_direct");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);  // 1 path, just the PT one, we do not compute the direct path for car

    // co2_emission Tests
    // First Journey
    // Car mode
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 7);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(0).street_network().mode(), pbnavitia::StreetNetworkMode::Car);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(0).co2_emission().value(), 12.144);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(0).co2_emission().unit(), "gEC");

    BOOST_CHECK_EQUAL(resp.journeys(0).sections(1).has_co2_emission(), false);
    // Walk mode
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(2).has_co2_emission(), false);
    // Tram mode
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(3).has_co2_emission(), true);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(3).pt_display_informations().physical_mode(), "Tramway");
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(3).co2_emission().value(), 0.58);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(3).co2_emission().unit(), "gEC");
    // Walk mode
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(4).has_co2_emission(), false);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(5).has_co2_emission(), false);

    // Car mode
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(6).has_co2_emission(), true);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(6).street_network().mode(), pbnavitia::StreetNetworkMode::Car);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(6).co2_emission().value(), 48.944);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(6).co2_emission().unit(), "gEC");
    // Aggregator co2_emission
    BOOST_CHECK_EQUAL(resp.journeys(0).has_co2_emission(), true);
    BOOST_CHECK_EQUAL(resp.journeys(0).co2_emission().value(), 12.144 + 0.58 + 48.944);
    BOOST_CHECK_EQUAL(resp.journeys(0).co2_emission().unit(), "gEC");

    BOOST_CHECK_EQUAL(resp.journeys(0).durations().walking(), 40);
    BOOST_CHECK_EQUAL(resp.journeys(0).durations().bike(), 0);
    BOOST_CHECK_EQUAL(resp.journeys(0).durations().car(), 30);
    BOOST_CHECK_EQUAL(resp.journeys(0).durations().total(), 75);
    BOOST_CHECK_EQUAL(resp.journeys(0).distances().walking(), 44);
    BOOST_CHECK_EQUAL(resp.journeys(0).distances().bike(), 0);
    BOOST_CHECK_EQUAL(resp.journeys(0).distances().car(), 332);
}

// car + bus using parking
BOOST_FIXTURE_TEST_CASE(car_parking_bus, streetnetworkmode_fixture<test_speed_provider>) {
    using namespace navitia;
    using navitia::seconds;
    using type::Mode_e;

    const auto distance_sd = S.distance_to(B) + B.distance_to(D);
    const auto distance_ar = A.distance_to(R);
    const double speed_factor = 1;

    origin.streetnetwork_params.mode = Mode_e::Car;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[Mode_e::Car];
    origin.streetnetwork_params.max_duration =
        seconds(distance_sd / double(get_default_speed()[Mode_e::Car]) / speed_factor)
        + b.data->geo_ref->default_time_parking_park
        + seconds(B.distance_to(D) / double(get_default_speed()[Mode_e::Walking]) / speed_factor) + seconds(2);
    origin.streetnetwork_params.speed_factor = speed_factor;

    destination.streetnetwork_params.mode = Mode_e::Walking;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[Mode_e::Walking];
    destination.streetnetwork_params.max_duration =
        seconds(distance_ar / double(get_default_speed()[Mode_e::Walking]) / speed_factor) + seconds(1);
    destination.streetnetwork_params.speed_factor = speed_factor;

    const auto resp = make_response();

    dump_response(resp, "car_parking_bus");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);  // 1 car->bus->walk (no direct, it's for car)

    // the car->bus->walk journey
    const auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 5);
    const auto sections = journey.sections();

    // section 0: goto parking
    BOOST_CHECK_EQUAL(sections.Get(0).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(sections.Get(0).origin().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(sections.Get(0).destination().poi().name(), "first parking");
    BOOST_CHECK_EQUAL(sections.Get(0).street_network().mode(), pbnavitia::StreetNetworkMode::Car);
    BOOST_CHECK_EQUAL(sections.Get(0).street_network().duration(), 6);
    const auto pathitems0 = sections.Get(0).street_network().path_items();
    BOOST_REQUIRE_EQUAL(pathitems0.size(), 2);
    BOOST_CHECK_EQUAL(pathitems0.Get(0).name(), "rue bs");
    BOOST_CHECK_EQUAL(pathitems0.Get(1).name(), "rue bd");

    // section 1: park
    BOOST_CHECK_EQUAL(sections.Get(1).type(), pbnavitia::SectionType::PARK);
    BOOST_CHECK_EQUAL(sections.Get(1).origin().poi().name(), "first parking");
    BOOST_CHECK_EQUAL(sections.Get(1).destination().address().name(), "rue bd");
    BOOST_CHECK_EQUAL(sections.Get(1).street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_CHECK_EQUAL(sections.Get(1).street_network().duration(), 1);
    const auto pathitems1 = sections.Get(1).street_network().path_items();
    BOOST_REQUIRE_EQUAL(pathitems1.size(), 1);
    BOOST_CHECK_EQUAL(pathitems1.Get(0).name(), "rue bd");

    // section 2: goto B
    BOOST_CHECK_EQUAL(sections.Get(2).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(sections.Get(2).origin().address().name(), "rue bd");
    BOOST_CHECK_EQUAL(sections.Get(2).destination().name(), "stop_point:stopB (Condom)");
    BOOST_CHECK_EQUAL(sections.Get(2).street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_CHECK_EQUAL(sections.Get(2).street_network().duration(), 10);
    const auto pathitems2 = sections.Get(2).street_network().path_items();
    BOOST_REQUIRE_EQUAL(pathitems2.size(), 2);
    BOOST_CHECK_EQUAL(pathitems2.Get(0).name(), "rue bd");
    BOOST_CHECK_EQUAL(pathitems2.Get(1).name(), "rue ab");

    // section 3: PT B->A
    BOOST_CHECK_EQUAL(sections.Get(3).type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(sections.Get(3).origin().name(), "stop_point:stopB (Condom)");
    BOOST_REQUIRE_EQUAL(sections.Get(3).destination().stop_point().codes().size(), 5);
    BOOST_CHECK_EQUAL(sections.Get(3).destination().name(), "stop_point:stopA (Condom)");

    // section 4: goto R
    BOOST_CHECK_EQUAL(sections.Get(4).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(sections.Get(4).origin().name(), "stop_point:stopA (Condom)");
    BOOST_CHECK_EQUAL(sections.Get(4).destination().address().name(), "rue ag");
    BOOST_REQUIRE_EQUAL(sections.Get(4).origin().stop_point().codes().size(), 5);
    BOOST_CHECK_EQUAL(sections.Get(4).street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_CHECK_EQUAL(sections.Get(4).street_network().duration(), 90);
    const auto pathitems4 = sections.Get(4).street_network().path_items();
    BOOST_REQUIRE_EQUAL(pathitems4.size(), 2);
    BOOST_CHECK_EQUAL(pathitems4.Get(0).name(), "rue ab");
    BOOST_CHECK_EQUAL(pathitems4.Get(1).name(), "rue ar");
}

// bus + car using parking
BOOST_FIXTURE_TEST_CASE(bus_car_parking, streetnetworkmode_fixture<test_speed_provider>) {
    using namespace navitia;
    using navitia::seconds;
    using type::Mode_e;

    const auto distance_sd = S.distance_to(B) + B.distance_to(D);
    const auto distance_ar = A.distance_to(R);
    const double speed_factor = 1;

    origin.streetnetwork_params.mode = Mode_e::Car;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[Mode_e::Car];
    origin.streetnetwork_params.max_duration =
        seconds(distance_sd / double(get_default_speed()[Mode_e::Car]) / speed_factor)
        + b.data->geo_ref->default_time_parking_park
        + seconds(B.distance_to(D) / double(get_default_speed()[Mode_e::Walking]) / speed_factor) + seconds(2);
    origin.streetnetwork_params.speed_factor = speed_factor;

    destination.streetnetwork_params.mode = Mode_e::Walking;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[Mode_e::Walking];
    destination.streetnetwork_params.max_duration =
        seconds(distance_ar / double(get_default_speed()[Mode_e::Walking]) / speed_factor) + seconds(1);
    destination.streetnetwork_params.speed_factor = speed_factor;
    datetimes = {navitia::test::to_posix_timestamp("20120614T070000")};

    ng::StreetNetwork sn_worker(*this->b.data->geo_ref);
    nr::RAPTOR raptor(*this->b.data);
    auto* data_ptr = this->b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    nr::make_response(pb_creator, raptor, this->destination, this->origin, this->datetimes, true,
                      navitia::type::AccessibiliteParams(), this->forbidden, {}, sn_worker, type::RTLevel::Base, 2_min);
    auto resp = pb_creator.get_response();

    dump_response(resp, "bus_car_parking");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);  // 1 direct car + 1 car->bus->walk

    // the car->bus->walk journey
    const auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 5);
    const auto sections = journey.sections();

    // section 0: goto to the station
    BOOST_CHECK_EQUAL(sections.Get(0).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(sections.Get(0).origin().address().name(), "rue ag");
    BOOST_CHECK_EQUAL(sections.Get(0).destination().stop_point().name(), "stop_point:stopA");
    BOOST_CHECK_EQUAL(sections.Get(0).street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_CHECK_EQUAL(sections.Get(0).street_network().duration(), 90);

    // section 1: PT A->B
    BOOST_CHECK_EQUAL(sections.Get(1).type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(sections.Get(1).origin().stop_point().name(), "stop_point:stopA");
    BOOST_CHECK_EQUAL(sections.Get(1).destination().stop_point().name(), "stop_point:stopB");

    // section 2: goto the parking
    BOOST_CHECK_EQUAL(sections.Get(2).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(sections.Get(2).origin().stop_point().name(), "stop_point:stopB");
    BOOST_CHECK_EQUAL(sections.Get(2).destination().poi().name(), "first parking");
    BOOST_CHECK_EQUAL(sections.Get(2).street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_CHECK_EQUAL(sections.Get(2).street_network().duration(), 10);
    const auto pathitems2 = sections.Get(2).street_network().path_items();
    BOOST_REQUIRE_EQUAL(pathitems2.size(), 2);
    BOOST_CHECK_EQUAL(pathitems2.Get(0).name(), "rue ab");
    BOOST_CHECK_EQUAL(pathitems2.Get(1).name(), "rue bd");

    // section 3: leave parking
    BOOST_CHECK_EQUAL(sections.Get(3).type(), pbnavitia::SectionType::LEAVE_PARKING);
    BOOST_CHECK_EQUAL(sections.Get(3).origin().poi().name(), "first parking");
    BOOST_CHECK_EQUAL(sections.Get(3).destination().address().name(), "rue bd");

    // section 4: car
    BOOST_CHECK_EQUAL(sections.Get(4).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(sections.Get(4).origin().address().name(), "rue bd");
    BOOST_CHECK_EQUAL(sections.Get(4).destination().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(sections.Get(4).street_network().mode(), pbnavitia::StreetNetworkMode::Car);
    BOOST_CHECK_EQUAL(sections.Get(4).street_network().duration(), 6);
    const auto pathitems4 = sections.Get(4).street_network().path_items();
    BOOST_REQUIRE_EQUAL(pathitems4.size(), 2);
    BOOST_CHECK_EQUAL(pathitems4.Get(0).name(), "rue bd");
    BOOST_CHECK_EQUAL(pathitems4.Get(1).name(), "rue bs");
}

// BSS test
BOOST_FIXTURE_TEST_CASE(bss_test, streetnetworkmode_fixture<test_speed_provider>) {
    origin.streetnetwork_params.mode = navitia::type::Mode_e::Bss;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bss];
    origin.streetnetwork_params.max_duration = navitia::minutes(15);
    origin.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.mode = navitia::type::Mode_e::Bss;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[navitia::type::Mode_e::Bss];
    destination.streetnetwork_params.max_duration = navitia::minutes(15);
    destination.streetnetwork_params.speed_factor = 1;

    auto resp = make_response();
    dump_response(resp, "bss");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    pbnavitia::Journey* journey = nullptr;
    for (int i = 0; i < resp.journeys_size(); ++i) {
        auto j = resp.mutable_journeys(i);
        if (j->sections_size() >= 2 && j->sections(1).type() == pbnavitia::SectionType::BSS_RENT) {
            journey = j;
            break;
        }
    }
    BOOST_REQUIRE(journey != nullptr);

    BOOST_REQUIRE_EQUAL(journey->sections_size(), 5);
    // we should have 5 sections
    // 1 walk, 1 boarding, 1 bike, 1 landing, and 1 final walking section
    auto section = journey->sections(0);

    // walk
    BOOST_CHECK(!section.id().empty());
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue bs");
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    auto pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue bs");
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);

    auto prev_section = section;
    // getting the bss bike
    section = journey->sections(1);
    BOOST_CHECK_EQUAL(prev_section.destination().name(), section.origin().name());
    BOOST_CHECK_EQUAL(prev_section.destination().uri(), section.origin().uri());
    BOOST_CHECK(!section.id().empty());
    // check bss station placemark, it must be a poi
    const auto& first_vls_station =
        section.destination();  // the last place of the walking section must be the vls station
    BOOST_CHECK_EQUAL(first_vls_station.embedded_type(), pbnavitia::POI);
    BOOST_CHECK_EQUAL(first_vls_station.name(), "first station (Condom)");

    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::BSS_RENT);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK(!pathitem.name().empty());  // projection should works, but where is irrelevant
    BOOST_CHECK_EQUAL(pathitem.duration(), b.data->geo_ref->default_time_bss_pickup.total_seconds());
    BOOST_CHECK_EQUAL(pathitem.length(), 0);

    BOOST_CHECK_EQUAL(journey->durations().walking(), 129);
    BOOST_CHECK_EQUAL(journey->durations().bike(), 110);
    BOOST_CHECK_EQUAL(journey->durations().car(), 0);
    BOOST_CHECK_EQUAL(journey->durations().total(), 309);
    BOOST_CHECK_EQUAL(journey->distances().walking(), 144);
    BOOST_CHECK_EQUAL(journey->distances().bike(), 451);
    BOOST_CHECK_EQUAL(journey->distances().car(), 0);

    // bike
    prev_section = section;
    section = journey->sections(2);
    BOOST_CHECK(!section.id().empty());
    BOOST_CHECK_EQUAL(prev_section.destination().name(), section.origin().name());
    BOOST_CHECK_EQUAL(prev_section.destination().uri(), section.origin().uri());
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().poi().name(), "first station");
    BOOST_CHECK_EQUAL(section.destination().poi().name(), "second station");
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Bike);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 5);
    int cpt_item(0);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    BOOST_CHECK_EQUAL(pathitem.duration(), 45);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    BOOST_CHECK_EQUAL(pathitem.duration(), 30);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    BOOST_CHECK_EQUAL(pathitem.duration(), 10);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    BOOST_CHECK_EQUAL(pathitem.duration(), 15);
    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    BOOST_CHECK_EQUAL(pathitem.duration(), 10);
    BOOST_CHECK_EQUAL(section.origin().poi().coord().lon(), section.street_network().coordinates(0).lon());
    BOOST_CHECK_EQUAL(section.origin().poi().coord().lat(), section.street_network().coordinates(0).lat());

    // putting back the bss bike
    prev_section = section;
    section = journey->sections(3);
    BOOST_CHECK(!section.id().empty());
    BOOST_CHECK_EQUAL(prev_section.destination().name(), section.origin().name());
    BOOST_CHECK_EQUAL(prev_section.destination().uri(), section.origin().uri());
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::BSS_PUT_BACK);
    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 1);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.duration(), b.data->geo_ref->default_time_bss_putback.total_seconds());
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    BOOST_CHECK_EQUAL(pathitem.length(), 0);
    const auto& last_vls_section = section.origin();  // the last place of the walking section must be the vls station
    BOOST_CHECK_EQUAL(last_vls_section.embedded_type(), pbnavitia::POI);
    BOOST_CHECK_EQUAL(last_vls_section.name(), "second station (Condom)");

    prev_section = section;
    // walking
    section = journey->sections(4);
    BOOST_CHECK(!section.id().empty());
    BOOST_CHECK_EQUAL(prev_section.destination().name(), section.origin().name());
    BOOST_CHECK_EQUAL(prev_section.destination().uri(), section.origin().uri());
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(section.origin().address().name(), "rue ag");
    BOOST_CHECK_EQUAL(section.destination().address().name(), "rue ag");
    BOOST_CHECK_EQUAL(section.street_network().mode(), pbnavitia::StreetNetworkMode::Walking);

    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 2);
    pathitem = section.street_network().path_items(0);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    pathitem = section.street_network().path_items(1);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ar");
    //    BOOST_CHECK_EQUAL(pathitem.duration(), 0); //projection
    BOOST_CHECK_CLOSE(pathitem.duration(), A.distance_to(R) / double(get_default_speed()[nt::Mode_e::Walking]), 1);
    BOOST_CHECK_EQUAL(section.origin().address().coord().lon(), section.street_network().coordinates(0).lon());
    BOOST_CHECK_EQUAL(section.origin().address().coord().lat(), section.street_network().coordinates(0).lat());
}

/**
 * With the test_default_speed it's difficult to test the length of the journeys, so here is a new fixture with
 * classical default speed
 */

BOOST_FIXTURE_TEST_CASE(biking_length_test, streetnetworkmode_fixture<normal_speed_provider>) {
    auto mode = navitia::type::Mode_e::Bike;
    origin.streetnetwork_params.mode = mode;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.max_duration = bt::pos_infin;
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = mode;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.max_duration = bt::pos_infin;

    pbnavitia::Response resp = make_response();

    pbnavitia::Journey journey;
    for (int idx = 0; idx < resp.journeys().size(); ++idx) {
        if (resp.journeys(idx).sections().size() == 1) {
            journey = resp.journeys(idx);
        }
    }

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);

    dump_response(resp, "biking length test");

    std::vector<pbnavitia::PathItem> path_items;
    for (int s = 0; s < journey.sections_size(); s++) {
        for (int i = 0; i < journey.sections(s).street_network().path_items_size(); ++i) {
            path_items.push_back(journey.sections(s).street_network().path_items(i));
        }
    }

    BOOST_REQUIRE_EQUAL(path_items.size(), 7);
    int cpt(0);
    auto pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_EQUAL(pathitem.duration(),
                      to_duration(B.distance_to(S), navitia::type::Mode_e::Bike).total_fractional_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), B.distance_to(S), 2);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue kb");
    BOOST_CHECK_EQUAL(pathitem.duration(),
                      to_duration(K.distance_to(B), navitia::type::Mode_e::Bike).total_fractional_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), B.distance_to(K), 2);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue jk");
    BOOST_CHECK_EQUAL(pathitem.duration(),
                      to_duration(K.distance_to(J), navitia::type::Mode_e::Bike).total_fractional_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), J.distance_to(K), 2);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ij");
    BOOST_CHECK_EQUAL(pathitem.duration(),
                      to_duration(I.distance_to(J), navitia::type::Mode_e::Bike).total_fractional_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), I.distance_to(J), 2);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue hi");
    BOOST_CHECK_EQUAL(pathitem.duration(),
                      to_duration(I.distance_to(H), navitia::type::Mode_e::Bike).total_fractional_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), I.distance_to(H), 2);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue gh");
    BOOST_CHECK_EQUAL(pathitem.duration(),
                      to_duration(G.distance_to(H), navitia::type::Mode_e::Bike).total_fractional_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), G.distance_to(H), 2);

    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ag");
    BOOST_CHECK_EQUAL(pathitem.duration(),
                      to_duration(distance_ag, navitia::type::Mode_e::Bike).total_fractional_seconds());
    BOOST_CHECK_CLOSE(pathitem.length(), distance_ag, 2);

    // we check the total
    double total_dur(0);
    double total_distance(0);
    for (const auto& item : path_items) {
        total_dur += item.duration();
        total_distance += item.length();
    }
    BOOST_CHECK_CLOSE(section.length(), total_distance, 1);
    BOOST_CHECK_CLOSE(section.duration(), total_dur, 2);
}

/**
 * Test length with a different speed factors
 */

BOOST_FIXTURE_TEST_CASE(speed_factor_length_test, streetnetworkmode_fixture<normal_speed_provider>) {
    auto mode = navitia::type::Mode_e::Walking;
    // you're supposed to be able to put any value here (only rounding might break tests)
    double speed_factor = 1.5;
    origin.streetnetwork_params.mode = mode;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.max_duration = bt::pos_infin;
    origin.streetnetwork_params.speed_factor = speed_factor;

    destination.streetnetwork_params.mode = mode;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.max_duration = bt::pos_infin;
    destination.streetnetwork_params.speed_factor = speed_factor;

    pbnavitia::Response resp = make_response();

    pbnavitia::Journey journey;
    for (int idx = 0; idx < resp.journeys().size(); ++idx) {
        if (resp.journeys(idx).sections().size() == 1) {
            journey = resp.journeys(idx);
        }
    }

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);

    dump_response(resp, "speed factor length test");

    std::vector<pbnavitia::PathItem> path_items;
    for (int s = 0; s < journey.sections_size(); ++s) {
        for (int i = 0; i < journey.sections(s).street_network().path_items_size(); ++i) {
            path_items.push_back(journey.sections(s).street_network().path_items(i));
        }
    }

    BOOST_REQUIRE_EQUAL(path_items.size(), 3);
    int cpt(0);
    // check on crowfly
    auto pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue bs");
    BOOST_CHECK_CLOSE(pathitem.duration() * speed_factor,
                      to_duration(B.distance_to(S), nt::Mode_e::Walking).total_seconds(),
                      5);  // we are on small distances so rounding is tough
    BOOST_CHECK_CLOSE(pathitem.length(), B.distance_to(S), 2);
    // check on regular street
    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ab");
    BOOST_CHECK_CLOSE(pathitem.duration(), 200 / (speed_factor * double(get_default_speed()[nt::Mode_e::Walking])), 2);
    BOOST_CHECK_CLOSE(pathitem.length(), 200, 2);
    // check again on crowfly
    pathitem = path_items[cpt++];
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ar");
    BOOST_CHECK_CLOSE(pathitem.duration() * speed_factor,
                      to_duration(A.distance_to(R), nt::Mode_e::Walking).total_seconds(), 5);
    BOOST_CHECK_CLOSE(pathitem.length(), A.distance_to(R), 2);

    // we check the total
    double total_dur(0);
    double total_distance(0);
    for (const auto& item : path_items) {
        total_dur += item.duration();
        total_distance += item.length();
    }
    BOOST_CHECK_CLOSE(section.length(), total_distance, 1);
    BOOST_CHECK_CLOSE(section.duration(), total_dur, 1);
}

BOOST_AUTO_TEST_CASE(projection_on_one_way) {
    /*
     *
     * A
     * ^ \
     * |  \
     * |   \<----A->C is a country road, sooooo slow
     * |    \
     * |     v
     * B----->C----------------->D
     *    ^
     *    |
     *  B->C and B->A are hyperloop roads, transport is really fast
     *
     * We want to go from near A to D, we want A->C->D even if A->B->C->D would be faster
     *
     */
    navitia::type::GeographicalCoord A = {0, 100, false};
    navitia::type::GeographicalCoord B = {0, 0, false};
    navitia::type::GeographicalCoord C = {100, 0, false};
    navitia::type::GeographicalCoord D = {200, 0, false};

    size_t AA = 0;
    size_t BB = 1;
    size_t CC = 2;
    size_t DD = 3;

    ed::builder b = {"20120614", [&](ed::builder& b) {
                         boost::add_vertex(ng::Vertex(A), b.data->geo_ref->graph);
                         boost::add_vertex(ng::Vertex(B), b.data->geo_ref->graph);
                         boost::add_vertex(ng::Vertex(C), b.data->geo_ref->graph);
                         boost::add_vertex(ng::Vertex(D), b.data->geo_ref->graph);
                         b.data->geo_ref->init();

                         size_t way_idx = 0;
                         for (const auto& name : {"ab", "bc", "ac", "cd"}) {
                             auto* way = new ng::Way();
                             way->name = "rue " + std::string(name);
                             way->idx = way_idx++;
                             way->way_type = "rue";
                             b.data->geo_ref->ways.push_back(way);
                         }

                         size_t e_idx(0);
                         // we add each edge as a one way street
                         boost::add_edge(BB, AA, ng::Edge(e_idx++, navitia::seconds(10)), b.data->geo_ref->graph);
                         // B->C is very cheap but will not be used
                         boost::add_edge(BB, CC, ng::Edge(e_idx++, navitia::seconds(1)), b.data->geo_ref->graph);
                         boost::add_edge(AA, CC, ng::Edge(e_idx++, navitia::seconds(1000)), b.data->geo_ref->graph);
                         boost::add_edge(CC, DD, ng::Edge(e_idx++, navitia::seconds(10)), b.data->geo_ref->graph);

                         b.data->geo_ref->ways[0]->edges.emplace_back(BB, AA);
                         b.data->geo_ref->ways[1]->edges.emplace_back(BB, CC);
                         b.data->geo_ref->ways[2]->edges.emplace_back(AA, CC);
                         b.data->geo_ref->ways[3]->edges.emplace_back(CC, DD);

                         b.data->build_proximity_list();
                     }};

    b.data->meta->production_date = boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), 7_days);
    // start from A
    auto start = A;
    start.set_lat(A.lat() - 0.000000001);
    std::string origin_uri = str(boost::format("coord:%1%:%2%") % start.lon() % start.lat());
    navitia::type::EntryPoint origin{navitia::type::Type_e::Coord, origin_uri};
    origin.streetnetwork_params.max_duration = bt::pos_infin;

    // to D
    std::string destination_uri = str(boost::format("coord:%1%:%2%") % D.lon() % D.lat());
    navitia::type::EntryPoint destination{navitia::type::Type_e::Coord, destination_uri};
    destination.streetnetwork_params.max_duration = bt::pos_infin;

    // first we want to check that the projection is done on A->B (the whole point of this test)
    auto starting_edge = ng::ProjectionData(start, *b.data->geo_ref);
    BOOST_REQUIRE(starting_edge.found);

    BOOST_CHECK_EQUAL(starting_edge.vertices[ng::ProjectionData::Direction::Target], AA);
    BOOST_CHECK_EQUAL(starting_edge.vertices[ng::ProjectionData::Direction::Source], BB);

    ng::StreetNetwork sn_worker(*b.data->geo_ref);
    nr::RAPTOR raptor(*b.data);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {navitia::test::to_posix_timestamp("20120614T080000")}, true,
                  navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, true);
    auto resp = pb_creator.get_response();

    dump_response(resp, "biking length test");

    BOOST_REQUIRE(resp.journeys_size() == 1);

    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 1);
    pbnavitia::Section section = resp.journeys(0).sections(0);
    BOOST_REQUIRE_EQUAL(section.type(), pbnavitia::SectionType::STREET_NETWORK);

    BOOST_REQUIRE_EQUAL(section.street_network().path_items_size(), 3);

    int cpt_item(0);
    auto pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ab");
    BOOST_CHECK_EQUAL(pathitem.duration(), 0);  // it takes the ab street, but without any duration

    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue ac");
    BOOST_CHECK_EQUAL(pathitem.duration(), 1000);

    pathitem = section.street_network().path_items(cpt_item++);
    BOOST_CHECK_EQUAL(pathitem.name(), "rue cd");
    BOOST_CHECK_EQUAL(pathitem.duration(), 10);
}

BOOST_AUTO_TEST_CASE(use_crow_fly) {
    navitia::type::EntryPoint ep;
    navitia::type::StopPoint sp;
    navitia::type::StopArea sa;
    auto* admin = new ng::Admin();
    sp.stop_area = &sa;
    sa.admin_list.push_back(admin);

    sa.uri = "sa:foo";
    admin->uri = "admin";
    navitia::type::Data data;
    data.geo_ref->admins.push_back(admin);
    data.geo_ref->admin_map[admin->uri] = 0;

    navitia::georef::Path empty_sn_path;
    navitia::georef::Path filled_sn_path;
    filled_sn_path.path_items.push_back({});

    ep.type = navitia::type::Type_e::Address;
    ep.uri = "foo";
    BOOST_CHECK(nr::use_crow_fly(ep, sp, empty_sn_path, data));
    BOOST_CHECK(!nr::use_crow_fly(ep, sp, filled_sn_path, data));

    ep.type = navitia::type::Type_e::POI;
    BOOST_CHECK(nr::use_crow_fly(ep, sp, empty_sn_path, data));
    BOOST_CHECK(!nr::use_crow_fly(ep, sp, filled_sn_path, data));

    ep.type = navitia::type::Type_e::Coord;
    BOOST_CHECK(nr::use_crow_fly(ep, sp, empty_sn_path, data));
    BOOST_CHECK(!nr::use_crow_fly(ep, sp, filled_sn_path, data));

    ep.type = navitia::type::Type_e::StopArea;
    BOOST_CHECK(nr::use_crow_fly(ep, sp, empty_sn_path, data));
    BOOST_CHECK(!nr::use_crow_fly(ep, sp, filled_sn_path, data));

    ep.type = navitia::type::Type_e::Admin;
    BOOST_CHECK(nr::use_crow_fly(ep, sp, empty_sn_path, data));
    BOOST_CHECK(!nr::use_crow_fly(ep, sp, filled_sn_path, data));

    ep.type = navitia::type::Type_e::Admin;
    ep.uri = "admin";
    BOOST_CHECK(nr::use_crow_fly(ep, sp, empty_sn_path, data));
    BOOST_CHECK(!nr::use_crow_fly(ep, sp, filled_sn_path, data));

    ep.type = navitia::type::Type_e::StopArea;
    ep.uri = "sa:foo";
    BOOST_CHECK(nr::use_crow_fly(ep, sp, empty_sn_path, data));
    BOOST_CHECK(nr::use_crow_fly(ep, sp, filled_sn_path, data));

    ep.type = navitia::type::Type_e::Admin;
    ep.uri = "admin";
    navitia::type::StopPoint sp2;
    navitia::type::StopArea sa2;
    sp2.stop_area = &sa2;
    BOOST_CHECK(nr::use_crow_fly(ep, sp2, empty_sn_path, data));
    BOOST_CHECK(!nr::use_crow_fly(ep, sp2, filled_sn_path, data));

    admin->main_stop_areas.push_back(&sa2);
    BOOST_CHECK(nr::use_crow_fly(ep, sp2, empty_sn_path, data));
    BOOST_CHECK(nr::use_crow_fly(ep, sp2, filled_sn_path, data));
}

struct isochrone_fixture {
    isochrone_fixture() : b("20150614") {
        b.vj("l1")("A", "8:25"_t)("B", "8:35"_t);
        b.vj("l3")("A", "8:26"_t)("B", "8:45"_t);
        b.vj("l4")("A", "8:27"_t)("C", "9:35"_t);
        b.vj("l5")("A", "8:28"_t)("C", "9:50"_t);
        b.vj("l6")("C", "8:29"_t)("B", "10:50"_t);

        b.make();
    }

    ed::builder b;
};

/**
 * classic isochrone from A, we should find 2 journeys
 */
BOOST_FIXTURE_TEST_CASE(isochrone, isochrone_fixture) {
    nr::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    navitia::type::EntryPoint ep{navitia::type::Type_e::StopPoint, "A"};
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    nr::make_isochrone(pb_creator, raptor, ep, "20150615T082000"_pts, true, {}, {}, {}, sn_worker, nt::RTLevel::Base,
                       3 * 60 * 60);
    auto result = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(result.journeys_size(), 2);

    std::cout << "1/ dep " << navitia::from_posix_timestamp(result.journeys(0).departure_date_time()) << std::endl;
    std::cout << "1/ arr " << navitia::from_posix_timestamp(result.journeys(0).arrival_date_time()) << std::endl;
    std::cout << "2/ dep " << navitia::from_posix_timestamp(result.journeys(1).departure_date_time()) << std::endl;
    std::cout << "2/ arr " << navitia::from_posix_timestamp(result.journeys(1).arrival_date_time()) << std::endl;

    // Note: since there is no 2nd pass for the isochrone, the departure dt is the requested dt
    BOOST_CHECK_EQUAL(result.journeys(0).departure_date_time(), "20150615T082000"_pts);
    BOOST_CHECK_EQUAL(result.journeys(0).arrival_date_time(), "20150615T083500"_pts);
    BOOST_CHECK_EQUAL(result.journeys(1).departure_date_time(), "20150615T082000"_pts);
    BOOST_CHECK_EQUAL(result.journeys(1).arrival_date_time(), "20150615T093500"_pts);

    {
        // We ask the same request with stop_point == center
        // And check that the response are exactly equivalent
        const navitia::type::EntryPoints stop_points{{ep}};
        navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
        nr::make_isochrone(pb_creator, raptor, ep, "20150615T082000"_pts, true, {}, {}, {}, sn_worker,
                           nt::RTLevel::Base, 3 * 60 * 60, std::numeric_limits<uint32_t>::max(), stop_points);
        BOOST_CHECK_EQUAL(result.DebugString(), pb_creator.get_response().DebugString());
    }
    {
        // We ask the same request with different stop_points
        // And check that the responses are not the same
        const navitia::type::EntryPoints stop_points{{navitia::type::Type_e::StopPoint, "B"}};
        navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
        nr::make_isochrone(pb_creator, raptor, ep, "20150615T082000"_pts, true, {}, {}, {}, sn_worker,
                           nt::RTLevel::Base, 3 * 60 * 60, std::numeric_limits<uint32_t>::max(), stop_points);
        BOOST_CHECK_NE(result.DebugString(), pb_creator.get_response().DebugString());
    }
}

/**
 * reverse isochrone test, we want to arrive in B before 11:00,
 * we should find 2 journeys
 */
BOOST_FIXTURE_TEST_CASE(reverse_isochrone, isochrone_fixture) {
    nr::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    navitia::type::EntryPoint ep{navitia::type::Type_e::StopPoint, "B"};
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    nr::make_isochrone(pb_creator, raptor, ep, "20150615T110000"_pts, false, {}, {}, {}, sn_worker, nt::RTLevel::Base,
                       3 * 60 * 60);
    auto result = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(result.journeys_size(), 2);

    BOOST_CHECK_EQUAL(result.journeys(0).departure_date_time(), "20150615T082900"_pts);
    BOOST_CHECK_EQUAL(result.journeys(0).arrival_date_time(), "20150615T110000"_pts);
    BOOST_CHECK_EQUAL(result.journeys(1).departure_date_time(), "20150615T082600"_pts);
    BOOST_CHECK_EQUAL(result.journeys(1).arrival_date_time(), "20150615T110000"_pts);

    {
        // We ask the same request with stop_point == center
        // And check that the responses are exactly equivalent
        const navitia::type::EntryPoints stop_points{{ep}};
        navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
        nr::make_isochrone(pb_creator, raptor, ep, "20150615T110000"_pts, false, {}, {}, {}, sn_worker,
                           nt::RTLevel::Base, 3 * 60 * 60, std::numeric_limits<uint32_t>::max(), stop_points);
        BOOST_CHECK_EQUAL(result.DebugString(), pb_creator.get_response().DebugString());
    }

    {
        // We ask the same request with different stop_points
        // And check that the responses are not the same
        const navitia::type::EntryPoints stop_points{{navitia::type::Type_e::StopPoint, "C"}};
        navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
        nr::make_isochrone(pb_creator, raptor, ep, "20150615T110000"_pts, false, {}, {}, {}, sn_worker,
                           nt::RTLevel::Base, 3 * 60 * 60, std::numeric_limits<uint32_t>::max(), stop_points);
        BOOST_CHECK_NE(result.DebugString(), pb_creator.get_response().DebugString());
    }
}

/**
 * only one hour from A, we cannot go to C
 */
BOOST_FIXTURE_TEST_CASE(isochrone_duration_limit, isochrone_fixture) {
    nr::RAPTOR raptor(*(b.data));
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    navitia::type::EntryPoint ep{navitia::type::Type_e::StopPoint, "A"};
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    nr::make_isochrone(pb_creator, raptor, ep, "20150615T082000"_pts, true, {}, {}, {}, sn_worker, nt::RTLevel::Base,
                       1 * 60 * 60);
    auto result = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(result.journeys_size(), 1);

    BOOST_CHECK_EQUAL(result.journeys(0).departure_date_time(), "20150615T082000"_pts);
    BOOST_CHECK_EQUAL(result.journeys(0).arrival_date_time(), "20150615T083500"_pts);
}

// test with disruption active
// we add 2 disruptions, and we check that the status of the journey is correct
BOOST_AUTO_TEST_CASE(with_information_disruptions) {
    ed::builder b("20150314");
    b.vj("l")("A", 8 * 3600 + 25 * 60)("B", 8 * 3600 + 35 * 60);

    b.make();
    std::set<ChannelType> channel_types;

    auto default_date = "20150314T000000"_dt;
    auto default_period = boost::posix_time::time_period(default_date, "20500317T000000"_dt);

    b.impact(nt::RTLevel::Adapted)
        .uri("too_bad1")
        .publish(default_period)
        .application_periods(default_period)
        .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
        .on(nt::Type_e::StopArea, "A", *b.data->pt_data)
        .msg("no luck");

    b.impact(nt::RTLevel::Adapted)
        .uri("too_bad2")
        .publish(default_period)
        .application_periods(default_period)
        .severity(nt::disruption::Effect::DETOUR)
        .on(nt::Type_e::Line, "l", *b.data->pt_data)
        .msg("no luck");

    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("A");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("B");
    navitia::type::EntryPoint origin(origin_type, "A");
    navitia::type::EntryPoint destination(destination_type, "B");

    ng::StreetNetwork sn_worker(*b.data->geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, "20150315T080000"_dt, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20150315T080000")}, true,
                  nt::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    const auto& j = resp.journeys(0);
    BOOST_CHECK_EQUAL(j.most_serious_disruption_effect(), "SIGNIFICANT_DELAYS");

    BOOST_REQUIRE_EQUAL(j.sections_size(), 1);
}

// test with network disruption too
// we add 2 disruptions, and we check that the status of the journey is correct
BOOST_AUTO_TEST_CASE(with_disruptions_on_network) {
    ed::builder b("20150314");
    b.vj("l")("A", 8 * 3600 + 25 * 60)("B", 8 * 3600 + 35 * 60);

    b.make();

    auto default_date = "20150314T000000"_dt;
    auto default_period = boost::posix_time::time_period(default_date, "20500317T000000"_dt);

    b.impact(nt::RTLevel::Adapted)
        .uri("too_bad")
        .publish(default_period)
        .application_periods(default_period)
        .severity(nt::disruption::Effect::SIGNIFICANT_DELAYS)
        .on(nt::Type_e::StopArea, "A", *b.data->pt_data)
        .msg("no luck");

    b.impact(nt::RTLevel::Adapted)
        .uri("detour")
        .publish(default_period)
        .application_periods(default_period)
        .severity(nt::disruption::Effect::DETOUR)
        .on(nt::Type_e::Network, "base_network", *b.data->pt_data);

    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("A");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("B");
    navitia::type::EntryPoint origin(origin_type, "A");
    navitia::type::EntryPoint destination(destination_type, "B");
    ng::StreetNetwork sn_worker(*b.data->geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, "20150314T080000"_dt, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20150314T080000")}, true,
                  navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    const auto& j = resp.journeys(0);
    BOOST_CHECK_EQUAL(j.most_serious_disruption_effect(),
                      "SIGNIFICANT_DELAYS");  // we should have the network's disruption's effect
}

BOOST_AUTO_TEST_CASE(journey_with_forbidden) {
    std::vector<std::string> forbidden;
    ed::builder b("20120614");
    b.vj("A")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 8 * 3600 + 20 * 60,
                                                                         8 * 3600 + 21 * 60);
    b.vj("B")("stop_area:stop1", 8 * 3600 + 10 * 60, 8 * 3600 + 11 * 60)("stop_area:stop2", 10 * 3600 + 20 * 60,
                                                                         10 * 3600 + 21 * 60);
    navitia::type::Data data;
    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2012, 06, 14), boost::gregorian::days(7));
    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop_area:stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop_area:stop2");
    navitia::type::EntryPoint origin(origin_type, "stop_area:stop1");
    navitia::type::EntryPoint destination(destination_type, "stop_area:stop2");

    ng::StreetNetwork sn_worker(*data.geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T021000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 3);
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections(1).pt_display_informations().uris().line(), "A");

    forbidden.emplace_back("A");
    pb_creator.init(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20120614T021000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 3);
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections(1).pt_display_informations().uris().line(), "B");
}

// We want to go from S to R, but we can only take the bus from A to
// B.  As doing S->A->B->R is worst that just S->R (more walking, more
// pt, worst arrival), we want that the direct path filter the pt
// solution.
BOOST_FIXTURE_TEST_CASE(direct_path_filtering_test, streetnetworkmode_fixture<test_speed_provider>) {
    datetimes = {navitia::test::to_posix_timestamp("20120614T080000")};

    origin.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    origin.streetnetwork_params.offset = 0;
    origin.streetnetwork_params.max_duration = navitia::seconds(15 * 60);
    origin.streetnetwork_params.speed_factor = 1;

    destination.streetnetwork_params.mode = navitia::type::Mode_e::Walking;
    destination.streetnetwork_params.offset = 0;
    destination.streetnetwork_params.speed_factor = 1;
    destination.streetnetwork_params.max_duration = navitia::seconds(15 * 60);

    forbidden = {"A", "B", "D", "M", "PM"};

    pbnavitia::Response resp = make_response();
    dump_response(resp, "direct_path_filtering");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 1);
    BOOST_CHECK_EQUAL(resp.journeys(0).sections(0).street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    // For a directpath we don't have next_request_date_time
    BOOST_CHECK_EQUAL(resp.has_next_request_date_time(), false);
}

// bus + car using parking
BOOST_FIXTURE_TEST_CASE(direct_path_car, streetnetworkmode_fixture<test_speed_provider>) {
    using namespace navitia;
    using navitia::seconds;
    using type::Mode_e;

    const double speed_factor = 1;

    origin.streetnetwork_params.mode = Mode_e::Car;
    origin.streetnetwork_params.offset = b.data->geo_ref->offsets[Mode_e::Car];
    origin.streetnetwork_params.max_duration = seconds(900);
    origin.streetnetwork_params.speed_factor = speed_factor;

    destination.streetnetwork_params.mode = Mode_e::Walking;
    destination.streetnetwork_params.offset = b.data->geo_ref->offsets[Mode_e::Walking];
    destination.streetnetwork_params.max_duration = seconds(900);
    destination.streetnetwork_params.speed_factor = speed_factor;

    ng::StreetNetwork sn_worker(*this->b.data->geo_ref);
    nr::RAPTOR raptor(*this->b.data);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    nr::make_response(pb_creator, raptor, this->origin, this->destination, this->datetimes, true,
                      navitia::type::AccessibiliteParams(), this->forbidden, {}, sn_worker, type::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    dump_response(resp, "direct_path_car");

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);  // 1 direct car

    // second journey, direct path with a car
    auto journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    auto sections = journey.sections();

    // car to the parking
    BOOST_CHECK_EQUAL(sections.Get(0).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(sections.Get(0).origin().address().label(), "rue bs (Condom)");
    BOOST_CHECK_EQUAL(sections.Get(0).destination().poi().label(), "second parking (Condom)");
    BOOST_CHECK_EQUAL(sections.Get(0).street_network().mode(), pbnavitia::StreetNetworkMode::Car);
    BOOST_CHECK_EQUAL(sections.Get(0).street_network().duration(), 38);

    // parking
    BOOST_CHECK_EQUAL(sections.Get(1).type(), pbnavitia::SectionType::PARK);
    BOOST_CHECK_EQUAL(sections.Get(1).origin().poi().label(), "second parking (Condom)");
    BOOST_CHECK_EQUAL(sections.Get(1).destination().address().label(), "rue ae (Condom)");
    BOOST_CHECK_EQUAL(sections.Get(1).street_network().duration(), 1);

    // walking to the destination
    BOOST_CHECK_EQUAL(sections.Get(2).type(), pbnavitia::SectionType::STREET_NETWORK);
    BOOST_CHECK_EQUAL(sections.Get(2).origin().address().label(), "rue ae (Condom)");
    BOOST_CHECK_EQUAL(sections.Get(2).destination().address().label(), "1 rue ag (Condom)");
    BOOST_CHECK_EQUAL(sections.Get(2).street_network().mode(), pbnavitia::StreetNetworkMode::Walking);
    BOOST_CHECK_EQUAL(sections.Get(2).street_network().duration(), 120);

    origin.streetnetwork_params.enable_direct_path = false;
    // direct path is disabled, we should only found the solution with car as fallback
    pb_creator.init(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    nr::make_response(pb_creator, raptor, this->origin, this->destination, this->datetimes, true,
                      navitia::type::AccessibiliteParams(), this->forbidden, {}, sn_worker, type::RTLevel::Base, 2_min);
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);  // 1 car->bus->walk
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 5);
}

// test with network disruption too
// we add 2 disruptions, and we check that the status of the journey is correct
BOOST_AUTO_TEST_CASE(stop_times_with_distinct_arrival_departure) {
    ed::builder b("20150314");
    b.sa("A")("stop_point:A", 0, 0, false);
    b.sa("B")("stop_point:B", 0, 0, false);
    b.sa("C")("stop_point:C", 0, 0, false);
    b.sa("D")("stop_point:D", 0, 0, false);
    b.vj("l")("stop_point:A", "8:00"_t, "8:05"_t)("stop_point:B", "9:00"_t, "9:05"_t)(
        "stop_point:C", "10:00"_t, "10:05"_t)("stop_point:D", "11:00"_t, "11:05"_t);

    b.make();

    nr::RAPTOR raptor(*b.data);

    navitia::type::Type_e origin_type = b.data->get_type_of_id("B");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("C");
    navitia::type::EntryPoint origin(origin_type, "B");
    navitia::type::EntryPoint destination(destination_type, "C");
    ng::StreetNetwork sn_worker(*b.data->geo_ref);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, "20150314T080000"_dt, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20150314T080000")}, true,
                  navitia::type::AccessibiliteParams(), {}, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    const auto& j = resp.journeys(0);
    BOOST_REQUIRE(j.sections_size() > 0);
    BOOST_CHECK_EQUAL(j.departure_date_time(), j.sections(0).begin_date_time());
    BOOST_CHECK_EQUAL(j.arrival_date_time(), j.sections(j.sections_size() - 1).end_date_time());
    for (int i = 1; i < j.sections_size(); ++i) {
        BOOST_CHECK_EQUAL(j.sections(i - 1).end_date_time(), j.sections(i).begin_date_time());
    }
}

/*
 * Test the section geometry without provided shapes contains
 * only its stop points coordinates.
 *
 *     S2    S3
 *     +-----+
 *    /
 *   /
 *  /
 * /
 * +S1
 */
BOOST_AUTO_TEST_CASE(section_geometry_without_shapes) {
    ed::builder b("20180309");
    b.sa("stop1")("stop_point:stop1", 0, 0, false);
    b.sa("stop2")("stop_point:stop2", 4, 5, false);
    b.sa("stop3")("stop_point:stop3", 10, 5, false);
    b.vj("vj")("stop_point:stop1", 1000, 1100)("stop_point:stop2", 1200, 1500)("stop_point:stop3", 1700, 2000);

    b.make();
    auto prod_date = boost::gregorian::date(2018, 3, 9);
    auto prod_len = boost::gregorian::days(1);
    b.data->meta->production_date = boost::gregorian::date_period(prod_date, prod_len);

    nr::RAPTOR raptor(*(b.data));

    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop3");
    navitia::type::EntryPoint origin(origin_type, "stop1");
    navitia::type::EntryPoint destination(destination_type, "stop3");
    std::vector<std::string> forbidden;
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T001500")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(journey.sections(0).stop_date_times().size(), 0);
    BOOST_REQUIRE_EQUAL(journey.sections(1).stop_date_times().size(), 3);
    BOOST_REQUIRE_EQUAL(journey.sections(2).stop_date_times().size(), 0);

    BOOST_CHECK_EQUAL(journey.sections(1).shape().size(), 3);

    BOOST_CHECK_EQUAL(journey.sections(1).shape(0).lon(), 0);
    BOOST_CHECK_EQUAL(journey.sections(1).shape(0).lat(), 0);

    BOOST_CHECK_EQUAL(journey.sections(1).shape(1).lon(), 4);
    BOOST_CHECK_EQUAL(journey.sections(1).shape(1).lat(), 5);

    BOOST_CHECK_EQUAL(journey.sections(1).shape(2).lon(), 10);
    BOOST_CHECK_EQUAL(journey.sections(1).shape(2).lat(), 5);
}

/*
 * Test the free radius filter with classic journeys
 *
 * We create 4 VJ to test this option (free radius filtering).
 *
 * Case 1 :
 * - Type : No filter radius
 * - Result : We have 4 journeys with 3 sections each
 *            for each journeys
 *            section 1 -> crow fly with real duration
 *            section 2 -> public transport
 *            section 3 -> crow fly with real duration
 *
 * Case 2 :
 * - Type : With a big from filter radius
 * - Result : We have 4 journeys with 3 sections each
 *            for each journeys
 *            section 1 -> crow fly with 0 duration
 *            section 2 -> public transport
 *            section 3 -> crow fly with real duration
 *
 * Case 3 :
 * - Type : With a half from filter radius
 * - Result : We have 4 journeys with 3 sections each
 *            * jouneys 1
 *            section 1 -> crow fly with 0 duration
 *            section 2 -> public transport
 *            section 3 -> crow fly with real duration
 *           * jouneys 2
 *            section 1 -> crow fly with 0 duration
 *            section 2 -> public transport
 *            section 3 -> crow fly with real duration
 *           * jouneys 3
 *            section 1 -> crow fly with real duration
 *            section 2 -> public transport
 *            section 3 -> crow fly with real duration
 *            * jouneys 4
 *            section 1 -> crow fly with real duration
 *            section 2 -> public transport
 *            section 3 -> crow fly with real duration
 *
 *
 *
 *                       SA1                          SA2
 *             |-------------------------|---------------------------|
 *             |                         |                           |
 *             |                         |                           |
 * x from      | S1             S2       |       S1             S2   |
 *   ------------  -------------  --------------- --------------     |
 *       10 m             10 m            10 m           10 m        |
 *             |                         |                           |
 *             |                         |                           |
 *             |                         |                           |
 *             |-------------------------|---------------------------|
 *
 *
 *
 *
 *                                    SA3
 *             |--------------------------------------------------|
 *             |                                                  |
 *             |                                                  |
 *             |                                                  |
 * x to        |  S1          S2           S3          S4         |
 *   -------------  ----------  -----------  -----------          |
 *        100 m        100 m       100 m       100 m              |
 *             |                                                  |
 *             |                                                  |
 *             |                                                  |
 *             |--------------------------------------------------|
 *
 */
BOOST_AUTO_TEST_CASE(journeys_with_free_radius_filter) {
    // Create Builder
    ed::builder b("20180309");

    // Create Area
    b.sa("stop_area:sa1")("stop_point:sa1:s1", 2.39592, 48.84848, false)("stop_point:sa1:s2", 2.39592, 48.84858, false);
    b.sa("stop_area:sa2")("stop_point:sa2:s1", 2.39592, 48.84868, false)("stop_point:sa2:s2", 2.39592, 48.84878, false);
    b.sa("stop_area:sa3")("stop_point:sa3:s1", 2.36381, 48.86650, false)("stop_point:sa3:s2", 2.36381, 48.86550, false)(
        "stop_point:sa3:s3", 2.36381, 48.86450, false)("stop_point:sa3:s4", 2.36381, 48.86350, false);
    b.vj("vj1")("stop_point:sa1:s1", "8:00"_t, "8:01"_t)("stop_point:sa3:s1", "8:20"_t, "8:11"_t);
    b.vj("vj2")("stop_point:sa1:s2", "8:00"_t, "8:01"_t)("stop_point:sa3:s2", "8:15"_t, "8:11"_t);
    b.vj("vj3")("stop_point:sa2:s1", "8:00"_t, "8:01"_t)("stop_point:sa3:s3", "8:10"_t, "8:11"_t);
    b.vj("vj4")("stop_point:sa2:s2", "8:00"_t, "8:01"_t)("stop_point:sa3:s4", "8:05"_t, "8:11"_t);

    b.make();
    b.data->build_proximity_list();
    auto prod_date = boost::gregorian::date(2018, 3, 9);
    auto prod_len = boost::gregorian::days(1);
    b.data->meta->production_date = boost::gregorian::date_period(prod_date, prod_len);

    // create Raptor
    nr::RAPTOR raptor(*(b.data));

    // from (nation)
    navitia::type::EntryPoint origin(navitia::type::Type_e::Coord, "from_coord");
    origin.streetnetwork_params.max_duration = navitia::time_duration(boost::date_time::pos_infin);
    origin.coordinates.set_lon(2.39592);
    origin.coordinates.set_lat(48.84838);

    // to (republique)
    navitia::type::EntryPoint destination(navitia::type::Type_e::Coord, "to_coord");
    destination.streetnetwork_params.max_duration = navitia::time_duration(boost::date_time::pos_infin);
    destination.coordinates.set_lon(2.36381);
    destination.coordinates.set_lat(48.86750);

    std::vector<std::string> forbidden;
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    //-----------------------------------
    // Case 1 : No filter radius

    // free radius to test
    uint32_t free_radius_from = 0;
    uint32_t free_radius_to = 0;

    // send request
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator1(data_ptr, "20180309T080100"_dt, null_time_period);
    make_response(pb_creator1, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T080000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, 8640,
                  10, 0, free_radius_from, free_radius_to);

    // get the response
    pbnavitia::Response resp = pb_creator1.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);

    auto journeys = sort_journeys(resp);

    // Journey 1
    pbnavitia::Journey journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    pbnavitia::Section section = journey.sections(0);
    BOOST_CHECK(section.duration() > 0);

    // Journey 2
    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK(section.duration() > 0);

    // Journey 3
    journey = journeys[2];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK(section.duration() > 0);

    // Journey 4
    journey = journeys[3];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK(section.duration() > 0);

    //-----------------------------------
    // Case 2 : With a big "from filter radius"

    // free radius to test
    free_radius_from = 200;  // 200 meters
    free_radius_to = 0;

    // send request
    navitia::PbCreator pb_creator2(data_ptr, "20180309T080100"_dt, null_time_period);
    make_response(pb_creator2, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T080000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, 8640,
                  10, 0, free_radius_from, free_radius_to);

    // get the response
    resp = pb_creator2.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);

    journeys = sort_journeys(resp);

    // Journey 1
    journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.duration(), 0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::CROW_FLY);

    // Journey 2
    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.duration(), 0);

    // Journey 3
    journey = journeys[2];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.duration(), 0);

    // Journey 4
    journey = journeys[3];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.duration(), 0);

    //-----------------------------------
    // Case 3 : With a half "from filter radius"

    // free radius to test
    free_radius_from = 30;  // 30 meters just to catch sa1:s1 and sa1:s2
    free_radius_to = 0;

    // send request
    navitia::PbCreator pb_creator3(data_ptr, "20180309T080100"_dt, null_time_period);
    make_response(pb_creator3, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T080000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, 8640,
                  10, 0, free_radius_from, free_radius_to);

    // get the response
    resp = pb_creator3.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);

    journeys = sort_journeys(resp);

    // Journey 1
    journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.duration(), 55);

    // Journey 2
    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.duration(), 41);

    // Journey 3
    journey = journeys[2];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.duration(), 0);

    // Journey 4
    journey = journeys[3];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.duration(), 0);
}

/*
 * Test the section geometry with provided shapes has
 * something different from the initial stop points
 * coordinates (as the shape is made of coordinates
 * which do no match exactly with the stop points).
 *
 * B    S2 C S3
 * +----+--+-+
 * |   /
 * |  /
 * | /
 * |/
 * +
 * AS1
 */
BOOST_AUTO_TEST_CASE(section_geometry_with_shapes) {
    ed::builder b("20180309");
    b.sa("stop1")("stop_point:stop1", 0, 0, false);
    b.sa("stop2")("stop_point:stop2", 5, 5, false);
    b.sa("stop3")("stop_point:stop3", 10, 5, false);

    static const navitia::type::GeographicalCoord S1(0, 0);
    static const navitia::type::GeographicalCoord S2(4, 5);
    static const navitia::type::GeographicalCoord S3(10, 5);

    static const navitia::type::GeographicalCoord A(0, 0);
    static const navitia::type::GeographicalCoord B(0, 5);
    static const navitia::type::GeographicalCoord C(8, 5);

    navitia::type::LineString shape = {A, B};
    static const navitia::type::LineString shape_S2 = ed::create_shape(S1, S2, shape);
    shape = {B, C};
    static const navitia::type::LineString shape_S3 = ed::create_shape(S2, S3, shape);

    b.vj("vj")("stop_point:stop1", 1000, 1100)("stop_point:stop2", 1200, 1500)
        .st_shape(shape_S2)("stop_point:stop3", 1700, 2000)
        .st_shape(shape_S3);

    b.make();
    b.data->meta->production_date =
        boost::gregorian::date_period(boost::gregorian::date(2018, 3, 9), boost::gregorian::days(1));

    nr::RAPTOR raptor(*(b.data));

    navitia::type::Type_e origin_type = b.data->get_type_of_id("stop1");
    navitia::type::Type_e destination_type = b.data->get_type_of_id("stop3");
    navitia::type::EntryPoint origin(origin_type, "stop1");
    navitia::type::EntryPoint destination(destination_type, "stop3");
    std::vector<std::string> forbidden;
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, boost::gregorian::not_a_date_time, null_time_period);
    make_response(pb_creator, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T001500")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min);
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    pbnavitia::Journey journey = resp.journeys(0);

    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    pbnavitia::Section section = journey.sections(0);

    BOOST_REQUIRE_EQUAL(journey.sections(0).stop_date_times().size(), 0);
    BOOST_REQUIRE_EQUAL(journey.sections(1).stop_date_times().size(), 3);
    BOOST_REQUIRE_EQUAL(journey.sections(2).stop_date_times().size(), 0);

    BOOST_CHECK_EQUAL(journey.sections(1).shape().size(), 5);

    BOOST_CHECK_EQUAL(journey.sections(1).shape(0).lon(), 0);
    BOOST_CHECK_EQUAL(journey.sections(1).shape(0).lat(), 0);

    BOOST_CHECK_EQUAL(journey.sections(1).shape(1).lon(), 0);
    BOOST_CHECK_EQUAL(journey.sections(1).shape(1).lat(), 5);

    BOOST_CHECK_EQUAL(journey.sections(1).shape(2).lon(), 4);
    BOOST_CHECK_EQUAL(journey.sections(1).shape(2).lat(), 5);

    BOOST_CHECK_EQUAL(journey.sections(1).shape(3).lon(), 8);
    BOOST_CHECK_EQUAL(journey.sections(1).shape(3).lat(), 5);

    BOOST_CHECK_EQUAL(journey.sections(1).shape(4).lon(), 10);
    BOOST_CHECK_EQUAL(journey.sections(1).shape(4).lat(), 5);
}

/**
 * @brief This test aims to highlight the min_nb_journeys option on a journey request.
 *
 * min_nb_journeys allows to ask differents same journeys.
 * The data for testing :
 *
 * start : sp1
 * stop  : sp2
 *
 * Vehicle journey with multiple stops times
 * date : 2018 03 09
 *
 *           sp1                   sp2
 *            x---------------------x
 *         - 8:01                - 8:10
 *         - 8:03                - 8:12
 *         - 8:05                - 8:14
 *         - 8:07                - 8:16
 *
 * We create several requests :
 *
 * Case 1 : Simple request without min_nb_journeys
 *          We must have only one journeys
 * Case 2 : Request with min_nb_journeys = 4 and clockwise = true
 *          We must have all journeys
 * Case 3 : Request with min_nb_journeys = 4 and clockwise = false
 *          We must have all journeys (reverse)
 */
BOOST_AUTO_TEST_CASE(journeys_with_min_nb_journeys) {
    // Create Builder
    ed::builder b("20180309");

    // Create Area
    b.sa("stop_area:sa1")("stop_point:sa1:sp1", 2.39592, 48.84848, false);
    b.sa("stop_area:sa2")("stop_point:sa2:sp2", 2.36381, 48.86650, false);
    b.vj("vj1")("stop_point:sa1:sp1", "8:00"_t, "8:01"_t)("stop_point:sa2:sp2", "8:10"_t, "8:11"_t);
    b.vj("vj2")("stop_point:sa1:sp1", "8:02"_t, "8:03"_t)("stop_point:sa2:sp2", "8:12"_t, "8:13"_t);
    b.vj("vj3")("stop_point:sa1:sp1", "8:04"_t, "8:05"_t)("stop_point:sa2:sp2", "8:14"_t, "8:15"_t);
    b.vj("vj4")("stop_point:sa1:sp1", "8:06"_t, "8:07"_t)("stop_point:sa2:sp2", "8:16"_t, "8:17"_t);

    b.make();
    b.data->build_proximity_list();
    auto prod_date = boost::gregorian::date(2018, 3, 9);
    auto prod_len = boost::gregorian::days(1);
    b.data->meta->production_date = boost::gregorian::date_period(prod_date, prod_len);

    // create Raptor
    nr::RAPTOR raptor(*(b.data));

    // from (nation)
    navitia::type::EntryPoint origin(navitia::type::Type_e::StopPoint, "stop_point:sa1:sp1");

    // to (republique)
    navitia::type::EntryPoint destination(navitia::type::Type_e::StopPoint, "stop_point:sa2:sp2");

    std::vector<std::string> forbidden;
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    //-----------------------------------
    // Case 1 : Simple request without multi journeys

    // send request
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator1(data_ptr, "20180309T080100"_dt, null_time_period);
    make_response(pb_creator1, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T080000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, 8640,
                  10, 0);

    // get the response
    pbnavitia::Response resp = pb_creator1.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);

    // Journey
    pbnavitia::Journey journey = resp.journeys(0);
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    pbnavitia::Section section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080100"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081000"_pts);

    //-----------------------------------
    // Case 2 : Request with min_nb_journeys = 4, with clockwise = true

    uint32_t min_nb_journeys = 4;

    // send request
    navitia::PbCreator pb_creator2(data_ptr, "20180309T080000"_dt, null_time_period);
    make_response(pb_creator2, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T080000")}, true,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, 8640,
                  10, 0, 0, 0, min_nb_journeys);

    // get the response
    resp = pb_creator2.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);

    auto journeys = sort_journeys(resp);

    // Journey 1
    journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080100"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081000"_pts);

    // Journey 2
    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080300"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081200"_pts);

    // Journey 3
    journey = journeys[2];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080500"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081400"_pts);

    // Journey 4
    journey = journeys[3];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080700"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081600"_pts);

    //-----------------------------------
    // Case 3 : Request with min_nb_journeys = 4, with clockwise = false

    min_nb_journeys = 4;

    // send request
    navitia::PbCreator pb_creator3(data_ptr, "20180309T083000"_dt, null_time_period);
    make_response(pb_creator3, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T083000")}, false,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, 8640,
                  10, 0, 0, 0, min_nb_journeys);

    // get the response
    resp = pb_creator3.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);

    journeys = sort_journeys(resp);

    // Journey 1
    journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080100"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081000"_pts);

    // Journey 2
    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080300"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081200"_pts);

    // Journey 3
    journey = journeys[2];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080500"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081400"_pts);

    // Journey 4
    journey = journeys[3];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:sp1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa2:sp2");
    BOOST_CHECK_EQUAL(journey.departure_date_time(), "20180309T080700"_pts);
    BOOST_CHECK_EQUAL(journey.arrival_date_time(), "20180309T081600"_pts);
}

/**
 * @brief This test aims to highlight the min_nb_journeys option on a journey request
 * with similar journeys filtering
 *
 * min_nb_journeys allows to ask differents same journeys.
 *
 * The data for testing :
 *
 * Vehicle journey with multiple stops times
 * date : 2018 03 09
 *
 *                   sp1                   sp2
 *            /-------x---------------------x--------\
 *           /      - 8:01                - 8:20      \
 *          /       - 8:02                - 8:21       \
 *   Start x        - 8:03                - 8:22        x Stop
 *         |        - 8:04                - 8:23        |
 *         |                                            |
 *         |          We add an other way to test       |
 *         \         similar journeys filtering         /
 *          \        (Faster but with more walking)    /
 *           \       sp1                   sp2        /
 *            \-------x---------------------x--------/
 *                 - 8:05                - 8:15
 *
 *
 * We create several requests :
 *
 * Case 1 : Request with min_nb_journeys = 4 and clockwise = true
 *          We must have all journeys
 * Case 2 : Request with min_nb_journeys = 4 and clockwise = false
 *          We must have all journeys (reverse)
 */
BOOST_AUTO_TEST_CASE(journeys_with_min_nb_journeys_with_similar_journeys_filtering) {
    // Create Builder
    ed::builder b("20180309");

    // Create Area
    b.sa("stop_area:sa1")("stop_point:sa1:s1", 2.39592, 48.84848, false)("stop_point:sa1:s2", 2.39592, 48.84858, false);
    b.sa("stop_area:sa3")("stop_point:sa3:s1", 2.36381, 48.86650, false)("stop_point:sa3:s2", 2.36381, 48.86550, false);

    // jouney 1
    b.vj("vj1")("stop_point:sa1:s1", "8:00"_t, "8:01"_t)("stop_point:sa3:s1", "8:20"_t, "8:21"_t);
    b.vj("vj2")("stop_point:sa1:s1", "8:00"_t, "8:02"_t)("stop_point:sa3:s1", "8:21"_t, "8:22"_t);
    b.vj("vj3")("stop_point:sa1:s1", "8:00"_t, "8:03"_t)("stop_point:sa3:s1", "8:22"_t, "8:23"_t);
    b.vj("vj4")("stop_point:sa1:s1", "8:00"_t, "8:04"_t)("stop_point:sa3:s1", "8:23"_t, "8:24"_t);

    // journey 2
    b.vj("vj5")("stop_point:sa1:s2", "8:00"_t, "8:05"_t)("stop_point:sa3:s2", "8:25"_t, "8:26"_t);

    b.make();
    b.data->build_proximity_list();
    auto prod_date = boost::gregorian::date(2018, 3, 9);
    auto prod_len = boost::gregorian::days(1);
    b.data->meta->production_date = boost::gregorian::date_period(prod_date, prod_len);

    // create Raptor
    nr::RAPTOR raptor(*(b.data));

    // from (nation)
    navitia::type::EntryPoint origin(navitia::type::Type_e::Coord, "from_coord");
    origin.streetnetwork_params.max_duration = navitia::time_duration(boost::date_time::pos_infin);
    origin.coordinates.set_lon(2.39592);
    origin.coordinates.set_lat(48.84838);

    // to (republique)
    navitia::type::EntryPoint destination(navitia::type::Type_e::Coord, "to_coord");
    destination.streetnetwork_params.max_duration = navitia::time_duration(boost::date_time::pos_infin);
    destination.coordinates.set_lon(2.36381);
    destination.coordinates.set_lat(48.86750);

    std::vector<std::string> forbidden;
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    //-----------------------------------
    // Case 1 : Request with min_nb_journeys = 4, with clockwise = true

    uint32_t min_nb_journeys = 4;
    bool clockwise = true;
    auto* data_ptr = b.data.get();

    // send request
    navitia::PbCreator pb_creator2(data_ptr, "20180309T080000"_dt, null_time_period);
    make_response(pb_creator2, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T080000")}, clockwise,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, 8640,
                  10, 0, 0, 0, min_nb_journeys);

    // get the response
    auto resp = pb_creator2.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);

    auto journeys = sort_journeys_by(resp, JourneySectionCompare());

    // Journey 1
    auto journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    auto section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:s1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa3:s1");
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080100"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082000"_pts);

    // Journey 2
    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:s1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa3:s1");
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080200"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082100"_pts);

    // Journey 3
    journey = journeys[2];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:s1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa3:s1");
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080300"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082200"_pts);

    // Journey 4
    journey = journeys[3];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:s1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa3:s1");
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080400"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082300"_pts);

    //-----------------------------------
    // Case 2 : Request with min_nb_journeys = 4, with clockwise = false

    min_nb_journeys = 4;
    clockwise = false;

    // send request
    navitia::PbCreator pb_creator3(data_ptr, "20180309T083000"_dt, null_time_period);
    make_response(pb_creator3, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T083000")}, clockwise,
                  navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min, 2_min, 8640,
                  10, 0, 0, 0, min_nb_journeys);

    // get the response
    resp = pb_creator3.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 4);

    journeys = sort_journeys_by(resp, JourneySectionCompare());

    // Journey 1
    journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:s1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa3:s1");
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080200"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082100"_pts);

    // Journey 2
    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:s1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa3:s1");
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080300"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082200"_pts);

    // Journey 3
    journey = journeys[2];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:s1");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa3:s1");
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080400"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082300"_pts);

    // Journey 4
    journey = journeys[3];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 3);
    section = journey.sections(1);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.origin().stop_point().name(), "stop_point:sa1:s2");
    BOOST_CHECK_EQUAL(section.destination().stop_point().name(), "stop_point:sa3:s2");
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080500"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082500"_pts);
}

namespace {

nr::NightBusFilter::Params get_default_filter_params() {
    return nr::NightBusFilter::Params{navitia::DateTimeUtils::set(0, "08:00"_t), true,
                                      nr::NightBusFilter::default_max_factor, nr::NightBusFilter::default_base_factor};
}

std::tuple<nr::Journey, nr::Journey, nr::Journey> build_night_bus_journeys(ed::builder& b) {
    b.vj("vj0")("A0", "08:55"_t, "08:55"_t)("C1", "23:30"_t, "23:30"_t);  // sooner departure, but worst journey
    b.vj("vj1")("A1", "09:00"_t, "09:00"_t)("C1", "12:00"_t, "12:00"_t);  // best journey clockwise: based on arrival
    b.vj("vj2")("B1", "22:00"_t, "22:00"_t)("C1", "23:00"_t, "23:00"_t);

    nt::PT_Data& d = *b.data->pt_data;
    const auto& stl0 = d.vehicle_journeys[0]->stop_time_list;
    const auto& stl1 = d.vehicle_journeys[1]->stop_time_list;
    const auto& stl2 = d.vehicle_journeys[2]->stop_time_list;

    nr::Journey j0;
    j0.sections.emplace_back(stl0[0], navitia::DateTimeUtils::set(0, "08:55"_t), stl0[1],
                             navitia::DateTimeUtils::set(0, "23:30"_t));
    j0.departure_dt = navitia::DateTimeUtils::set(0, "08:15"_t);
    j0.arrival_dt = navitia::DateTimeUtils::set(0, "23:45"_t);

    nr::Journey j1;
    j1.sections.emplace_back(stl1[0], navitia::DateTimeUtils::set(0, "09:00"_t), stl1[1],
                             navitia::DateTimeUtils::set(0, "10:00"_t));
    j1.departure_dt = navitia::DateTimeUtils::set(0, "08:30"_t);
    j1.arrival_dt = navitia::DateTimeUtils::set(0, "10:30"_t);

    nr::Journey j2;
    j2.sections.emplace_back(stl2[0], navitia::DateTimeUtils::set(0, "22:00"_t), stl2[1],
                             navitia::DateTimeUtils::set(0, "23:00"_t));
    j2.departure_dt = navitia::DateTimeUtils::set(0, "21:55"_t);
    j2.arrival_dt = navitia::DateTimeUtils::set(0, "23:10"_t);

    return std::make_tuple(j0, j1, j2);
}

class Night_bus_fixture {
private:
    ed::builder b;

public:
    nr::NightBusFilter::Params filter_params;
    nr::Journey j0, j1, j2;
    Night_bus_fixture() : b("20120614") {
        filter_params = get_default_filter_params();
        std::tie(j0, j1, j2) = build_night_bus_journeys(b);
    }
};
}  // namespace

BOOST_FIXTURE_TEST_CASE(night_bus_should_be_treated_as_way_later, Night_bus_fixture) {
    BOOST_CHECK_EQUAL(nr::is_way_later(j1, j2, filter_params), false);
    BOOST_CHECK_EQUAL(nr::is_way_later(j2, j1, filter_params), true);
}

BOOST_FIXTURE_TEST_CASE(night_bus_should_treated_as_way_later_anti_clockwise, Night_bus_fixture) {
    filter_params.requested_datetime = navitia::DateTimeUtils::set(0, "23:30"_t);
    filter_params.clockwise = false;

    BOOST_CHECK_EQUAL(nr::is_way_later(j1, j2, filter_params), true);
    BOOST_CHECK_EQUAL(nr::is_way_later(j2, j1, filter_params), false);
}

BOOST_FIXTURE_TEST_CASE(night_bus_filter_should_be_order_agnostic, Night_bus_fixture) {
    {
        nr::RAPTOR::Journeys journeys = {j0, j1, j2};
        nr::filter_late_journeys(journeys, filter_params);

        BOOST_REQUIRE_EQUAL(journeys.size(), 1);

        auto vj = journeys.begin()->sections[0].get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, "vehicle_journey:vj1:1");
    }
    {
        nr::RAPTOR::Journeys journeys = {j2, j1, j0};
        nr::filter_late_journeys(journeys, filter_params);

        BOOST_REQUIRE_EQUAL(journeys.size(), 1);

        auto vj = journeys.begin()->sections[0].get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, "vehicle_journey:vj1:1");
    }
}

BOOST_FIXTURE_TEST_CASE(night_bus_filter_change_parameters, Night_bus_fixture) {
    // With the default parameters, the request departure is 08:00
    // The max_pseudo_duration is 7h, so journeys departing after 15:00 are deleted
    nr::NightBusFilter::Params filter_params = get_default_filter_params();
    {
        nr::RAPTOR::Journeys journeys = {j0, j1, j2};
        nr::filter_late_journeys(journeys, filter_params);

        BOOST_REQUIRE_EQUAL(journeys.size(), 1);

        auto vj = journeys.begin()->sections[0].get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, "vehicle_journey:vj1:1");
    }

    // With the custom parameters, the max_pseudo_duration is 15h
    // The second vj departing at 22:00 is now kept with the request departure at 08:00
    filter_params.max_factor = 7.0;
    filter_params.base_factor = 3600;
    {
        nr::RAPTOR::Journeys journeys = {j1, j2};
        nr::filter_late_journeys(journeys, filter_params);

        BOOST_REQUIRE_EQUAL(journeys.size(), 2);

        auto vj1 = journeys.front().sections[0].get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj1->uri, "vehicle_journey:vj1:1");
        auto vj2 = journeys.back().sections[0].get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj2->uri, "vehicle_journey:vj2:2");
    }
}

BOOST_FIXTURE_TEST_CASE(should_filter_late_journeys_with_empty_list, Night_bus_fixture) {
    nr::RAPTOR::Journeys journeys;

    BOOST_CHECK_NO_THROW(nr::filter_late_journeys(journeys, filter_params));
}

/**
 * @brief This test aims to highlight the timeframe_duration option on a journey request
 *
 * The data for testing :
 *
 * Vehicle journey with multiple stops times
 * date : 2018 03 09
 *
 *                   sa1                   sa3
 *                  x------------------------x
 *               - 08:00:00               - 08:05:00
 *               - 08:10:00               - 08:15:00
 *               - 08:20:00               - 08:25:00
 *               - 08:30:00               - 08:35:00
 *                             ... x20
 *               - 08:00:00               - 08:05:00 (24H after the first)
 *
 */
BOOST_AUTO_TEST_CASE(journeys_with_time_frame_duration) {
    // Create Builder
    ed::builder b("20180309");

    // Create Area
    b.sa("stop_area:sa1")("stop_point:sa1:s1", 2.39592, 48.84848, false);
    b.sa("stop_area:sa3")("stop_point:sa3:s1", 2.36381, 48.86650, false);

    // Create multi VJ
    auto dep_time = "08:00:00"_t;
    auto arr_time = "08:05:00"_t;
    for (int nb = 0; nb < 20; ++nb) {
        b.vj("A", "1", "", false, "vjC_" + std::to_string(nb))("stop_point:sa1:s1", dep_time + nb * "00:10::00"_t)(
            "stop_point:sa3:s1", arr_time + nb * "00:10::00"_t);
    }
    // Add a VJ 24H after to test the time frame duration max limit (24H)
    b.vj("A", "1", "", false, "vjC_out_of_24_bound")("stop_point:sa1:s1", "08:00:00"_t)("stop_point:sa3:s1",
                                                                                        "08:05:00"_t);

    b.make();
    b.data->build_proximity_list();
    auto prod_date = boost::gregorian::date(2018, 3, 9);
    auto prod_len = boost::gregorian::days(1);
    b.data->meta->production_date = boost::gregorian::date_period(prod_date, prod_len);

    // create Raptor
    nr::RAPTOR raptor(*(b.data));

    // from
    navitia::type::EntryPoint origin(navitia::type::Type_e::StopPoint, "stop_point:sa1:s1");

    // to
    navitia::type::EntryPoint destination(navitia::type::Type_e::StopPoint, "stop_point:sa3:s1");

    std::vector<std::string> forbidden;
    ng::StreetNetwork sn_worker(*b.data->geo_ref);

    auto* data_ptr = b.data.get();

    //-----------------------------------
    // Case 1 :
    // clockwise = true
    // timeframe_duration = 0
    // timeframe_max_bound = 24H (very late search limit)
    bool clockwise = true;

    // send request
    navitia::PbCreator pb_creator_case1(data_ptr, "20180309T075900"_dt, null_time_period);
    make_response(pb_creator_case1, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T075900")},
                  clockwise, navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min, 24 * 60 * 60, 10, 0, 0, 0);

    // get the response
    auto resp = pb_creator_case1.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_GE(resp.journeys_size(), 1);

    //-----------------------------------
    // Case 2 :
    // clockwise = true
    // min_nb_journeys = none
    // timeframe_max_bound = 24H (very late search limit)
    //
    // timeframe_duration = 10*60 (10 min)
    // We have a timeframe duration = 10 min related to the first journeys (08:00:00),
    // Even though, the second journey's departure is later than the timeframe's limit
    // we still keep it, because it satisfies the min_nb_journey criteria
    uint64_t timeframe_duration = 10 * 60;
    clockwise = true;

    // send request
    navitia::PbCreator pb_creator_case2(data_ptr, "20180309T075900"_dt, null_time_period);
    make_response(pb_creator_case2, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T075900")},
                  clockwise, navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min, 24 * 60 * 60, 10, 0, 0, 0, boost::none, 1.5, 900, timeframe_duration);

    // get the response
    resp = pb_creator_case2.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_GE(resp.journeys_size(), 1);

    auto journeys = sort_journeys_by(resp, JourneySectionCompare());

    // Journey 1
    auto journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    auto section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T080000"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T080500"_pts);

    //-----------------------------------
    // Case 3 :
    // clockwise = false !!
    // timeframe_max_bound = 24H (very late search limit)
    //
    // timeframe_duration = 20*60 (20 min)
    // We have a timeframe duration = 20 min related to the first
    // journeys (08:30:00), as explained above, even though some journeys' arrivals are
    // not in the interval, we still keep them
    // The response must contain at least 2 journeys.
    timeframe_duration = 20 * 60;
    clockwise = false;

    // send request
    navitia::PbCreator pb_creator_case3(data_ptr, "20180309T080000"_dt, null_time_period);
    make_response(pb_creator_case3, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T083500")},
                  clockwise, navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min, 24 * 60 * 60, 10, 0, 0, 0, boost::none, 1.5, 900, timeframe_duration);

    // get the response
    resp = pb_creator_case3.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_GE(resp.journeys_size(), 2);

    journeys = sort_journeys_by(resp, JourneySectionCompare());

    journey = journeys[0];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T081000"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T081500"_pts);

    journey = journeys[1];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T082000"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T082500"_pts);

    journey = journeys[2];
    BOOST_REQUIRE_EQUAL(journey.sections_size(), 1);
    section = journey.sections(0);
    BOOST_CHECK_EQUAL(section.type(), pbnavitia::SectionType::PUBLIC_TRANSPORT);
    BOOST_CHECK_EQUAL(section.begin_date_time(), "20180309T083000"_pts);
    BOOST_CHECK_EQUAL(section.end_date_time(), "20180309T083500"_pts);

    //-----------------------------------
    // Case 4 :
    // clockwise = true
    // timeframe_duration = 60*60 (1H)
    // timeframe_max_bound = 24H (very late search limit)
    //
    // We have a time frame duration = 1H related to the first Journeys (08:30:00) and
    // a min_nb_journeys = 20.
    // We don't have 20 journeys in 1H, so we continue until 20 journeys as
    // min_nb_journeys is the limiting criteria.
    // The response must contain 20 journeys.
    timeframe_duration = 1 * 60 * 60;
    clockwise = true;

    // send request
    navitia::PbCreator pb_creator_case4(data_ptr, "20180309T075900"_dt, null_time_period);
    make_response(pb_creator_case4, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T075900")},
                  clockwise, navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min, 24 * 60 * 60, 10, 0, 0, 0, 20, 3, 6000, timeframe_duration);

    // get the response
    resp = pb_creator_case4.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 20);

    //-----------------------------------
    // Case 5 :
    // clockwise = true
    // timeframe_duration = 4*60*60 (4H)
    // timeframe_max_bound = 1H (very late search limit)
    //
    // the limit max is less than timeframe_duration.
    // We compute until the max limit
    // The response must contain only 6 journeys.
    timeframe_duration = 60 * 60 * 4;  // 4H
    clockwise = true;
    uint32_t timeframe_max_duration = 60 * 60;

    // send request
    navitia::PbCreator pb_creator_case5(data_ptr, "20180309T075900"_dt, null_time_period);
    make_response(pb_creator_case5, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T075900")},
                  clockwise, navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min, timeframe_max_duration, 10, 0, 0, 0, boost::none, 1.5, 900, timeframe_duration);

    // get the response
    resp = pb_creator_case5.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 6);

    //-----------------------------------
    // Case 6 :
    // clockwise = true
    // min_nb_journeys = none
    // timeframe_duration = 40*60 (40min)
    // timeframe_max_bound = 1H
    //
    // the reponse should contain at least 4 journeys
    timeframe_duration = 40 * 60;  // 40 min
    clockwise = true;
    timeframe_max_duration = 60 * 60;

    // send request
    navitia::PbCreator pb_creator_case6(data_ptr, "20180309T075900"_dt, null_time_period);
    make_response(pb_creator_case6, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T075900")},
                  clockwise, navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min, timeframe_max_duration, 10, 0, 0, 0, boost::none, 1.5, 900, timeframe_duration);

    // get the response
    resp = pb_creator_case6.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_GE(resp.journeys_size(), 4);
    // We should have next_request_date_time in the response
    BOOST_CHECK_EQUAL(resp.has_next_request_date_time(), true);

    //-----------------------------------
    // Case 7 :
    // clockwise = true
    // min_nb_journeys = none
    // timeframe_duration = 1 day
    // timeframe_max_bound = 10 min
    //
    // We test the case where max_duraion < timeframe_duration
    // Even though we ask for 10 journeys, the search bound is limited to 10min
    // Raptor can find only one solution within ten minutes
    timeframe_duration = 24 * 60 * 60;  // 1 day
    clockwise = true;
    timeframe_max_duration = 10 * 60;  // 10min

    // send request
    navitia::PbCreator pb_creator_case7(data_ptr, "20180309T075900"_dt, null_time_period);
    make_response(pb_creator_case7, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T075900")},
                  clockwise, navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min, timeframe_max_duration, 10, 0, 0, 0, boost::none, 1.5, 900, timeframe_duration);

    // get the response
    resp = pb_creator_case7.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::ITINERARY_FOUND);
    BOOST_REQUIRE_EQUAL(resp.journeys_size(), 1);
    BOOST_REQUIRE_EQUAL(resp.journeys(0).sections_size(), 1);
    // We should have next_request_date_time in the response
    BOOST_CHECK_EQUAL(resp.has_next_request_date_time(), true);

    //-----------------------------------
    // Case 8 :
    // clockwise = true
    // min_nb_journeys = none
    // timeframe_duration = 1 day
    // timeframe_max_bound = 10s
    //
    // max_duration is too small, no solution is found
    timeframe_duration = 24 * 60 * 60;  // 1 day
    clockwise = true;
    timeframe_max_duration = 10;  // 10 seconds

    // send request
    navitia::PbCreator pb_creator_case8(data_ptr, "20180309T075900"_dt, null_time_period);
    make_response(pb_creator_case8, raptor, origin, destination, {ntest::to_posix_timestamp("20180309T075900")},
                  clockwise, navitia::type::AccessibiliteParams(), forbidden, {}, sn_worker, nt::RTLevel::Base, 2_min,
                  2_min, timeframe_max_duration, 10, 0, 0, 0, boost::none, 1.5, 900, timeframe_duration);

    // get the response
    resp = pb_creator_case8.get_response();
    BOOST_REQUIRE_EQUAL(resp.response_type(), pbnavitia::NO_SOLUTION);
}

// basic journey without min_nb_journey nor timeframe_limit
BOOST_AUTO_TEST_CASE(keep_going_tests_simple) {
    // no solution found: stop the search
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(0,                                  // total_nb_journeys
                                                   1,                                  // nb_try
                                                   true,                               // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),  // request_date_secs
                                                   boost::none,                        // min_nb_journeys
                                                   boost::none,                        // timeframe_limit
                                                   10),                                // max_transfers
                      false);

    // some solutions have been found: stop the search
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(3,                                  // total_nb_journeys
                                                   1,                                  // nb_try
                                                   true,                               // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),  // request_date_secs
                                                   boost::none,                        // min_nb_journeys
                                                   boost::none,                        // timeframe_limit
                                                   10),                                // max_transfers
                      false);
}

// journeys with min_nb_journeys=3
BOOST_AUTO_TEST_CASE(keep_going_tests_min_nb_journeys) {
    // only one solution found: continue to search
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                  // total_nb_journeys
                                                   1,                                  // nb_try
                                                   true,                               // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),  // request_date_secs
                                                   3,                                  // min_nb_journeys
                                                   boost::none,                        // timeframe_limit
                                                   10),                                // max_transfers
                      true);

    // 3 solutions found, stop the search
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(3,                                  // total_nb_journeys
                                                   1,                                  // nb_try
                                                   true,                               // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),  // request_date_secs
                                                   3,                                  // min_nb_journeys
                                                   boost::none,                        // timeframe_limit
                                                   10),                                // max_transfers
                      false);

    // only one solution found, but MAX_NB_RAPTOR_CALL is reached: stop the search
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                  // total_nb_journeys
                                                   101,                                // nb_try
                                                   true,                               // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),  // request_date_secs
                                                   3,                                  // min_nb_journeys
                                                   boost::none,                        // timeframe_limit
                                                   10),                                // max_transfers
                      false);

    // max_transfers=0, only one solution found
    // MAX_NB_RAPTOR_CALL is increased since we are in "simple mode": continue the search
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                  // total_nb_journeys
                                                   101,                                // nb_try
                                                   true,                               // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),  // request_date_secs
                                                   3,                                  // min_nb_journeys
                                                   boost::none,                        // timeframe_limit
                                                   0),                                 // max_transfers
                      true);
    // we finally stop after 1000 try
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                  // total_nb_journeys
                                                   1001,                               // nb_try
                                                   true,                               // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),  // request_date_secs
                                                   3,                                  // min_nb_journeys
                                                   boost::none,                        // timeframe_limit
                                                   0),                                 // max_transfers
                      false);
}

// journeys with a timeframe_limit
BOOST_AUTO_TEST_CASE(keep_going_tests_timeframe) {
    // request_date_secs < timeframe_limit: continue
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   1,                                    // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),    // request_date_secs
                                                   boost::none,                          // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      true);

    // request_date_secs == timeframe_limit: stop
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   1,                                    // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 900),  // request_date_secs
                                                   boost::none,                          // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      false);

    // request_date_secs < timeframe_limit but MAX_NB_RAPTOR_CALL is reached: stop the search
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   101,                                  // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 100),  // request_date_secs
                                                   boost::none,                          // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      false);

    // request_date_secs < timeframe_limit and max_transfers=0 (simple mode)
    //  MAX_NB_RAPTOR_CALL is increased: continue
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   101,                                  // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 100),  // request_date_secs
                                                   boost::none,                          // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   0),                                   // max_transfers
                      true);
    // last try
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   1000,                                 // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 100),  // request_date_secs
                                                   boost::none,                          // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   0),                                   // max_transfers
                      true);

    // after 1001 try: stop
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   1001,                                 // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 100),  // request_date_secs
                                                   boost::none,                          // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   0),                                   // max_transfers
                      false);
    // or before if we reach the timeframe_limit
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   102,                                  // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 900),  // request_date_secs
                                                   boost::none,                          // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   0),                                   // max_transfers
                      false);
}

// journeys with a timeframe_limit
BOOST_AUTO_TEST_CASE(keep_going_tests_timeframe_and_min_nb_journey) {
    // request_date_secs < timeframe_limit && total_nb_journeys < min_nb_journeys: continue
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   1,                                    // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),    // request_date_secs
                                                   5,                                    // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      true);

    // request_date_secs == timeframe_limit && total_nb_journeys < min_nb_journeys: continue
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   1,                                    // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 900),  // request_date_secs
                                                   5,                                    // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      true);

    // request_date_secs < timeframe_limit && total_nb_journeys == min_nb_journeys: continue
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(5,                                    // total_nb_journeys
                                                   1,                                    // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 9),    // request_date_secs
                                                   5,                                    // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      true);

    // request_date_secs == timeframe_limit && total_nb_journeys == min_nb_journeys: continue
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(5,                                    // total_nb_journeys
                                                   1,                                    // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 900),  // request_date_secs
                                                   5,                                    // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      false);

    // request_date_secs < timeframe_limit && total_nb_journeys < min_nb_journeys: we try really hard!
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   100,                                  // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),    // request_date_secs
                                                   5,                                    // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      true);

    // but we finally give up
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   101,                                  // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),    // request_date_secs
                                                   5,                                    // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   10),                                  // max_transfers
                      false);
    // except if it's really easy then we try harder
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   1000,                                 // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),    // request_date_secs
                                                   5,                                    // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   0),                                   // max_transfers
                      true);

    // until we stop trying
    BOOST_CHECK_EQUAL(navitia::routing::keep_going(1,                                    // total_nb_journeys
                                                   1001,                                 // nb_try
                                                   true,                                 // clockwise
                                                   navitia::DateTimeUtils::set(1, 2),    // request_date_secs
                                                   5,                                    // min_nb_journeys
                                                   navitia::DateTimeUtils::set(1, 900),  // timeframe_limit
                                                   0),                                   // max_transfers
                      false);
}

navitia::routing::map_stop_point_duration init_dummy_map_stop_point_duration(size_t size) {
    navitia::routing::map_stop_point_duration res;

    while (size) {
        --size;
        res[navitia::routing::SpIdx{static_cast<unsigned int>(size)}] =
            navitia::time_duration{boost::date_time::special_values::min_date_time};
    }
    return res;
}

BOOST_AUTO_TEST_CASE(test_filter_backtrack) {
    using namespace navitia::routing;

    ed::builder b("20150101", [](ed::builder& b) {
        b.vj("1")("A", "0:30"_t)("B", "1:00"_t, "1:01"_t)("C", "2:00"_t);
        b.vj("2")("C", "2:10"_t)("B", "3:01"_t);
    });

    auto vj1 = b.data->pt_data->vehicle_journeys_map["vehicle_journey:1:0"];
    auto vj2 = b.data->pt_data->vehicle_journeys_map["vehicle_journey:2:1"];

    const auto& st_1A = vj1->get_stop_time(navitia::type::RankStopTime(0));

    const auto& st_1B = vj1->get_stop_time(navitia::type::RankStopTime(1));
    const auto& st_1C = vj1->get_stop_time(navitia::type::RankStopTime(2));

    const auto& st_2C = vj2->get_stop_time(navitia::type::RankStopTime(0));
    const auto& st_2B = vj2->get_stop_time(navitia::type::RankStopTime(1));

    auto solution = std::list<Journey>{};

    {
        Journey journey_1;

        auto s_1 = Journey::Section{st_1A, "0:30"_t, st_1B, "1:00"_t};
        auto s_2 = Journey::Section{st_1B, "1:00"_t, st_1C, "2:00"_t};
        auto s_3 = Journey::Section{st_2C, "2:10"_t, st_2B, "3:01"_t};

        journey_1.sections.push_back(s_1);
        journey_1.sections.push_back(s_2);
        journey_1.sections.push_back(s_3);

        solution.push_back(journey_1);
    }
    {
        Journey journey_2;

        auto s_1 = Journey::Section{st_1A, "0:30"_t, st_1B, "1:00"_t};
        journey_2.sections.push_back(s_1);
        solution.push_back(journey_2);
    }

    auto departures = init_dummy_map_stop_point_duration(b.data->pt_data->stop_points.size());
    auto destinations = init_dummy_map_stop_point_duration(b.data->pt_data->stop_points.size());

    modify_backtracking_journeys(solution, departures, destinations, true);
    BOOST_CHECK_EQUAL(solution.size(), 2);

    const auto& j1 = solution.front();
    BOOST_CHECK_EQUAL(j1.sections.size(), 1);

    {
        const auto* vj = j1.sections.front().get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, vj1->uri);
        BOOST_CHECK_EQUAL(j1.sections.front().get_in_st->order().val, 0);
        BOOST_CHECK_EQUAL(j1.sections.front().get_in_dt, "0:30"_t);
    }

    {
        const auto* vj = j1.sections.front().get_out_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, vj1->uri);
        BOOST_CHECK_EQUAL(j1.sections.front().get_out_st->order().val, 1);
        BOOST_CHECK_EQUAL(j1.sections.front().get_out_dt, "1:00"_t);
    }
}

BOOST_AUTO_TEST_CASE(backtracking_journey_stay_in) {
    using namespace navitia::routing;

    ed::builder b("20150101", [](ed::builder& b) {
        b.vj("Stay1", "1111111", "block1", true)("A", "0:30"_t, "0:31"_t)("B", "1:00"_t, "1:01"_t)("C", "2:00"_t);
        b.vj("Stay2", "1111111", "block1", true)("C", "2:00"_t)("D", "2:10"_t)("B", "3:01"_t);
    });

    auto vj1 = b.data->pt_data->vehicle_journeys_map["vehicle_journey:Stay1:0"];
    auto vj2 = b.data->pt_data->vehicle_journeys_map["vehicle_journey:Stay2:1"];

    const auto& st_1A = vj1->get_stop_time(navitia::type::RankStopTime(0));
    const auto& st_2B = vj2->get_stop_time(navitia::type::RankStopTime(2));

    auto solution = std::list<Journey>{};

    {
        Journey journey_stay_in;

        auto s_1 = Journey::Section{st_1A, "0:30"_t, st_2B, "3:01"_t};

        journey_stay_in.sections.push_back(s_1);

        solution.push_back(journey_stay_in);
    }
    auto departures = init_dummy_map_stop_point_duration(b.data->pt_data->stop_points.size());
    auto destinations = init_dummy_map_stop_point_duration(b.data->pt_data->stop_points.size());

    modify_backtracking_journeys(solution, departures, destinations, true);

    const auto& j = solution.front();
    {
        const auto* vj = j.sections.front().get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, vj1->uri);
        BOOST_CHECK_EQUAL(j.sections.front().get_in_st->order().val, 0);
        BOOST_CHECK_EQUAL(j.sections.front().get_in_dt, "0:30"_t);
    }
    {
        const auto* vj = j.sections.front().get_out_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, vj1->uri);
        BOOST_CHECK_EQUAL(j.sections.front().get_out_st->order().val, 1);
        BOOST_CHECK_EQUAL(j.sections.front().get_out_dt, "1:00"_t);
    }
}

BOOST_AUTO_TEST_CASE(backtracking_journey_arrival_before) {
    using namespace navitia::routing;

    ed::builder b("20150101", [](ed::builder& b) {
        b.vj("1")("A", "0:30"_t)("B", "1:00"_t)("C", "2:00"_t);
        b.vj("2")("C", "2:10"_t)("A", "3:00"_t, "3:01"_t)("E", "4:01"_t);
    });

    auto vj1 = b.data->pt_data->vehicle_journeys_map["vehicle_journey:1:0"];
    auto vj2 = b.data->pt_data->vehicle_journeys_map["vehicle_journey:2:1"];

    const auto& st_1A = vj1->get_stop_time(navitia::type::RankStopTime(0));
    const auto& st_1C = vj1->get_stop_time(navitia::type::RankStopTime(2));

    const auto& st_2C = vj2->get_stop_time(navitia::type::RankStopTime(0));
    const auto& st_2E = vj2->get_stop_time(navitia::type::RankStopTime(2));

    auto solution = std::list<Journey>{};

    {
        Journey journey_1;

        auto s_1 = Journey::Section{st_1A, "0:30"_t, st_1C, "2:00"_t};
        auto s_2 = Journey::Section{st_2C, "2:10"_t, st_2E, "4:01"_t};

        journey_1.sections.push_back(s_1);
        journey_1.sections.push_back(s_2);

        solution.push_back(journey_1);
    }
    auto departures = init_dummy_map_stop_point_duration(b.data->pt_data->stop_points.size());
    auto destinations = init_dummy_map_stop_point_duration(b.data->pt_data->stop_points.size());
    modify_backtracking_journeys(solution, departures, destinations, false);

    const auto& j = solution.front();
    {
        const auto* vj = j.sections.front().get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, vj2->uri);
        BOOST_CHECK_EQUAL(j.sections.front().get_in_st->order().val, 1);
        BOOST_CHECK_EQUAL(j.sections.front().get_in_dt, "3:01"_t);
    }
    {
        const auto* vj = j.sections.front().get_out_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, vj2->uri);
        BOOST_CHECK_EQUAL(j.sections.front().get_out_st->order().val, 2);
        BOOST_CHECK_EQUAL(j.sections.front().get_out_dt, "4:01"_t);
    }
}

BOOST_AUTO_TEST_CASE(test_filter_backtrack_with_empty_matrix) {
    using namespace navitia::routing;

    ed::builder b("20150101", [](ed::builder& b) {
        b.vj("1")("A", "0:30"_t)("B", "1:00"_t, "1:01"_t)("C", "2:00"_t);
        b.vj("2")("C", "2:10"_t)("B", "3:01"_t);
    });

    auto vj1 = b.data->pt_data->vehicle_journeys_map["vehicle_journey:1:0"];
    auto vj2 = b.data->pt_data->vehicle_journeys_map["vehicle_journey:2:1"];

    const auto& st_1A = vj1->get_stop_time(navitia::type::RankStopTime(0));

    const auto& st_1B = vj1->get_stop_time(navitia::type::RankStopTime(1));
    const auto& st_1C = vj1->get_stop_time(navitia::type::RankStopTime(2));

    const auto& st_2C = vj2->get_stop_time(navitia::type::RankStopTime(0));
    const auto& st_2B = vj2->get_stop_time(navitia::type::RankStopTime(1));

    auto solution = std::list<Journey>{};

    {
        Journey journey_1;

        auto s_1 = Journey::Section{st_1A, "0:30"_t, st_1B, "1:00"_t};
        auto s_2 = Journey::Section{st_1B, "1:00"_t, st_1C, "2:00"_t};
        auto s_3 = Journey::Section{st_2C, "2:10"_t, st_2B, "3:01"_t};

        journey_1.sections.push_back(s_1);
        journey_1.sections.push_back(s_2);
        journey_1.sections.push_back(s_3);

        solution.push_back(journey_1);
    }
    {
        Journey journey_2;

        auto s_1 = Journey::Section{st_1A, "0:30"_t, st_1B, "1:00"_t};
        journey_2.sections.push_back(s_1);
        solution.push_back(journey_2);
    }

    auto departures = init_dummy_map_stop_point_duration(0);
    auto destinations = init_dummy_map_stop_point_duration(0);

    modify_backtracking_journeys(solution, departures, destinations, true);
    BOOST_CHECK_EQUAL(solution.size(), 2);

    const auto& j1 = solution.front();
    // Nothing should be changed
    BOOST_CHECK_EQUAL(j1.sections.size(), 3);

    {
        const auto* vj = j1.sections.front().get_in_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, vj1->uri);
        BOOST_CHECK_EQUAL(j1.sections.front().get_in_st->order().val, 0);
        BOOST_CHECK_EQUAL(j1.sections.front().get_in_dt, "0:30"_t);
    }

    {
        const auto* vj = j1.sections.front().get_out_st->vehicle_journey;
        BOOST_CHECK_EQUAL(vj->uri, vj1->uri);
        BOOST_CHECK_EQUAL(j1.sections.front().get_out_st->order().val, 1);
        BOOST_CHECK_EQUAL(j1.sections.front().get_out_dt, "1:00"_t);
    }
}
