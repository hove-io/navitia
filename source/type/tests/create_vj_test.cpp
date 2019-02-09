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
#define BOOST_TEST_MODULE create_vj_test

#include <boost/test/unit_test.hpp>
#include "ed/build_helper.h"
#include "type/pt_data.h"
#include "type/type.h"
#include "tests/utils_test.h"
#include "type/comment_container.h"
#include <memory>
#include <boost/archive/text_oarchive.hpp>
#include <string>
#include <type_traits>


struct logger_initialized {
    logger_initialized()   { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );


BOOST_AUTO_TEST_CASE(create_vj_test) {
    using year = navitia::type::ValidityPattern::year_bitset;
    namespace nt = navitia::type;

    ed::builder b("20120614");
    const auto* base_vj = b.vj("A", "00111110011111")("stop1", 8000, 8000)("stop2", 8100,8100).make();
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
    vp.days = year("0000100" "0000100");
    const auto* adapted_vj = mvj->create_discrete_vj("adapted", nt::RTLevel::Adapted, vp, base_vj->route, sts, pt_data);

    BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("0011111" "0011111"));
    BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("0011011" "0011011"));
    BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("0011011" "0011011"));

    BOOST_CHECK_EQUAL(adapted_vj->base_validity_pattern()->days, year("0000000" "0000000"));
    BOOST_CHECK_EQUAL(adapted_vj->adapted_validity_pattern()->days, year("0000100" "0000100"));
    BOOST_CHECK_EQUAL(adapted_vj->rt_validity_pattern()->days, year("0000100" "0000100"));

    // a bigger delay in rt
    sts.at(1).arrival_time = 8300;
    sts.at(1).departure_time = 8300;
    vp.days = year("0000000" "0000110");
    const auto* rt_vj = mvj->create_discrete_vj("rt", nt::RTLevel::RealTime, vp, base_vj->route, sts, pt_data);

    BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("0011111" "0011111"));
    BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("0011011" "0011011"));
    BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("0011011" "0011001"));

    BOOST_CHECK_EQUAL(adapted_vj->base_validity_pattern()->days, year("0000000" "0000000"));
    BOOST_CHECK_EQUAL(adapted_vj->adapted_validity_pattern()->days, year("0000100" "0000100"));
    BOOST_CHECK_EQUAL(adapted_vj->rt_validity_pattern()->days, year("0000100" "0000000"));

    BOOST_CHECK_EQUAL(rt_vj->base_validity_pattern()->days, year("0000000" "0000000"));
    BOOST_CHECK_EQUAL(rt_vj->adapted_validity_pattern()->days, year("0000000" "0000000"));
    BOOST_CHECK_EQUAL(rt_vj->rt_validity_pattern()->days, year("0000000" "0000110"));
}

