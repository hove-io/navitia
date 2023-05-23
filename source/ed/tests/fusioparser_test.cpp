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

#include <boost/range/algorithm/find_if.hpp>
#include "ed/data.h"
#include "ed/connectors/gtfs_parser.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include <map>
#include <boost/range/algorithm.hpp>
#include "conf.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "ed/connectors/fusio_parser.h"

namespace bg = boost::gregorian;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

const std::string ntfs_path = std::string(navitia::config::fixtures_dir) + "/ed/ntfs";

BOOST_AUTO_TEST_CASE(parse_small_ntfs_dataset) {
    using namespace ed;

    Data data;
    connectors::FusioParser parser(ntfs_path);

    parser.fill(data);

    // we check that the data have been correctly loaded
    BOOST_REQUIRE_EQUAL(data.networks.size(), 1);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "ligne flixible");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "ligneflexible");

    BOOST_REQUIRE_EQUAL(data.stop_areas.size(), 11);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->name, "Arrêt A");
    BOOST_CHECK_EQUAL(data.stop_areas[0]->uri, "SA:A");
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lat(), 45.0296, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lon(), 0.5881, 0.1);

    // Check website, license of contributor
    BOOST_REQUIRE_EQUAL(data.contributors.size(), 1);
    BOOST_REQUIRE_EQUAL(data.contributors[0]->website, "http://www.hove.com");
    BOOST_REQUIRE_EQUAL(data.contributors[0]->license, "LICENSE");

    // Check datasets
    BOOST_REQUIRE_EQUAL(data.datasets.size(), 5);
    BOOST_CHECK_EQUAL(data.datasets[0]->desc, "dataset_desc1");
    BOOST_CHECK_EQUAL(data.datasets[0]->uri, "dataset_id1");
    BOOST_CHECK_EQUAL(data.datasets[0]->validation_period, boost::gregorian::date_period("20180312"_d, "20180401"_d));
    BOOST_CHECK_EQUAL(data.datasets[0]->realtime_level == nt::RTLevel::Base, true);
    BOOST_CHECK_EQUAL(data.datasets[0]->system, "dataset_system1");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->dataset->uri, "dataset_id1");

    // timzeone check
    // no timezone is given for the stop area in this dataset, to the agency time zone (the default one) is taken
    for (auto sa : data.stop_areas) {
        BOOST_CHECK_EQUAL(sa->time_zone_with_name.first, "Europe/Paris");
    }

    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 8);
    BOOST_CHECK_EQUAL(data.stop_points[0]->uri, "SP:A");
    BOOST_CHECK_EQUAL(data.stop_points[0]->name, "Arrêt A");
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lat(), 45.0296, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lon(), 0.5881, 0.1);
    BOOST_CHECK_EQUAL(data.stop_points[0]->stop_area, data.stop_areas[0]);
    BOOST_CHECK_EQUAL(data.stop_points[0]->fare_zone, "4");

    // check stops properties
    navitia::type::hasProperties has_properties;
    has_properties.set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
    has_properties.set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(data.stop_points[0]->accessible(has_properties.properties()), true);
    BOOST_CHECK_EQUAL(data.stop_points[1]->accessible(has_properties.properties()), false);

    for (int i = 1; i < 8; i++) {
        BOOST_CHECK_EQUAL(data.stop_points[i]->accessible(has_properties.properties()), false);
    }

    // we should have one zonal stop point
    BOOST_REQUIRE_EQUAL(
        boost::count_if(data.stop_points,
                        [](const types::StopPoint* sp) { return sp->is_zonal && sp->name == "Beleymas" && sp->area; }),
        1);

    BOOST_REQUIRE_EQUAL(data.lines.size(), 4);
    BOOST_REQUIRE_EQUAL(data.routes.size(), 4);

    // object_codes
    BOOST_REQUIRE_EQUAL(data.object_codes.size(), 21);
    // routes
    for (uint i = 0; i < data.routes.size(); ++i) {
        auto obj_codes_routes_map = data.object_codes[ed::types::make_pt_object(data.routes[i])];
        BOOST_REQUIRE_EQUAL(obj_codes_routes_map.size(), 1);
        for (const auto& obj_codes : obj_codes_routes_map) {
            BOOST_CHECK_EQUAL(obj_codes.second[0], "route_" + std::to_string(i + 1));
        }
    }
    // companies
    auto obj_codes_map = data.object_codes[ed::types::make_pt_object(data.companies[0])];
    BOOST_REQUIRE_EQUAL(obj_codes_map.size(), 1);
    for (const auto& obj_codes : obj_codes_map) {
        BOOST_CHECK_EQUAL(obj_codes.second[0], "A");
        BOOST_CHECK_EQUAL(obj_codes.second[1], "B");
    }
    obj_codes_map = data.object_codes[ed::types::make_pt_object(data.companies[1])];
    BOOST_REQUIRE_EQUAL(obj_codes_map.size(), 1);
    for (const auto& obj_codes : obj_codes_map) {
        BOOST_CHECK_EQUAL(obj_codes.second[0], "A");
    }

    // check comments
    BOOST_REQUIRE_EQUAL(data.comment_by_id.size(), 2);
    nt::Comment bob, bobette;
    bob.value = "bob is in the kitchen";
    bobette.value = "test comment";
    bobette.type = "ODT";
    BOOST_CHECK_EQUAL(data.comment_by_id["bob"], bob);
    BOOST_CHECK_EQUAL(data.comment_by_id["bobette"], bobette);

    // 7 objects have comments
    // the file contains wrongly formated comments, but they are skiped
    BOOST_REQUIRE_EQUAL(data.comments.size(), 5);

    std::map<ed::types::pt_object_header, std::vector<std::string>> expected_comments = {
        {ed::types::make_pt_object(data.routes[0]), {"bob"}},
        {ed::types::make_pt_object(data.vehicle_journeys[4]), {"bobette"}},
        {ed::types::make_pt_object(data.vehicle_journeys[5]), {"bobette"}},  // split too
        {ed::types::make_pt_object(data.stop_areas[2]), {"bob"}},
        {ed::types::make_pt_object(data.stop_points[3]), {"bobette"}}};

    BOOST_CHECK_EQUAL_COLLECTIONS(data.comments.begin(), data.comments.end(), expected_comments.begin(),
                                  expected_comments.end());

    std::map<const ed::types::StopTime*, std::vector<std::string>> expected_st_comments = {
        {{data.stops[6]}, {"bob", "bobette"}},  // stoptimes are split
        {{data.stops[7]}, {"bob", "bobette"}},
        {{data.stops[12]}, {"bob"}},
        {{data.stops[13]}, {"bob"}},
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.stoptime_comments.begin(), data.stoptime_comments.end(),
                                  expected_st_comments.begin(), expected_st_comments.end());
    // manage itl
    const types::VehicleJourney* vj = data.vehicle_journeys.front();
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0]->local_traffic_zone, std::numeric_limits<uint16_t>::max());
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1]->local_traffic_zone, 1);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2]->local_traffic_zone, 1);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[3]->local_traffic_zone, 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[4]->local_traffic_zone, std::numeric_limits<uint16_t>::max());

    // stop_headsign
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0]->headsign, "N1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1]->headsign, "N1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2]->headsign, "N1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[3]->headsign, "N2");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[4]->headsign, "N2");

    // vj_headsign feed from trip_headsign if not empty
    BOOST_REQUIRE_EQUAL(vj->headsign, "vehiclejourney1");
    // vj_name feed from trip_short_name if empty from vj_headsign
    BOOST_REQUIRE_EQUAL(vj->name, "trip_1_short_name");

    navitia::type::hasVehicleProperties has_vehicle_properties;
    has_vehicle_properties.set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(vj->accessible(has_vehicle_properties.vehicles()), true);

    for (const auto vj : data.vehicle_journeys) {
        for (size_t position = 0; position < vj->stop_time_list.size(); ++position) {
            BOOST_CHECK_EQUAL(vj->stop_time_list[position]->order, position);
        }
    }

    // feed_info
    std::map<std::string, std::string> feed_info_test = {{"feed_start_date", "20150325"},
                                                         {"feed_end_date", "20150826"},
                                                         {"feed_publisher_name", "Ile de France open data"},
                                                         {"feed_publisher_url", "http://www.hove.com"},
                                                         {"feed_license", "ODBL"},
                                                         {"feed_creation_datetime", "20150415T153234"}};
    BOOST_CHECK_EQUAL_COLLECTIONS(data.feed_infos.begin(), data.feed_infos.end(), feed_info_test.begin(),
                                  feed_info_test.end());

    BOOST_REQUIRE_EQUAL(data.meta.production_date, boost::gregorian::date_period(boost::gregorian::date(2015, 3, 25),
                                                                                 boost::gregorian::date(2015, 8, 27)));

    /* Line groups.
     * 3 groups in the file, 3 use cases :
     *   - The first one has 2 lines, both linked to the group in line_group_links.txt.
     *     One link is twice in the file and one of the entry should be ignored,
     *   - The second one has only one line, linked only via the main_line_id field of line_groups.txt,
     *   - The third one has an unknown main_line_id and should be ignored
     *
     * At the start of the vector data.line_group_links we are going to have each main_line, since
     * those links are created in LineGroupFusioHandler, to allow use case number 2 where the line is not
     * in line_group_links.txt.
     */

    // Check group, only 2 must be created since the last one in line_groups.txt reference an unknown main_line_id
    BOOST_REQUIRE_EQUAL(data.line_groups.size(), 2);
    BOOST_REQUIRE_EQUAL(data.line_group_links.size(), 3);

    // First group
    BOOST_REQUIRE(data.line_groups[0]->main_line != nullptr);

    BOOST_CHECK_EQUAL(data.line_groups[0]->main_line->uri, "l2");
    BOOST_CHECK_EQUAL(data.line_group_links[0].line_group->uri, "lg1");
    BOOST_CHECK_EQUAL(data.line_group_links[2].line_group->uri, "lg1");
    BOOST_CHECK_EQUAL(data.line_group_links[0].line->uri, "l2");
    BOOST_CHECK_EQUAL(data.line_group_links[2].line->uri, "l3");
    BOOST_CHECK_EQUAL(data.line_group_links[0].line->uri, "l2");
    BOOST_CHECK_EQUAL(data.line_group_links[2].line->uri, "l3");

    // Second group, only one line, only defined as the main_line
    BOOST_REQUIRE(data.line_groups[1]->main_line != nullptr);

    BOOST_CHECK_EQUAL(data.line_groups[1]->main_line->uri, "l3");
    BOOST_CHECK_EQUAL(data.line_group_links[1].line_group->uri, "lg2");
    BOOST_CHECK_EQUAL(data.line_group_links[1].line->uri, "l3");

    // stop_headsign
    const types::VehicleJourney* vj1 = data.vehicle_journeys.at(6);

    // vj_name feed from trip_short_name if empty from vj_headsign
    BOOST_REQUIRE_EQUAL(vj1->name.substr(0, 6), "NULL");
    // headsign of vj and stop_times
    BOOST_REQUIRE_EQUAL(vj1->headsign.substr(0, 6), "NULL");

    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[0]->headsign, "HS");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[1]->headsign, "HS");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[2]->headsign, "HS");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[3]->headsign, "HS");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[4]->headsign, "NULL");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[5]->headsign, "NULL");

    // Shift the frequency
    data.finalize_frequency();
    // Frequencies and boarding / alighting times (times are in UTC, first vj of trip_5 is UTC+1 in NTFS)
    vj1 = data.vehicle_journeys.at(8);
    BOOST_REQUIRE_EQUAL(vj1->is_frequency(), true);
    // start time is moved 5 minutes in the past because of boarding_time
    BOOST_REQUIRE_EQUAL(vj1->start_time, "04:55:00"_t);
    BOOST_REQUIRE_EQUAL(vj1->end_time, "26:55:00"_t);
    BOOST_REQUIRE_EQUAL(vj1->headway_secs, "01:00:00"_t);

    // vehicle_journey without trip_headsign takes trip_id value
    BOOST_REQUIRE_EQUAL(vj1->headsign, "trip_5_dst_1");
    // vj_name feed from trip_short_name if empty from vj_headsign
    BOOST_REQUIRE_EQUAL(vj1->name, "trip_5_short_name");

    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(0)->departure_time, 300);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(0)->boarding_time, 0);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(0)->alighting_time, 300);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(1)->departure_time, 900);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(1)->boarding_time, 0);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(1)->alighting_time, 1800);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(2)->departure_time, 1500);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(2)->boarding_time, 600);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(2)->alighting_time, 2400);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(3)->departure_time, 2100);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(3)->boarding_time, 2100);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(3)->alighting_time, 2400);
}
/*
complete production without beginning_date
*/
BOOST_AUTO_TEST_CASE(complete_production_date_without_beginning_date) {
    using namespace ed;
    connectors::FusioParser parser(ntfs_path);
    std::string beginning_date;
    bg::date start_date(bg::date(2015, 1, 1));
    bg::date end_date(bg::date(2015, 1, 15));

    bg::date_period period = parser.complete_production_date(beginning_date, start_date, end_date);
    BOOST_CHECK_EQUAL(period, bg::date_period(start_date, end_date + bg::days(1)));
}

