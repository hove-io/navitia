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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>
#include "proximity_list/proximity_list.h"
#include "proximity_list/proximitylist_api.h"
#include "type/data.h"
#include "georef/georef.h"
#include "type/pt_data.h"
#include "type/pb_converter.h"

using namespace navitia::type;
using namespace navitia::proximitylist;

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );

BOOST_AUTO_TEST_CASE(distances_grand_cercle)
{
    GeographicalCoord a(0,0);
    GeographicalCoord b(0,0);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
    BOOST_CHECK_CLOSE(a.distance_to(b), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(a), 0, 1e-6);

    GeographicalCoord nantes(-1.57, 47.22);
    GeographicalCoord paris(2.36, 48.85);
    BOOST_CHECK_CLOSE(nantes.distance_to(paris), 340000, 10);
    BOOST_CHECK_CLOSE(paris.distance_to(nantes), 340000, 10);

    a.set_lon(180);
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);

    a.set_lon(84); a.set_lat(89.999);
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);


    a.set_lon(34); a.set_lat(-89.999);
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);


    a.set_lon(0); a.set_lat(45);
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/4,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/4,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
}

BOOST_AUTO_TEST_CASE(approx_distance){
    GeographicalCoord canaltp(2.3921, 48.8296);
    GeographicalCoord tour_eiffel(2.29447,48.85834);
    double coslat = ::cos(canaltp.lat() * 0.0174532925199432958);
    BOOST_CHECK_CLOSE(canaltp.distance_to(tour_eiffel), ::sqrt(canaltp.approx_sqr_distance(tour_eiffel, coslat)), 1);
    BOOST_CHECK_CLOSE(tour_eiffel.distance_to(canaltp), ::sqrt(tour_eiffel.approx_sqr_distance(canaltp, coslat)), 1);
    BOOST_CHECK_CLOSE(canaltp.distance_to(tour_eiffel), ::sqrt(tour_eiffel.approx_sqr_distance(canaltp, coslat)), 1);
    BOOST_CHECK_CLOSE(tour_eiffel.distance_to(canaltp), ::sqrt(canaltp.approx_sqr_distance(tour_eiffel, coslat)), 1);
}

