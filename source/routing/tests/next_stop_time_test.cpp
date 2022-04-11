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
#define BOOST_TEST_MODULE next_stop_time_test
#include <boost/test/unit_test.hpp>

#include "routing/next_stop_time.h"
#include "routing/dataraptor.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "type/pt_data.h"
#include "type/datetime.h"

using namespace navitia;
using namespace navitia::routing;

static JppIdx get_first_jpp_idx(const ed::builder& b, const std::string& sa) {
    const auto sp_idx = SpIdx(*b.data->pt_data->stop_areas_map[sa]->stop_point_list.front());
    return b.data->dataRaptor->jpps_from_sp[sp_idx].front().idx;
}

// TESTS SIMPLES
/**
 * Teste dropoff, pickup
 * ========== ===== =====
 * stop_point sp1   sp2
 * ========== ===== =====
 * arrivee          8100
 * ========== ===== =====
 * depart     8000
 * ========== ===== =====
 *
 * Aucun n'a d'heure de départ ni d'arrivée valide
 */

BOOST_AUTO_TEST_CASE(dropoff_pickup) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime sp2_departure = 8150;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20120614", [&](ed::builder& b) {
        // no pickup nor drop off allowed
        b.vj("A")(spa1, sp1_departure, sp1_departure, 0, false, false)(spa2, sp2_arrival, sp2_arrival, 0, false, false);
    });
    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;

    // SP1
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, sp1_departure - 1, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, sp1_departure - 1, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, sp1_departure, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, sp1_departure, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, sp1_departure + 1, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, sp1_departure + 1, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp1, sp1_departure - 1, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, sp1_departure - 1, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp1, sp1_departure, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, sp1_departure, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp1, sp1_departure + 1, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, sp1_departure + 1, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    // SP2
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, sp2_departure - 1, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, sp2_departure - 1, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, sp2_departure, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, sp2_departure, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, sp2_arrival + 1, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, sp2_arrival + 1, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp2, sp2_departure - 1, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp2, sp2_departure - 1, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp2, sp2_departure, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp2, sp2_departure, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp2, sp2_arrival + 1, nt::RTLevel::Base, false);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp2, sp2_arrival + 1, nt::RTLevel::Base, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
}
/**Test sur un cas simple
 *
 *  ========== ===== ===== =====
 *  stop_point sp1   sp2   sp3
 *  ========== ===== ===== =====
 *  arrivee          8100  8200
 *  ========== ===== ===== =====
 *  depart     8000  8150
 *  ========== ===== ===== =====
 *
 *  Pour chaque stop point on demande le prochain départ et la prochaine arrivee
 *  à l'heure -1, l'heure, et l'heure + 1.
 *  A SP1, on ne veut pas d'arrivée, à SP3, on ne veut pas de départ.
 *  À chaque demande a l'heure + 1, on veut le trajet le lendemain.target
 */
BOOST_AUTO_TEST_CASE(base) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime sp2_departure = 8150;
    DateTime sp3_arrival = 8200;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A")(spa1, sp1_departure)(spa2, sp2_arrival, sp2_departure)(spa3, sp3_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    auto jpp3 = get_first_jpp_idx(b, spa3);
    const type::StopTime *st1, *st2;
    DateTime dt1, dt2;
    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(DateTimeUtils::date(dt_test), sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, DateTimeUtils::hour(sp1_departure));
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(DateTimeUtils::date(dt_test), sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    // SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }

    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // SP3
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, dt_test);
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
}

/**
 * Cas de passe minuit avec changement d'heure entre les deux arrêts
 *
 * ========== =============== ================
 * stop point sp1             sp2
 * ========== =============== ================
 * arrivee                    (24*3600) + 100
 * ========== =============== ================
 * depart     (24*3600) - 100
 * ========== =============== ================
 * Même cas que pour le test précédent.
 */
