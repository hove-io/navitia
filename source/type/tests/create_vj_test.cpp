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
#define BOOST_TEST_MODULE create_vj_test

#include <boost/test/unit_test.hpp>
#include "ed/build_helper.h"
#include "type/pt_data.h"
#include "type/meta_vehicle_journey.h"
#include "tests/utils_test.h"
#include "type/comment_container.h"
#include <memory>
#include <boost/archive/text_oarchive.hpp>
#include <string>
#include <type_traits>
#include "utils/logger.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(create_vj_test) {
    using year = navitia::type::ValidityPattern::year_bitset;
    namespace nt = navitia::type;

    ed::builder b("20120614");
    const auto* base_vj = b.vj("A", "00111110011111")("stop1", 8000, 8000)("stop2", 8100, 8100).make();
    auto& pt_data = *b.data->pt_data;
    auto* mvj = pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0));
    BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("00111110011111"));
    BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("00111110011111"));
    BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("00111110011111"));

    // a delay in adapted
    auto sts = base_vj->stop_time_list;
    sts.at(1).arrival_time = 8200;
    sts.at(1).departure_time = 8200;
    auto vp = *base_vj->base_validity_pattern();
    vp.days = year(
        "0000100"
        "0000100");
    const auto* adapted_vj = mvj->create_discrete_vj("vehicle_journey:adapted", "adapted", "adapted_headsign",
                                                     nt::RTLevel::Adapted, vp, base_vj->route, sts, pt_data);

    BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("0011111"
                                                                   "0011111"));
    BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("0011011"
                                                                      "0011011"));
    BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("0011011"
                                                                 "0011011"));

    BOOST_CHECK_EQUAL(adapted_vj->base_validity_pattern()->days, year("0000000"
                                                                      "0000000"));
    BOOST_CHECK_EQUAL(adapted_vj->adapted_validity_pattern()->days, year("0000100"
                                                                         "0000100"));
    BOOST_CHECK_EQUAL(adapted_vj->rt_validity_pattern()->days, year("0000100"
                                                                    "0000100"));
    BOOST_CHECK_EQUAL(adapted_vj->headsign, "adapted_headsign");
    BOOST_CHECK_EQUAL(adapted_vj->name, "adapted");

    // a bigger delay in rt
    sts.at(1).arrival_time = 8300;
    sts.at(1).departure_time = 8300;
    vp.days = year(
        "0000000"
        "0000110");
    const auto* rt_vj = mvj->create_discrete_vj("vehicle_journey:rt", "rt", "rt_headsign", nt::RTLevel::RealTime, vp,
                                                base_vj->route, sts, pt_data);

    BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("0011111"
                                                                   "0011111"));
    BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("0011011"
                                                                      "0011011"));
    BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("0011011"
                                                                 "0011001"));

    BOOST_CHECK_EQUAL(adapted_vj->base_validity_pattern()->days, year("0000000"
                                                                      "0000000"));
    BOOST_CHECK_EQUAL(adapted_vj->adapted_validity_pattern()->days, year("0000100"
                                                                         "0000100"));
    BOOST_CHECK_EQUAL(adapted_vj->rt_validity_pattern()->days, year("0000100"
                                                                    "0000000"));

    BOOST_CHECK_EQUAL(rt_vj->base_validity_pattern()->days, year("0000000"
                                                                 "0000000"));
    BOOST_CHECK_EQUAL(rt_vj->adapted_validity_pattern()->days, year("0000000"
                                                                    "0000000"));
    BOOST_CHECK_EQUAL(rt_vj->rt_validity_pattern()->days, year("0000000"
                                                               "0000110"));
    BOOST_CHECK_EQUAL(rt_vj->headsign, "rt_headsign");
    BOOST_CHECK_EQUAL(rt_vj->name, "rt");
}

