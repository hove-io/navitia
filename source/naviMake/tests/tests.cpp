#include "naviMake/data.h"
#include "naviMake/gtfs_parser.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_indexes
#include <boost/test/unit_test.hpp>
#include <string>

const std::string gtfs_path = "../../source/naviMake/tests/fixture/gtfs";


BOOST_AUTO_TEST_CASE(parse_gtfs){
    navimake::Data data;
    navimake::connectors::GtfsParser parser(gtfs_path, "20110101");
    parser.fill(data);

    BOOST_CHECK_EQUAL(data.lines.size(), 2);
    BOOST_CHECK_EQUAL(data.routes.size(), 8);
    BOOST_CHECK_EQUAL(data.stop_areas.size(), 6);
    BOOST_CHECK_EQUAL(data.stop_points.size(), 11);
    BOOST_CHECK_EQUAL(data.vehicle_journeys.size(), 8);
    BOOST_CHECK_EQUAL(data.stops.size(), 32);
    BOOST_CHECK_EQUAL(data.connections.size(), 0);
    BOOST_CHECK_EQUAL(data.route_points.size(), 8);


    navimake::types::Line* line = data.lines[0];
    BOOST_CHECK_EQUAL(line->code, "4");
    BOOST_CHECK_EQUAL(line->name, "Metro 4 – Porte de clignancourt – Porte d'Orléans");
    BOOST_CHECK_EQUAL(line->color, "");


    navimake::types::Route* route = data.routes[0];
    BOOST_CHECK_EQUAL(route->line->code, "4");

    navimake::types::StopArea* stop_area = data.stop_areas[0];
    BOOST_CHECK_EQUAL(stop_area->name, "Gare du Nord");
    BOOST_CHECK_EQUAL(stop_area->external_code, "frpno");
    BOOST_CHECK_CLOSE(stop_area->coord.x, 2.355022, 0.0000001);
    BOOST_CHECK_CLOSE(stop_area->coord.y, 48.880342, 0.0000001);
    
    navimake::types::StopPoint* stop_point = data.stop_points[0];
    BOOST_CHECK_EQUAL(stop_point->name, "Gare du Nord Surface");
    BOOST_CHECK_EQUAL(stop_point->external_code, "frpno:surface");
    BOOST_CHECK_CLOSE(stop_point->coord.x, 2.355381, 0.0000001);
    BOOST_CHECK_CLOSE(stop_point->coord.y, 48.880531, 0.0000001);
    BOOST_CHECK_EQUAL(stop_point->stop_area, stop_area);

    navimake::types::VehicleJourney* vj = data.vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->name, "41");
    BOOST_CHECK_EQUAL(vj->name, vj->external_code);
    BOOST_CHECK_EQUAL(vj->route, route);

    navimake::types::StopTime* stop = data.stops[0];
    BOOST_CHECK_EQUAL(stop->ODT, false);
    BOOST_CHECK_EQUAL(stop->arrival_time, 21600);
    BOOST_CHECK_EQUAL(stop->departure_time, 21601);
    BOOST_CHECK_EQUAL(stop->route_point->stop_point->external_code, "frpno:metro4");
    BOOST_CHECK_EQUAL(stop->route_point->stop_point->name, "Gare du Nord metro ligne 4");
    BOOST_CHECK_EQUAL(stop->route_point->stop_point, data.stop_points[2]);

}


BOOST_AUTO_TEST_CASE(city_transformer){
    navimake::types::City city;
    navimake::types::Department* dep = new navimake::types::Department();
    dep->idx = 7;
    city.name = "city";
    city.comment = "test city";
    city.id = "1";
    city.idx = 1;
    city.external_code = "city01";
    city.main_postal_code = "42000";
    city.main_city = true;
    city.use_main_stop_area_property = false;

    city.department = dep;
    city.coord.x = -54.08523;
    city.coord.x = 5.59273;

  //  city.postal_code_list.push_back("42001");
   // city.postal_code_list.push_back("42002");

    navimake::types::City::Transformer transformer;
    navitia::type::City city_n = transformer(city);

    BOOST_CHECK_EQUAL(city_n.idx, city.idx);
    BOOST_CHECK_EQUAL(city_n.id, city.id);
    BOOST_CHECK_EQUAL(city_n.external_code, city.external_code);
    BOOST_CHECK_EQUAL(city_n.comment, city.comment);
    BOOST_CHECK_EQUAL(city_n.name, city.name);
    BOOST_CHECK_EQUAL(city_n.main_postal_code, city.main_postal_code);
    BOOST_CHECK_EQUAL(city_n.main_city, city.main_city);
    BOOST_CHECK_EQUAL(city_n.use_main_stop_area_property, city.use_main_stop_area_property);
    BOOST_CHECK_EQUAL(city_n.department_idx , city.department->idx);
    BOOST_CHECK_EQUAL(city_n.coord.x , city.coord.x);
    BOOST_CHECK_EQUAL(city_n.coord.y , city.coord.y);

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
    connection.external_code = "connection_12";
    connection.departure_stop_point = origin;
    connection.destination_stop_point = destination;
    connection.duration = 10;
    connection.max_duration = 15;
    connection.connection_kind = navimake::types::Connection::LinkConnection;

    navimake::types::Connection::Transformer transformer;
    navitia::type::Connection connection_n = transformer(connection);

    BOOST_CHECK_EQUAL(connection_n.idx, connection.idx);
    BOOST_CHECK_EQUAL(connection_n.id, connection.id);
    BOOST_CHECK_EQUAL(connection_n.external_code, connection.external_code);
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
    stop_area.external_code = "stop_area_12";
    stop_area.name = "somewhere";
    stop_area.comment = "comment";

    stop_area.coord.x = -54.08523;
    stop_area.coord.x = 5.59273;
    stop_area.additional_data = "other data";
    stop_area.main_connection = true;
    stop_area.main_stop_area = true;
    stop_area.properties = 18;

    navimake::types::StopArea::Transformer transformer;
    navitia::type::StopArea stop_area_n = transformer(stop_area);

    BOOST_CHECK_EQUAL(stop_area_n.idx, stop_area.idx);
    BOOST_CHECK_EQUAL(stop_area_n.id, stop_area.id);
    BOOST_CHECK_EQUAL(stop_area_n.external_code, stop_area.external_code);
    BOOST_CHECK_EQUAL(stop_area_n.name, stop_area.name);
    BOOST_CHECK_EQUAL(stop_area_n.comment, stop_area.comment);
    BOOST_CHECK_EQUAL(stop_area_n.coord.x, stop_area.coord.x);
    BOOST_CHECK_EQUAL(stop_area_n.coord.y, stop_area.coord.y);
    BOOST_CHECK_EQUAL(stop_area_n.additional_data, stop_area.additional_data);
}
