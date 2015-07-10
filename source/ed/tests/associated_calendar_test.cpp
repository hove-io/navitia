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
#define BOOST_TEST_MODULE associated_calendar_test
#include <boost/test/unit_test.hpp>
#include "ed/build_helper.h"
#include "ed/data.h"
#include "ed/types.h"
#include "boost/date_time/gregorian_calendar.hpp"

using namespace ed;

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

//helper for lazyness
static boost::gregorian::date date(std::string str) {
    return boost::gregorian::from_undelimited_string(str);
}

static boost::gregorian::date_period period(std::string beg, std::string end) {
    boost::gregorian::date start_date = boost::gregorian::from_undelimited_string(beg);
    boost::gregorian::date end_date = boost::gregorian::from_undelimited_string(end); //end is not in the period
    return {start_date, end_date};
}

struct calendar_fixture {
    calendar_fixture() : b("20140210") {
        cal = new ed::types::Calendar(b.data->meta->production_date.begin());
        cal->uri="cal1";
        boost::gregorian::date start = boost::gregorian::from_undelimited_string("20140210");
        boost::gregorian::date end = boost::gregorian::from_undelimited_string("20140214"); //end is not in the period
        cal->period_list.push_back({start, end});
        cal->week_pattern = std::bitset<7>("1111111");
        //b.data->pt_data->calendars.push_back(cal);
    }
    ed::builder b;
    ed::types::Calendar* cal;
};

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test1, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                    13/02/2014
               +-----------------------------+
    */
    cal->build_validity_pattern(b.data->meta->production_date);
    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140212")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140214")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140514")));
}

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test2, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                   13/02/2014
               +----------------------------+
    */
    navitia::type::ExceptionDate exd;
    exd.date = date("20140212");
    exd.type = navitia::type::ExceptionDate::ExceptionType::sub;
    cal->exceptions.push_back(exd);

    cal->build_validity_pattern(b.data->meta->production_date);

    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140212")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
}

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test3, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                   13/02/2014
               +----------------------------+
    */
    navitia::type::ExceptionDate exd;
    exd.date = date("20140212");
    exd.type = navitia::type::ExceptionDate::ExceptionType::add;
    cal->exceptions.push_back(exd);

    cal->build_validity_pattern(b.data->meta->production_date);

    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
}

BOOST_AUTO_TEST_CASE(get_differences_test) {
    std::bitset<12> cal ("111000111000");
    std::bitset<12> vj  ("101010101010");
    //the dif are the differences between cal and vj restricted on the active days of the calendar
    ed::Data data;
    auto res = data.get_difference(cal, vj);

    BOOST_CHECK_EQUAL(res, std::bitset<12>("010000010000"));
}

BOOST_AUTO_TEST_CASE(get_differences_test_empty_cal) {
    std::bitset<12> cal ("000000000000");
    std::bitset<12> vj ("101010101010");
    ed::Data data;
    auto res = data.get_difference(cal, vj);

    BOOST_CHECK_EQUAL(res, std::bitset<12>("000000000000"));
}

BOOST_AUTO_TEST_CASE(get_differences_test_full_cal) {
    std::bitset<12> cal ("111111111111");
    std::bitset<12> vj ("101010101010");
    ed::Data data;
    auto res = data.get_difference(cal, vj);

    BOOST_CHECK_EQUAL(res, std::bitset<12>("010101010101"));
}
/*
                                          Janvier
                                    di lu ma me je ve sa
                                              1  2  3  4
                                     5  6  7  8  9 10 11
                                    12 13 14 15 16 17 18
                                    19 20 21 22 23 24 25
                                    26 27 28 29 30 31

*/