/*
complete production with beginning_date
*/
BOOST_AUTO_TEST_CASE(complete_production_date_with_beginning_date) {
    using namespace ed;
    connectors::FusioParser parser(ntfs_path);
    /*
beginning_date  20150103

                                                    |--------------------------------------|
                                                start_date(20150105)               end_date(20150115)
production date :                                   |-----------------------------------------|
                                                 start_date                                 end_date + 1 Day
    */
    std::string beginning_date("20150103");
    bg::date start_date(bg::date(2015, 1, 5));
    bg::date end_date(bg::date(2015, 1, 15));

    bg::date_period period = parser.complete_production_date(beginning_date, start_date, end_date);

    BOOST_CHECK_EQUAL(period, bg::date_period(start_date, end_date + bg::days(1)));

    /*
beginning_date                                          20150107
                                                           |

                                                    |--------------------------------------|
                                                start_date(20150105)               end_date(20150115)
production date :                                          |---------------------------------|
                                                        beginning_date                   end_date + 1 Day
    */
    beginning_date = "20150107";
    start_date = bg::date(2015, 1, 5);
    end_date = bg::date(2015, 1, 15);

    period = parser.complete_production_date(beginning_date, start_date, end_date);

    BOOST_CHECK_EQUAL(period, bg::date_period(bg::date(2015, 1, 7), end_date + bg::days(1)));
}

