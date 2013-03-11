#include "naviMake/data.h"
#include "naviMake/gtfs_parser.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>
#include <string>
#include "config.h"
#include "naviMake/build_helper.h"

const std::string gtfs_path = "/navimake/gtfs";




BOOST_AUTO_TEST_CASE(city_transformer){
    navimake::types::City city;
    navimake::types::Department* dep = new navimake::types::Department();
    dep->idx = 7;
    city.name = "city";
    city.comment = "test city";
    city.id = "1";
    city.idx = 1;
    city.uri = "city01";
    city.main_postal_code = "42000";
    city.main_city = true;
    city.use_main_stop_area_property = false;

    city.department = dep;
    city.coord.set_lon(-54.08523);
    city.coord.set_lat(5.59273);

  //  city.postal_code_list.push_back("42001");
   // city.postal_code_list.push_back("42002");

    navimake::types::City::Transformer transformer;
    navitia::type::City city_n = transformer(city);

    BOOST_CHECK_EQUAL(city_n.idx, city.idx);
    BOOST_CHECK_EQUAL(city_n.id, city.id);
    BOOST_CHECK_EQUAL(city_n.uri, city.uri);
    BOOST_CHECK_EQUAL(city_n.comment, city.comment);
    BOOST_CHECK_EQUAL(city_n.name, city.name);
    BOOST_CHECK_EQUAL(city_n.main_postal_code, city.main_postal_code);
    BOOST_CHECK_EQUAL(city_n.main_city, city.main_city);
    BOOST_CHECK_EQUAL(city_n.use_main_stop_area_property, city.use_main_stop_area_property);
    BOOST_CHECK_EQUAL(city_n.department_idx , city.department->idx);
    BOOST_CHECK_EQUAL(city_n.coord.lon() , city.coord.lon());
    BOOST_CHECK_EQUAL(city_n.coord.lat() , city.coord.lat());

    delete dep;
}

BOOST_AUTO_TEST_CASE(connection_transformer){
    navimake::types::Connection connection;
    navimake::types::StopPoint* origin = new navimake::types::StopPoint();
    navimake::types::StopPoint* destination = new navimake::types::StopPoint();

    origin->idx = 8;
    destination->idx = 523;
    
    connection.id = "12";
    connection.idx = 32;
    connection.uri = "connection_12";
    connection.departure_stop_point = origin;
    connection.destination_stop_point = destination;
    connection.duration = 10;
    connection.max_duration = 15;
    connection.connection_kind = navimake::types::Connection::LinkConnection;

    navimake::types::Connection::Transformer transformer;
    navitia::type::Connection connection_n = transformer(connection);

    BOOST_CHECK_EQUAL(connection_n.idx, connection.idx);
    BOOST_CHECK_EQUAL(connection_n.id, connection.id);
    BOOST_CHECK_EQUAL(connection_n.uri, connection.uri);
    BOOST_CHECK_EQUAL(connection_n.departure_stop_point_idx, origin->idx);
    BOOST_CHECK_EQUAL(connection_n.destination_stop_point_idx, destination->idx);
    BOOST_CHECK_EQUAL(connection_n.duration, connection.duration);
    BOOST_CHECK_EQUAL(connection_n.max_duration, connection.max_duration);

    delete destination;
    delete origin;
}


BOOST_AUTO_TEST_CASE(stop_area_transformer){
    navimake::types::StopArea stop_area;

    stop_area.id = "12";
    stop_area.idx = 32;
    stop_area.uri = "stop_area_12";
    stop_area.name = "somewhere";
    stop_area.comment = "comment";

    stop_area.coord.set_lon(-54.08523);
    stop_area.coord.set_lat(5.59273);
    stop_area.additional_data = "other data";
    stop_area.main_connection = true;
    stop_area.main_stop_area = true;

    navimake::types::StopArea::Transformer transformer;
    navitia::type::StopArea stop_area_n = transformer(stop_area);

    BOOST_CHECK_EQUAL(stop_area_n.idx, stop_area.idx);
    BOOST_CHECK_EQUAL(stop_area_n.id, stop_area.id);
    BOOST_CHECK_EQUAL(stop_area_n.uri, stop_area.uri);
    BOOST_CHECK_EQUAL(stop_area_n.name, stop_area.name);
    BOOST_CHECK_EQUAL(stop_area_n.comment, stop_area.comment);
    BOOST_CHECK_EQUAL(stop_area_n.coord.lon(), stop_area.coord.lon());
    BOOST_CHECK_EQUAL(stop_area_n.coord.lat(), stop_area.coord.lat());
    BOOST_CHECK_EQUAL(stop_area_n.additional_data, stop_area.additional_data);
}
