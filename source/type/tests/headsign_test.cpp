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
#define BOOST_TEST_MODULE test_headsign

#include <boost/test/unit_test.hpp>
#include "utils/functions.h"
#include "ed/build_helper.h"
#include "type/pt_data.h"
#include "type/type.h"
#include "tests/utils_test.h"
#include "type/headsign_handler.h"
#include <memory>
#include <boost/archive/text_oarchive.hpp>
#include <string>
#include <type_traits>


struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );

namespace nt = navitia::type;

struct HeadsignFixture {
    ed::builder b;
    HeadsignFixture() : b("20120614") {
        b.vj("A", "1111", "", true, "A1", "metaVJA")("stop00", 8000)("stop01", 8100);
        b.vj("A", "1111", "", true, "A2", "metaVJA")("stop10", 8000)("stop11", 8100)("stop12", 8200);
        b.vj("C")("stop20", 8000)("stop21", 8100)("stop22", 8200);
        b.vj("D")("stop30", 8000);
        b.vj("E")("stop40", 8000);
        b.data->pt_data->index();
        b.finish();
    }
};

BOOST_FIXTURE_TEST_CASE(headsign_handler_functionnal_test, HeadsignFixture) {
    nt::HeadsignHandler& headsign_handler = b.data->pt_data->headsign_handler;
    auto& vj_vec = b.data->pt_data->vehicle_journeys;

    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(0), "A00");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(1), vj_vec[1]->name);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[0]->stop_time_list.at(0)), "A00");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[0]->stop_time_list.at(1)), vj_vec[1]->name);

    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(0), vj_vec[1]->name);
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(1), "B11");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(2), "B11");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(0)), vj_vec[1]->name);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(1)), "B11");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(2)), "B11");

    headsign_handler.affect_headsign_to_stop_time(vj_vec[2]->stop_time_list.at(1), "C21");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(0)), vj_vec[2]->name);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(1)), "C21");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(2)), vj_vec[2]->name);

    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), "D31");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), vj_vec[3]->name);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[3]->stop_time_list.at(0)), vj_vec[3]->name);

    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[4]->stop_time_list.at(0)), vj_vec[4]->name);

    // check that we can retrieve every vj from matching headsign
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("metaVJA").size(), 2);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("metaVJA"), vj_vec[0]));
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("metaVJA"), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("A00").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("A00"), vj_vec[0]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("B11").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("B11"), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign(vj_vec[2]->name).size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[2]->name), vj_vec[2]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("C21").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("C21"), vj_vec[2]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign(vj_vec[3]->name).size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[3]->name), vj_vec[3]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign(vj_vec[4]->name).size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[4]->name), vj_vec[4]));
}

struct HeadsignHandlerTest: nt::HeadsignHandler {
    const std::unordered_map<const nt::VehicleJourney*, boost::container::flat_map<uint16_t, std::string>>&
    get_headsign_changes() const {return headsign_changes;}
    const std::unordered_map<std::string, std::unordered_set<const nt::MetaVehicleJourney*>>&
    get_headsign_mvj() const {return headsign_mvj;}
};

BOOST_FIXTURE_TEST_CASE(headsign_handler_internal_test, HeadsignFixture) {
    HeadsignHandlerTest headsign_handler;
    auto& vj_vec = b.data->pt_data->vehicle_journeys;
    // done for the "actual" handler in build helper but not on test handler
    for (const auto& vj: vj_vec) {
        headsign_handler.change_name_and_register_as_headsign(*vj, vj->name);
    }

    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(0), "A00");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(1), "metaVJA");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(0), "metaVJA");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(1), "B11");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(2), "B11");
    // We check that we can have the same headsign for different vjs
    headsign_handler.affect_headsign_to_stop_time(vj_vec[2]->stop_time_list.at(1), "metaVJA");

    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), "D31");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), vj_vec[3]->name);

    // check that we only store changes, no more
    BOOST_REQUIRE_EQUAL(headsign_handler.get_headsign_changes().size(), 3);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_changes().at(vj_vec[0]).size(), 2);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_changes().at(vj_vec[1]).size(), 2);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_changes().at(vj_vec[2]).size(), 2);

    // check that we store all matches, no more
    BOOST_REQUIRE_EQUAL(headsign_handler.get_headsign_mvj().size(), 6);
    // check that only 2 meta-vj are stored for metaVJA (but 3 vj are behind)
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at("metaVJA").size(), 2);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at("A00").size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at("B11").size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at(vj_vec[2]->name).size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at(vj_vec[3]->name).size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().at(vj_vec[4]->name).size(), 1);
}
