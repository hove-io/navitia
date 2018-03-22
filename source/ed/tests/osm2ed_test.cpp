/* Copyright © 2001-2018, Canal TP and/or its affiliates. All rights reserved.
  
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
#define BOOST_TEST_MODULE osm2ed

#include <boost/test/unit_test.hpp>
#include "ed/types.h"
#include "ed/osm2ed.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};

BOOST_GLOBAL_FIXTURE( logger_initialized );

using namespace ed::connectors;

BOOST_AUTO_TEST_CASE(osm2ed_with_no_param_should_start_without_throwing) 
{
    const char* argv [] = { "osm2ed_test" };
    BOOST_CHECK_NO_THROW(osm2ed(1, argv));
}

BOOST_AUTO_TEST_CASE(poi_uri_should_construct_based_on_id) 
{
    ed::types::Poi poi("123456");
    BOOST_CHECK_EQUAL(poi.uri, "poi:123456");
}

BOOST_AUTO_TEST_CASE(osm_poi_id_node_should_match_mimirs_one) 
{
    OsmPoi osm_poi(OsmObjectType::Node, 123456);
    BOOST_CHECK_EQUAL(osm_poi.uri, "poi:osm:node:123456");
}

BOOST_AUTO_TEST_CASE(osm_poi_id_way_should_match_mimirs_one) 
{
    OsmPoi osm_poi(OsmObjectType::Way, 123456);
    BOOST_CHECK_EQUAL( osm_poi.uri, "poi:osm:way:123456");
}

BOOST_AUTO_TEST_CASE(osm_poi_id_relation_should_match_mimirs_one) 
{
    OsmPoi osm_poi(OsmObjectType::Relation, 123456);
    BOOST_CHECK_EQUAL(osm_poi.uri, "poi:osm:relation:123456");
}