struct associated_cal_fixture {
    associated_cal_fixture() : b("20140101"), data(),
        // Same vehicleJourney.validity_pattern : Associated vehicle_journey
        always_on_cal(new ed::types::Calendar(b.data->meta->production_date.begin())),
        // 2 days of period 01/04/2014 - 11/04/2014 : Associated vehicle_journey
        wednesday_cal(new ed::types::Calendar(b.data->meta->production_date.begin())),
        // monday where vehiclejourney.validity_pattern not valide for this day : Not Associated vehicle_journey
        monday_cal(new ed::types::Calendar(b.data->meta->production_date.begin())) {
        {
            always_on_cal->uri="always_on";
            always_on_cal->period_list.push_back(period("20140101", "20140111"));
            always_on_cal->week_pattern = std::bitset<7>{"0111100"};
            data.calendars.push_back(always_on_cal);
        }

        {
            wednesday_cal->uri="wednesday";
            wednesday_cal->period_list.push_back(period("20140101", "20140111"));
            wednesday_cal->week_pattern = std::bitset<7>{"0010000"};
            data.calendars.push_back(wednesday_cal);
        }

        {
            monday_cal->uri="monday";
            monday_cal->period_list.push_back(period("20140105", "20140111"));
            monday_cal->week_pattern = std::bitset<7>{"1000000"};
            data.calendars.push_back(monday_cal);
        }

        data.meta.production_date = {boost::gregorian::date(2014, 1, 1), boost::gregorian::date(2015, 1, 1)};

        ed::types::Network* network = new ed::types::Network();
        network->idx = data.networks.size();
        network->uri = "network:R";
        network->name = "network:R";
        data.networks.push_back(network);

        auto* line = new ed::types::Line();
        line->idx = data.lines.size();
        line->name = "line:A";
        line->network = network;
        data.lines.push_back(line);

        auto *route = new ed::types::Route();
        route->idx = data.routes.size();
        route->line = line;
        data.routes.push_back(route);

        auto* journey_pattern = new ed::types::JourneyPattern();
        journey_pattern->idx = data.journey_patterns.size();
        journey_pattern->uri ="jp";
        journey_pattern->route = route;
        data.journey_patterns.push_back(journey_pattern);

        auto st1 = new ed::types::StopTime();
        st1->arrival_time = 10 * 3600 + 15 * 60;
        st1->departure_time = 10 * 3600 + 15 * 60;
        data.stops.push_back(st1);

        auto st2 = new ed::types::StopTime();
        st2->arrival_time = 11 * 3600 + 10 * 60;
        st2->departure_time = 11 * 3600 + 10 * 60;
        data.stops.push_back(st2);

        st1->tmp_stop_point = new ed::types::StopPoint();
        data.stop_points.push_back(st1->tmp_stop_point);
        st2->tmp_stop_point = new ed::types::StopPoint();
        data.stop_points.push_back(st2->tmp_stop_point);


        //b.vj("network:R", "line:A", "", "", true, "vj1")
        //        ("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)
        //        ("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);

        vj1 = new ed::types::VehicleJourney();
        vj1->idx = data.vehicle_journeys.size();
        vj1->name = "vj1";
        vj1->uri = "vj1";
        vj1->journey_pattern = journey_pattern;
        vj1->stop_time_list.push_back(st1);
        vj1->stop_time_list.push_back(st2);
        data.vehicle_journeys.push_back(vj1);

        //b.vj("network:R", "line:A", "", "", true, "vj2", "meta_vj")
        //        ("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)
        //        ("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);

        vj2 = new ed::types::VehicleJourney();
        vj2->idx = data.vehicle_journeys.size();
        vj2->name = "vj2";
        vj2->uri = "vj2";
        vj2->journey_pattern = journey_pattern;
        vj2->stop_time_list.push_back(st1);
        vj2->stop_time_list.push_back(st2);
        data.vehicle_journeys.push_back(vj2);

        //b.vj("network:R", "line:A", "", "", true, "vj2_bis", "meta_vj")
        //        ("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)
        //        ("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);

        vj2_b = new ed::types::VehicleJourney();
        vj2_b->idx = data.vehicle_journeys.size();
        vj2_b->name = "vj2_bis";
        vj2_b->uri = "vj2_bis";
        vj2_b->journey_pattern = journey_pattern;
        vj2_b->stop_time_list.push_back(st1);
        vj2_b->stop_time_list.push_back(st2);
        data.vehicle_journeys.push_back(vj2_b);

        // meta_vj

        ed::types::MetaVehicleJourney mvj;
        mvj.theoric_vj.push_back(vj1);
        mvj.theoric_vj.push_back(vj2);
        mvj.theoric_vj.push_back(vj2_b);
        vj1->meta_vj_name = "meta_vj";
        vj2->meta_vj_name = "meta_vj";
        vj2_b->meta_vj_name = "meta_vj";
        data.meta_vj_map.insert({"meta_vj", mvj});

        always_on_cal->line_list.push_back(line);
        wednesday_cal->line_list.push_back(line);
        monday_cal->line_list.push_back(line);

        // The whole week but saturday and sunday
        vj1->validity_pattern = new ed::types::ValidityPattern();
        vj1->validity_pattern->idx = data.validity_patterns.size();
        vj1->validity_pattern->beginning_date = b.data->meta->production_date.begin();

        vj1->validity_pattern->add(date("20140101"), date("20140111"), always_on_cal->week_pattern);

        data.validity_patterns.push_back(vj1->validity_pattern);

        //for the meta vj we use the same, but split
        vj2->validity_pattern = new ed::types::ValidityPattern();
        vj2->idx = data.validity_patterns.size();
        vj2->validity_pattern->beginning_date = b.data->meta->production_date.begin();
        vj2->validity_pattern->add(date("20140101"), date("20140105"), always_on_cal->week_pattern);
        data.validity_patterns.push_back(vj2->validity_pattern);

        vj2_b->validity_pattern = new ed::types::ValidityPattern();
        vj2_b->idx = data.validity_patterns.size();
        vj2_b->validity_pattern->beginning_date = b.data->meta->production_date.begin();
        vj2_b->validity_pattern->add(date("20140106"), date("20140111"), always_on_cal->week_pattern);
        data.validity_patterns.push_back(vj2_b->validity_pattern);

        data.complete();
    }

