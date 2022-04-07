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
#define BOOST_TEST_MODULE journey_pattern_container_test

#include "routing/journey_pattern_container.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "type/pt_data.h"
#include <boost/test/unit_test.hpp>

namespace nr = navitia::routing;
namespace nt = navitia::type;

// helper for check_jp_container
static void check_vj(const nr::JourneyPatternContainer& jp_container,
                     const std::pair<nr::JpIdx, const nr::JourneyPattern&>& jp,
                     const nt::VehicleJourney& vj,
                     std::set<const nt::VehicleJourney*>& vjs) {
    // check if each vj is indexed only one time
    BOOST_CHECK_EQUAL(vjs.count(&vj), 0);  // else, same vj in several jp
    vjs.insert(&vj);

    // check jp_from_vj
    BOOST_CHECK_EQUAL(jp_container.get_jp_from_vj()[nr::VjIdx(vj)], jp.first);

    // jpps checks
    nr::RankJourneyPatternPoint order(0);
    BOOST_REQUIRE_EQUAL(jp.second.jpps.size(), vj.stop_time_list.size());
    for (const auto& jpp_idx : jp.second.jpps) {
        const auto& jpp = jp_container.get(jpp_idx);
        BOOST_CHECK_EQUAL(jpp.order, order);      // order is coherent
        BOOST_CHECK_EQUAL(jpp.jp_idx, jp.first);  // jpp.jp_idx is coherent
        BOOST_CHECK_EQUAL(jp_container.get_jpp(get_corresponding_stop_time(vj, order)), jpp_idx);  // st->jpp

        // stop point of the jpp is coherent with the vj
        BOOST_CHECK_EQUAL(jpp.sp_idx, nr::SpIdx(*get_corresponding_stop_time(vj, order).stop_point));
        ++order;
    }
}
// check the consistency of the jp_container. Returns the number of
// (unique) vj in the container to check if every vj is indexed.
static size_t check_jp_container(const nr::JourneyPatternContainer& jp_container) {
    // to check that a vj is not indexed several times
    std::set<const nt::VehicleJourney*> vjs;

    // check jp coherence
    for (const auto jp : jp_container.get_jps()) {
        // check that the jp getter is coherent
        BOOST_CHECK_EQUAL(jp.second, jp_container.get(jp.first));

        // check vj and jp coherence
        jp.second.for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
            check_vj(jp_container, jp, vj, vjs);
            return true;
        });
    }
    // check route coherence
    for (const auto jps_from_route : jp_container.get_jps_from_route()) {
        for (const auto& jp_idx : jps_from_route.second) {
            const auto& jp = jp_container.get(jp_idx);
            BOOST_CHECK_EQUAL(jps_from_route.first, jp.route_idx);  // jp.route_idx coherence

            // vj <-> route coherence
            jp.for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                BOOST_CHECK_EQUAL(jps_from_route.first, nr::RouteIdx(*vj.route));
                return true;
            });
        }
    }
    // check phy_mode coherence
    for (const auto jps_from_phy_mode : jp_container.get_jps_from_phy_mode()) {
        for (const auto& jp_idx : jps_from_phy_mode.second) {
            const auto& jp = jp_container.get(jp_idx);
            BOOST_CHECK_EQUAL(jps_from_phy_mode.first, jp.phy_mode_idx);  // jp.phy_mode_idx coherence

            // vj <-> phy_mode coherence
            jp.for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                BOOST_CHECK_EQUAL(jps_from_phy_mode.first, nr::PhyModeIdx(*vj.physical_mode));
                return true;
            });
        }
    }
    return vjs.size();
}

// a pool of comparable vj => 1 jp
BOOST_AUTO_TEST_CASE(non_overtaking_one_jp) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "8:05"_t, "8:05"_t)("B", "8:15"_t, "8:15"_t)("C", "8:25"_t, "8:25"_t);

    // same as the 2nd, must be in the same jp
    b.vj("1", "000111")("A", "8:05"_t, "8:05"_t)("B", "8:15"_t, "8:15"_t)("C", "8:25"_t, "8:25"_t);

    // it overtakes the others, but the validity pattern doesn't overlap
    b.vj("1", "111000")("A", "7:55"_t, "7:55"_t)("B", "8:15"_t, "8:15"_t)("C", "8:35"_t, "8:35"_t);

    b.make();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    BOOST_CHECK_EQUAL(check_jp_container(jps), 0);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 0);
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 4);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 1);
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 4);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 1);
}

// 2 comparable vjs except one stop time is not at the same stop point => 2 jp
BOOST_AUTO_TEST_CASE(not_same_stop_points) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "8:05"_t, "8:05"_t)("B", "8:15"_t, "8:15"_t)("D", "8:25"_t, "8:25"_t);

    b.make();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

// 2 vjs not on the same route => 2 jp
BOOST_AUTO_TEST_CASE(not_same_route) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("2", "000111")("A", "8:05"_t, "8:05"_t)("B", "8:15"_t, "8:15"_t)("C", "8:25"_t, "8:25"_t);

    b.make();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    BOOST_CHECK_EQUAL(jps.get_jps_from_route().size(), 2);
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

// a vj overtake another => 2 jp
BOOST_AUTO_TEST_CASE(overtaking) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "7:55"_t, "7:55"_t)("B", "8:15"_t, "8:15"_t)("C", "8:35"_t, "8:35"_t);

    b.make();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

// a vj lightly overtake (at the same time on a stop point) another => 2 jp
BOOST_AUTO_TEST_CASE(lightly_overtaking1) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "7:55"_t, "7:55"_t)("B", "8:00"_t, "8:00"_t)("C", "8:20"_t, "8:20"_t);

    b.make();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

// a vj lightly overtake (at the same time on a stop point) another => 2 jp
BOOST_AUTO_TEST_CASE(lightly_overtaking2) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:15"_t, "8:15"_t)("C", "8:35"_t, "8:35"_t);

    b.make();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    jps.load(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}
