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
#define BOOST_TEST_MODULE journey_pattern_container_test

#include "routing/journey_pattern_container.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "type/pt_data.h"
#include <boost/test/unit_test.hpp>

namespace nr = navitia::routing;
namespace nt = navitia::type;

namespace navitia { namespace routing {

std::ostream& operator<<(std::ostream& os, const JourneyPatternPoint& jpp) {
    return os << "Jpp(" << jpp.sp_idx << ", " << jpp.order << ")";
}
std::ostream& operator<<(std::ostream& os, const JourneyPattern& jp) {
    return os << "Jp(" << jp.jpps << ", " << jp.discrete_vjs << ", " << jp.freq_vjs << ")";
}

}} // namespace navitia::rounting

template<typename VJ> static void
check_vjs(const nr::JourneyPatternContainer& jp_container,
          const std::pair<nr::JpIdxT, const nr::JourneyPattern&>& jp,
          const std::vector<const VJ*>& jp_vjs,
          std::set<const nt::VehicleJourney*>& vjs) {
    for (const nt::VehicleJourney* vj: jp_vjs) {
        BOOST_CHECK_EQUAL(vjs.count(vj), 0);// else, same vj in several jp
        vjs.insert(vj);
        uint32_t order = 0;
        BOOST_REQUIRE_EQUAL(jp.second.jpps.size(), vj->stop_time_list.size());
        for (const auto& jpp_idx: jp.second.jpps) {
            const auto& jpp = jp_container.get(jpp_idx);
            BOOST_CHECK_EQUAL(jpp.order, order);
            BOOST_CHECK_EQUAL(jpp.sp_idx,
                              nr::SpIdx(*vj->stop_time_list.at(order).journey_pattern_point->stop_point));
            ++order;
        }
    }
}
static size_t check_jp_container(const nr::JourneyPatternContainer& jp_container) {
    std::set<const nt::VehicleJourney*> vjs;
    for (const auto& jp: jp_container.get_jps()) {
        BOOST_CHECK_EQUAL(jp.second, jp_container.get(jp.first));
        check_vjs(jp_container, jp, jp.second.discrete_vjs, vjs);
        check_vjs(jp_container, jp, jp.second.freq_vjs, vjs);
    }
    for (const auto& jps_from_route: jp_container.get_jps_from_route()) {
        for (const auto& jp_idx: jps_from_route.second) {
            const auto& jp = jp_container.get(jp_idx);
            for (const auto* vj: jp.discrete_vjs) {
                BOOST_CHECK_EQUAL(jps_from_route.first, nr::RouteIdx(*vj->journey_pattern->route));
            }
            for (const auto* vj: jp.freq_vjs) {
                BOOST_CHECK_EQUAL(jps_from_route.first, nr::RouteIdx(*vj->journey_pattern->route));
            }
        }
    }
    return vjs.size();
}

BOOST_AUTO_TEST_CASE(non_overtaking_one_jp) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "8:05"_t, "8:05"_t)("B", "8:15"_t, "8:15"_t)("C", "8:25"_t, "8:25"_t);

    // same as 2, must be in the same jp
    b.vj("1", "000111")("A", "8:05"_t, "8:05"_t)("B", "8:15"_t, "8:15"_t)("C", "8:25"_t, "8:25"_t);

    // it overtake, but the validity pattern doesn't overlap
    b.vj("1", "111000")("A", "7:55"_t, "7:55"_t)("B", "8:15"_t, "8:15"_t)("C", "8:35"_t, "8:35"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    BOOST_CHECK_EQUAL(check_jp_container(jps), 0);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 0);
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 4);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 1);
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 4);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 1);
}

BOOST_AUTO_TEST_CASE(not_same_stop_points) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "8:05"_t, "8:05"_t)("B", "8:15"_t, "8:15"_t)("D", "8:25"_t, "8:25"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

BOOST_AUTO_TEST_CASE(not_same_route) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("2", "000111")("A", "8:05"_t, "8:05"_t)("B", "8:15"_t, "8:15"_t)("C", "8:25"_t, "8:25"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    BOOST_CHECK_EQUAL(jps.get_jps_from_route().size(), 2);
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

BOOST_AUTO_TEST_CASE(overtaking) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "7:55"_t, "7:55"_t)("B", "8:15"_t, "8:15"_t)("C", "8:35"_t, "8:35"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

BOOST_AUTO_TEST_CASE(lightly_overtaking1) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "7:55"_t, "7:55"_t)("B", "8:00"_t, "8:00"_t)("C", "8:20"_t, "8:20"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

BOOST_AUTO_TEST_CASE(lightly_overtaking2) {
    ed::builder b("20150101");
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:10"_t, "8:10"_t)("C", "8:20"_t, "8:20"_t);
    b.vj("1", "000111")("A", "8:00"_t, "8:00"_t)("B", "8:15"_t, "8:15"_t)("C", "8:35"_t, "8:35"_t);

    b.data->pt_data->index();
    b.finish();
    b.data->build_raptor();
    b.data->build_uri();
    const nt::PT_Data& d = *b.data->pt_data;

    nr::JourneyPatternContainer jps;
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
    jps.init(d);
    BOOST_CHECK_EQUAL(check_jp_container(jps), 2);
    BOOST_CHECK_EQUAL(jps.nb_jps(), 2);
}