BOOST_AUTO_TEST_CASE(passe_minuit_1) {
    DateTime sp1_departure = 86300;
    DateTime sp2_arrival = 86500;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20120614", [&](ed::builder& b) { b.vj("A")(spa1, sp1_departure)(spa2, sp2_arrival); });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;

    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    // SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 101);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        // we ask for the previous departure from 23:59 the day 1
        // the previous departure is the day 1, at 100 (same as the day 0 at 86400 + 100)
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
}

/**
 * Cas de passe minuit avec changement d'heure entre l'heure d'arrivée et de départ
 *BOOST_REQUIRE
 * ========== =============== ================ ================
 * stop point sp1             sp2              sp3
 * ========== =============== ================ ================
 * arrivee                    (24*3600) - 100  (24*3600) + 200
 * ========== =============== ================ ================
 * depart     8000            (24*3600) + 100
 * ========== =============== ================ ================
 * Même cas que pour le test précédent.
 */

BOOST_AUTO_TEST_CASE(passe_minuit_2) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 86300;
    DateTime sp2_departure = 86500;
    DateTime sp3_arrival = 86600;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A")(spa1, sp1_departure)(spa2, sp2_arrival, sp2_departure)(spa3, sp3_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    auto jpp3 = get_first_jpp_idx(b, spa3);
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;
    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    // SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 101);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 101);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    // SP3
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
}
/**
 * Test validity pattern
 * ========== ===== =====
 * stop point sp1   sp2
 * ========== ===== =====
 * arrivee          8100
 * ========== ===== =====
 * depart     8000
 * ========== ===== =====
 *
 * Validity pattern : Non valide le premier jour, valide le deuxieme jour,
 * invalide le troisieme jour, valide le 4eme
 *
 * On teste le premier jour, doit renvoyer un les horaires le lendemain
 * On test le deuxième jour, doit renvoyer les horaires le deuxieme jour
 */
BOOST_AUTO_TEST_CASE(base_vp) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A", "1010", "", true)(spa1, sp1_departure, sp1_departure)(spa2, sp2_arrival, sp2_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;

    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(2, sp2_arrival - 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
}

/**
 * Test deux vj
 *
 * VJ 1
 * ========== ===== =====
 * stop point sp1   sp2
 * ========== ===== =====
 * arrivee          8100
 * ========== ===== =====
 * depart     8000
 * ========== ===== =====
 *
 * VJ 2
 * ========== ===== =====
 * stop point sp1   sp2
 * ========== ===== =====
 * arrivee          9100
 * ========== ===== =====
 * depart     9000
 * ========== ===== =====
 *
 * Validity pattern : Non valide le premier jour, valide le deuxieme jour
 *
 * On teste le premier jour, doit renvoyer un les horaires le lendemain
 * On teste le deuxième jour, doit renvoyer les horaires le deuxieme jour
 */
BOOST_AUTO_TEST_CASE(vj2) {
    DateTime sp1_departure1 = 8000;
    DateTime sp2_arrival1 = 8100;
    DateTime sp1_departure2 = 9000;
    DateTime sp2_arrival2 = 9100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A")(spa1, sp1_departure1)(spa2, sp2_arrival1);
        b.vj("A")(spa1, sp1_departure2)(spa2, sp2_arrival2);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime* st1;
    uint32_t dt1;
    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1 - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1 + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure2);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure2 - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure2);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure2);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure2);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure2 + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // SP2
    {
        DateTime dt_test = DateTimeUtils::set(0, sp2_arrival1 + 1);
        std::tie(st1, dt1) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp2_arrival1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival1);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp2_arrival1);
        std::tie(st1, dt1) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp2_arrival1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival1);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival1 - 1);
        std::tie(st1, dt1) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp2_arrival2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival2);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp2_arrival2 + 1);
        std::tie(st1, dt1) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp2_arrival2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival2);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
}

/**
 * Test validity pattern et deux vj
 *
 * VJ 1
 * ========== ===== =====
 * stop point sp1   sp2
 * ========== ===== =====
 * arrivee          8100
 * ========== ===== =====
 * depart     8000
 * ========== ===== =====
 *
 * VJ 2
 * ========== ===== =====
 * stop point sp1   sp2
 * ========== ===== =====
 * arrivee          9100
 * ========== ===== =====
 * depart     9000
 * ========== ===== =====
 *
 * Validity pattern : Non valide le premier jour, valide le deuxieme jour
 *
 * On teste le premier jour, doit renvoyer un les horaires le lendemain
 * On teste le deuxième jour, doit renvoyer les horaires le deuxieme jour
 */

