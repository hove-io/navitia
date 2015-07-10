/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

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

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

const std::string ntfs_path = std::string(navitia::config::fixtures_dir) + "/ed/ntfs";

BOOST_AUTO_TEST_CASE(parse_small_ntfs_dataset) {
    using namespace ed;

    Data data;
    connectors::FusioParser parser(ntfs_path);

    parser.fill(data);

    //we check that the data have been correctly loaded
    BOOST_REQUIRE_EQUAL(data.networks.size(), 1);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "ligne flixible");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "ligneflexible");

    BOOST_REQUIRE_EQUAL(data.stop_areas.size(), 11);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->name, "Arrêt A");
    BOOST_CHECK_EQUAL(data.stop_areas[0]->uri, "SA:A");
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lat(), 45.0296, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lon(), 0.5881, 0.1);

    //timzeone check
    //no timezone is given for the stop area in this dataset, to the agency time zone (the default one) is taken
    for (auto sa: data.stop_areas) {
        BOOST_CHECK_EQUAL(sa->time_zone_with_name.first, "Europe/Paris");
    }

    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 8);
    BOOST_CHECK_EQUAL(data.stop_points[0]->uri, "SP:A");
    BOOST_CHECK_EQUAL(data.stop_points[0]->name, "Arrêt A");
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lat(), 45.0296, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lon(), 0.5881, 0.1);
    BOOST_CHECK_EQUAL(data.stop_points[0]->stop_area, data.stop_areas[0]);

    //we should have one zonal stop point
    BOOST_REQUIRE_EQUAL(boost::count_if(data.stop_points, [](const types::StopPoint* sp) {
                            return sp->is_zonal && sp->name == "Beleymas" && sp->area;
                        }), 1);

    BOOST_REQUIRE_EQUAL(data.lines.size(), 3);
    BOOST_REQUIRE_EQUAL(data.routes.size(), 3);

    //chekc comments
    BOOST_REQUIRE_EQUAL(data.comment_by_id.size(), 2);
    BOOST_CHECK_EQUAL(data.comment_by_id["bob"], "bob is in the kitchen");
    BOOST_CHECK_EQUAL(data.comment_by_id["bobette"], "test comment");

    //7 objects have comments
    //the file contains wrongly formated comments, but they are skiped
    BOOST_REQUIRE_EQUAL(data.comments.size(), 5);

    std::map<ed::types::pt_object_header, std::vector<std::string>> expected_comments = {
        {ed::types::make_pt_object(data.routes[0]), {"bob"}},
        {ed::types::make_pt_object(data.vehicle_journeys[4]), {"bobette"}},
        {ed::types::make_pt_object(data.vehicle_journeys[5]), {"bobette"}}, //split too
        {ed::types::make_pt_object(data.stop_areas[2]), {"bob"}},
        {ed::types::make_pt_object(data.stop_points[3]), {"bobette"}}
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.comments.begin(), data.comments.end(),
                                  expected_comments.begin(), expected_comments.end());


    std::map<const ed::types::StopTime*, std::vector<std::string>> expected_st_comments = {
            {{data.stops[6]}, {"bob", "bobette"}}, //stoptimes are split
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

    // feed_info
    std::map<std::string, std::string> feed_info_test ={
        {"feed_start_date","20150325"},
        {"feed_end_date","20150826"},
        {"feed_publisher_name","Ile de France open data"},
        {"feed_publisher_url","http://www.canaltp.fr"},
        {"feed_license","ODBL"}
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(data.feed_infos.begin(), data.feed_infos.end(),
                                  feed_info_test.begin(), feed_info_test.end());
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

}

