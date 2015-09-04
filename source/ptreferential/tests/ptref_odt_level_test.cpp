
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
    nt::Data data;
    std::vector<std::string> forbidden;
    std::vector<nt::idx_t> final_idx;
    nt::Network* current_ntw;
    nt::Line* current_ln;
    nt::Route* current_rt;
    nt::JourneyPattern* current_jp;
    void add_network(const std::string& ntw_name){
        nt::Network* nt = new nt::Network();
        nt->uri = ntw_name;
        nt->name = ntw_name;
        data.pt_data->networks.push_back(nt);
        current_ntw = nt;
    }
    void add_line(const std::string& ln_name){
        nt::Line* ln = new nt::Line();
        ln->uri = ln_name;
        ln->name = ln_name;
        data.pt_data->lines.push_back(ln);
        current_ln = ln;
        current_ntw->line_list.push_back(ln);
    }
    void add_route(const std::string& rt_name){
        nt::Route* rt = new nt::Route();
        rt->uri = rt_name;
        rt->name = rt_name;
        data.pt_data->routes.push_back(rt);
        current_rt = rt;
        current_ln->route_list.push_back(rt);
    }
    void add_journey_pattern(const std::string& jp_name){
        nt::JourneyPattern* jp = new nt::JourneyPattern();
        jp->uri = jp_name;
        jp->name = jp_name;

        data.pt_data->journey_patterns.push_back(jp);
        current_jp = jp;
        current_rt->journey_pattern_list.push_back(jp);
    }
    void set_odt_journey_patterns(){
        for (nt::JourneyPattern* jp : data.pt_data->journey_patterns){
            jp->odt_properties.reset();
        }
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


    //helper for the tests
    std::vector<nt::idx_t> make_query(nt::Type_e requested_type,
                                        const nt::OdtLevel_e odt_level) {
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
    set_odt_journey_patterns();
    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal),
                      ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 2 :
    line 2 :
        JP111 is virtual and JP112 is none odt
    line 2 :
        all journeypattern odt_level = none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test2) {
    set_odt_journey_patterns();
    nt::JourneyPattern* JP111 = data.pt_data->journey_patterns_map["JP111"];
    JP111->odt_properties.set_estimated();

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal),
                      ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 3 :
    line 1 :
        JP111 and JP112 are zonal
    line 2 :
        all journeypattern odt_level = none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test3) {
    set_odt_journey_patterns();
    nt::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line1");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 4 :
    line 2 :
        JP111 is zonal and JP112 is zonal
    line 2 :
        JP211 is zonal and JP212 is none odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test4) {
    set_odt_journey_patterns();
    nt::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP211"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled),
                      ptref_error);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops),
                      ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 5 :
    line 2 :
        JP111 and JP112 are zonal
    line 2 :
        JP211 and JP212 are zonal

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test5) {
    set_odt_journey_patterns();
    nt::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP211"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP212"];
    jp->odt_properties.set_estimated();
    jp->odt_properties.set_zonal();

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled),
                      ptref_error);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops),
                      ptref_error);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

/*
Test 6 :
    line 1 :
        JP111 is zonal and JP112 is none
    line 2 :
        JP211 is none and JP212 is none

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test6) {
    set_odt_journey_patterns();
    nt::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->odt_properties.set_zonal();

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops);
    BOOST_CHECK_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_CHECK_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line1");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
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
    set_odt_journey_patterns();
    nt::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->odt_properties.set_zonal();

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line2");

    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->lines[final_idx.front()]->uri, "Line1");



    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
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
    set_odt_journey_patterns();
    nt::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP211"];
    jp->odt_properties.set_zonal();

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
        JP111 is zonal and JP112 is zonal odt
    line 2 :
        JP211 is zonal and JP212 is zonal odt

    Call make_query function with OdtLevel : none, mixt, zonal and all
*/

BOOST_AUTO_TEST_CASE(test9) {
    set_odt_journey_patterns();
    nt::JourneyPattern* jp = data.pt_data->journey_patterns_map["JP111"];
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP112"];
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP211"];
    jp->odt_properties.set_zonal();

    jp = data.pt_data->journey_patterns_map["JP212"];
    jp->odt_properties.set_zonal();

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::scheduled), ptref_error);

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, nt::OdtLevel_e::with_stops), ptref_error);
    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::zonal);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
    final_idx = make_query(nt::Type_e::Line, nt::OdtLevel_e::all);
    BOOST_REQUIRE_EQUAL(final_idx.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()