BOOST_AUTO_TEST_CASE(vp2) {
    DateTime sp1_departure1 = 8000;
    DateTime sp2_arrival1 = 8100;
    DateTime sp1_departure2 = 9000;
    DateTime sp2_arrival2 = 9100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A", "10", "", true)(spa1, sp1_departure1)(spa2, sp2_arrival1);
        b.vj("A", "10", "", true)(spa1, sp1_departure2)(spa2, sp2_arrival2);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;

    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1 - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // SP2
    {
        DateTime dt_test = DateTimeUtils::set(2, sp2_arrival1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival2));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival2);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival2 - 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival1));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival1);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
}
/**
 * Cas de passe minuit avec changement d'heure entre l'heure d'arrivée et de départ
 *
 * ========== =============== ================ ================
 * stop point sp1             sp2              sp3
 * ========== =============== ================ ================
 * arrivee                    (24*3600) - 100  (24*3600) + 200
 * ========== =============== ================ ================
 * depart     8000            (24*3600) + 100
 * ========== =============== ================ ================
 * Même cas que pour le test précédent.
 */

BOOST_AUTO_TEST_CASE(passe_minuit_2_vp) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 86300;
    DateTime sp2_departure = 86500;
    DateTime sp3_arrival = 86600;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A", "10", "", true)(spa1, sp1_departure)(spa2, sp2_arrival, sp2_departure)(spa3, sp3_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    auto jpp3 = get_first_jpp_idx(b, spa3);
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;
    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_CHECK(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    // SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 101);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        // We test if we can find the train leaving after midnight
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 101);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // SP3
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK(st1 == nullptr);  // No departure because this is a terminus
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st2 == nullptr);
        // There are no trip leaving before
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
}

/**
 * This one is an overmidnight, plus we leave from a stop point at exactly
 * midnight
 *
 * ========== =============== ================ ================
 * stop point sp1             sp2              sp3
 * ========== =============== ================ ================
 * arrivee                    (24*3600) - 100  (24*3600) + 200
 * ========== =============== ================ ================
 * depart     8000            (24*3600)
 * ========== =============== ================ ================
 * Même cas que pour le test précédent.
 */