    void check_vj(const ed::types::VehicleJourney* vj) {
        BOOST_REQUIRE(vj);

        BOOST_REQUIRE(data.meta_vj_map.find(vj->meta_vj_name) != data.meta_vj_map.end());

        const auto* meta_vj = &data.meta_vj_map[vj->meta_vj_name];
        BOOST_REQUIRE_EQUAL(meta_vj->associated_calendars.size(), 2);

        auto it_associated_always_cal = meta_vj->associated_calendars.find(always_on_cal->uri);
        BOOST_REQUIRE(it_associated_always_cal != meta_vj->associated_calendars.end());

        //no restriction
        auto associated_always_cal = it_associated_always_cal->second;
        BOOST_CHECK_EQUAL(associated_always_cal->calendar, always_on_cal);
        BOOST_CHECK(associated_always_cal->exceptions.empty());

        auto it_associated_wednesday_cal = meta_vj->associated_calendars.find(wednesday_cal->uri);
        BOOST_REQUIRE(it_associated_wednesday_cal != meta_vj->associated_calendars.end());

        //no restriction
        auto associated_wednesday_cal = it_associated_wednesday_cal->second;
        BOOST_CHECK_EQUAL(associated_wednesday_cal->calendar, wednesday_cal);
        BOOST_CHECK(associated_wednesday_cal->exceptions.empty());

        auto it_associated_monday_cal = meta_vj->associated_calendars.find(monday_cal->uri);
        BOOST_REQUIRE(it_associated_monday_cal == meta_vj->associated_calendars.end());
    }
    ed::builder b;
    ed::Data data;
    ed::types::Calendar* always_on_cal;
    ed::types::Calendar* wednesday_cal;
    ed::types::Calendar* monday_cal;
    ed::types::VehicleJourney *vj1, *vj2, *vj2_b;
};

BOOST_FIXTURE_TEST_CASE(associated_val_test1, associated_cal_fixture) {
    check_vj(vj1);
}

BOOST_FIXTURE_TEST_CASE(meta_vj_association_test, associated_cal_fixture) {
    //we split the vj under a meta vj (like it's done for dst)
    //should have the same thing than non split vj
    check_vj(vj2);
    check_vj(vj2_b);
}
