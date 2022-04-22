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
#define BOOST_TEST_MODULE ptref_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential_utils.h"
#include "ptreferential/ptreferential.h"
#include "ptreferential/ptref_graph.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "utils/logger.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>
#include "type/pt_data.h"

using namespace navitia::ptref;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

class Params {
public:
    nt::Data data;
    std::vector<std::string> forbidden;
    nt::Indexes final_idx;
    nt::Network* current_ntw;
    nt::Line* current_ln;
    nt::Route* current_rt;
    void add_network(const std::string& ntw_name) {
        auto* nt = new nt::Network();
        nt->uri = ntw_name;
        nt->name = ntw_name;
        data.pt_data->networks.push_back(nt);
        current_ntw = nt;
    }
    void add_line(const std::string& ln_name) {
        auto* ln = new nt::Line();
        ln->uri = ln_name;
        ln->name = ln_name;
        data.pt_data->lines.push_back(ln);
        current_ln = ln;
        current_ntw->line_list.push_back(ln);
    }
    void add_route(const std::string& rt_name) {
        auto* rt = new nt::Route();
        rt->uri = rt_name;
        rt->name = rt_name;
        data.pt_data->routes.push_back(rt);
        current_rt = rt;
        current_ln->route_list.push_back(rt);
    }
    void add_vj(const std::string& vj_name) {
        auto mvj = data.pt_data->meta_vjs.get_or_create(vj_name);
        std::vector<nt::StopTime> sts;
        std::string headsign = vj_name + "_hs";
        sts.emplace_back();
        sts.back().stop_point = data.pt_data->stop_points.at(0);
        mvj->create_discrete_vj("vehicle_journey:" + vj_name, vj_name, headsign, nt::RTLevel::Base,
                                nt::ValidityPattern(), current_rt, std::move(sts), *data.pt_data);
    }
    void set_estimated(const std::string& vj_name) {
        data.pt_data->vehicle_journeys_map.at(vj_name)->stop_time_list.front().set_date_time_estimated(true);
    }
    void set_zonal(const std::string& vj_name) {
        data.pt_data->vehicle_journeys_map.at(vj_name)->stop_time_list.front().stop_point =
            data.pt_data->stop_points.at(1);
    }
    void reset_vj() {
        for (auto* vj : data.pt_data->vehicle_journeys) {
            vj->stop_time_list.front().set_date_time_estimated(false);
            vj->stop_time_list.front().stop_point = data.pt_data->stop_points.at(0);
        }
    }

    Params() {
        auto* normal_sp = new nt::StopPoint();
        normal_sp->uri = "normal";
        normal_sp->name = "normal";
        data.pt_data->stop_points.push_back(normal_sp);

        auto* zonal_sp = new nt::StopPoint();
        zonal_sp->is_zonal = true;
        zonal_sp->uri = "zonal";
        zonal_sp->name = "zonal";
        data.pt_data->stop_points.push_back(zonal_sp);
        /*
        Line1 :
            Route11 :
                VJ111
                VJ112
           Route12 :
                VJ121
                VJ122

        Line2 :
            Route21 :
                VJ211
                VJ212

        */
        add_network("R1");
        add_line("Line1");
        add_route("Route11");
        add_vj("VJ111");
        add_vj("VJ112");

        add_route("Route12");
        add_vj("VJ121");
        add_vj("VJ122");

        add_network("R2");
        add_line("Line2");
        add_route("Route21");
        add_vj("VJ211");
        add_vj("VJ212");
        data.pt_data->sort_and_index();
        data.build_uri();
    }

    // helper for the tests
    nt::Indexes make_query(nt::Type_e requested_type, const nt::OdtLevel_e odt_level) {
        return navitia::ptref::make_query(requested_type, "", forbidden, odt_level, boost::none, boost::none, data);
    }
};
BOOST_FIXTURE_TEST_SUITE(odt_level, Params)

/*
Test 1 :
    line 1 :
        all journeypattern odt_level = none
    line 2 :
        all journeypattern odt_level = none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test1) {
    reset_vj();
    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal), ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 2 :
    line 2 :
        VJ111 is virtual and VJ112 is none odt
    line 2 :
        all journeypattern odt_level = none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test2) {
    reset_vj();
    set_estimated("vehicle_journey:VJ111");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line2"}));

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal), ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 3 :
    line 1 :
        VJ111 and VJ112 are zonal
    line 2 :
        all journeypattern odt_level = none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test3) {
    reset_vj();
    set_estimated("vehicle_journey:VJ111");
    set_zonal("vehicle_journey:VJ111");

    set_estimated("vehicle_journey:VJ112");
    set_zonal("vehicle_journey:VJ112");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line2"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line2"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line1"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 4 :
    line 2 :
        VJ111 is zonal and VJ112 is zonal
    line 2 :
        VJ211 is zonal and VJ212 is none odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test4) {
    reset_vj();
    set_estimated("vehicle_journey:VJ111");
    set_zonal("vehicle_journey:VJ111");

    set_estimated("vehicle_journey:VJ112");
    set_zonal("vehicle_journey:VJ112");

    set_estimated("vehicle_journey:VJ211");
    set_zonal("vehicle_journey:VJ211");

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled), ptref_error);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops), ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 5 :
    line 2 :
        VJ111 and VJ112 are zonal
    line 2 :
        VJ211 and VJ212 are zonal

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test5) {
    reset_vj();
    set_estimated("vehicle_journey:VJ111");
    set_zonal("vehicle_journey:VJ111");

    set_estimated("vehicle_journey:VJ112");
    set_zonal("vehicle_journey:VJ112");

    set_estimated("vehicle_journey:VJ211");
    set_zonal("vehicle_journey:VJ211");

    set_estimated("vehicle_journey:VJ212");
    set_zonal("vehicle_journey:VJ212");

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled), ptref_error);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops), ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 6 :
    line 1 :
        VJ111 is zonal and VJ112 is none
    line 2 :
        VJ211 is none and VJ212 is none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test6) {
    reset_vj();
    set_zonal("vehicle_journey:VJ111");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line2"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line2"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line1"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 7 :
    line 2 :
        VJ111 is zonal and VJ112 is zonal odt
    line 2 :
        VJ211 is none and VJ212 is none odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test7) {
    reset_vj();
    set_zonal("vehicle_journey:VJ111");

    set_zonal("vehicle_journey:VJ112");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line2"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line2"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(final_idx, data), std::set<std::string>({"Line1"}));

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 8 :
    line 2 :
        VJ111 is zonal and VJ112 is zonal odt
    line 2 :
        VJ211 is zonal and VJ212 is none odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test8) {
    reset_vj();
    set_zonal("vehicle_journey:VJ111");

    set_zonal("vehicle_journey:VJ112");

    set_zonal("vehicle_journey:VJ211");

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled), ptref_error);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops), ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 9 :
    line 2 :
        VJ111 is zonal and VJ112 is zonal odt
    line 2 :
        VJ211 is zonal and VJ212 is zonal odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test9) {
    reset_vj();
    set_zonal("vehicle_journey:VJ111");

    set_zonal("vehicle_journey:VJ112");

    set_zonal("vehicle_journey:VJ211");

    set_zonal("vehicle_journey:VJ212");

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled), ptref_error);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops), ptref_error);
    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
