#include "naviMake/data.h"
#include "naviMake/gtfs_parser.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>
#include <string>
#include "config.h"
#include "naviMake/build_helper.h"
#include "utils/csv.h"

const std::string gtfs_path = "/navimake/gtfs";

BOOST_AUTO_TEST_CASE(required_files) {
    std::vector<std::string> files = {"agency", "calendar", "calendar",
        "routes", "stop_times", "trips"};
    for(auto file : files)
    {
        navimake::Data data;
        navimake::connectors::GtfsParser parser(std::string(FIXTURES_DIR) +  gtfs_path + "_sans_"+file);
        BOOST_REQUIRE_THROW(parser.fill(data, "20130305"), navimake::connectors::FileNotFoundException);
    }
}

BOOST_AUTO_TEST_CASE(parse_agencies) {
    std::vector<std::string> fields={"agency_id", "agency_name", "agency_url", 
        "agency_timezone", "agency_lang", "agency_phone", "agency_fare_url"},
    required_fields = {"agency_name", "agency_url", "agency_timezone"};

    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        navimake::Data data;
        navimake::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_THROW(parser.parse_agency(data, csv), navimake::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        navimake::Data data;
        navimake::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_agency(data, csv));
    }

    //Check que les networks sont bien remplis
    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",") << "\n";
        sstream << "ratp, RATP,,,,,\n";
        sstream << ", ACME,,,,,";
        navimake::Data data;
        navimake::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_agency(data, csv));
        BOOST_REQUIRE_EQUAL(data.networks.size(), 2);
        BOOST_CHECK_EQUAL(data.networks[0]->name, "RATP");
        BOOST_CHECK_EQUAL(data.networks[0]->uri, "ratp");
        BOOST_CHECK_EQUAL(data.networks[1]->name, "ACME");
        BOOST_CHECK_EQUAL(data.networks[1]->uri, "2");
    }
}

BOOST_AUTO_TEST_CASE(parse_stops) {
    std::vector<std::string> fields={"stop_id","stop_code", "stop_name", "stop_desc", "stop_lat",
                                     "stop_lon", "zone_id", "stop_url", "location_type", "parent_station",
                                     "stop_timezone", "wheelchair_boarding"},
            required_fields = {"stop_id", "stop_name", "stop_lat", "stop_lon"};

    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        navimake::Data data;
        navimake::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_THROW(parser.parse_stops(data, csv), navimake::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        navimake::Data data;
        navimake::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_stops(data, csv));
    }
}



BOOST_AUTO_TEST_CASE(parse_gtfs){
    navimake::Data data;
    navimake::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
    parser.fill(data);


    /* BOOST_CHECK_EQUAL(data.lines.size(), 2);
    BOOST_CHECK_EQUAL(data.journey_patterns.size(), 3);
    BOOST_CHECK_EQUAL(data.stop_areas.size(), 6);
    BOOST_CHECK_EQUAL(data.stop_points.size(), 11);
    BOOST_CHECK_EQUAL(data.vehicle_journeys.size(), 8);
    BOOST_CHECK_EQUAL(data.stops.size(), 32);
    BOOST_CHECK_EQUAL(data.connections.size(), 0);
    BOOST_CHECK_EQUAL(data.journey_pattern_points.size(), 12);*/

    //Agency
    BOOST_REQUIRE_EQUAL(data.networks.size(), 2);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "RATP");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "ratp");
    BOOST_CHECK_EQUAL(data.networks[1]->name, "ACME");
    BOOST_CHECK_EQUAL(data.networks[1]->uri, "2");

    //Stop areas
    BOOST_REQUIRE_EQUAL(data.stop_areas.size(), 6);

    //Stop points


    navimake::types::Line* line = data.lines[0];
    BOOST_CHECK_EQUAL(line->code, "4");
    BOOST_CHECK_EQUAL(line->name, "Metro 4 – Porte de clignancourt – Porte d'Orléans");
    BOOST_CHECK_EQUAL(line->color, "");


    navimake::types::JourneyPattern* journey_pattern = data.journey_patterns[0];
    BOOST_CHECK_EQUAL(journey_pattern->route->line->code, "4");

    navimake::types::StopArea* stop_area = data.stop_areas[0];
    BOOST_CHECK_EQUAL(stop_area->name, "Gare du Nord");
    BOOST_CHECK_EQUAL(stop_area->uri, "frpno");
    BOOST_CHECK_CLOSE(stop_area->coord.lon(), 2.355022, 0.0000001);
    BOOST_CHECK_CLOSE(stop_area->coord.lat(), 48.880342, 0.0000001);
    
    navimake::types::StopPoint* stop_point = data.stop_points[0];
    BOOST_CHECK_EQUAL(stop_point->name, "Gare du Nord Surface");
    BOOST_CHECK_EQUAL(stop_point->uri, "frpno:surface");
    BOOST_CHECK_CLOSE(stop_point->coord.lon(), 2.355381, 0.0000001);
    BOOST_CHECK_CLOSE(stop_point->coord.lat(), 48.880531, 0.0000001);
    BOOST_CHECK_EQUAL(stop_point->stop_area, stop_area);

    navimake::types::VehicleJourney* vj = data.vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->uri, "41");
    BOOST_CHECK_EQUAL(vj->name, "Porte de Clignancourt");
    BOOST_CHECK_EQUAL(vj->journey_pattern, journey_pattern);

    navimake::types::StopTime* stop = data.stops[0];
    BOOST_CHECK_EQUAL(stop->ODT, false);
    BOOST_CHECK_EQUAL(stop->arrival_time, 21600);
    BOOST_CHECK_EQUAL(stop->departure_time, 21601);
    BOOST_CHECK_EQUAL(stop->journey_pattern_point->stop_point->uri, "frpno:metro4");
    BOOST_CHECK_EQUAL(stop->journey_pattern_point->stop_point->name, "Gare du Nord metro ligne 4");
    BOOST_CHECK_EQUAL(stop->journey_pattern_point->stop_point, data.stop_points[2]);

}