BOOST_AUTO_TEST_CASE(clean_up_useless_vjs_test) {
    using year = navitia::type::ValidityPattern::year_bitset;
    namespace nt = navitia::type;

    {
        // clean_up_useless_vjs() is called to clean VJs, with all validity patterns = 0

        ed::builder b("20120614");
        auto& pt_data = *b.data->pt_data;

        // Add new base VJ uri_A (all validity patterns = 0)
        const auto* empty_base_vj = b.vj("A", "000000", "block_id_A", false, "uri_A")("stop1", 8000, 8000)("stop2", 8100, 8100).make();
        auto* mvj = pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0));
        BOOST_CHECK_EQUAL(empty_base_vj->base_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(empty_base_vj->adapted_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(empty_base_vj->rt_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 1);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 1);
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);

        // Add new base VJ uri_B
        const auto* base_vj = b.vj("A", "000001", "block_id_A", false, "uri_B")("stop1", 8000, 8000)("stop2", 8100, 8100).make();
        BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("000001"));
        BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("000001"));
        BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("000001"));
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 2);
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 2);

        // Cleaning VJs with all validity patterns = 0
        // Vj uri_A have to disappear
        mvj->clean_up_useless_vjs(pt_data);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 1);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 1);
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 2);

        // Add new base VJ uri_C
        base_vj = b.vj("A", "000010", "block_id_A", false, "uri_C")("stop1", 8000, 8000)("stop2", 8100, 8100).make();
        BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("000010"));
        BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("000010"));
        BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("000010"));
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 2);

        // All MetaVehiceJourneys are kept, even after suppression of useless VJs...
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 3);
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0))->get_label(), "uri_A");
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(1))->get_label(), "uri_B");
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(2))->get_label(), "uri_C");
    }

    {
        // clean_up_useless_vjs() is not called to clean VJs, with all validity patterns = 0, because it
        // is already called at the end of the "create_discrete_vj" function.
        // This function is use to add a new adapted vj.

        ed::builder b("20120614");
        auto& pt_data = *b.data->pt_data;

        // Add new base VJ uri_A (all validity patterns = 0)
        const auto* empty_base_vj = b.vj("A", "000000", "block_id_A", false, "uri_A")("stop1", 8000, 8000)("stop2", 8100, 8100).make();
        BOOST_CHECK_EQUAL(empty_base_vj->base_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(empty_base_vj->adapted_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(empty_base_vj->rt_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 1);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 1);
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 1);
        // Add new base VJ uri_B
        const auto* base_vj = b.vj("A", "000001", "block_id_A", false, "uri_B")("stop1", 8000, 8000)("stop2", 8100, 8100).make();
        BOOST_CHECK_EQUAL(base_vj->base_validity_pattern()->days, year("000001"));
        BOOST_CHECK_EQUAL(base_vj->adapted_validity_pattern()->days, year("000001"));
        BOOST_CHECK_EQUAL(base_vj->rt_validity_pattern()->days, year("000001"));
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 2);
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 2);

        // Add new adapted VJ
        auto sts = base_vj->stop_time_list;
        sts.at(1).arrival_time = 8200;
        sts.at(1).departure_time = 8200;
        nt::ValidityPattern vp(boost::gregorian::date(2012,06,14));
        vp.days = year("000000");
        // we add an adapted vj in the first MetaVehicleJourney, corresponding to the base VJ with all
        // validity pattern = 0. So the cleaning function will suppress it.
        auto* mvj = pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0));
        const auto* adapted_vj = mvj->create_discrete_vj("adapted", nt::RTLevel::Adapted, vp, base_vj->route, sts, pt_data);
        BOOST_CHECK_EQUAL(adapted_vj->base_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(adapted_vj->adapted_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(adapted_vj->rt_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 2);
        // when we added an adapted vj, we don't create new MetaVehiceJourney, but we hang up this one inside
        // an existing mvj.
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 2);

        // Add new realtime VJ
        sts = base_vj->stop_time_list;
        sts.at(1).arrival_time = 8300;
        sts.at(1).departure_time = 8300;
        nt::ValidityPattern vp2(boost::gregorian::date(2012,06,14));
        vp2.days = year("000000");
        // we add a realtime vj in the first MetaVehicleJourney, corresponding to the adapted VJ with all
        // validity pattern = 0. So the cleaning function will suppress it.
        mvj = pt_data.meta_vjs.get_mut(navitia::Idx<nt::MetaVehicleJourney>(0));
        adapted_vj = mvj->create_discrete_vj("realime", nt::RTLevel::RealTime, vp2, base_vj->route, sts, pt_data);
        BOOST_CHECK_EQUAL(adapted_vj->base_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(adapted_vj->adapted_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(adapted_vj->rt_validity_pattern()->days, year("000000"));
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys_map.size(), 2);
        BOOST_CHECK_EQUAL(pt_data.vehicle_journeys.size(), 2);
        // when we added a realtime vj, we don't create new MetaVehiceJourney, but we hang up this one inside
        // an existing mvj.
        BOOST_CHECK_EQUAL(pt_data.meta_vjs.size(), 2);
    }
}

