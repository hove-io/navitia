
/* Copyright © 2001-2019, Canal TP and/or its affiliates. All rights reserved.

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
#define BOOST_TEST_MODULE chaos_db_disruption_tests

#include "type/type.h"
#include "type/datetime.h"
#include "kraken/fill_disruption_from_database.h"

#include <google/protobuf/repeated_field.h>
#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <string>
#include <exception>
#include <tuple>

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

BOOST_AUTO_TEST_CASE(chaos_fill_disruptions_tests) {
    std::string connection_string = std::getenv("CHAOS_DB_CONNECTION_STR");

    navitia::type::PT_Data pt_data;
    navitia::type::MetaData metadata;
    metadata.production_date = boost::gregorian::date_period("20010101"_d, "20230101"_d);
    metadata.publication_date = "20010101T010000"_dt;
    metadata.shape = "shape";
    metadata.publisher_name = "publisher_name";
    metadata.publisher_url = "publisher_url";
    metadata.license = "license";
    metadata.instance_name = "instance_name";
    metadata.dataset_created_at = "20010101T010203"_dt;
    metadata.poi_source = "poi_source";
    metadata.street_network_source = "kraken";

    std::vector<chaos::Disruption> disruptions;

    auto disruption_callback = [&disruptions](const chaos::Disruption& d, navitia::type::PT_Data&,
                                              const navitia::type::MetaData&) { disruptions.emplace_back(d); };

    navitia::DisruptionDatabaseReader reader(pt_data, metadata, disruption_callback);

    try {
        navitia::fill_disruption_from_database(
            connection_string,  // "host=172.17.0.2 user=postgres dbname=chaos_loading",
            metadata.production_date, reader, {"shortterm.tr_sytral"});
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    BOOST_CHECK_EQUAL(disruptions.size(), 5);
    BOOST_CHECK_EQUAL(disruptions[0].id(), "095324ea-9370-11e9-9678-005056a40962");
    BOOST_CHECK_EQUAL(disruptions[1].id(), "76c5f3b2-936c-11e9-9678-005056a40962");
    BOOST_CHECK_EQUAL(disruptions[2].id(), "8a1c6cf0-8d15-11e9-b1ac-005056a40962");
    BOOST_CHECK_EQUAL(disruptions[3].id(), "bd3ee57a-968b-11e9-951b-005056a40962");
    BOOST_CHECK_EQUAL(disruptions[4].id(), "e14423e0-90d7-11e9-a640-005056a40962");

    BOOST_CHECK_EQUAL(disruptions[0].reference(), "msa-20190620172619");
    BOOST_CHECK_EQUAL(disruptions[0].publication_period().start(), 1561044000);
    BOOST_CHECK_EQUAL(disruptions[0].publication_period().end(), 1567272000);
    BOOST_CHECK_EQUAL(disruptions[0].created_at(), 1561044501);
    BOOST_CHECK_EQUAL(disruptions[0].updated_at(), 1561045070);
    BOOST_CHECK_EQUAL(disruptions[0].has_cause(), true);
    BOOST_REQUIRE_EQUAL(disruptions[0].impacts().size(), 1);
    BOOST_CHECK_EQUAL(disruptions[0].impacts().Get(0).id(), "0953b040-9370-11e9-9678-005056a40962");
    BOOST_REQUIRE_EQUAL(disruptions[0].tags().size(), 1);
    BOOST_CHECK_EQUAL(disruptions[0].note(), "");
    BOOST_REQUIRE_EQUAL(disruptions[0].localization().size(), 0);
    BOOST_CHECK_EQUAL(disruptions[0].contributor(), "shortterm.tr_sytral");
    BOOST_REQUIRE_EQUAL(disruptions[0].properties().size(), 0);
}