BOOST_AUTO_TEST_CASE(passe_minuit_3_vp) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 86300;
    DateTime sp2_departure = 86400;
    DateTime sp3_arrival = 86600;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.vj("A", "10", "", true)(spa1, sp1_departure)(spa2, sp2_arrival, sp2_departure)(spa3, sp3_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    auto jpp3 = get_first_jpp_idx(b, spa3);
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;
    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_CHECK(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    // SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 101);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 101);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, 0));
        BOOST_CHECK(st1 != nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, 0));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // SP3
    {
        // The day 0 is unactive
        DateTime dt_test = DateTimeUtils::set(0, sp3_arrival);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
}

// TEST EN FREQUENCE Mêmes tests, mais en frequence.

/** Test sur un cas simple
 *
 *  ========== ===== ===== =====
 *  stop_point sp1   sp2   sp3
 *  ========== ===== ===== =====
 *  arrivee          8100  8200
 *  ========== ===== ===== =====
 *  depart     8000  8150
 *  ========== ===== ===== =====
 *
 *  Start time : 6000
 *  End time : 7000
 *  Headway : 300
 *
 *  Pour chaque stop point on demande le prochain départ et la prochaine arrivee
 *  à l'heure -1, l'heure, et l'heure + 1.
 *  A SP1, on ne veut pas d'arrivée, à SP3, on ne veut pas de départ.
 *  À chaque demande a l'heure + 1, on veut le trajet le lendemain.target
 */
BOOST_AUTO_TEST_CASE(freq_base) {
    DateTime sp1_departure = 000;
    DateTime sp2_arrival = 100;
    DateTime sp2_departure = 150;
    DateTime sp3_arrival = 200;
    DateTime start_time = 6000;
    DateTime end_time = 7000;
    uint32_t headway_sec = 300;
    DateTime last_time = start_time + headway_sec * std::ceil((end_time - start_time) / headway_sec);
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.frequency_vj("A", start_time, end_time, headway_sec)(spa1, sp1_departure)(spa2, sp2_arrival, sp2_departure)(
            spa3, sp3_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    auto jpp3 = get_first_jpp_idx(b, spa3);
    const type::StopTime *st1, *st2;
    DateTime dt1, dt2;
    // SP1
    {
        DateTime dt_test = DateTimeUtils::set(0, start_time - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, start_time);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, start_time + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time + headway_sec));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, end_time - (headway_sec) + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, last_time));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, end_time);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, start_time));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, end_time + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, start_time));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    // SP2
    {
        uint32_t hour = start_time + (sp2_departure - sp1_departure);
        uint32_t hour2 = start_time + (sp2_arrival - sp1_departure);
        DateTime dt_test = DateTimeUtils::set(0, hour - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, hour2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
    }
    {
        uint32_t arrival_hour = start_time + (sp2_arrival - sp1_departure);
        uint32_t departure_hour = start_time + (sp2_departure - sp1_departure);
        DateTime dt_test = DateTimeUtils::set(0, departure_hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    {
        uint32_t arrival_hour = start_time + (sp2_arrival - sp1_departure);
        uint32_t departure_hour = start_time + (sp2_departure - sp1_departure);
        DateTime dt_test = DateTimeUtils::set(0, departure_hour + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour + headway_sec));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    {
        uint32_t arrival_hour = 7000;
        uint32_t departure_hour = 7050;
        DateTime dt_test = DateTimeUtils::set(0, arrival_hour - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour - headway_sec));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    {
        uint32_t arrival_hour = 7000;
        uint32_t departure_hour = 7050;
        DateTime dt_test = DateTimeUtils::set(0, arrival_hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    {
        uint32_t arrival_hour = 7000;
        uint32_t departure_hour = 7050;
        DateTime dt_test = DateTimeUtils::set(0, arrival_hour + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    // SP3
    {
        uint32_t hour = start_time + (sp3_arrival - sp2_departure);
        DateTime dt_test = DateTimeUtils::set(0, hour - 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_CHECK(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        uint32_t hour = start_time + (sp3_arrival - sp2_departure);
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_CHECK(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        uint32_t hour = start_time + (sp3_arrival - sp1_departure);
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp3, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, hour));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa3);
    }
}
BOOST_AUTO_TEST_CASE(freq_pam) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime start_time = 6000;
    DateTime end_time = DateTimeUtils::SECONDS_PER_DAY + 1000;
    uint32_t headway_sec = 500;
    DateTime last_time = start_time + headway_sec * std::ceil((end_time - start_time) / headway_sec);
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.frequency_vj("A", start_time, end_time, headway_sec)(spa1, sp1_departure)(spa2, sp2_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime *st1, *st2;
    DateTime dt1, dt2;
    // SP1
    // Depart avant la periode
    {
        uint32_t hour = start_time - 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    // Depart dans la periode
    {
        uint32_t hour = start_time;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        uint32_t hour = start_time + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time + headway_sec));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    // Depart dans la periode apres minuit
    {
        DateTime dt_test = DateTimeUtils::set(0, last_time - headway_sec - 10);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, last_time - headway_sec));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    // Depart à minuit
    {
        DateTime dt_test = DateTimeUtils::set(1, 0);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, 100));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::inf);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    // SP2
    // Arrivee apres la periode
    {
        DateTime dt_test = DateTimeUtils::set(0, end_time + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time + sp2_arrival - sp1_departure));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Depart dans la periode
    {
        DateTime dt_test = DateTimeUtils::set(0, start_time + sp2_arrival - sp1_departure + 1);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time + sp2_arrival - sp1_departure));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Depart dans la periode apres minuit
    {
        DateTime dt_test = DateTimeUtils::set(0, last_time - headway_sec - 10);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time - 2 * headway_sec + sp2_arrival - sp1_departure));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Depart à minuit
    {
        DateTime dt_test = DateTimeUtils::set(1, 0);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::inf);
        BOOST_CHECK_EQUAL(dt2, 86100);
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
}

/// Tests avec deux fréquences, une passe minuit et pas l'autre
BOOST_AUTO_TEST_CASE(freq_base_pam) {
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime start_time1 = 6000;
    DateTime end_time1 = 50000;
    DateTime start_time2 = 60000;
    DateTime end_time2 = DateTimeUtils::SECONDS_PER_DAY + 1000;
    uint32_t headway_sec = 500;
    DateTime last_time1 = start_time1 + headway_sec * std::ceil((end_time1 - start_time1) / headway_sec);
    DateTime last_time2 = start_time2 + headway_sec * std::ceil((end_time2 - start_time2) / headway_sec);

    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20120614", [&](ed::builder& b) {
        b.frequency_vj("A", start_time1, end_time1, headway_sec)(spa1, sp1_departure)(spa2, sp2_arrival);
        b.frequency_vj("A", start_time2, end_time2, headway_sec)(spa1, sp1_departure)(spa2, sp2_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime *st1, *st2;
    DateTime dt1, dt2;
    // SP1
    // Depart entre end_time2 et start_time1 (resultat : start_time1)
    {
        uint32_t hour = start_time1 - 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart a start_time1 (resultat : start_time1)
    {
        uint32_t hour = start_time1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart entre start_time1 et end_time1 (resultat : start_time1 + headway_secs)
    {
        uint32_t hour = start_time1 + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time1 + headway_sec));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart a end_time1 (resultat : end_time1)
    {
        uint32_t hour = end_time1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, end_time1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart entre end_time1 et start_time2 (resutat : start_time2)
    {
        uint32_t hour = end_time1 + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart à start_time2
    {
        uint32_t hour = start_time2;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }

    // Depart entre start_time2 et 86400 dans la derniere periode de la journee (resultat : 86000)
    {
        uint32_t hour = start_time2 + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time2 + headway_sec));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart entre start_time2 et 86400 dans la apres periode de la journee (resultat : 86500)
    {
        uint32_t hour = 86100;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, 100));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart à 86400 (resultat : 86500)
    {
        uint32_t hour = 86400;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, 100));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart entre 0 et end_time2 (resultat : 86500)
    {
        uint32_t hour = 100;
        DateTime dt_test = DateTimeUtils::set(1, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, 100));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart à last_time2 (resultat : last_time2)
    {
        uint32_t hour = last_time2;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, last_time2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // Depart à end_time2 (resultat : start_time1 le lendemain)
    {
        uint32_t hour = end_time2;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, start_time1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->stop_point->stop_area->name, spa1);
    }
    // SP2
    // Arrivee entre end_time2 et start_time1 (resultat : last_time2)
    {
        uint32_t hour = end_time2 + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time2 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee a start_time1 (resultat : start_time1)
    {
        uint32_t hour = start_time1 + sp2_arrival - sp1_departure;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time1 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre start_time1 et end_time1 (resutat : start_time1 + headway_secs)
    {
        uint32_t hour = start_time1 + sp2_arrival - sp1_departure + headway_sec + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time1 + headway_sec + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee a last_time1 (resultat : last_time1)
    {
        uint32_t hour = last_time1 + sp2_arrival - sp1_departure;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time1 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre end_time1 et start_time2 (resultat : last_time1)
    {
        uint32_t hour = end_time1 + sp2_arrival - sp1_departure + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time1 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee a start_time2 (resultat : start_time2)
    {
        uint32_t hour = start_time2 + sp2_arrival - sp1_departure;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time2 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre start_time2 et 86400 (resultat : start_time2 + headway_secs)
    {
        uint32_t hour = start_time2 + sp2_arrival - sp1_departure + headway_sec + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time2 + sp2_arrival - sp1_departure + headway_sec));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre start_time2 et 86400 (resultat : 86100)
    {
        uint32_t hour = 86100;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, 86100);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee a 86400 (resutat : 86100)
    {
        uint32_t hour = 86400;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, 86100);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre 0 et end_time2 (resultat : 86600)
    {
        uint32_t hour = 86610;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, 86600);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre last_time2 et end_time2 (last_time2)
    {
        uint32_t hour = last_time2 + sp2_arrival - sp1_departure + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt2, last_time2 + sp2_arrival - sp1_departure);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->stop_point->stop_area->name, spa2);
    }
}

using boost::gregorian::date;
/*
 * test get_next_departure for frequency VJ
 * */
struct classic_freq_dataset {
    // the vj is from 10:00 to 15:00 every 20mn
    // it leaves st 30s after
    // so it leaves st from 10:00:30 to 15:00:30 every 20mn
    classic_freq_dataset() {
        vp = type::ValidityPattern(date(2012, 7, 7), "11");

        vj.validity_patterns[type::RTLevel::Base] = &vp;
        vj.start_time = 10 * 60 * 60;
        vj.end_time = 15 * 60 * 60;
        vj.headway_secs = 20 * 60;

        st.departure_time = 30;
        st.boarding_time = 30;
    }

    type::FrequencyVehicleJourney vj;
    type::StopTime st;
    type::ValidityPattern vp;
};

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_classic_case_before_period, classic_freq_dataset) {
    DateTime dt = DateTimeUtils::set(1, 8 * 60 * 60);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 10 * 60 * 60 + 30));
}

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_classic_case_after_period, classic_freq_dataset) {
    DateTime dt = DateTimeUtils::set(1, 17 * 60 * 60);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::inf);
}

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_classic_case_in_period, classic_freq_dataset) {
    // 14:00:30 is in period and the next stop will be 14:00:30
    DateTime dt = DateTimeUtils::set(1, 14 * 60 * 60 + 30);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 14 * 60 * 60 + 30));
}

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_classic_case_in_period_2, classic_freq_dataset) {
    // 14:00:31 is in period and the next stop will be 14:20:30
    DateTime dt = DateTimeUtils::set(1, 14 * 60 * 60 + 30 + 1);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 14 * 60 * 60 + 20 * 60 + 30));
}

