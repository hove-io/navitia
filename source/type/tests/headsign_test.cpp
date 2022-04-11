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
#define BOOST_TEST_MODULE test_headsign

#include <boost/test/unit_test.hpp>
#include "utils/functions.h"
#include "ed/build_helper.h"
#include "type/pt_data.h"
#include "tests/utils_test.h"
#include "type/headsign_handler.h"
#include <memory>
#include <boost/archive/text_oarchive.hpp>
#include <string>
#include <type_traits>
#include "utils/logger.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace nt = navitia::type;

struct HeadsignFixture {
    ed::builder b;
    HeadsignFixture()
        : b("20120614", [](ed::builder& b) {
              b.vj("A").name("A1").meta_vj("metaVJA").vp("01")("stop00", 8000)("stop01", 8100);
              b.vj("A").name("A2").meta_vj("metaVJA").vp("10")("stop10", 8000)("stop11", 8100)("stop12", 8200);
              b.vj("C")("stop20", 8000)("stop21", 8100)("stop22", 8200);
              b.vj("D")("stop30", 8000);
              b.vj("E")("stop40", 8000);
          }) {}
};

BOOST_FIXTURE_TEST_CASE(headsign_handler_functionnal_test, HeadsignFixture) {
    nt::HeadsignHandler& headsign_handler = b.data->pt_data->headsign_handler;
    const auto& vj_vec = b.data->pt_data->vehicle_journeys;

    // check the initial name and headsign
    BOOST_CHECK_EQUAL(vj_vec[0]->name, "A1");
    BOOST_CHECK_EQUAL(vj_vec[0]->headsign, "A1");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(0), "A00");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(1), vj_vec[1]->headsign);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[0]->stop_time_list.at(0)), "A00");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[0]->stop_time_list.at(1)), vj_vec[1]->headsign);

    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(0), vj_vec[1]->headsign);
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(1), "B11");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(2), "B11");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(0)), vj_vec[1]->headsign);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(1)), "B11");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(2)), "B11");

    headsign_handler.affect_headsign_to_stop_time(vj_vec[2]->stop_time_list.at(1), "C21");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(0)), vj_vec[2]->headsign);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(1)), "C21");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(2)), vj_vec[2]->headsign);

    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), "D31");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), vj_vec[3]->headsign);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[3]->stop_time_list.at(0)), vj_vec[3]->headsign);

    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[4]->stop_time_list.at(0)), vj_vec[4]->headsign);

    // check that we can retrieve every vj from matching headsign
    // As meta_vj name is not assigned to headsign no vjs for "metaVJA"
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("metaVJA").size(), 0);
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("metaVJA").size(), 0);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("A1"), vj_vec[0]));
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("A2"), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("A00").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("A00"), vj_vec[0]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("B11").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("B11"), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign(vj_vec[2]->headsign).size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[2]->headsign), vj_vec[2]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("C21").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("C21"), vj_vec[2]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign(vj_vec[3]->headsign).size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[3]->headsign), vj_vec[3]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign(vj_vec[4]->headsign).size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[4]->headsign), vj_vec[4]));
}

struct HeadsignHandlerTest : nt::HeadsignHandler {
    const std::unordered_map<const nt::VehicleJourney*, boost::container::flat_map<nt::RankStopTime, std::string>>&
    get_headsign_changes() const {
        return headsign_changes;
    }
    const std::unordered_map<std::string, std::unordered_set<const nt::MetaVehicleJourney*>>& get_headsign_mvj() const {
        return headsign_mvj;
    }
};

BOOST_FIXTURE_TEST_CASE(headsign_handler_internal_test, HeadsignFixture) {
    HeadsignHandlerTest headsign_handler;
    auto& vj_vec = b.data->pt_data->vehicle_journeys;
    // done for the "actual" handler in build helper but not on test handler
    for (const auto& vj : vj_vec) {
        headsign_handler.change_name_and_register_as_headsign(*vj, vj->headsign);
    }

    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(0), "A00");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(1), "metaVJA");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(0), "metaVJA");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(1), "B11");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(2), "B11");
    // We check that we can have the same headsign for different vjs
    headsign_handler.affect_headsign_to_stop_time(vj_vec[2]->stop_time_list.at(1), "metaVJA");

    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), "D31");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), vj_vec[3]->headsign);

    // check that we only store changes, no more
    BOOST_REQUIRE_EQUAL(headsign_handler.get_headsign_changes().size(), 3);
    // Three headsign changes instead of 2 for vj_vec[0]: "A1", "A00" and "metaVJA"
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_changes().at(vj_vec[0]).size(), 3);
    // Three headsign changes instead of 2 for vj_vec[1]: "A2", "metaVJA" and "B11"
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_changes().at(vj_vec[1]).size(), 3);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_changes().at(vj_vec[2]).size(), 2);

    // check that we store all matches, no more
    // 8 changes instead of 6: "A1", "A2", "vj 2", "vj 3", "vj 4", "A00", "metaVJA", "B11"
    BOOST_REQUIRE_EQUAL(headsign_handler.get_headsign_mvj().size(), 8);

    // check that only 2 meta-vj are stored for metaVJA (but 3 vj are behind)
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at("metaVJA").size(), 2);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at("A00").size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at("B11").size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at(vj_vec[2]->headsign).size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at(vj_vec[3]->headsign).size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at(vj_vec[4]->headsign).size(), 1);
}