/*
 Test start_date and en_date in file feed_info, with beginning_date

beginning_date  20150310

                                                    |--------------------------------------|
                                                start_date(20150325)               end_date(20150826)
production date :                                   |-----------------------------------------|
                                                 start_date                                 end_date + 1 Day

 */
BOOST_AUTO_TEST_CASE(ntfs_with_feed_start_end_date_1) {
    using namespace ed;

    ed::Data data;
    ed::connectors::FusioParser parser(ntfs_path);
    parser.fill(data, "20150310");

    // feed_info
    std::map<std::string, std::string> feed_info_test = {{"feed_start_date", "20150325"},
                                                         {"feed_end_date", "20150826"},
                                                         {"feed_publisher_name", "Ile de France open data"},
                                                         {"feed_publisher_url", "http://www.hove.com"},
                                                         {"feed_license", "ODBL"},
                                                         {"feed_creation_datetime", "20150415T153234"}};
    BOOST_CHECK_EQUAL_COLLECTIONS(data.feed_infos.begin(), data.feed_infos.end(), feed_info_test.begin(),
                                  feed_info_test.end());

    BOOST_REQUIRE_EQUAL(data.meta.production_date, boost::gregorian::date_period(boost::gregorian::date(2015, 3, 25),
                                                                                 boost::gregorian::date(2015, 8, 27)));
}

