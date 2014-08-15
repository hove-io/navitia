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
#define BOOST_TEST_MODULE test_best_stoptime
#include <boost/test/unit_test.hpp>

#include "routing/best_stoptime.cpp"
#include "ed/build_helper.h"
#include "type/type.h"
#include "type/datetime.h"


using namespace navitia;
using namespace navitia::routing;

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
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime sp2_departure = 8150;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A")(spa1, sp1_departure, sp1_departure, 0, false, false)
             (spa2, sp2_arrival, sp2_arrival, 0, false, false);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;

    //SP1
    {
        std::tie(st1, dt1) = earliest_stop_time(jpp1, sp1_departure - 1, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, sp1_departure - 1, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) = earliest_stop_time(jpp1, sp1_departure, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, sp1_departure, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) = earliest_stop_time(jpp1, sp1_departure + 1, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, sp1_departure + 1, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    //SP2
    {
        std::tie(st1, dt1) = earliest_stop_time(jpp2, sp2_departure - 1, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, sp2_departure - 1, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) = earliest_stop_time(jpp2, sp2_departure, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, sp2_departure, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        std::tie(st1, dt1) = earliest_stop_time(jpp2, sp2_arrival + 1, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, sp2_arrival + 1, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
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
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime sp2_departure = 8150;
    DateTime sp3_arrival = 8200;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";
    b.vj("A")(spa1, sp1_departure)
             (spa2, sp2_arrival, sp2_departure)
             (spa3, sp3_arrival);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp3 = b.data->pt_data->stop_areas_map[spa3]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    DateTime dt1, dt2;
    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(DateTimeUtils::date(dt_test), sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, DateTimeUtils::hour(sp1_departure));
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(DateTimeUtils::date(dt_test), sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    //SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }

    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    //SP3
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, dt_test);
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
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
    ed::builder b("20120614");
    DateTime sp1_departure = 86300;
    DateTime sp2_arrival = 86500;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A")(spa1, sp1_departure)
             (spa2, sp2_arrival);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;


    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    //SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 101);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st1 == nullptr);
        BOOST_CHECK(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
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
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 86300;
    DateTime sp2_departure = 86500;
    DateTime sp3_arrival = 86600;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";
    b.vj("A")(spa1, sp1_departure)
             (spa2, sp2_arrival, sp2_departure)
             (spa3, sp3_arrival);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp3 = b.data->pt_data->stop_areas_map[spa3]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;
    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    //SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 101);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 101);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    //SP3
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
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
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A", "1010", "", true)(spa1, sp1_departure, sp1_departure)
                             (spa2, sp2_arrival, sp2_arrival);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;

    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    //SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(2, sp2_arrival - 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
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
    ed::builder b("20120614");
    DateTime sp1_departure1 = 8000;
    DateTime sp2_arrival1 = 8100;
    DateTime sp1_departure2 = 9000;
    DateTime sp2_arrival2 = 9100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A")(spa1, sp1_departure1)
             (spa2, sp2_arrival1);
    b.vj("A")(spa1, sp1_departure2)
                             (spa2, sp2_arrival2);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1;
    uint32_t dt1;
    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1 - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1 + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure2);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure2 - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure2);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure2);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp1_departure2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure2);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure2 + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    //SP2
    {
        DateTime dt_test = DateTimeUtils::set(0, sp2_arrival1 + 1);
        std::tie(st1, dt1) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp2_arrival1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival1);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp2_arrival1);
        std::tie(st1, dt1) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp2_arrival1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival1);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival1 - 1);
        std::tie(st1, dt1) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp2_arrival2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival2);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp2_arrival2 + 1);
        std::tie(st1, dt1) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, sp2_arrival2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival2);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
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
    ed::builder b("20120614");
    DateTime sp1_departure1 = 8000;
    DateTime sp2_arrival1 = 8100;
    DateTime sp1_departure2 = 9000;
    DateTime sp2_arrival2 = 9100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A", "10", "", true)(spa1, sp1_departure1)
                             (spa2, sp2_arrival1);
    b.vj("A", "10", "", true)(spa1, sp1_departure2)
                             (spa2, sp2_arrival2);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;

    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1 - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, sp1_departure1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure1);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    //SP2
    {
        DateTime dt_test = DateTimeUtils::set(2, sp2_arrival1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival2));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival2);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival2 - 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival1));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival1);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
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
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 86300;
    DateTime sp2_departure = 86500;
    DateTime sp3_arrival = 86600;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";
    b.vj("A", "10", "", true)(spa1, sp1_departure)
                             (spa2, sp2_arrival, sp2_departure)
                             (spa3, sp3_arrival);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp3 = b.data->pt_data->stop_areas_map[spa3]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;
    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_CHECK(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    //SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 101);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        //We test if we can find the train leaving after midnight
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 101);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp2_departure));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    //SP3
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK(st1 == nullptr); // No departure because this is a terminus
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st2 == nullptr);
        // There are no trip leaving before
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
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
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 86300;
    DateTime sp2_departure = 86400;
    DateTime sp3_arrival = 86600;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";
    b.vj("A", "10", "", true)(spa1, sp1_departure)
                             (spa2, sp2_arrival, sp2_departure)
                             (spa3, sp3_arrival);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp3 = b.data->pt_data->stop_areas_map[spa3]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    uint32_t dt1, dt2;
    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, sp1_departure));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp1_departure + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_CHECK(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    //SP2
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival - 101);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure - 101);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, 0));
        BOOST_CHECK(st1 != nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_departure);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(2, 0));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st1->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp2_arrival + 1);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp2_arrival));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp2_arrival);
        BOOST_CHECK_EQUAL(st2->departure_time, sp2_departure);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    //SP3
    {
        //The day 0 is unactive
        DateTime dt_test = DateTimeUtils::set(0, sp3_arrival);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->arrival_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
    }
    {
        DateTime dt_test = DateTimeUtils::set(1, sp3_arrival + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(1, sp3_arrival));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->departure_time, sp3_arrival);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
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
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime sp2_departure = 8150;
    DateTime sp3_arrival = 8200;
    DateTime start_time = 6000;
    DateTime end_time = 7000;
    uint32_t headway_sec = 300;
    DateTime last_time = start_time + headway_sec * std::ceil((end_time - start_time)/headway_sec);

    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    std::string spa3 = "stop3";
    b.vj("A")(spa1, sp1_departure)
             (spa2, sp2_arrival, sp2_departure)
             (spa3, sp3_arrival).frequency(start_time, end_time, headway_sec);

    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp3 = b.data->pt_data->stop_areas_map[spa3]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    DateTime dt1, dt2;
    //SP1
    {
        DateTime dt_test = DateTimeUtils::set(0, start_time - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, start_time);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, start_time + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time + headway_sec));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, end_time - (headway_sec) + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, last_time));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, end_time);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, start_time));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        DateTime dt_test = DateTimeUtils::set(0, end_time + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, start_time));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    //SP2
    {
        uint32_t hour = start_time + (sp2_departure - sp1_departure);
        uint32_t hour2 = start_time + (sp2_arrival - sp1_departure);
        DateTime dt_test = DateTimeUtils::set(0, hour - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, hour2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    {
        uint32_t arrival_hour = start_time + (sp2_arrival- sp1_departure);
        uint32_t departure_hour = start_time + (sp2_departure - sp1_departure);
        DateTime dt_test = DateTimeUtils::set(0, departure_hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    {
        uint32_t arrival_hour = start_time + (sp2_arrival- sp1_departure);
        uint32_t departure_hour = start_time + (sp2_departure - sp1_departure);
        DateTime dt_test = DateTimeUtils::set(0, departure_hour + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour + headway_sec));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    {
        uint32_t arrival_hour = 7000;
        uint32_t departure_hour = 7050;
        DateTime dt_test = DateTimeUtils::set(0, arrival_hour - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour - headway_sec));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    {
        uint32_t arrival_hour = 7000;
        uint32_t departure_hour = 7050;
        DateTime dt_test = DateTimeUtils::set(0, arrival_hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    {
        uint32_t arrival_hour = 7000;
        uint32_t departure_hour = 7050;
        DateTime dt_test = DateTimeUtils::set(0, arrival_hour + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, departure_hour));
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, arrival_hour));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa2);
        BOOST_CHECK(st2 == st1);
    }
    //SP3
    {
        uint32_t hour = start_time + (sp3_arrival - sp2_departure);
        DateTime dt_test = DateTimeUtils::set(0, hour - 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_CHECK(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        uint32_t hour = start_time + (sp3_arrival - sp2_departure);
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_CHECK(st1 == nullptr);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        uint32_t hour = start_time + (sp3_arrival - sp1_departure);
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp3, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp3, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, hour));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa3);
    }
}
BOOST_AUTO_TEST_CASE(freq_pam) {
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime start_time = 6000;
    DateTime end_time = DateTimeUtils::SECONDS_PER_DAY + 1000;
    uint32_t headway_sec = 500;
    DateTime last_time = start_time + headway_sec * std::ceil((end_time - start_time)/headway_sec);

    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A")(spa1, sp1_departure)
             (spa2, sp2_arrival).frequency(start_time, end_time, headway_sec);

    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    DateTime dt1, dt2;
    //SP1
    //Depart avant la periode
    {
        uint32_t hour = start_time - 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    //Depart dans la periode
    {
        uint32_t hour = start_time;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    {
        uint32_t hour = start_time + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time + headway_sec));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    //Depart dans la periode apres minuit
    {
        DateTime dt_test = DateTimeUtils::set(0, last_time - headway_sec - 10);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, last_time - headway_sec));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    //Depart à minuit
    {
        DateTime dt_test = DateTimeUtils::set(1, 0);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, 100));
        BOOST_CHECK_EQUAL(dt2, 0);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
        BOOST_CHECK(st2 == nullptr);
    }
    //SP2
    //Arrivee apres la periode
    {
        DateTime dt_test = DateTimeUtils::set(0, end_time + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time + sp2_arrival - sp1_departure));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    //Depart dans la periode
    {
        DateTime dt_test = DateTimeUtils::set(0, start_time + sp2_arrival - sp1_departure + 1);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time + sp2_arrival - sp1_departure));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    //Depart dans la periode apres minuit
    {
        DateTime dt_test = DateTimeUtils::set(0, last_time - headway_sec - 10);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time - 2 * headway_sec + sp2_arrival - sp1_departure ));
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    //Depart à minuit
    {
        DateTime dt_test = DateTimeUtils::set(1, 0);
        std::tie(st1, dt1) = earliest_stop_time(jpp2, dt_test, *(b.data), false, false);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK_EQUAL(dt2, 86100);
        BOOST_CHECK(st1 == nullptr);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
}