struct midnight_freq_dataset {
    // the vj is from 17:00 to 10:00 every 20mn
    // it leaves st 30s after
    midnight_freq_dataset() {
        vp = type::ValidityPattern(date(2012, 7, 7), "11");

        vj.validity_patterns[type::RTLevel::Base] = &vp;
        vj.start_time = 17 * 60 * 60;
        vj.end_time = 10 * 60 * 60;
        vj.headway_secs = 20 * 60;

        st.departure_time = 30;
        st.boarding_time = 30;
    }

    type::FrequencyVehicleJourney vj;
    type::StopTime st;
    type::ValidityPattern vp;
};

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_overnight_before_start_after_end, midnight_freq_dataset) {
    DateTime dt = DateTimeUtils::set(1, 16 * 60 * 60);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    // next departure is the beginning of the service
    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 17 * 60 * 60 + 30));
}

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_overnight_before_midnight, midnight_freq_dataset) {
    // the next one after 23:10 is 23:20
    DateTime dt = DateTimeUtils::set(1, 23 * 60 * 60 + 10 * 60);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 23 * 60 * 60 + 20 * 60 + 30));
}

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_overnight_before_end, midnight_freq_dataset) {
    // it's 5:08:30, the next one is 5:20:30, but we have to check the day before (which is ok is this case)
    DateTime dt = DateTimeUtils::set(1, 5 * 60 * 60 + 8 * 60 + 30);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);
    std::cout << "rs:" << str(next_dt) << std::endl;

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 5 * 60 * 60 + 20 * 60 + 30));
}