BOOST_AUTO_TEST_CASE(clean_up_useless_vjs_test) {
    using year = navitia::type::ValidityPattern::year_bitset;
    namespace nt = navitia::type;

    // clean_up_useless_vjs() is called to clean VJs, with all validity patterns = 0, for a given Meta VJ.
    // VJ can be base/adapted/realtime type

    ed::builder b("20120614");
    auto& pt_data = *b.data->pt_data;

    // Add new base VJ uri_A (all validity patterns = 0)
    const auto* empty_base_vj = b.vj("line_A", "000000", "block_id_A", false, "uri_A", "headsign_A", "meta_vj_name_A")(
                                     "stop1", 8000, 8000)("stop2", 8100, 8100)
                                    .make();
    BOOST_CHECK_EQUAL(empty_base_vj->base_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(empty_base_vj->adapted_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(empty_base_vj->rt_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(empty_base_vj->uri, "vehicle_journey:uri_A");
    BOOST_CHECK_EQUAL(empty_base_vj->name, "uri_A");
    BOOST_CHECK_EQUAL(empty_base_vj->headsign, "headsign_A");
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 1);
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 1);
    // Meta VJ
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    auto* mvj = pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0));
    BOOST_CHECK_EQUAL(mvj->get_label(), "meta_vj_name_A");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 1);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 0);

    // Add new base VJ uri_B
    // clean_up_useless_vjs is called before creating uri_B VJ.
    // clean_up_useless_vjs will suppress uri_A VJ.
    const auto* base_vj = b.vj("line_A", "000001", "block_id_A", false, "uri_B", "headsign_B", "meta_vj_name_A")(
                               "stop1", 8000, 8000)("stop2", 8100, 8100)
                              .make();
    BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("000001"));
    BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("000001"));
    BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("000001"));
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 1);
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 1);
    // Meta VJ
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    mvj = pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0));
    BOOST_CHECK_EQUAL(mvj->get_label(), "meta_vj_name_A");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 1);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 0);

    // Add new base VJ uri_C
    // clean_up_useless_vjs is called before creating uri_C VJ.
    // clean_up_useless_vjs will suppress nothing.
    base_vj = b.vj("line_A", "000011", "block_id_A", false, "uri_C", "headsign_C", "meta_vj_name_A")(
                   "stop1", 8000, 8000)("stop2", 8100, 8100)
                  .make();
    BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("000011"));
    BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("000011"));
    BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("000011"));
    BOOST_CHECK_EQUAL(base_vj->name, "uri_C");
    BOOST_CHECK_EQUAL(base_vj->headsign, "headsign_C");
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 2);
    // Meta VJ
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    mvj = pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0));
    BOOST_CHECK_EQUAL(mvj->get_label(), "meta_vj_name_A");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 2);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 0);

    // Add new adapted VJ (all validity patterns = 0)
    // clean_up_useless_vjs is called before creating adapted VJ.
    // clean_up_useless_vjs will suppress nothing.
    const auto* adapted_vj = b.vj("line_A", "000000", "block_id_A", false, "uri_adapted", "headsign_adapted",
                                  "meta_vj_name_A", "", nt::RTLevel::Adapted)("stop1", 8000, 8000)("stop2", 8100, 8100)
                                 .make();
    BOOST_CHECK_EQUAL(adapted_vj->base_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(adapted_vj->adapted_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(adapted_vj->rt_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(adapted_vj->name, "uri_adapted");
    BOOST_CHECK_EQUAL(adapted_vj->headsign, "headsign_adapted");
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 3);
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 3);
    // Meta VJ
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_CHECK_EQUAL(mvj->get_label(), "meta_vj_name_A");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 2);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 1);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 0);

    // Add new realtime VJ
    // clean_up_useless_vjs is called before creating realtime VJ.
    // clean_up_useless_vjs will suppress the adapted VJ.
    const auto* rt_vj = b.vj("line_A", "001110", "block_id_A", false, "uri_realtime", "headsign_realtime",
                             "meta_vj_name_A", "", nt::RTLevel::RealTime)("stop1", 8000, 8000)("stop2", 8100, 8100)
                            .make();
    BOOST_CHECK_EQUAL(rt_vj->base_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(rt_vj->adapted_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(rt_vj->rt_validity_pattern()->days, year("001110"));
    BOOST_CHECK_EQUAL(rt_vj->name, "uri_realtime");
    BOOST_CHECK_EQUAL(rt_vj->headsign, "headsign_realtime");
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 3);
    BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 3);
    // Meta VJ
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    BOOST_CHECK_EQUAL(mvj->get_label(), "meta_vj_name_A");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 2);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 1);
}

BOOST_AUTO_TEST_CASE(create_only_rt_vjs_test) {
    using year = navitia::type::ValidityPattern::year_bitset;
    namespace nt = navitia::type;

    // Create a route with a Meta VJ.
    // Inside the MVJ, we add 2 Realtime VJ
    // There is no base VJ inside (Usefull for adding a new trip in Realtime mode)
    ed::builder b("20120614");
    auto& pt_data = *b.data->pt_data;

    const auto* rt_vj = b.vj("line_A", "000011", "block_id_A", false, "uri_A", "headsign_A", "meta_vj_name_A", "",
                             nt::RTLevel::RealTime)("stop1", 8000, 8000)("stop2", 8100, 8100)
                            .make();
    BOOST_CHECK_EQUAL(rt_vj->base_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(rt_vj->adapted_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(rt_vj->rt_validity_pattern()->days, year("000011"));
    const auto* rt_vj2 = b.vj("line_A", "001100", "block_id_A", false, "uri_B", "headsign_B", "meta_vj_name_A", "",
                              nt::RTLevel::RealTime)("stop1", 8000, 8000)("stop2", 8100, 8100)
                             .make();
    BOOST_CHECK_EQUAL(rt_vj2->base_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(rt_vj2->adapted_validity_pattern()->days, year("000000"));
    BOOST_CHECK_EQUAL(rt_vj2->rt_validity_pattern()->days, year("001100"));
    // Meta VJ
    BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
    auto* mvj = pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0));
    BOOST_CHECK_EQUAL(mvj->get_label(), "meta_vj_name_A");
    BOOST_CHECK_EQUAL(mvj->get_base_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_adapted_vj().size(), 0);
    BOOST_CHECK_EQUAL(mvj->get_rt_vj().size(), 2);
}