/// Tests avec deux fréquences, une passe minuit et pas l'autre
BOOST_AUTO_TEST_CASE(freq_base_pam) {
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    DateTime start_time1 = 6000;
    DateTime end_time1 = 50000;
    DateTime start_time2 = 60000;
    DateTime end_time2 = DateTimeUtils::SECONDS_PER_DAY + 1000;
    uint32_t headway_sec = 500;
    DateTime last_time1 = start_time1 + headway_sec * std::ceil((end_time1 - start_time1)/headway_sec);
    DateTime last_time2 = start_time2 + headway_sec * std::ceil((end_time2 - start_time2)/headway_sec);

    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A")(spa1, sp1_departure)
             (spa2, sp2_arrival).frequency(start_time1, end_time1, headway_sec);
    b.vj("A")(spa1, sp1_departure)
             (spa2, sp2_arrival).frequency(start_time2, end_time2, headway_sec);

    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    auto jpp2 = b.data->pt_data->stop_areas_map[spa2]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime *st1, *st2;
    DateTime dt1, dt2;
    //SP1
    //Depart entre end_time2 et start_time1 (resultat : start_time1)
    {
        uint32_t hour = start_time1 - 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart a start_time1 (resultat : start_time1)
    {
        uint32_t hour = start_time1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart entre start_time1 et end_time1 (resultat : start_time1 + headway_secs)
    {
        uint32_t hour = start_time1 + 1 ;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time1 + headway_sec));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart a end_time1 (resultat : end_time1)
    {
        uint32_t hour = end_time1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, end_time1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart entre end_time1 et start_time2 (resutat : start_time2)
    {
        uint32_t hour = end_time1 + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart à start_time2
    {
        uint32_t hour = start_time2;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }

    // Depart entre start_time2 et 86400 dans la derniere periode de la journee (resultat : 86000)
    {
        uint32_t hour = start_time2 + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, start_time2 + headway_sec));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart entre start_time2 et 86400 dans la apres periode de la journee (resultat : 86500)
    {
        uint32_t hour = 86100;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, 100));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart à 86400 (resultat : 86500)
    {
        uint32_t hour = 86400;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, 100));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart entre 0 et end_time2 (resultat : 86500)
    {
        uint32_t hour = 100;
        DateTime dt_test = DateTimeUtils::set(1, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, 100));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart à last_time2 (resultat : last_time2)
    {
        uint32_t hour = last_time2;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(0, last_time2));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    // Depart à end_time2 (resultat : start_time1 le lendemain)
    {
        uint32_t hour = end_time2;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st1, dt1) = earliest_stop_time(jpp1, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt1, DateTimeUtils::set(1, start_time1));
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    //SP2
    // Arrivee entre end_time2 et start_time1 (resultat : last_time2)
    {
        uint32_t hour = end_time2 + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time2 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee a start_time1 (resultat : start_time1)
    {
        uint32_t hour = start_time1 + sp2_arrival - sp1_departure;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time1 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre start_time1 et end_time1 (resutat : start_time1 + headway_secs)
    {
        uint32_t hour = start_time1 + sp2_arrival - sp1_departure + headway_sec + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time1 + headway_sec + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee a last_time1 (resultat : last_time1)
    {
        uint32_t hour = last_time1 + sp2_arrival - sp1_departure;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time1 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre end_time1 et start_time2 (resultat : last_time1)
    {
        uint32_t hour = end_time1 + sp2_arrival - sp1_departure + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, last_time1 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee a start_time2 (resultat : start_time2)
    {
        uint32_t hour = start_time2 + sp2_arrival - sp1_departure;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time2 + sp2_arrival - sp1_departure));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre start_time2 et 86400 (resultat : start_time2 + headway_secs)
    {
        uint32_t hour = start_time2 + sp2_arrival - sp1_departure + headway_sec + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, DateTimeUtils::set(0, start_time2 + sp2_arrival - sp1_departure + headway_sec));
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre start_time2 et 86400 (resultat : 86100)
    {
        uint32_t hour = 86100;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, 86100);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee a 86400 (resutat : 86100)
    {
        uint32_t hour = 86400;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, 86100);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre 0 et end_time2 (resultat : 86600)
    {
        uint32_t hour = 86610;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, 86600);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
    // Arrivee entre last_time2 et end_time2 (last_time2)
    {
        uint32_t hour = last_time2 + sp2_arrival - sp1_departure + 1;
        DateTime dt_test = DateTimeUtils::set(0, hour);
        std::tie(st2, dt2) = tardiest_stop_time(jpp2, dt_test, *(b.data), false, false);
        BOOST_CHECK_EQUAL(dt2, last_time2 + sp2_arrival - sp1_departure);
        BOOST_REQUIRE(st2 != nullptr);
        BOOST_CHECK_EQUAL(st2->journey_pattern_point->stop_point->stop_area->name, spa2);
    }
}

/// Tests fréquence et pas fréquence sur le meme journey pattern


/**
 * Test calendars
 * ========== ===== =====
 * stop point sp1   sp2
 * ========== ===== =====
 * arrivee          8100
 * ========== ===== =====
 * depart     8000
 * ========== ===== =====
 *
 * Validity pattern: Not valid the first and third day, valid the second and fourth day
 *
 * 2 calendars: one valid for the vj one not valid
 *
 *
 */
BOOST_AUTO_TEST_CASE(test_calendar) {
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A", "1010", "", true)(spa1, sp1_departure, sp1_departure)
                             (spa2, sp2_arrival, sp2_arrival);

    auto cal(new type::Calendar(b.data->meta->production_date.begin()));
    cal->uri="cal1";
    auto associated_cal = new type::AssociatedCalendar();
    associated_cal->calendar = cal;
    b.data->pt_data->meta_vj.begin()->second->associated_calendars.insert({cal->uri, associated_cal});

    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime* st1;
    uint32_t dt1;

    //calendar is not associated to stop time => no answer
    {
        uint32_t hour = sp1_departure - 1;
        std::tie(st1, dt1) = earliest_stop_time(jpp1, hour, *(b.data), cal->uri);
        BOOST_CHECK_EQUAL(dt1, sp1_departure);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
    {
        uint32_t hour = sp1_departure;
        std::tie(st1, dt1) = earliest_stop_time(jpp1, hour, *(b.data), cal->uri);
        BOOST_CHECK_EQUAL(dt1, sp1_departure);
        BOOST_REQUIRE(st1 != nullptr);
        BOOST_CHECK_EQUAL(st1->departure_time, sp1_departure);
        BOOST_CHECK_EQUAL(st1->journey_pattern_point->stop_point->stop_area->name, spa1);
    }
}


/**
 * Test calendars
 * ========== ===== =====
 * stop point sp1   sp2
 * ========== ===== =====
 * arrivee          8100
 * ========== ===== =====
 * depart     8000
 * ========== ===== =====
 *
 * Validity pattern: Not valid the first and third day, valid the second and fourth day
 *
 * no calendar associated => no answer
 *
 *
 */
BOOST_AUTO_TEST_CASE(test_no_calendar) {
    ed::builder b("20120614");
    DateTime sp1_departure = 8000;
    DateTime sp2_arrival = 8100;
    std::string spa1 = "stop1";
    std::string spa2 = "stop2";
    b.vj("A", "1010", "", true)(spa1, sp1_departure, sp1_departure)
                             (spa2, sp2_arrival, sp2_arrival);
    b.finish();
    b.data->pt_data->index();
    b.data->build_uri();
    b.data->build_raptor();

    auto jpp1 = b.data->pt_data->stop_areas_map[spa1]
                ->stop_point_list.front()
                ->journey_pattern_point_list.front();
    const type::StopTime*st1;
    uint32_t dt1;

    const std::string calendar = "calendar_bob";
    //calendar is not associated to stop time => no answer
    {
        uint32_t hour = sp1_departure - 1;
        std::tie(st1, dt1) = earliest_stop_time(jpp1, hour, *(b.data), calendar);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK(st1 == nullptr);
    }
    {
        uint32_t hour = sp1_departure;
        std::tie(st1, dt1) = earliest_stop_time(jpp1, hour, *(b.data), calendar);
        BOOST_CHECK_EQUAL(dt1, 0);
        BOOST_CHECK(st1 == nullptr);
    }
}

