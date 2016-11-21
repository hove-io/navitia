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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_comments

#include <boost/test/unit_test.hpp>
#include "ed/build_helper.h"
#include "type/pt_data.h"
#include "type/type.h"
#include "tests/utils_test.h"
#include "type/comment_container.h"
#include <memory>
#include <boost/archive/text_oarchive.hpp>
#include <string>
#include <type_traits>


struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );


BOOST_AUTO_TEST_CASE(comment_map_test) {

    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    b.data->pt_data->index();
    b.finish();

    nt::Comments& comments_container = b.data->pt_data->comments;

    const nt::Line* l = b.data->pt_data->lines[0];
    comments_container.add(l, "bob");
    nt::Line* l2 = b.data->pt_data->lines[0]; //to test with const or no const type
    comments_container.add(l2, "bobette");

    comments_container.add(b.data->pt_data->stop_areas[0], "sabob");
    comments_container.add(b.data->pt_data->stop_points[0], "spbob");
    comments_container.add(b.data->pt_data->vehicle_journeys[0], "vj com");

    comments_container.add(b.data->pt_data->vehicle_journeys[0]->stop_time_list.front(), "st com");

    std::vector<std::string> expected = {"bob", "bobette"};
    BOOST_CHECK_EQUAL(comments_container.get(b.data->pt_data->lines[0]), expected);

    expected = {"sabob"};
    BOOST_CHECK_EQUAL(comments_container.get(b.data->pt_data->stop_areas[0]), expected);

    expected = {"spbob"};
    BOOST_CHECK_EQUAL(comments_container.get(b.data->pt_data->stop_points[0]), expected);

    expected = {"vj com"};
    BOOST_CHECK_EQUAL(comments_container.get(b.data->pt_data->vehicle_journeys[0]), expected);

    expected = {"st com"};
    BOOST_CHECK_EQUAL(comments_container.get(b.data->pt_data->vehicle_journeys[0]->stop_time_list.front()), expected);


    boost::archive::text_oarchive oa(std::cout);
    oa & comments_container;
}