/*
 Test start_date and en_date in file feed_info, with beginning_date

beginning_date                                          20150327
                                                           |

                                                    |--------------------------------------|
                                                start_date(20150325)               end_date(20150826)
production date :                                          |---------------------------------|
                                                        beginning_date                   end_date + 1 Day

 */
BOOST_AUTO_TEST_CASE(ntfs_with_feed_start_end_date_2) {
    ed::Data data;

    ed::connectors::FusioParser parser(ntfs_path);
    parser.fill(data, "20150327");

    // feed_info
    std::map<std::string, std::string> feed_info_test = {{"feed_start_date", "20150325"},
                                                         {"feed_end_date", "20150826"},
                                                         {"feed_publisher_name", "Ile de France open data"},
                                                         {"feed_publisher_url", "http://www.hove.com"},
                                                         {"feed_license", "ODBL"},
                                                         {"feed_creation_datetime", "20150415T153234"}};
    BOOST_CHECK_EQUAL_COLLECTIONS(data.feed_infos.begin(), data.feed_infos.end(), feed_info_test.begin(),
                                  feed_info_test.end());
    BOOST_REQUIRE_EQUAL(data.meta.production_date, boost::gregorian::date_period(boost::gregorian::date(2015, 3, 27),
                                                                                 boost::gregorian::date(2015, 8, 27)));
}

BOOST_AUTO_TEST_CASE(sync_ntfs) {
    ed::Data data;

    ed::connectors::FusioParser parser(ntfs_path + "_v5");
    parser.fill(data, "20150327");

    // we check that the data have been correctly loaded
    BOOST_REQUIRE_EQUAL(data.lines.size(), 4);
    BOOST_REQUIRE_EQUAL(data.stop_point_connections.size(), 1);
    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 8);
    BOOST_CHECK_EQUAL(data.lines[0]->name, "ligne A Flexible");
    BOOST_CHECK_EQUAL(data.lines[0]->uri, "l1");
    BOOST_CHECK_EQUAL(data.lines[0]->text_color, "FFD700");
    BOOST_REQUIRE_EQUAL(data.routes.size(), 4);

    navitia::type::hasProperties has_properties;
    has_properties.set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
    has_properties.set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(data.stop_point_connections[0]->accessible(has_properties.properties()), true);
    BOOST_CHECK_EQUAL(data.stop_points[0]->accessible(has_properties.properties()), true);
    // ntfs_v5 retrocompatibility test on zone_id as fare_zone_id is absent in stops.txt
    BOOST_CHECK_EQUAL(data.stop_points[0]->fare_zone, "4");

    for (int i = 1; i < 8; i++) {
        BOOST_CHECK_EQUAL(data.stop_points[i]->accessible(has_properties.properties()), false);
    }

    BOOST_REQUIRE_EQUAL(data.datasets.size(), 1);
    BOOST_REQUIRE_EQUAL(data.contributors.size(), 1);
    BOOST_REQUIRE_EQUAL(data.contributors[0], data.datasets[0]->contributor);
    BOOST_CHECK_EQUAL(data.datasets[0]->desc, "dataset_test");
    BOOST_CHECK_EQUAL(data.datasets[0]->uri, "d1");
    BOOST_CHECK_EQUAL(data.datasets[0]->validation_period, boost::gregorian::date_period("20150826"_d, "20150926"_d));
    BOOST_CHECK_EQUAL(data.datasets[0]->realtime_level == nt::RTLevel::Base, true);
    BOOST_CHECK_EQUAL(data.datasets[0]->system, "obiti");

    // accepted side-effect (no link) as ntfs_v5 fixture does not contain datasets.txt, which is now required
    BOOST_CHECK(data.vehicle_journeys[0]->dataset == nullptr);

    const ed::types::VehicleJourney* vj = data.vehicle_journeys.front();
    navitia::type::hasVehicleProperties has_vehicle_properties;
    has_vehicle_properties.set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(vj->accessible(has_vehicle_properties.vehicles()), true);

    vj = data.vehicle_journeys.back();
    BOOST_CHECK_EQUAL(vj->accessible(has_vehicle_properties.vehicles()), false);
}