struct midnight_freq_dataset_no_valid_first_day {
    // same as midnight_freq_dataset, but not valid on the first day
    midnight_freq_dataset_no_valid_first_day() {
        vp = type::ValidityPattern(date(2012, 7, 7), "10");

        vj.validity_patterns[type::RTLevel::Base] = &vp;
        vj.start_time = 17 * 60 * 60;
        vj.end_time = 10 * 60 * 60;
        vj.headway_secs = 20 * 60;

        st.departure_time = 30;
        st.boarding_time = 30;
    }

    type::FrequencyVehicleJourney vj;
    type::StopTime st;
    type::ValidityPattern vp;
};

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_overnight_before_end_no_valid, midnight_freq_dataset_no_valid_first_day) {
    // it's 5:08, the next one is 5:20, but we have to check the day before
    // since the day before was not valid, we have to wait for the begining of the service
    DateTime dt = DateTimeUtils::set(1, 5 * 60 * 60 + 8 * 60 + 30);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 17 * 60 * 60 + 30));
}

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_overnight_after_end_no_valid, midnight_freq_dataset_no_valid_first_day) {
    // it's 10:08, the next one is 17:20, and we don't have to check the day before
    DateTime dt = DateTimeUtils::set(1, 10 * 60 * 60 + 8 * 60 + 30);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 17 * 60 * 60 + 30));
}

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_overnight_begin_service, midnight_freq_dataset_no_valid_first_day) {
    // it's 17:00:30, the next one is 17:00:30, same time, and we don't have to check the day before
    DateTime dt = DateTimeUtils::set(1, 17 * 60 * 60 + 30);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 17 * 60 * 60 + 30));
}

