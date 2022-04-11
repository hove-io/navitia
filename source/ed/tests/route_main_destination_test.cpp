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
#define BOOST_TEST_MODULE route_main_destination_test
#include <boost/test/unit_test.hpp>
#include "ed/data.h"
#include "ed/types.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(route_main_destination) {
    /*
    Route1
        VJ1         VJ2     VJ3
        A1          A1      A1
        A2          A2      A2
        A3          A3      A3
                    A4
    */
    ed::Data data;
    data.meta.production_date = {"20140101"_d, "20150101"_d};
    data.tz_wrapper.tz_handler =
        navitia::type::TimeZoneHandler{"utc", "20140101"_d, {{0, {data.meta.production_date}}}};
    auto* route = new ed::types::Route();
    route->idx = 0;
    data.routes.push_back(route);

    auto* vp1 = new navitia::type::ValidityPattern(data.meta.production_date.begin(), "0001");
    data.validity_patterns.push_back(vp1);

    auto* vj1 = new ed::types::VehicleJourney();
    vj1->validity_pattern = vp1;
    data.vehicle_journeys.push_back(vj1);
    auto* vj2 = new ed::types::VehicleJourney();
    vj2->validity_pattern = vp1;
    data.vehicle_journeys.push_back(vj2);
    auto* vj3 = new ed::types::VehicleJourney();
    vj3->validity_pattern = vp1;
    data.vehicle_journeys.push_back(vj3);

    vj1->route = route;
    vj2->route = route;
    vj3->route = route;

    auto st1 = new ed::types::StopTime();
    st1->arrival_time = 10 * 3600 + 15 * 60;
    st1->departure_time = 10 * 3600 + 15 * 60;
    data.stops.push_back(st1);
    vj1->stop_time_list.push_back(st1);
    vj2->stop_time_list.push_back(st1);
    vj3->stop_time_list.push_back(st1);

    auto st2 = new ed::types::StopTime();
    st2->arrival_time = 11 * 3600 + 10 * 60;
    st2->departure_time = 11 * 3600 + 10 * 60;
    data.stops.push_back(st2);
    vj1->stop_time_list.push_back(st2);
    vj2->stop_time_list.push_back(st2);
    vj3->stop_time_list.push_back(st2);

    auto st3 = new ed::types::StopTime();
    st3->arrival_time = 11 * 3600 + 10 * 60;
    st3->departure_time = 11 * 3600 + 10 * 60;
    data.stops.push_back(st3);
    vj1->stop_time_list.push_back(st3);
    vj2->stop_time_list.push_back(st3);
    vj3->stop_time_list.push_back(st3);

    auto st4 = new ed::types::StopTime();
    st4->arrival_time = 12 * 3600 + 10 * 60;
    st4->departure_time = 12 * 3600 + 10 * 60;
    data.stops.push_back(st4);
    vj2->stop_time_list.push_back(st4);

    st1->stop_point = new ed::types::StopPoint();
    st1->stop_point->name = "A1";
    data.stop_points.push_back(st1->stop_point);
    st1->stop_point->stop_area = new ed::types::StopArea();
    st1->stop_point->stop_area->name = st1->stop_point->name;
    st1->stop_point->stop_area->idx = 0;
    data.stop_areas.push_back(st1->stop_point->stop_area);

    st2->stop_point = new ed::types::StopPoint();
    st2->stop_point->name = "A2";
    data.stop_points.push_back(st2->stop_point);
    st2->stop_point->stop_area = new ed::types::StopArea();
    st2->stop_point->stop_area->name = st2->stop_point->name;
    st2->stop_point->stop_area->idx = 1;
    data.stop_areas.push_back(st2->stop_point->stop_area);

    st3->stop_point = new ed::types::StopPoint();
    st3->stop_point->name = "A3";
    data.stop_points.push_back(st3->stop_point);
    st3->stop_point->stop_area = new ed::types::StopArea();
    st3->stop_point->stop_area->name = st3->stop_point->name;
    st3->stop_point->stop_area->idx = 2;
    data.stop_areas.push_back(st3->stop_point->stop_area);

    st4->stop_point = new ed::types::StopPoint();
    st4->stop_point->name = "A4";
    data.stop_points.push_back(st4->stop_point);
    st4->stop_point->stop_area = new ed::types::StopArea();
    st4->stop_point->stop_area->name = st4->stop_point->name;
    st4->stop_point->stop_area->idx = 3;
    data.stop_areas.push_back(st4->stop_point->stop_area);

    data.complete();
    data.build_route_destination();
    BOOST_CHECK_EQUAL(route->destination->name, "A3");
    /*
    Route1 (Destination READY_DESTINATION)
        VJ1         VJ2     VJ3
        A1          A1      A1
        A2          A2      A2
        A3          A3      A3
                    A4
    */
    // Already destination
    auto* ready = new ed::types::StopArea();
    ready->name = "READY_DESTINATION";
    data.stop_areas.push_back(ready);
    route->destination = ready;
    data.build_route_destination();
    BOOST_CHECK_EQUAL(route->destination->name, "READY_DESTINATION");

    // Route without StopTimes
    auto* route1 = new ed::types::Route();
    route1->idx = 1;
    data.routes.push_back(route1);
    vj1 = new ed::types::VehicleJourney();
    data.vehicle_journeys.push_back(vj1);
    vj1->route = route1;
    data.build_route_destination();
    BOOST_CHECK(!route1->destination);
}