BOOST_AUTO_TEST_CASE(admin_stations_retrocompatibilty_tests) {
    // admin_stations.txt file contains "stop_id" and "station_id"
    // Following the norm, we only read "stop_id"
    {
        ed::Data data;
        ed::connectors::FusioParser parser(ntfs_path);
        parser.fill(data);
        BOOST_REQUIRE_EQUAL(data.admin_stop_areas.size(), 1);
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->admin, "admin:A");
        BOOST_REQUIRE_EQUAL(data.admin_stop_areas[0]->stop_area.size(), 1);
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->stop_area[0]->name, "Arrêt A");
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->stop_area[0]->uri, "SA:A");
    }

    // admin_stations.txt file contains only "stop_id"
    {
        ed::Data data;
        ed::connectors::FusioParser parser(ntfs_path + "_dst");
        parser.fill(data);
        BOOST_REQUIRE_EQUAL(data.admin_stop_areas.size(), 1);
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->admin, "admin:A");
        BOOST_REQUIRE_EQUAL(data.admin_stop_areas[0]->stop_area.size(), 1);
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->stop_area[0]->name, "Brunoy-Wittlich");
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->stop_area[0]->uri, "SCF:SA:SAOCE87976902");

        std::string prefix = "Navitia:";
        for (const auto& sa : data.stop_areas) {
            if (sa->uri == "SCF:SA:SAOCE87976902") {
                continue;
            }
            BOOST_CHECK_EQUAL(sa->uri.substr(0, prefix.length()), prefix);
        }
    }

    // For retrocompatibity
    // admin_stations.txt file contains only "station_id"
    // TODO : to remove after the data team update, it will become useless (NAVP-1285)
    {
        ed::Data data;
        ed::connectors::FusioParser parser(ntfs_path + "_v5");
        parser.fill(data);
        BOOST_REQUIRE_EQUAL(data.admin_stop_areas.size(), 1);
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->admin, "admin:A");
        BOOST_REQUIRE_EQUAL(data.admin_stop_areas[0]->stop_area.size(), 1);
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->stop_area[0]->name, "Arrêt A");
        BOOST_CHECK_EQUAL(data.admin_stop_areas[0]->stop_area[0]->uri, "SA:A");
    }
}

BOOST_AUTO_TEST_CASE(grid_rel_calendar_line_retrocompatibilty_tests) {
    // For backward compatibility
    // grid_rel_calendar_line.txt file contains "line_external_code"
    {
        ed::Data data;
        ed::connectors::FusioParser parser(ntfs_path + "_dst");
        parser.fill(data);
        BOOST_REQUIRE_EQUAL(data.calendars.size(), 6);
        // Test only one, it's the same logic for the 6 calendars
        BOOST_CHECK_EQUAL(data.calendars[0]->uri, "REG_LaV");
        BOOST_CHECK_EQUAL(data.calendars[0]->line_list.size(), 1);
        BOOST_CHECK_EQUAL(data.calendars[0]->line_list[0]->uri, "Nav55");
    }
}