BOOST_AUTO_TEST_CASE(find_nearest){
    constexpr double M_TO_DEG = 1.0/111320.0;
    ProximityList<unsigned int> pl;

    GeographicalCoord c;
    std::vector<GeographicalCoord> coords;

    // Exemple d'illustration issu de wikipedia
    c.set_lon(M_TO_DEG *2); c.set_lat(M_TO_DEG *3); pl.add(c, 1); coords.push_back(c);
    c.set_lon(M_TO_DEG *5); c.set_lat(M_TO_DEG *4); pl.add(c, 2); coords.push_back(c);
    c.set_lon(M_TO_DEG *9); c.set_lat(M_TO_DEG *6); pl.add(c, 3); coords.push_back(c);
    c.set_lon(M_TO_DEG *4); c.set_lat(M_TO_DEG *7); pl.add(c, 4); coords.push_back(c);
    c.set_lon(M_TO_DEG *8); c.set_lat(M_TO_DEG *1); pl.add(c, 5); coords.push_back(c);
    c.set_lon(M_TO_DEG *7); c.set_lat(M_TO_DEG *2); pl.add(c, 6); coords.push_back(c);

    pl.build();

    std::vector<unsigned int> expected {1,4,2,6,5,3};
    for(size_t i=0; i < expected.size(); ++i)
        BOOST_CHECK_EQUAL(pl.items[i].element, expected[i]);


    c.set_lon(M_TO_DEG *2); c.set_lat(M_TO_DEG *3);
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 1);

    c.set_lon(M_TO_DEG *7.1); c.set_lat(M_TO_DEG *1.9);
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 6);

    c.set_lon(M_TO_DEG *100); c.set_lat(M_TO_DEG *6);
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 3);

    c.set_lon(M_TO_DEG *2); c.set_lat(M_TO_DEG *4);

    expected = {1};
    auto tmp1 = pl.find_within(c, 1.1);
    std::vector<unsigned int> tmp;
    for(auto p : tmp1) tmp.push_back(p.first);
    BOOST_CHECK_EQUAL(tmp1[0].second, coords[0]);
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2};
    tmp1 = pl.find_within(c, 3.1);
    tmp.clear();
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4};
    tmp.clear();
    tmp1 = pl.find_within(c, 3.7);
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 5.4);
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4,5,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 6.8);
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,3,4,5,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 7.3);
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(test_api) {
    navitia::type::Data data;
    //Everything in the range
    auto sa = new navitia::type::StopArea();
    sa->coord.set_lon(-1.554514);
    sa->coord.set_lat(47.218515);
    sa->idx = 0;
    data.pt_data->stop_areas.push_back(sa);
    sa = new navitia::type::StopArea();
    sa->coord.set_lon(-1.554384);
    sa->coord.set_lat(47.217804);
    sa->idx = 1;
    data.pt_data->stop_areas.push_back(sa);
    data.geo_ref->init();
    data.build_proximity_list();
    navitia::type::GeographicalCoord c;
    c.set_lon(-1.554514);
    c.set_lat(47.218515);
    navitia::PbCreator pb_creator(&data, boost::gregorian::not_a_date_time, null_time_period);
    auto result = find(pb_creator, c, 200,
                    {navitia::type::Type_e::StopArea, navitia::type::Type_e::POI},
                    "", 1, 10, 0, data);
    BOOST_CHECK_EQUAL(result.places_nearby().size(), 2);
    //One object out of the range
    sa = new navitia::type::StopArea();
    sa->coord.set_lon(-1.556949);
    sa->coord.set_lat(47.217231);
    sa->idx = 2;
    data.pt_data->stop_areas.push_back(sa);
    data.build_proximity_list();
    result.Clear();
    pb_creator.init(&data, boost::gregorian::not_a_date_time, null_time_period);
    result = find(pb_creator, c, 200,
                    {navitia::type::Type_e::StopArea, navitia::type::Type_e::POI},
                    "", 1, 10, 0, data);
    BOOST_CHECK_EQUAL(result.places_nearby().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_api_type) {
    navitia::type::Data data;
    //One object not in asked type and everything in the range
    auto sa = new navitia::type::StopArea();
    sa->coord.set_lon(-1.554514);
    sa->coord.set_lat(47.218515);
    sa->idx = 0;
    data.pt_data->stop_areas.push_back(sa);

    auto poi  = new navitia::georef::POI();
    poi->coord.set_lon(-1.554514);
    poi->coord.set_lat(47.218515);
    data.geo_ref->pois.push_back(poi);
    data.geo_ref->init();
    data.build_proximity_list();
    navitia::type::GeographicalCoord c;
    c.set_lon(-1.554514);
    c.set_lat(47.218515);
    navitia::PbCreator pb_creator(&data, boost::gregorian::not_a_date_time, null_time_period);
    auto result = find(pb_creator, c, 200,
                    {navitia::type::Type_e::StopArea},
                    "", 1, 10, 0, data);
    BOOST_CHECK_EQUAL(result.places_nearby().size(), 1);
}

BOOST_AUTO_TEST_CASE(test_filter) {
    navitia::type::Data data;
    //One object not in asked type and everything in the range
    auto sa = new navitia::type::StopArea();
    sa->coord.set_lon(-1.554514);
    sa->coord.set_lat(47.218515);
    sa->idx = 0;
    sa->name = "pouet";
    data.pt_data->stop_areas.push_back(sa);
    data.build_proximity_list();
    navitia::type::GeographicalCoord c;
    c.set_lon(-1.554514);
    c.set_lat(47.218515);
    navitia::PbCreator pb_creator(&data, boost::gregorian::not_a_date_time, null_time_period);
    auto result = find(pb_creator, c, 200,
                    {navitia::type::Type_e::StopArea},
                    "stop_area.name=pouet", 1, 10, 0, data);
    BOOST_CHECK_EQUAL(result.places_nearby().size(), 1);
    result.Clear();
    pb_creator.init(&data, boost::gregorian::not_a_date_time, null_time_period);
    result = find(pb_creator, c, 200,
                    {navitia::type::Type_e::StopArea},
                    "stop_area.name=paspouet", 1, 10, 0, data);
    BOOST_CHECK_EQUAL(result.places_nearby().size(), 0);

    //Bad request
    pb_creator.init(&data, boost::gregorian::not_a_date_time, null_time_period);
    result = find(pb_creator, c, 200,
                    {navitia::type::Type_e::StopArea},
                    "stop_area.name=paspouet bachibouzouk", 1, 10, 0, data);
    BOOST_CHECK_EQUAL(result.error().id(), pbnavitia::Error::unable_to_parse);
}

BOOST_AUTO_TEST_CASE(test_poi_filter) {
    navitia::type::Data data;
    //One object not in asked type and everything in the range
    auto poitype_1 = new navitia::georef::POIType();
    poitype_1->uri = "poi_type0";
    poitype_1->idx = 0;
    data.geo_ref->poitypes.push_back(poitype_1);
    auto poitype_2 = new navitia::georef::POIType();
    poitype_2->uri = "poi_type1";
    poitype_2->idx = 1;
    data.geo_ref->poitypes.push_back(poitype_2);

    {
        auto sa = new navitia::type::StopArea();
        sa->coord.set_lon(-1.554514);
        sa->coord.set_lat(47.218515);
        sa->idx = 0;
        data.pt_data->stop_areas.push_back(sa);
    }
    size_t poi_idx(0);
    {//a first poi with type 1
        auto poi  = new navitia::georef::POI();
        poi->poitype_idx = 0;
        poi->idx = poi_idx++;
        poi->coord.set_lon(-1.554514);
        poi->coord.set_lat(47.218515);
        data.geo_ref->pois.push_back(poi);
    }
    {//a second poi not far with the second poi type
        auto poi  = new navitia::georef::POI();
        poi->uri = "bob";
        poi->poitype_idx = 1;
        poi->idx = poi_idx++;
        poi->coord.set_lon(-1.554513);
        poi->coord.set_lat(47.218516);
        data.geo_ref->pois.push_back(poi);
    }
    {//a third one far far away
        auto poi  = new navitia::georef::POI();
        poi->coord.set_lon(-1.554514);
        poi->idx = poi_idx++;
        poi->coord.set_lat(50.218515);
        data.geo_ref->pois.push_back(poi);
    }
    {//a fourth one near and with again the second poi type
        auto poi  = new navitia::georef::POI();
        poi->uri = "bobette";
        poi->poitype_idx = 1;
        poi->coord.set_lon(-1.554514);
        poi->idx = poi_idx++;
        poi->coord.set_lat(47.218513);
        data.geo_ref->pois.push_back(poi);
    }
    {//a stop area in the same position as the second poi to be sure that it is not taken
        auto sa = new navitia::type::StopArea();
        sa->coord.set_lon(-1.554513);
        sa->coord.set_lat(47.218516);
        sa->idx = 0;
        data.pt_data->stop_areas.push_back(sa);
    }
    data.geo_ref->init();
    data.build_proximity_list();
    data.build_uri();
    navitia::type::GeographicalCoord c;
    c.set_lon(-1.554514);
    c.set_lat(47.218515);
    navitia::PbCreator pb_creator(&data, boost::gregorian::not_a_date_time, null_time_period);
    auto result = find(pb_creator, c, 200,
                    {navitia::type::Type_e::POI},
                    "poi_type.uri=poi_type1", 1, 10, 0, data);

    //we want the bob and bobette pois
    BOOST_CHECK_EQUAL(result.places_nearby().size(), 2);
    std::set<std::string> poi_names;
    for (int i = 0; i < result.places_nearby().size(); ++i) {
        poi_names.insert(result.places_nearby(i).uri());
    }
    BOOST_CHECK(poi_names.find("bob") != poi_names.end());
    BOOST_CHECK(poi_names.find("bobette") != poi_names.end());
}
