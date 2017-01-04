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

#include "utils/init.h"
#include "mock_kraken.h"
#include "type/type.h"
#include "type/pt_data.h"

using namespace navitia::georef;
/*
 * Here we test prepare a data for autocomplete test
 * The data contains One Admin, 6 stop_areas and three addresses
 */
int main(int argc, const char* const argv[]) {
    navitia::init_app();

    ed::builder b = {"20140614"};

    b.sa("IUT", 0, 0);
    b.sa("Gare", 0, 0);
    b.sa("Becharles", 0, 0);
    b.sa("Luther King", 0, 0);
    b.sa("Napoleon III", 0, 0);
    b.sa("MPT kerfeunteun", 0, 0);
    b.data->pt_data->index();

    boost::add_vertex(navitia::georef::Vertex(100, 80, false), b.data->geo_ref->graph);
    boost::add_vertex(navitia::georef::Vertex(110, 80, false), b.data->geo_ref->graph);
    boost::add_vertex(navitia::georef::Vertex(120, 80, false), b.data->geo_ref->graph);
    boost::add_vertex(navitia::georef::Vertex(130, 80, false), b.data->geo_ref->graph);

    size_t e_idx = 0;
    boost::add_edge(0, 1, navitia::georef::Edge(e_idx++, 1_s), b.data->geo_ref->graph);
    boost::add_edge(1, 2, navitia::georef::Edge(e_idx++, 1_s), b.data->geo_ref->graph);
    boost::add_edge(2, 3, navitia::georef::Edge(e_idx++, 1_s), b.data->geo_ref->graph);
    boost::add_edge(3, 4, navitia::georef::Edge(e_idx++, 1_s), b.data->geo_ref->graph);

    auto* w = new Way;
    w->idx = 0;
    w->name = "rue DU TREGOR";
    w->uri = w->name;
    w->edges.push_back(std::make_pair(0, 1));
    b.data->geo_ref->ways.push_back(w);
    w = new Way;
    w->name = "rue VIS";
    w->uri = w->name;
    w->idx = 1;
    w->edges.push_back(std::make_pair(1, 2));
    b.data->geo_ref->ways.push_back(w);
    w = new Way;
    w->idx = 2;
    w->name = "quai NEUF";
    w->uri = w->name;
    w->edges.push_back(std::make_pair(2, 3));
    b.data->geo_ref->ways.push_back(w);

    Admin* ad = new Admin;
    ad->name = "Quimper";
    ad->uri = "Quimper";
    ad->level = 8;
    ad->postal_codes.push_back("29000");
    ad->idx = 0;
    ad->insee = "92232";
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();


    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();

    mock_kraken kraken(b, "main_autocomplete_test", argc, argv);
    return 0;
}