BOOST_FIXTURE_TEST_CASE(test_get_next_dep_overnight_midnight_case, midnight_freq_dataset_no_valid_first_day) {
    // it's midnight, the next one is 00:00:30, so we have to check the day before
    // since the day before is not valid, we have to wait for the begining of the service
    DateTime dt = DateTimeUtils::set(1, 0);

    auto next_dt = get_next_stop_time(StopEvent::pick_up, dt, vj, st);

    BOOST_REQUIRE_EQUAL(next_dt, DateTimeUtils::set(1, 17 * 60 * 60 + 30));
}

/*
 * To be able to return stop_schedules requests with duration of multiple days we need to check that earliest
 * stop time is able to search further than the next day.
 * This is a line circulating only on sundays.
 */
BOOST_AUTO_TEST_CASE(next_stop_time_with_distant_bounds) {
    DateTime sp1_departure = "10:00:00"_t;
    DateTime sp2_arrival = "10:30:00"_t;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20180107", [&](ed::builder& b) {
        b.vj("A", "10000001", "")(spa1, sp1_departure, sp1_departure)(spa2, sp2_arrival, sp2_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime* st;
    uint32_t dt;

    {
        // A bound in the past give no results
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure - 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(0, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(0, sp2_arrival - 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::drop_off, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(0, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // By default we only look for today and tomorrow so no results
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(0, sp2_arrival + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // Looking for the next 6 days should yield no results too
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(7, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(0, sp2_arrival + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(7, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // Looking for the next 7 days but with a bound just before the departure should yield no results too
        // Even if we are going to look for a departure on the right day we should ignore it with the bound
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       false, DateTimeUtils::set(7, sp1_departure - 1));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(0, sp2_arrival + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       false, DateTimeUtils::set(7, sp2_arrival - 1));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // Looking for the next 7 days with a bound on the dot should work.
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       false, DateTimeUtils::set(7, sp1_departure));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp1_departure));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(0, sp2_arrival + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       false, DateTimeUtils::set(7, sp2_arrival));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp2_arrival));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->departure_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
    {
        // Look for a departure today and in the next 7 days should return the departure from the next sunday
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(8, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp1_departure));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(0, sp2_arrival + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(8, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp2_arrival));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->departure_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
    {
        // By default we only look today and yesterday so no results
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties());
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // Looking for the previous 6 days should yield no results too
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(1, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(1, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // Looking for the previous 7 days but with a bound just after the departure should yield no results too
        // Even if we are going to look for a departure on the right day we should ignore it with the bound
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       false, DateTimeUtils::set(0, sp1_departure + 1));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       false, DateTimeUtils::set(0, sp2_arrival + 1));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // Looking for the previous 7 days but with a bound on the dot.
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       false, DateTimeUtils::set(0, sp1_departure));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp1_departure));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       false, DateTimeUtils::set(0, sp2_arrival));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->departure_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
    {
        // Look for a departure today and the previous 7 days should return the departure from the first sunday
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(0, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp1_departure));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), false, DateTimeUtils::set(0, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->departure_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
}

