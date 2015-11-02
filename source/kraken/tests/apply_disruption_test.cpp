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
#define BOOST_TEST_MODULE test_realtime
#include <boost/test/unit_test.hpp>
#include "tests/utils_test.h"
#include "routing/raptor.h"
#include "type/data.h"
#include "ed/build_helper.h"
#include "kraken/apply_disruption.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

namespace nt = navitia::type;
namespace pt = boost::posix_time;

struct SimpleDataset {
    SimpleDataset() {
        b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj("A", "000001", "", true, "vj:A-2")("stop1", "08:00"_t)("stop2", "09:00"_t);
    }

    ed::builder b = ed::builder("20150928");
    void apply(const nt::disruption::Disruption& dis) {
        return navitia::apply_disruption(dis, *b.data->pt_data, *b.data->meta);
    }
};

BOOST_FIXTURE_TEST_CASE(simple_train_cancellation, SimpleDataset) {
    using btp = boost::posix_time::time_period;
    const auto& disrup = b.impact(nt::RTLevel::RealTime)
                     .severity(nt::disruption::Effect::NO_SERVICE)
                     .on(nt::Type_e::VehicleJourney, "vj:A-1")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();

    const auto& pt_data = b.data->pt_data;
    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 2);
    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 1);
    const auto& vj = pt_data->vehicle_journeys_map.at("vj:A-1");
    BOOST_CHECK_EQUAL(vj->base_validity_pattern(), vj->rt_validity_pattern());

    //TODO
//    apply(*disrup);

//    // we should not have created any objects save for one validity_pattern
//    BOOST_REQUIRE_EQUAL(pt_data->vehicle_journeys.size(), 2);
//    BOOST_REQUIRE_EQUAL(pt_data->meta_vjs.size(), 2);
//    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
//    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
//    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
//    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());

//    //the rt vp must be empty
//    BOOST_CHECK_EQUAL(vj->rt_validity_pattern()->days, navitia::type::ValidityPattern::year_bitset());

//    // we add a second time the realtime message, it should not change anything
//    navitia::handle_realtime(feed_id, timestamp, trip_update, *b.data);

//    // we should not have created any objects save for one validity_pattern
//    BOOST_CHECK_EQUAL(pt_data->vehicle_journeys.size(), 1);
//    BOOST_CHECK_EQUAL(pt_data->routes.size(), 1);
//    BOOST_CHECK_EQUAL(pt_data->lines.size(), 1);
//    BOOST_CHECK_EQUAL(pt_data->validity_patterns.size(), 2);
//    BOOST_CHECK_NE(vj->base_validity_pattern(), vj->rt_validity_pattern());
}
