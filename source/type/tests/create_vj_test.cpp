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
    logger_initialized()   { init_logger(); }
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
