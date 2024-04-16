
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
#define BOOST_TEST_MODULE chaos_db_disruption_tests

#include "type/datetime.h"
#include "type/meta_vehicle_journey.h"
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
            metadata.production_date, reader, {"shortterm.tr_sytral"}, 1000000);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    BOOST_REQUIRE_EQUAL(disruptions.size(), 5);
    BOOST_CHECK_EQUAL(disruptions[0].id(), "095324ea-9370-11e9-9678-005056a40962");
    BOOST_CHECK_EQUAL(disruptions[1].id(), "76c5f3b2-936c-11e9-9678-005056a40962");
    BOOST_CHECK_EQUAL(disruptions[2].id(), "8a1c6cf0-8d15-11e9-b1ac-005056a40962");
    BOOST_CHECK_EQUAL(disruptions[3].id(), "bd3ee57a-968b-11e9-951b-005056a40962");
    BOOST_CHECK_EQUAL(disruptions[4].id(), "e14423e0-90d7-11e9-a640-005056a40962");

    auto& disruption = disruptions[0];
    BOOST_CHECK_EQUAL(disruption.reference(), "msa-20190620172619");
    BOOST_CHECK_EQUAL(disruption.publication_period().start(), 1561044000);
    BOOST_CHECK_EQUAL(disruption.publication_period().end(), 1567272000);
    BOOST_CHECK_EQUAL(disruption.created_at(), 1561044501);
    BOOST_CHECK_EQUAL(disruption.updated_at(), 1561045070);
    BOOST_CHECK_EQUAL(disruption.has_cause(), true);
    BOOST_REQUIRE_EQUAL(disruption.impacts().size(), 1);
    BOOST_REQUIRE_EQUAL(disruption.tags().size(), 1);
    BOOST_CHECK_EQUAL(disruption.note(), "");
    BOOST_CHECK_EQUAL(disruption.localization().size(), 0);
    BOOST_CHECK_EQUAL(disruption.contributor(), "shortterm.tr_sytral");
    BOOST_CHECK_EQUAL(disruption.properties().size(), 0);

    auto& impact = disruption.impacts().Get(0);
    BOOST_CHECK_EQUAL(impact.id(), "0953b040-9370-11e9-9678-005056a40962");
    BOOST_CHECK_EQUAL(impact.created_at(), 1561044501);
    BOOST_CHECK_EQUAL(impact.updated_at(), 1561045070);
    BOOST_REQUIRE_EQUAL(impact.application_periods().size(), 1);
    BOOST_REQUIRE_EQUAL(impact.messages().size(), 2);
    BOOST_CHECK_EQUAL(impact.send_notifications(), false);
    // The same line can be present in more than one line_sections of the same impact
    BOOST_REQUIRE_EQUAL(impact.informed_entities().size(), 2);
    BOOST_CHECK_EQUAL(impact.informed_entities().Get(0).pt_object_type(), chaos::PtObject_Type_line_section);
    auto& line_section_1 = impact.informed_entities().Get(0);
    BOOST_CHECK_EQUAL(line_section_1.pt_line_section().line().uri(), "line:tcl:D");
    BOOST_CHECK_EQUAL(line_section_1.pt_line_section().start_point().uri(), "stop_area:tcl:SA:5083");
    BOOST_CHECK_EQUAL(line_section_1.pt_line_section().end_point().uri(), "stop_area:tcl:SA:5062");
    BOOST_CHECK_EQUAL(line_section_1.pt_line_section().routes().size(), 0);

    BOOST_CHECK_EQUAL(impact.informed_entities().Get(1).pt_object_type(), chaos::PtObject_Type_line_section);
    auto line_section_2 = impact.informed_entities().Get(1);
    BOOST_CHECK_EQUAL(line_section_2.pt_line_section().line().uri(), "line:tcl:D");
    BOOST_CHECK_EQUAL(line_section_2.pt_line_section().start_point().uri(), "stop_area:tcl:SA:5062");
    BOOST_CHECK_EQUAL(line_section_2.pt_line_section().end_point().uri(), "stop_area:tcl:SA:5083");
    BOOST_CHECK_EQUAL(line_section_2.pt_line_section().routes().size(), 0);

    auto& application_period = impact.application_periods().Get(0);
    BOOST_CHECK_EQUAL(application_period.start(), 1561044000);
    BOOST_CHECK_EQUAL(application_period.end(), 1567272000);

    auto& message1 = impact.messages().Get(0);
    BOOST_CHECK_EQUAL(message1.text(), "Troncon metro D Laennec / Monplaisir. Test");
    BOOST_CHECK_EQUAL(message1.has_channel(), true);
    BOOST_CHECK_EQUAL(message1.channel().id(), "8e76d508-23d7-11e9-9312-005056a40962");
    BOOST_CHECK_EQUAL(message1.channel().name(), "titre");
    BOOST_CHECK_EQUAL(message1.channel().content_type(), "text/plain");
    BOOST_CHECK_EQUAL(message1.channel().max_size(), 50);
    BOOST_CHECK_EQUAL(message1.channel().created_at(), 1548774432);
    BOOST_CHECK_EQUAL(message1.channel().updated_at(), 1551277632);
    BOOST_REQUIRE_EQUAL(message1.channel().types().size(), 1);
    BOOST_CHECK_EQUAL(message1.channel().types().Get(0), chaos::Channel::title);
    BOOST_CHECK_EQUAL(message1.created_at(), 1561044501);
    BOOST_CHECK_EQUAL(message1.updated_at(), 0);

    auto& message2 = impact.messages().Get(1);
    BOOST_CHECK_EQUAL(message2.text(), "<p>Troncon metro D Laennec / Monplaisir. Test<br></p>");
    BOOST_CHECK_EQUAL(message2.has_channel(), true);
    BOOST_CHECK_EQUAL(message2.channel().id(), "d1ed724a-14bb-11e9-a032-005056a40962");
    BOOST_CHECK_EQUAL(message2.channel().name(), "web et mobile");
    BOOST_CHECK_EQUAL(message2.channel().content_type(), "text/html");
    BOOST_CHECK_EQUAL(message2.channel().max_size(), 5000);
    BOOST_CHECK_EQUAL(message2.channel().created_at(), 1547113252);
    BOOST_CHECK_EQUAL(message2.channel().updated_at(), 0);
    BOOST_REQUIRE_EQUAL(message2.channel().types().size(), 2);
    BOOST_CHECK_EQUAL(message2.channel().types().Get(0), chaos::Channel::web);
    BOOST_CHECK_EQUAL(message2.channel().types().Get(1), chaos::Channel::mobile);
    BOOST_CHECK_EQUAL(message2.created_at(), 1561044501);
    BOOST_CHECK_EQUAL(message2.updated_at(), 0);

    auto& tag = disruption.tags().Get(0);
    BOOST_CHECK_EQUAL(tag.id(), "65b76758-498f-11e9-b3b3-005056a40962");
    BOOST_CHECK_EQUAL(tag.name(), "TEST");
    BOOST_CHECK_EQUAL(tag.created_at(), 1552921584);
    BOOST_CHECK_EQUAL(tag.updated_at(), 0);

    // Verify all the elements of a complete line_sections in an impact
    auto& dis = disruptions[3];
    BOOST_CHECK_EQUAL(dis.id(), "bd3ee57a-968b-11e9-951b-005056a40962");
    BOOST_REQUIRE_EQUAL(dis.impacts().size(), 1);
    auto& imp = dis.impacts().Get(0);
    BOOST_CHECK_EQUAL(imp.id(), "bd3fa10e-968b-11e9-951b-005056a40962");
    BOOST_REQUIRE_EQUAL(imp.informed_entities().size(), 2);

    auto& ls_1 = imp.informed_entities().Get(0);
    BOOST_CHECK_EQUAL(ls_1.pt_line_section().line().uri(), "line:tcl:8");
    BOOST_CHECK_EQUAL(ls_1.pt_object_type(), chaos::PtObject_Type_line_section);

    BOOST_CHECK_EQUAL(ls_1.pt_line_section().start_point().uri(), "stop_area:tcl:SA:5085");
    BOOST_CHECK_EQUAL(ls_1.pt_line_section().end_point().uri(), "stop_area:tcl:SA:5085");
    BOOST_CHECK_EQUAL(ls_1.pt_line_section().routes().size(), 2);
    BOOST_CHECK_EQUAL(ls_1.pt_line_section().routes().Get(0).uri(), "route:tcl:8-F");
    BOOST_CHECK_EQUAL(ls_1.pt_line_section().routes().Get(1).uri(), "route:tcl:8-B");

    auto& ls_2 = imp.informed_entities().Get(1);
    BOOST_CHECK_EQUAL(ls_2.pt_object_type(), chaos::PtObject_Type_line_section);
    BOOST_CHECK_EQUAL(ls_2.pt_line_section().line().uri(), "line:tcl:C20E");
    BOOST_CHECK_EQUAL(ls_2.pt_line_section().start_point().uri(), "stop_area:tcl:SA:5085");
    BOOST_CHECK_EQUAL(ls_2.pt_line_section().end_point().uri(), "stop_area:tcl:SA:5085");
    BOOST_CHECK_EQUAL(ls_2.pt_line_section().routes().size(), 2);
    BOOST_CHECK_EQUAL(ls_2.pt_line_section().routes().Get(0).uri(), "route:tcl:C20E-B");
    BOOST_CHECK_EQUAL(ls_2.pt_line_section().routes().Get(1).uri(), "route:tcl:C20E-F");
}
