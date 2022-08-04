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
#define BOOST_TEST_MODULE access_point_test

#include <boost/test/unit_test.hpp>

#include "ed/build_helper.h"
#include "access_point/access_point_api.h"
#include "utils/logger.h"
#include "tests/utils_test.h"

#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;
using namespace navitia;
using boost::posix_time::time_period;
using std::string;
using std::vector;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace {

class AccessPointTestFixture {
public:
    ed::builder b;
    const type::Data& data;

    AccessPointTestFixture() : b("20190101"), data(*b.data) {
        b.sa("saA", 2.361795, 48.871871)("spA", 2.361795, 48.871871);
        b.sa("saB", 2.362795, 48.872871)("spB", 2.362795, 48.872871);
        b.sa("saC", 2.363795, 48.873871)("spC", 2.363795, 48.873871);
        b.vj("A")("spA", "08:00"_t)("spB", "10:00"_t);
        b.vj("B")("spC", "09:00"_t)("spD", "11:00"_t);
        b.vj("C")("spA", "10:00"_t)("spC", "12:00"_t);
        b.data->complete();
        b.make();

        b.add_access_point("spA", "AP1", true, true, 10, 23, 48.874871, 2.364795);
        b.add_access_point("spA", "AP2", true, false, 13, 26, 48.875871, 2.365795);
        b.add_access_point("spC", "AP3", true, false, 12, 36, 48.876871, 2.366795);
        b.add_access_point("spC", "AP2", true, false, 13, 26, 48.875871, 2.365795);
    }
};

void complete_pb_creator(navitia::PbCreator& pb_creator, const type::Data& data) {
    const ptime since = "20190101T000000"_dt;
    const ptime until = "21190101T000000"_dt;
    pb_creator.init(&data, since, time_period(since, until));
}

bool ap_comp(pbnavitia::AccessPoint& ap1, pbnavitia::AccessPoint& ap2) {
    return ap1.uri() < ap2.uri();
}

BOOST_FIXTURE_TEST_CASE(test_access_point_reading, AccessPointTestFixture) {
    string filter = "";
    int count = 20;
    int depth = 0;
    int start_page = 0;
    vector<string> forbidden_uris = {};

    navitia::PbCreator pb_creator;
    complete_pb_creator(pb_creator, data);
    access_point::access_points(pb_creator, filter, count, depth, start_page, forbidden_uris);
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.access_points().size(), 3);
    auto access_points = resp.access_points();
    sort(access_points.begin(), access_points.end(), ap_comp);
    auto ap = access_points.Get(0);
    BOOST_REQUIRE_EQUAL(ap.uri(), "AP1");
    BOOST_REQUIRE_EQUAL(ap.is_entrance(), true);
    BOOST_REQUIRE_EQUAL(ap.is_exit(), true);
    BOOST_REQUIRE_EQUAL(ap.length(), 10);
    BOOST_REQUIRE_EQUAL(ap.traversal_time(), 23);
    ap = access_points.Get(1);
    BOOST_REQUIRE_EQUAL(ap.uri(), "AP2");
    BOOST_REQUIRE_EQUAL(ap.is_entrance(), true);
    BOOST_REQUIRE_EQUAL(ap.is_exit(), false);
    BOOST_REQUIRE_EQUAL(ap.length(), 13);
    BOOST_REQUIRE_EQUAL(ap.traversal_time(), 26);
    ap = access_points.Get(2);
    BOOST_REQUIRE_EQUAL(ap.uri(), "AP3");
    BOOST_REQUIRE_EQUAL(ap.is_entrance(), true);
    BOOST_REQUIRE_EQUAL(ap.is_exit(), false);
    BOOST_REQUIRE_EQUAL(ap.length(), 12);
    BOOST_REQUIRE_EQUAL(ap.traversal_time(), 36);
}

BOOST_FIXTURE_TEST_CASE(test_access_point_filtering, AccessPointTestFixture) {
    {
        string filter = "stop_point.uri=spA";
        int count = 20;
        int depth = 0;
        int start_page = 0;
        vector<string> forbidden_uris = {};

        navitia::PbCreator pb_creator;
        complete_pb_creator(pb_creator, data);
        access_point::access_points(pb_creator, filter, count, depth, start_page, forbidden_uris);
        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.access_points().size(), 2);

        auto access_points = resp.access_points();
        sort(access_points.begin(), access_points.end(), ap_comp);
        auto ap = access_points.Get(0);
        BOOST_REQUIRE_EQUAL(ap.uri(), "AP1");
        ap = access_points.Get(1);
        BOOST_REQUIRE_EQUAL(ap.uri(), "AP2");
    }
    {
        string filter = "stop_point.uri=spB";
        int count = 20;
        int depth = 0;
        int start_page = 0;
        vector<string> forbidden_uris = {};

        navitia::PbCreator pb_creator;
        complete_pb_creator(pb_creator, data);
        access_point::access_points(pb_creator, filter, count, depth, start_page, forbidden_uris);
        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.access_points().size(), 0);
    }
    {
        string filter = "stop_area.uri=saC";
        int count = 20;
        int depth = 0;
        int start_page = 0;
        vector<string> forbidden_uris = {};

        navitia::PbCreator pb_creator;
        complete_pb_creator(pb_creator, data);
        access_point::access_points(pb_creator, filter, count, depth, start_page, forbidden_uris);
        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.access_points().size(), 2);

        auto access_points = resp.access_points();
        sort(access_points.begin(), access_points.end(), ap_comp);
        auto ap = access_points.Get(0);
        BOOST_REQUIRE_EQUAL(ap.uri(), "AP2");
        ap = access_points.Get(1);
        BOOST_REQUIRE_EQUAL(ap.uri(), "AP3");
    }
}

}  // namespace