BOOST_AUTO_TEST_CASE(stop_time_precision_retrocompatibilty_tests) {
    // Test 1 : parse stop_times/stop_time_precision field
    // 0 -> false
    // 1 -> false
    // 2 -> true
    {
        ed::Data data;
        ed::connectors::FusioParser parser(ntfs_path);
        parser.fill(data);
        for (const auto& stop : data.stops) {
            if (stop->stop_point->uri == "id_for_stop_time_precision_true") {
                BOOST_REQUIRE_EQUAL(stop->date_time_estimated, true);
            }
            if (stop->stop_point->uri == "id_for_stop_time_precision_false_1") {
                BOOST_REQUIRE_EQUAL(stop->date_time_estimated, false);
            }
            if (stop->stop_point->uri == "id_for_stop_time_precision_false_2") {
                BOOST_REQUIRE_EQUAL(stop->date_time_estimated, false);
            }
        }
    }

    // Test 2, the old way : parse stop_times/date_time_estimated filed
    // 0 -> false
    // 1 -> true
    {
        ed::Data data;
        ed::connectors::FusioParser parser(ntfs_path + "_dst");
        parser.fill(data);
        for (const auto& stop : data.stops) {
            if (stop->stop_point->uri == "SCF:SP:SPOCENoctilien87591180") {
                BOOST_REQUIRE_EQUAL(stop->date_time_estimated, true);
            }
            if (stop->stop_point->uri == "SCF:SP:SPOCENoctilien87594929") {
                BOOST_REQUIRE_EQUAL(stop->date_time_estimated, false);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(gtfs_stop_code_tests) {
    ed::Data data;
    ed::connectors::FusioParser parser(ntfs_path);
    parser.fill(data);
    BOOST_REQUIRE_EQUAL(data.if_object_code_exist("stop_code_for_sa", "gtfs_stop_code"), true);
    BOOST_REQUIRE_EQUAL(data.if_object_code_exist("stop_code_for_sp", "gtfs_stop_code"), true);
    BOOST_REQUIRE_EQUAL(data.if_object_code_exist("stop_code_not_exist", "gtfs_stop_code"), false);
    BOOST_REQUIRE_EQUAL(data.if_object_code_exist("J", "gtfs_stop_code"), true);
    BOOST_REQUIRE_EQUAL(data.if_object_code_exist("E", "external_code"), true);
    BOOST_REQUIRE_EQUAL(data.if_object_code_exist("A", "external_code"), true);
}

ed::types::VehicleJourney* find_vj_by_heasign(ed::Data& data, const std::string headsign) {
    auto it = boost::find_if(data.vehicle_journeys, [&](const auto vj) { return vj->headsign == headsign; });

    if (it == data.vehicle_journeys.end())
        throw std::invalid_argument("No Vj with headsign : " + headsign);

    return *it;
}

BOOST_AUTO_TEST_CASE(gtfs_hlp_pickup_and_drop_off_from_data) {
    ed::connectors::FusioParser parser(ntfs_path + "_hlp");
    ed::Data data;
    parser.fill(data);
    data.complete();

    /*
     * When nothing is stated for 'pickup_type' and 'drop_off_type' in the data (stop_times.txt)
     * we allow ONLY FOR a stay-in on DIFFERENT stop points :
     *      * pickup at the last stop for the starting vehicle of a stay-in  (A)
     *      * drop-off at the first stop for the ending vehicle of a stay-in  (B)
     */
    {
        auto start_stay_in_vehicle = find_vj_by_heasign(data, "start_stayin_diff_sp");
        auto start_vehicle_first = *start_stay_in_vehicle->stop_time_list.begin();
        BOOST_CHECK_EQUAL(start_vehicle_first->pick_up_allowed, true);
        BOOST_CHECK_EQUAL(start_vehicle_first->drop_off_allowed, false);

        auto start_vehicle_last = *start_stay_in_vehicle->stop_time_list.rbegin();
        BOOST_CHECK_EQUAL(start_vehicle_last->pick_up_allowed, true);  // (A)
        BOOST_CHECK_EQUAL(start_vehicle_last->drop_off_allowed, true);

        auto end_stay_in_vehicle = find_vj_by_heasign(data, "end_stayin_diff_sp");
        auto end_vehicle_start = *end_stay_in_vehicle->stop_time_list.begin();
        BOOST_CHECK_EQUAL(end_vehicle_start->pick_up_allowed, true);
        BOOST_CHECK_EQUAL(end_vehicle_start->drop_off_allowed, true);  // (B)

        auto end_vehicle_last = *end_stay_in_vehicle->stop_time_list.rbegin();
        BOOST_CHECK_EQUAL(end_vehicle_last->pick_up_allowed, false);
        BOOST_CHECK_EQUAL(end_vehicle_last->drop_off_allowed, true);
    }

    {
        /*
         * If 'pickup_type' and 'drop_off_type' are stated in the data, we want to honour it.
         */
        auto start_stay_in_vehicle = find_vj_by_heasign(data, "start_stayin_diff_sp_with_data");
        auto start_vehicle_first = *start_stay_in_vehicle->stop_time_list.begin();
        BOOST_CHECK_EQUAL(start_vehicle_first->pick_up_allowed, true);
        BOOST_CHECK_EQUAL(start_vehicle_first->drop_off_allowed, false);

        auto start_vehicle_last = *start_stay_in_vehicle->stop_time_list.rbegin();
        BOOST_CHECK_EQUAL(start_vehicle_last->pick_up_allowed, false);  // forbidden in the data
        BOOST_CHECK_EQUAL(start_vehicle_last->drop_off_allowed, true);

        auto end_stay_in_vehicle = find_vj_by_heasign(data, "end_stayin_diff_sp_with_data");
        auto end_vehicle_start = *end_stay_in_vehicle->stop_time_list.begin();
        BOOST_CHECK_EQUAL(end_vehicle_start->pick_up_allowed, true);
        BOOST_CHECK_EQUAL(end_vehicle_start->drop_off_allowed, false);  // forbidden in the data

        auto end_vehicle_last = *end_stay_in_vehicle->stop_time_list.rbegin();
        BOOST_CHECK_EQUAL(end_vehicle_last->pick_up_allowed, false);
        BOOST_CHECK_EQUAL(end_vehicle_last->drop_off_allowed, true);
    }

    /*
     * No special case for a stray-in on the same stop point
     */
    {
        auto start_stay_in_vehicle = find_vj_by_heasign(data, "start_stayin_same_sp");
        auto start_vehicle_first = *start_stay_in_vehicle->stop_time_list.begin();
        BOOST_CHECK_EQUAL(start_vehicle_first->pick_up_allowed, true);
        BOOST_CHECK_EQUAL(start_vehicle_first->drop_off_allowed, false);

        auto start_vehicle_last = *start_stay_in_vehicle->stop_time_list.rbegin();
        BOOST_CHECK_EQUAL(start_vehicle_last->pick_up_allowed, false);  // (A)
        BOOST_CHECK_EQUAL(start_vehicle_last->drop_off_allowed, true);

        auto end_stay_in_vehicle = find_vj_by_heasign(data, "end_stayin_same_sp");
        auto end_vehicle_start = *end_stay_in_vehicle->stop_time_list.begin();
        BOOST_CHECK_EQUAL(end_vehicle_start->pick_up_allowed, true);
        BOOST_CHECK_EQUAL(end_vehicle_start->drop_off_allowed, false);  // (B)

        auto end_vehicle_last = *end_stay_in_vehicle->stop_time_list.rbegin();
        BOOST_CHECK_EQUAL(end_vehicle_last->pick_up_allowed, false);
        BOOST_CHECK_EQUAL(end_vehicle_last->drop_off_allowed, true);
    }

    {
        /*
         * If 'pickup_type' and 'drop_off_type' are stated in the data, we want to honour it.
         * The vehicle skips certain stops
         */
        auto start_stay_in_vehicle = find_vj_by_heasign(data, "start_stayin_same_sp_with_data");
        auto start_vehicle_first = *start_stay_in_vehicle->stop_time_list.begin();
        BOOST_CHECK_EQUAL(start_vehicle_first->pick_up_allowed, true);
        BOOST_CHECK_EQUAL(start_vehicle_first->drop_off_allowed, false);
        BOOST_CHECK_EQUAL(start_vehicle_first->skipped_stop, false);

        auto start_vehicle_last = *start_stay_in_vehicle->stop_time_list.rbegin();
        BOOST_CHECK_EQUAL(start_vehicle_last->pick_up_allowed, false);  // forbidden in the data
        BOOST_CHECK_EQUAL(start_vehicle_last->drop_off_allowed, true);
        BOOST_CHECK_EQUAL(start_vehicle_first->skipped_stop, false);

        BOOST_CHECK_EQUAL(start_stay_in_vehicle->stop_time_list[2]->skipped_stop, false);
        BOOST_CHECK_EQUAL(start_stay_in_vehicle->stop_time_list[2]->pick_up_allowed, false);
        BOOST_CHECK_EQUAL(start_stay_in_vehicle->stop_time_list[2]->drop_off_allowed, true);

        auto end_stay_in_vehicle = find_vj_by_heasign(data, "end_stayin_same_sp_with_data");
        auto end_vehicle_start = *end_stay_in_vehicle->stop_time_list.begin();
        BOOST_CHECK_EQUAL(end_vehicle_start->pick_up_allowed, true);
        BOOST_CHECK_EQUAL(end_vehicle_start->drop_off_allowed, false);  // forbidden in the data

        auto end_vehicle_last = *end_stay_in_vehicle->stop_time_list.rbegin();
        BOOST_CHECK_EQUAL(end_vehicle_last->pick_up_allowed, false);
        BOOST_CHECK_EQUAL(end_vehicle_last->drop_off_allowed, true);

        BOOST_CHECK_EQUAL(end_stay_in_vehicle->stop_time_list[2]->skipped_stop, true);
        BOOST_CHECK_EQUAL(end_stay_in_vehicle->stop_time_list[2]->pick_up_allowed, false);
        BOOST_CHECK_EQUAL(end_stay_in_vehicle->stop_time_list[2]->drop_off_allowed, false);
    }
}

BOOST_AUTO_TEST_CASE(adresses_tests) {
    {
        ed::Data data;
        ed::connectors::FusioParser parser(ntfs_path + "_v5");
        parser.fill(data);

        BOOST_REQUIRE_EQUAL(data.addresses_from_ntfs.size(), 9);
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[0]->id, "SA:A:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[0]->street_name, "SA:A STREET_NAME");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[0]->house_number, "9");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[1]->id, "SA:B:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[1]->street_name, "SA:B STREET_NAME");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[1]->house_number, "99999999");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[2]->id, "SA:C:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[2]->street_name, "");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[2]->house_number, "");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[3]->id, "SA:D:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[3]->street_name, "SA:D STREET_NAME");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[3]->house_number, "");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[4]->id, "SP:A:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[4]->street_name, "");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[4]->house_number, "");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[5]->id, "SP:D:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[5]->street_name, "SP:D STREET_NAME");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[5]->house_number, "25");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[6]->id, "SP:E:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[6]->street_name, "SP:E STREET_NAME");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[6]->house_number, "0");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[7]->id, "SP:F:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[7]->street_name, "SP:F STREET_NAME");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[7]->house_number, "");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[8]->id, "SP:J:ADD_ID");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[8]->street_name, "SP:J STREET_NAME");
        BOOST_CHECK_EQUAL(data.addresses_from_ntfs[8]->house_number, "FACE AU 23");
    }
}

