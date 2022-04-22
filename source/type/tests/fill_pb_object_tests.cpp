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
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include "type/pt_data.h"
#include "type/response.pb.h"
#include "ed/build_helper.h"
#include "type/pb_converter.h"
#include "tests/utils_test.h"
#include "utils/functions.h"

using namespace navitia::type;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(test_pt_displayinfo_destination) {
    ed::builder b("20120614", [](ed::builder& b) {
        b.vj("A")("stop1", 8000, 8050);
        b.vj("A")("stop1", 8000, 8050);
        b.vj("V")("stop2", 8000, 8050);
    });

    auto pt_display_info = new pbnavitia::PtDisplayInfo();
    Route* r = b.data->pt_data->routes_map["A:0"];
    r->destination = b.sas.find("stop1")->second;
    boost::gregorian::date d1(2014, 06, 14);
    boost::posix_time::ptime t1(d1, boost::posix_time::seconds(10));  // 10 sec after midnight
    boost::posix_time::ptime t2(d1, boost::posix_time::hours(10));    // 10 hours after midnight
    boost::posix_time::time_period period(t1, t2);

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, pt::not_a_date_time, period);
    pb_creator.fill(r, pt_display_info, 0);

    BOOST_CHECK_EQUAL(pt_display_info->direction(), "stop1");
    pt_display_info->Clear();

    r->destination = new nt::StopArea();
    r->destination->name = "bob";
    pb_creator.fill(r, pt_display_info, 0);

    BOOST_CHECK_EQUAL(pt_display_info->direction(), "bob");
}

BOOST_AUTO_TEST_CASE(test_pt_displayinfo_destination_without_vj) {
    auto* route = new Route();
    ed::builder b("20120614", [&](ed::builder& b) { b.data->pt_data->routes.push_back(route); });

    auto pt_display_info = new pbnavitia::PtDisplayInfo();
    boost::gregorian::date d1(2014, 06, 14);
    boost::posix_time::ptime t1(d1, boost::posix_time::seconds(10));  // 10 sec after midnight
    boost::posix_time::ptime t2(d1, boost::posix_time::hours(10));    // 10 hours after midnight
    boost::posix_time::time_period period(t1, t2);
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, pt::not_a_date_time, period);
    pb_creator.fill(route, pt_display_info, 0);

    BOOST_CHECK_EQUAL(pt_display_info->direction(), "");
    pt_display_info->Clear();
}

BOOST_AUTO_TEST_CASE(physical_and_commercial_modes_stop_area) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        // Physical_mode = Tram
        b.vj_with_network("Network1", "A", "11110000", "", true, "", "", "", "physical_mode:0x0")("stop1", 8000, 8050)(
            "stop2", 8200, 8250);
        // Physical_mode = Metro
        b.vj("A", "11110000", "", true, "", "", "", "physical_mode:0x1")("stop1", 8000, 8050)("stop2", 8200, 8250)(
            "stop3", 8500, 8500);
        // Physical_mode = Car
        b.vj_with_network("Network2", "B", "00001111", "", true, "", "", "", "physical_mode:Car")("stop4", 8000, 8050)(
            "stop5", 8200, 8250)("stop6", 8500, 8500);

        nt::Line* ln = b.lines.find("A")->second;
        ln->network = b.data->pt_data->networks_map["Network1"];
        ln->commercial_mode = b.data->pt_data->commercial_modes_map["0x1"];

        ln = b.lines.find("B")->second;
        ln->network = b.data->pt_data->networks_map["Network2"];
        ln->commercial_mode = b.data->pt_data->commercial_modes_map["Car"];
    });

    auto stop_area = new pbnavitia::StopArea();
    boost::gregorian::date d1(2014, 06, 14);
    boost::posix_time::ptime t1(d1, boost::posix_time::seconds(10));  // 10 sec after midnight
    boost::posix_time::ptime t2(d1, boost::posix_time::hours(10));    // 10 hours after midnight
    boost::posix_time::time_period period(t1, t2);
    const nt::StopArea* sa = b.sas.find("stop1")->second;

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, pt::not_a_date_time, period);
    pb_creator.fill(sa, stop_area, 2);

    BOOST_CHECK_EQUAL(stop_area->physical_modes().size(), 2);
    BOOST_CHECK_EQUAL(stop_area->commercial_modes().size(), 1);

    stop_area->Clear();
    sa = b.sas.find("stop6")->second;
    pb_creator.fill(sa, stop_area, 2);
    BOOST_CHECK_EQUAL(stop_area->physical_modes().size(), 1);
    BOOST_CHECK_EQUAL(stop_area->commercial_modes().size(), 1);
}

// Test that the geojson isn't in the pb object when disabling geojson output
BOOST_AUTO_TEST_CASE(disable_geojson_on_route_line) {
    ed::builder b("20161026", [](ed::builder& b) { b.vj("A"); });

    Route* r = b.data->pt_data->routes_map["A:0"];
    auto route = new pbnavitia::Route;
    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, pt::not_a_date_time, null_time_period, false);
    pb_creator.fill(r, route, 3);

    BOOST_REQUIRE(route->has_geojson());
    BOOST_REQUIRE(route->line().has_geojson());

    route->Clear();
    navitia::PbCreator pb_creator_no_geojson(data_ptr, pt::not_a_date_time, null_time_period, true);
    pb_creator_no_geojson.fill(r, route, 3);

    BOOST_REQUIRE(!route->has_geojson());
    BOOST_REQUIRE(!route->line().has_geojson());
}

