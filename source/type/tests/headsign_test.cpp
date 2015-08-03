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
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace nt = navitia::type;

BOOST_AUTO_TEST_CASE(headsign_handler_functionnal_test) {

    ed::builder b("20120614");
    b.vj("A")("stop00", 8000, 8050)("stop01", 8100,8150);
    b.vj("B")("stop10", 8000, 8050)("stop11", 8100,8150)("stop12", 8200, 8250);
    b.vj("C")("stop20", 8000, 8050)("stop21", 8100,8150)("stop22", 8200, 8250);
    b.vj("D")("stop30", 8000, 8050);
    auto& vj_vec = b.data->pt_data->vehicle_journeys;
    b.data->pt_data->index();
    b.finish();

    nt::HeadsignHandler& headsign_handler = b.data->pt_data->headsign_handler;

    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(0), "A00");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(1), vj_vec[1]->name);
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(0), vj_vec[1]->name);
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(1), "B11");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(2), "B11");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[2]->stop_time_list.at(1), "C21");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), "D31");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), vj_vec[3]->name);

    // check that stop_time headsigns are correct
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[0]->stop_time_list.at(0)), "A00");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[0]->stop_time_list.at(1)), vj_vec[1]->name);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(0)), vj_vec[1]->name);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(1)), "B11");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[1]->stop_time_list.at(2)), "B11");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(0)), vj_vec[2]->name);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(1)), "C21");
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[2]->stop_time_list.at(2)), vj_vec[2]->name);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign(vj_vec[3]->stop_time_list.at(0)), vj_vec[3]->name);

    // check that we can retrieve every vj from matching headsign
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("vehicle_journey 0").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[0]->name), vj_vec[0]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("A00").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("A00"), vj_vec[0]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("vehicle_journey 1").size(), 2);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[1]->name), vj_vec[0]));
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[1]->name), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("B11").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("B11"), vj_vec[1]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign(vj_vec[2]->name).size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[2]->name), vj_vec[2]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign("C21").size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign("C21"), vj_vec[2]));
    BOOST_CHECK_EQUAL(headsign_handler.get_vj_from_headsign(vj_vec[3]->name).size(), 1);
    BOOST_CHECK(navitia::contains(headsign_handler.get_vj_from_headsign(vj_vec[3]->name), vj_vec[3]));

    boost::archive::text_oarchive oa(std::cout);
    oa & b.data->pt_data;
}

struct HeadsignHandlerTest: nt::HeadsignHandler {
    std::unordered_map<const nt::VehicleJourney*, boost::container::flat_map<uint16_t, std::string>>
    get_map_vj_map_stop_time_headsign_change() {return map_vj_map_stop_time_headsign_change;}
    std::unordered_map<std::string, std::unordered_set<const nt::MetaVehicleJourney*>>
    get_headsign_mvj() {return headsign_mvj;}
};

BOOST_AUTO_TEST_CASE(headsign_handler_internal_test) {

    ed::builder b("20120614");
    b.vj("A")("stop00", 8000, 8050)("stop01", 8100,8150);
    b.vj("B")("stop10", 8000, 8050)("stop11", 8100,8150)("stop12", 8200, 8250);
    b.vj("C")("stop20", 8000, 8050)("stop21", 8100,8150)("stop22", 8200, 8250);
    b.vj("D")("stop30", 8000, 8050);
    auto& vj_vec = b.data->pt_data->vehicle_journeys;
    b.data->pt_data->index();
    b.finish();

    HeadsignHandlerTest headsign_handler;

    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(0), "A00");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[0]->stop_time_list.at(1), vj_vec[1]->name);
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(0), vj_vec[1]->name);
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(1), "B11");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[1]->stop_time_list.at(2), "B11");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[2]->stop_time_list.at(1), "C21");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), "D31");
    headsign_handler.affect_headsign_to_stop_time(vj_vec[3]->stop_time_list.at(0), vj_vec[3]->name);

    // check that we only store changes, no more
    BOOST_CHECK_EQUAL(headsign_handler.get_map_vj_map_stop_time_headsign_change().size(), 3);
    BOOST_CHECK_EQUAL(headsign_handler.get_map_vj_map_stop_time_headsign_change()[vj_vec[0]].size(), 3);
    BOOST_CHECK_EQUAL(headsign_handler.get_map_vj_map_stop_time_headsign_change()[vj_vec[1]].size(), 2);
    BOOST_CHECK_EQUAL(headsign_handler.get_map_vj_map_stop_time_headsign_change()[vj_vec[2]].size(), 2);

    // check that we store all matches, no more
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj().size(), 7);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj()[vj_vec[0]->name].size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj()["A00"].size(), 1);
    // check that only one meta-vj is stored for B (but 2 vj are behind)
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj()[vj_vec[1]->name].size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj()["B11"].size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj()[vj_vec[2]->name].size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj()["C21"].size(), 1);
    BOOST_CHECK_EQUAL(headsign_handler.get_headsign_mvj()[vj_vec[3]->name].size(), 1);
}
