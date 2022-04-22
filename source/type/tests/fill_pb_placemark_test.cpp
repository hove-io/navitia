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
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>
#include "type/data.h"
#include "georef/adminref.h"
#include "georef/georef.h"
#include "type/pb_converter.h"

BOOST_AUTO_TEST_CASE(get_label_test) {
    // test the sfinae functions get_label

    struct bob {
        std::string label = "sponge bob";
    };
    struct bobette {
        std::string name = "bobette";
    };
    struct bobitto {
        std::string name = "bobitto";
        std::string label = "bobitto's the best";
        // label should be prefered to name
    };
    struct bobitte {
        std::string name = "bobitte";
        std::string get_label() const { return name + "'s the best"; }
        // get_label should be prefered to name
    };

    BOOST_CHECK_EQUAL(navitia::get_label(std::make_unique<bob>().get()), "sponge bob");
    BOOST_CHECK_EQUAL(navitia::get_label(std::make_unique<bobette>().get()), "bobette");
    BOOST_CHECK_EQUAL(navitia::get_label(std::make_unique<bobitto>().get()), "bobitto's the best");
    BOOST_CHECK_EQUAL(navitia::get_label(std::make_unique<bobitte>().get()), "bobitte's the best");
}

BOOST_AUTO_TEST_CASE(name_formater_sa) {
    navitia::type::Data d;
    auto admin1 = new navitia::georef::Admin();
    admin1->name = "admin1";
    admin1->level = 1;
    d.geo_ref->admins.push_back(admin1);
    auto admin8 = new navitia::georef::Admin();
    admin8->name = "admin8";
    admin8->level = 8;
    d.geo_ref->admins.push_back(admin8);
    auto sa1 = new navitia::type::StopArea();
    sa1->name = "sa1";
    sa1->uri = "sa:sa1";
    d.pt_data->stop_areas.push_back(sa1);

    d.compute_labels();

    BOOST_CHECK_EQUAL(navitia::get_label(sa1), sa1->name);
    sa1->admin_list.push_back(admin1);
    sa1->admin_list.push_back(admin8);
    d.compute_labels();
    BOOST_CHECK_EQUAL(navitia::get_label(sa1), sa1->name + " (" + admin8->name + ")");
}

BOOST_AUTO_TEST_CASE(fill_pb_object_sa) {
    navitia::type::Data d;
    auto admin1 = new navitia::georef::Admin();
    d.geo_ref->admins.push_back(admin1);
    admin1->name = "admin1";
    admin1->level = 1;
    auto admin8 = new navitia::georef::Admin();
    admin8->name = "admin8";
    admin8->level = 8;
    d.geo_ref->admins.push_back(admin8);
    auto sa1 = new navitia::type::StopArea();
    sa1->name = "sa1";
    sa1->uri = "sa:sa1";
    sa1->admin_list.push_back(admin1);
    sa1->admin_list.push_back(admin8);
    d.pt_data->stop_areas.push_back(sa1);
    d.compute_labels();

    navitia::PbCreator pb_creator(&d, pt::not_a_date_time, null_time_period);
    auto pb = new pbnavitia::PtObject();
    pb_creator.fill(sa1, pb, 0);
    BOOST_CHECK_EQUAL(pb->name(), sa1->name + " (" + admin8->name + ")");
    BOOST_CHECK_EQUAL(pb->uri(), sa1->uri);
    BOOST_CHECK_EQUAL(pb->embedded_type(), pbnavitia::STOP_AREA);
    BOOST_CHECK(pb->has_stop_area());
    BOOST_CHECK_EQUAL(pb->stop_area().uri(), sa1->uri);
    BOOST_CHECK_EQUAL(pb->stop_area().name(), sa1->name);
    BOOST_CHECK_EQUAL(pb->stop_area().label(), sa1->name + " (" + admin8->name + ")");
}

BOOST_AUTO_TEST_CASE(name_formater_poi) {
    navitia::type::Data d;
    auto admin1 = new navitia::georef::Admin();
    admin1->name = "admin1";
    admin1->level = 1;
    d.geo_ref->admins.push_back(admin1);
    auto admin8 = new navitia::georef::Admin();
    admin8->name = "admin8";
    admin8->level = 8;
    d.geo_ref->admins.push_back(admin8);
    auto poi1 = new navitia::georef::POI();
    poi1->name = "poi1";
    poi1->uri = "poi:poi1";
    d.geo_ref->pois.push_back(poi1);
    d.compute_labels();

    BOOST_CHECK_EQUAL(navitia::get_label(poi1), poi1->name);
    poi1->admin_list.push_back(admin1);
    poi1->admin_list.push_back(admin8);
    d.compute_labels();
    BOOST_CHECK_EQUAL(navitia::get_label(poi1), poi1->name + " (" + admin8->name + ")");
}