template <typename C>
std::set<std::string> uris(const C& objs) {
    std::set<std::string> uris;
    for (const auto* obj : objs) {
        uris.insert(obj->uri);
    }
    return uris;
}

/*
 * We have 3 contributors:
 *
 * a default one
 * c1 contains dataset "d1" and "d2"
 * c2 contains dataset "d3"
 *
 * 3VJ:
 * - vja  on "d1"
 * - vja2 on "d3"
 * - vjb  on "d2"
 */
BOOST_AUTO_TEST_CASE(ptref_indexes_test) {
    nt::VehicleJourney* vj_a = nullptr;
    ed::builder b("20160101", [&](ed::builder& b) {
        auto* c1 = b.add<nt::Contributor>("c1", "name-c1");
        auto* d1 = b.add<nt::Dataset>("d1", "name-d1");
        d1->contributor = c1;
        c1->dataset_list.insert(d1);

        auto* d2 = b.add<nt::Dataset>("d2", "name-d2");
        d2->contributor = c1;
        c1->dataset_list.insert(d2);

        auto* c2 = b.add<nt::Contributor>("c2", "name-c2");
        auto* d3 = b.add<nt::Dataset>("d3", "name-d3");
        d3->contributor = c2;
        c2->dataset_list.insert(d3);

        vj_a = b.vj("A")("stop1", 8000, 8050).make();
        vj_a->dataset = d1;
        auto* vj_a2 = b.vj("A")("stop1", 8000, 8050).make();
        vj_a2->dataset = d3;
        auto* vj_b = b.vj("B")("stop2", 8000, 8050).make();
        vj_b->dataset = d2;
    });

    auto objs = navitia::ptref_indexes<nt::Contributor>(vj_a, *b.data);
    BOOST_CHECK_EQUAL_RANGE(uris(objs), std::set<std::string>({"c1"}));

    // the contributors of the line A is c1 and c2
    objs = navitia::ptref_indexes<nt::Contributor>(b.get<nt::Line>("A"), *b.data);
    BOOST_CHECK_EQUAL_RANGE(uris(objs), std::set<std::string>({"c1", "c2"}));

    // the contributors of the line B is only c1
    objs = navitia::ptref_indexes<nt::Contributor>(b.get<nt::Line>("B"), *b.data);
    BOOST_CHECK_EQUAL_RANGE(uris(objs), std::set<std::string>({"c1"}));
}

BOOST_AUTO_TEST_CASE(label_formater_line) {
    auto network_rer = std::make_unique<navitia::type::Network>();
    network_rer->name = "RER";
    auto comm_mode_rer = std::make_unique<navitia::type::CommercialMode>();
    comm_mode_rer->name = "Rer";
    auto rer_a = std::make_unique<navitia::type::Line>();
    rer_a->name = "a";
    rer_a->network = network_rer.get();
    BOOST_CHECK_EQUAL(rer_a->get_label(), "RER a");
    rer_a->commercial_mode = comm_mode_rer.get();
    BOOST_CHECK_EQUAL(rer_a->get_label(), "Rer a");
    rer_a->code = "A";
    BOOST_CHECK_EQUAL(rer_a->get_label(), "Rer A");
    network_rer->name = "Transilien";
    BOOST_CHECK_EQUAL(rer_a->get_label(), "Transilien Rer A");
    rer_a->commercial_mode = nullptr;
    BOOST_CHECK_EQUAL(rer_a->get_label(), "Transilien A");
}

BOOST_AUTO_TEST_CASE(pb_convertor_ptref) {
    ed::builder b("20161026", [](ed::builder& b) { b.vj("A great \"uri\" for café"); });

    auto modes = navitia::ptref_indexes<navitia::type::PhysicalMode>(b.data->pt_data->lines.front(), *b.data);
    BOOST_CHECK_EQUAL(modes.size(), 1);
}

BOOST_AUTO_TEST_CASE(fill_crowfly_section_test) {
    ed::builder b("20120614");

    auto pt_journey = new pbnavitia::Journey();
    boost::gregorian::date d1(2014, 06, 14);
    boost::posix_time::ptime t1(d1, boost::posix_time::seconds(10));
    boost::posix_time::ptime t2(d1, boost::posix_time::hours(10));
    boost::posix_time::time_period period(t1, t2);

    auto* data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, pt::not_a_date_time, period);

    pb_creator.fill_crowfly_section(EntryPoint(Type_e::StopPoint, "Totorigin"),
                                    EntryPoint(Type_e::StopPoint, "Tatastination"), navitia::time_duration(0, 0, 0),
                                    Mode_e::Car, t1, pt_journey);

    BOOST_CHECK_EQUAL(pt_journey->sections(0).street_network().duration(), 0);
    BOOST_CHECK_EQUAL(pt_journey->sections(0).street_network().mode(), pbnavitia::Walking);
}
