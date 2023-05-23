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

#include <boost/geometry.hpp>
#include "utils/init.h"
#include "routing/tests/routing_api_test_data.h"
#include "mock_kraken.h"

/*
 *               2 minutes connection
 *    A       B----------------------->C        D
 *    8h     8h05                     14h07     15h
 *    15    15h05                     15h10     16h
 *    15h                                       15h10   (only day 2)
 *
 * Two long waiting time
 *            2 minutes connection
 *    E      F----------------------->G         H
 *    8h    9h                        17h       18h
 *
 *  Isochrone equal duration test
 *
 *   I1     I2        I3
 *   8h     9h
 *   8h               9h
 */

/* Data for tad_zonal
 * Geographical Zone of odt:
 * area_1 -> {(10.0, 10.0), (10.0, 15.0),(15.0, 15.0),(15.0, 10.0),(10.0, 10.0)}
 * area_1 -> {(20.0, 20.0), (20.0, 25.0),(25.0, 25.0),(25.0, 20.0),(20.0, 20.0)}
 * StopPoint P(12, 12), Q (22, 22): is_zonal=true
 * admin:93700 coord : {12, 12} in zone area_1
 * admin:75000 coord : {22, 22} in zone area_2
 *
 *    admin:93700   P       Q   admin:75000
 *                  8h    8h05
 *                  15    15h05
 */

static boost::gregorian::date_period period(std::string beg, std::string end) {
    boost::gregorian::date start_date = boost::gregorian::from_undelimited_string(beg);
    boost::gregorian::date end_date = boost::gregorian::from_undelimited_string(end);  // end is not in the period
    return {start_date, end_date};
}

namespace bg = boost::geometry;
using point_t = bg::model::point<double, 2, bg::cs::cartesian>;

int main(int argc, const char* const argv[]) {
    navitia::init_app();
    ed::builder b = {"20120614"};
    b.generate_dummy_basis();
    b.sa("A", 1., 1.)("stop_point:uselessA", 1., 1.);
    b.sa("B", 2., 2.);
    b.sa("E", 3., 3.);
    b.sa("H", 4., 4.);
    b.sa("G", 5., 5.);
    b.vj("l1")("A", 8 * 3600)("B", 8 * 3600 + 5 * 60);
    b.vj("l2")("A", 15 * 3600)("B", 15 * 3600 + 5 * 60);
    b.vj("l3")("C", 14 * 3600 + 7 * 60)("D", 15 * 3600);
    auto* vj = b.vj("l4")("C", 15 * 3600 + 10 * 60)("D", 16 * 3600).make();
    b.vj("l5", "10", "", true)("A", 15 * 3600)("D", 15 * 3600 + 10 * 60);
    b.vj("l6")("E", 8 * 3600)("F", 9 * 3600);
    b.vj("l7", "111111")("G", 17 * 3600)("H", 18 * 3600);
    b.vj("l8")("I1", 8 * 3600)("I2", 9 * 3600);
    b.vj("l9")("I1", 8 * 3600)("I3", 9 * 3600);
    b.connection("B", "C", 2 * 60);
    b.connection("F", "G", 2 * 60);
    // Empty license for global feed publisher
    b.data->meta->license = "";
    // and we set the streetnetwork provider as osm to test the osm licence
    b.data->meta->street_network_source = "osm";
    b.data->pt_data->codes.add(b.sps.at("A"), "external_code", "stop_point:A");
    b.data->pt_data->codes.add(b.sps.at("A"), "source", "Ain");
    b.data->pt_data->codes.add(b.sps.at("A"), "source", "Aisne");

    auto* ds = new navitia::type::Dataset();
    ds->idx = b.data->pt_data->datasets.size();
    ds->uri = "base_dataset";
    ds->name = "base dataset";
    ds->validation_period = period("20160101", "20161230");
    ds->vehiclejourney_list.insert(vj);
    vj->dataset = ds;

    auto* cr = new navitia::type::Contributor();
    cr->idx = b.data->pt_data->contributors.size();
    cr->uri = "base_contributor";
    cr->name = "base contributor";
    cr->license = "L-contributor";
    cr->website = "www.hove.com";
    cr->dataset_list.insert(ds);
    ds->contributor = cr;

    b.data->pt_data->datasets.push_back(ds);
    b.data->pt_data->contributors.push_back(cr);

    // data for tad zonal
    navitia::type::MultiPolygon area_1, area_2;
    area_1.resize(1);
    bg::append(area_1[0].outer(), point_t(10.0, 10.0));
    bg::append(area_1[0].outer(), point_t(10.0, 15.0));
    bg::append(area_1[0].outer(), point_t(15.0, 15.0));
    bg::append(area_1[0].outer(), point_t(15.0, 10.0));
    bg::append(area_1[0].outer(), point_t(10.0, 10.0));
    area_2.resize(1);
    bg::append(area_2[0].outer(), point_t(20.0, 20.0));
    bg::append(area_2[0].outer(), point_t(20.0, 25.0));
    bg::append(area_2[0].outer(), point_t(25.0, 25.0));
    bg::append(area_2[0].outer(), point_t(25.0, 20.0));
    bg::append(area_2[0].outer(), point_t(20.0, 20.0));

    b.sa("P", 12., 12.)("stop_point:uselessP", 12., 12.);
    b.sa("Q", 22., 22.);
    b.vj("lpq1")("P", 8 * 3600)("Q", 8 * 3600 + 5 * 60);
    b.vj("lpq2")("P", 15 * 3600)("Q", 15 * 3600 + 5 * 60);
    b.data->pt_data->codes.add(b.sps.at("P"), "external_code", "stop_point:P");
    b.data->pt_data->codes.add(b.sps.at("P"), "source", "Pin");

    auto* sp = b.sps.at("P");
    sp->is_zonal = true;
    b.data->pt_data->add_stop_point_area(area_1, sp);
    sp = b.sps.at("Q");
    sp->is_zonal = true;
    b.data->pt_data->add_stop_point_area(area_2, sp);

    auto drancy = new navitia::georef::Admin(0, "admin:93700", "Drancy", 8, "93700", "Drancy", {12, 12}, {"93700"});
    auto paris = new navitia::georef::Admin(1, "admin:75000", "Paris", 8, "75000", "Paris", {22, 22}, {"75000"});
    auto paris12 =
        new navitia::georef::Admin(2, "admin:75012", "Paris 12em", 9, "75012", "Paris 12", {23, 23}, {"75012"});
    auto aligre =
        new navitia::georef::Admin(3, "admin:75012", "Aligre market", 10, "75012", "Aligre", {23, 23}, {"75012"});

    b.data->geo_ref->admins.assign({drancy, paris, paris12, aligre});
    b.data->complete();
    b.manage_admin();
    b.make();

    auto stop_area_A = b.data->pt_data->stop_areas[0];
    // we willingly add cities in the wrong order of level to make sure they'll get sorted
    stop_area_A->admin_list.assign({aligre, paris, paris12});

    mock_kraken kraken(b, argc, argv);
    return 0;
}