BOOST_AUTO_TEST_CASE(fill_pb_object_poi) {
    navitia::type::Data d;
    auto admin1 = new navitia::georef::Admin();
    admin1->name = "admin1";
    admin1->level = 1;
    d.geo_ref->admins.push_back(admin1);
    auto admin8 = new navitia::georef::Admin();
    admin8->name = "admin8";
    admin8->level = 8;
    d.geo_ref->admins.push_back(admin8);
    auto poi1 = new navitia::georef::POI();
    poi1->name = "poi1";
    poi1->uri = "poi:poi1";
    d.geo_ref->pois.push_back(poi1);
    poi1->admin_list.push_back(admin1);
    poi1->admin_list.push_back(admin8);
    d.compute_labels();

    auto pb = new pbnavitia::PtObject();
    navitia::PbCreator pb_creator(&d, pt::not_a_date_time, null_time_period);
    pb_creator.fill(poi1, pb, 0);
    BOOST_CHECK_EQUAL(pb->name(), poi1->name + " (" + admin8->name + ")");
    BOOST_CHECK_EQUAL(pb->uri(), poi1->uri);
    BOOST_CHECK_EQUAL(pb->embedded_type(), pbnavitia::POI);
    BOOST_CHECK(pb->has_poi());
    BOOST_CHECK_EQUAL(pb->poi().uri(), poi1->uri);
    BOOST_CHECK_EQUAL(pb->poi().name(), poi1->name);
    BOOST_CHECK_EQUAL(pb->poi().label(), poi1->name + " (" + admin8->name + ")");
}

BOOST_AUTO_TEST_CASE(fill_pb_object_poi_address) {
    auto poi = std::make_unique<navitia::georef::POI>();
    poi->name = "poi";
    poi->label = "poi label";
    poi->address_number = 5;
    poi->address_name = "poi address";
    poi->poitype_idx = 0;

    auto poi_type = std::make_unique<navitia::georef::POIType>();
    poi_type->name = "VLS";

    navitia::type::Data d;
    d.geo_ref->poitypes.push_back(poi_type.release());

    auto pb = std::make_unique<pbnavitia::PtObject>();
    navitia::PbCreator pb_creator(&d, pt::not_a_date_time, null_time_period);
    pb_creator.fill(poi.get(), pb.get(), 2);

    BOOST_CHECK_EQUAL(pb->poi().address().label(), std::to_string(poi->address_number) + " " + poi->address_name);
}

BOOST_AUTO_TEST_CASE(name_formater_stop_point) {
    navitia::type::Data d;
    auto admin1 = new navitia::georef::Admin();
    admin1->name = "admin1";
    admin1->level = 1;
    d.geo_ref->admins.push_back(admin1);
    auto admin8 = new navitia::georef::Admin();
    admin8->name = "admin8";
    admin8->level = 8;
    d.geo_ref->admins.push_back(admin8);
    auto stop_point1 = new navitia::type::StopPoint();
    stop_point1->name = "stop_point1";
    stop_point1->uri = "stop_point:stop_point1";
    d.pt_data->stop_points.push_back(stop_point1);
    d.compute_labels();

    BOOST_CHECK_EQUAL(navitia::get_label(stop_point1), stop_point1->name);
    stop_point1->admin_list.push_back(admin1);
    stop_point1->admin_list.push_back(admin8);
    BOOST_CHECK_EQUAL(navitia::get_label(stop_point1), stop_point1->name);
}

BOOST_AUTO_TEST_CASE(fill_pb_object_stop_point) {
    navitia::type::Data d;
    auto admin1 = new navitia::georef::Admin();
    admin1->name = "admin1";
    admin1->level = 1;
    d.geo_ref->admins.push_back(admin1);
    auto admin8 = new navitia::georef::Admin();
    admin8->name = "admin8";
    admin8->level = 8;
    d.geo_ref->admins.push_back(admin8);
    auto stop_point1 = new navitia::type::StopPoint();
    stop_point1->name = "stop_point1";
    stop_point1->uri = "stop_point:stop_point1";
    stop_point1->admin_list.push_back(admin1);
    stop_point1->admin_list.push_back(admin8);
    d.pt_data->stop_points.push_back(stop_point1);
    d.compute_labels();

    auto pb = new pbnavitia::PtObject();
    navitia::PbCreator pb_creator(&d, pt::not_a_date_time, null_time_period);
    pb_creator.fill(stop_point1, pb, 0);
    BOOST_CHECK_EQUAL(pb->name(), stop_point1->name + " (" + admin8->name + ")");
    BOOST_CHECK_EQUAL(pb->uri(), stop_point1->uri);
    BOOST_CHECK_EQUAL(pb->embedded_type(), pbnavitia::STOP_POINT);
    BOOST_CHECK(pb->has_stop_point());
    BOOST_CHECK_EQUAL(pb->stop_point().uri(), stop_point1->uri);
    BOOST_CHECK_EQUAL(pb->stop_point().name(), stop_point1->name);
    BOOST_CHECK_EQUAL(pb->stop_point().label(), stop_point1->name + " (" + admin8->name + ")");
}