BOOST_AUTO_TEST_CASE(freq_next_stop_time_with_distant_bounds) {
    DateTime sp1_departure = "10:00:00"_t;
    DateTime sp2_arrival = "10:30:00"_t;
    DateTime start_time = sp1_departure;
    DateTime end_time = "11:00:00"_t;
    uint32_t headway_sec = 3600;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";

    ed::builder b("20180107", [&](ed::builder& b) {
        b.frequency_vj("A", start_time, end_time, headway_sec, "", "10000001")(spa1, sp1_departure)(spa2, sp2_arrival);
    });

    NextStopTime next_st(*b.data);

    auto jpp1 = get_first_jpp_idx(b, spa1);
    auto jpp2 = get_first_jpp_idx(b, spa2);
    const type::StopTime* st;
    DateTime dt;

    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true);
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp1_departure + headway_sec));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(0, sp2_arrival + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true);
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp2_arrival + headway_sec));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
    {
        // Looking for a departure just after the end of the frequency without bound. No result because by
        // default we only check on today and the next day.
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + headway_sec + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true);
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(0, sp2_arrival + headway_sec + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true);
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // No departure until saturday either
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + headway_sec + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true, DateTimeUtils::set(7, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(0, sp2_arrival + headway_sec + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true, DateTimeUtils::set(7, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // With a bound just before the start of the frequency no results
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + headway_sec + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       true, DateTimeUtils::set(7, sp1_departure - 1));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(0, sp2_arrival + headway_sec + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       true, DateTimeUtils::set(7, sp2_arrival - 1));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        // With a bound on the start of the frequency we have one
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + headway_sec + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       true, DateTimeUtils::set(7, sp1_departure));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp1_departure));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(0, sp2_arrival + headway_sec + 1);
        std::tie(st, dt) =
            next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       true, DateTimeUtils::set(7, sp2_arrival));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp2_arrival));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure + headway_sec + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true, DateTimeUtils::set(8, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp1_departure));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(0, sp2_arrival + headway_sec + 1);
        std::tie(st, dt) = next_st.earliest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true, DateTimeUtils::set(8, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp2_arrival));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure + headway_sec - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true);
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp1_departure));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(7, sp2_arrival + headway_sec - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true);
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(7, sp2_arrival));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true);
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true);
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true, DateTimeUtils::set(1, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true, DateTimeUtils::set(1, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       true, DateTimeUtils::set(0, sp1_departure + headway_sec + 1));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       true, DateTimeUtils::set(0, sp2_arrival + headway_sec + 1));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::inf);
        BOOST_REQUIRE(st == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       true, DateTimeUtils::set(0, sp1_departure + headway_sec));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp1_departure + headway_sec));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) =
            next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base, nt::VehicleProperties(),
                                       true, DateTimeUtils::set(0, sp2_arrival + headway_sec));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp2_arrival + headway_sec));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(7, sp1_departure - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::pick_up, jpp1, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true, DateTimeUtils::set(0, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp1_departure + headway_sec));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa1);

        dt_test = DateTimeUtils::set(7, sp2_arrival - 1);
        std::tie(st, dt) = next_st.tardiest_stop_time(StopEvent::drop_off, jpp2, dt_test, nt::RTLevel::Base,
                                                      nt::VehicleProperties(), true, DateTimeUtils::set(0, 0));
        BOOST_CHECK_EQUAL(dt, DateTimeUtils::set(0, sp2_arrival + headway_sec));
        BOOST_REQUIRE(st != nullptr);
        BOOST_CHECK_EQUAL(st->stop_point->stop_area->name, spa2);
    }
}
