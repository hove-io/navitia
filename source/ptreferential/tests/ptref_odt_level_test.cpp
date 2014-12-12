
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
#define BOOST_TEST_MODULE ptref_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential.h"
#include "ptreferential/reflexion.h"
#include "ptreferential/ptref_graph.h"
#include "ed/build_helper.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>
#include "type/pt_data.h"

namespace navitia{namespace ptref {
template<typename T> std::vector<type::idx_t> get_indexes(Filter filter,  Type_e requested_type, const type::Data & d);
}}

using namespace navitia::ptref;

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

class Params{
public:
    navitia::type::Data data;
    std::vector<std::string> forbidden;
    std::vector<navitia::type::idx_t> final_idx;
    navitia::type::Network* current_ntw;
    navitia::type::Line* current_ln;
    navitia::type::Route* current_rt;
    navitia::type::JourneyPattern* current_jp;
    void add_network(const std::string& ntw_name){
        navitia::type::Network* nt = new navitia::type::Network();
        nt->uri = ntw_name;
        nt->name = ntw_name;
        data.pt_data->networks.push_back(nt);
        current_ntw = nt;
    }
    void add_line(const std::string& ln_name){
        navitia::type::Line* ln = new navitia::type::Line();
        ln->uri = ln_name;
        ln->name = ln_name;
        data.pt_data->lines.push_back(ln);
        current_ln = ln;
        current_ntw->line_list.push_back(ln);
    }
    void add_route(const std::string& rt_name){
        navitia::type::Route* rt = new navitia::type::Route();
        rt->uri = rt_name;
        rt->name = rt_name;
        data.pt_data->routes.push_back(rt);
        current_rt = rt;
        current_ln->route_list.push_back(rt);
    }
    void add_journey_pattern(const std::string& jp_name){
        navitia::type::JourneyPattern* jp = new navitia::type::JourneyPattern();
        jp->uri = jp_name;
        jp->name = jp_name;
        data.pt_data->journey_patterns.push_back(jp);
        current_jp = jp;
        current_rt->journey_pattern_list.push_back(jp);
    }
    Params(){
        /*
        Line1 :
            Route11 :
                JP111 :   SP1     SP2     SP3
                JP112 :   SP1     SP2     SP3     SP4
           Route12 :
                JP121 :   SP3     SP2     SP1
                JP122 :   SP4     SP3     SP2     SP1

        Line2 :
            Route21 :
                JP211 :   SP1     SP2     SP3
                JP212 :   SP1     SP2     SP3     SP4

        */
        add_network("R1");
        add_line("Line1");
        add_route("Route11");
        add_journey_pattern("JP111");
        add_journey_pattern("JP112");

        add_route("Route12");
        add_journey_pattern("JP121");
        add_journey_pattern("JP122");

        add_network("R2");
        add_line("Line2");
        add_route("Route21");
        add_journey_pattern("JP211");
        add_journey_pattern("JP212");
        data.build_uri();
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

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::none, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::mixt,
                                 data), ptref_error);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::zonal,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 2 :
    line 2 :
        JP111 is mixt and JP112 is none odt
    line 2 :
        all journeypattern odt_level = none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test2) {

    navitia::type::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    //jp->odt_level = navitia::type::OdtLevel_e::mixt;
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::none, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::mixt, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line1");

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::zonal,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 3 :
    line 2 :
        JP111 is mixt and JP112 is mixt odt
    line 2 :
        all journeypattern odt_level = none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test3) {

    navitia::type::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::none, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::mixt, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line1");

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::zonal,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 4 :
    line 2 :
        JP111 is mixt and JP112 is mixt odt
    line 2 :
        JP211 is mixt and JP212 is none odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test4) {

    navitia::type::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP211"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::none,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::mixt, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::zonal,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 5 :
    line 2 :
        JP111 is mixt and JP112 is mixt odt
    line 2 :
        JP211 is mixt and JP212 is mixt odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test5) {

    navitia::type::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP211"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP212"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, true);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::none,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::mixt, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::zonal,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 6 :
    line 2 :
        JP111 is zonal and JP112 is none odt
    line 2 :
        JP211 is none and JP212 is none odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test6) {

    navitia::type::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::none, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::mixt, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line1");

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::zonal,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 7 :
    line 2 :
        JP111 is zonal and JP112 is zonal odt
    line 2 :
        JP211 is none and JP212 is none odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test7) {

    navitia::type::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::none, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::mixt, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line1");

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::zonal,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 8 :
    line 2 :
        JP111 is zonal and JP112 is zonal odt
    line 2 :
        JP211 is zonal and JP212 is none odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test8) {

    navitia::type::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP211"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::none,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::mixt, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::zonal,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 9 :
    line 2 :
        JP111 is zonal and JP112 is zonal odt
    line 2 :
        JP211 is zonal and JP212 is zonal odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test9) {

    navitia::type::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP211"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    jp = data.pt_data->journey_patterns_map["JP212"];
    jp->set_odt(navitia::type::hasOdtProperties::NONE_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::VIRTUAL_ODT, false);
    jp->set_odt(navitia::type::hasOdtProperties::ZONAL_ODT, true);

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Line, "",
                                 forbidden, navitia::type::OdtLevel_e::none,
                                 data), ptref_error);

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::mixt, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line1");

    final_idx = make_query(navitia::type::Type_e::Line, "",
                                        forbidden, navitia::type::OdtLevel_e::zonal, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");
    final_idx = make_query(navitia::type::Type_e::Line, "",
                     forbidden, navitia::type::OdtLevel_e::all, data);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()