BOOST_AUTO_TEST_CASE(input_output_tests) {
    ed::Data data;
    ed::connectors::FusioParser parser(ntfs_path + "_v5");
    parser.fill(data);

    BOOST_REQUIRE_EQUAL(data.access_points.size(), 2);
    BOOST_CHECK_EQUAL(data.access_points[0]->uri, "IO:1");
    BOOST_CHECK_EQUAL(data.access_points[0]->name, "IO 1");
    BOOST_CHECK_EQUAL(data.access_points[0]->stop_code, "stop_code_io_1");
    BOOST_CHECK_EQUAL(data.access_points[0]->parent_station, "SA:A");
    BOOST_CHECK_EQUAL(data.access_points[0]->visible, 1);
    BOOST_CHECK_EQUAL(data.access_points[1]->uri, "IO:2");
    BOOST_CHECK_EQUAL(data.access_points[1]->name, "IO 2");
    BOOST_CHECK_EQUAL(data.access_points[1]->stop_code, "stop_code_io_2");
    BOOST_CHECK_EQUAL(data.access_points[1]->parent_station, "SA:B");
    BOOST_CHECK_EQUAL(data.access_points[1]->visible, 1);
}

BOOST_AUTO_TEST_CASE(pathway_tests) {
    ed::Data data;
    ed::connectors::FusioParser parser(ntfs_path + "_v5");
    parser.fill(data);

    BOOST_REQUIRE_EQUAL(data.pathways.size(), 3);

    auto test = [](const ed::types::PathWay& pathway, const std::string& expected_uri, const std::string& expected_name,
                   const std::string& expected_from_stop_id, const std::string& expected_to_stop_id,
                   const int expected_pathway_mode, const bool expected_is_bidirectional, const int expected_length,
                   const int expected_traversal_time, const int expected_stair_count, const int expected_max_slope,
                   const int expected_min_width, const std::string& expected_signposted_as,
                   const std::string& expected_reversed_signposted_as) {
        BOOST_CHECK_EQUAL(pathway.uri, expected_uri);
        BOOST_CHECK_EQUAL(pathway.name, expected_name);
        BOOST_CHECK_EQUAL(pathway.from_stop_id, expected_from_stop_id);
        BOOST_CHECK_EQUAL(pathway.to_stop_id, expected_to_stop_id);
        BOOST_CHECK_EQUAL(pathway.pathway_mode, expected_pathway_mode);
        BOOST_CHECK_EQUAL(pathway.is_bidirectional, expected_is_bidirectional);
        BOOST_CHECK_EQUAL(pathway.length, expected_length);
        BOOST_CHECK_EQUAL(pathway.traversal_time, expected_traversal_time);
        BOOST_CHECK_EQUAL(pathway.stair_count, expected_stair_count);
        BOOST_CHECK_EQUAL(pathway.max_slope, expected_max_slope);
        BOOST_CHECK_EQUAL(pathway.min_width, expected_min_width);
        BOOST_CHECK_EQUAL(pathway.signposted_as, expected_signposted_as);
        BOOST_CHECK_EQUAL(pathway.reversed_signposted_as, expected_reversed_signposted_as);
    };

    test(*data.pathways[0], "SP:A:IO:1", "SP:A:IO:1", "SP:A", "IO:1", 3, true, 68, 87, -1, -1, -1, "", "");

    test(*data.pathways[1], "SP:B:IO:2", "SP:B:IO:2", "SP:B", "IO:2", 3, false, 68, 87, 3, 30, 2, "", "");

    test(*data.pathways[2], "SP:B:IO:1", "SP:B:IO:1", "SP:B", "IO:1", 3, true, 42, 60, 40, 30, 2, "", "");
}
