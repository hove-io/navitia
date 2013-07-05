#include "ed/data.h"
#include "ed/gtfs_parser.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include <string>
#include "config.h"
#include "ed/build_helper.h"
#include "utils/csv.h"

const std::string gtfs_path = "/ed/gtfs";

BOOST_AUTO_TEST_CASE(required_files) {
    std::vector<std::string> files = {"agency", "routes", "stop_times", "trips"};
    for(auto file : files)
    {
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) +  gtfs_path + "_sans_"+file);
        BOOST_REQUIRE_THROW(parser.fill(data, "20130305"), ed::connectors::FileNotFoundException);
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
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_THROW(parser.parse_agency(data, csv), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_agency(data, csv));
    }

    //Check que les networks sont bien remplis
    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",") << "\n";
        sstream << "ratp, RATP,,,,,\n";
        sstream << ", ACME,,,,,";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true); 
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
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_THROW(parser.parse_stops(data, csv), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_stops(data, csv));
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(required_fields, ",") << "\n";
        sstream << "\"a\", \"A\",\"bad_lon\",\"bad_lat\"";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_stops(data, csv));
    }
}

BOOST_AUTO_TEST_CASE(parse_transfers) {

    std::vector<std::string> fields={"from_stop_id","to_stop_id","transfer_type",
                                     "min_transfer_type"},
            required_fields = {"from_stop_id","to_stop_id","transfer_type"};

    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_THROW(parser.parse_transfers(data, csv), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_transfers(data, csv));
    }
}

BOOST_AUTO_TEST_CASE(parse_lines) {
    std::vector<std::string> fields={"route_id","agency_id","route_short_name",
                                     "route_long_name", "route_desc",
                                     "route_type", "route_url", "route_color",
                                     "route_text_color"},
            required_fields = {"route_id", "route_short_name", "route_long_name",
                               "route_type"};

    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_THROW(parser.parse_lines(data, csv), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_lines(data, csv));
    }
}


BOOST_AUTO_TEST_CASE(parse_trips) {
    std::vector<std::string> fields={"route_id","service_id","trip_id",
                "trip_headsign", "trip_short_name", "direction_id", "block_id",
                "shape_id", "wheelchair_accessible"},
            required_fields = {"route_id", "service_id", "trip_id"};

    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_THROW(parser.parse_trips(data, csv), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        CsvReader csv(sstream, ',' , true);
        BOOST_REQUIRE_NO_THROW(parser.parse_trips(data, csv));
    }
}

BOOST_AUTO_TEST_CASE(parse_gtfs){
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
    parser.fill(data);
    data.complete();
    //data.clean();
    //data.sort();
    std::cout << "Journey patterns size : " << data.journey_patterns.size() <<
        std::endl;
    for(auto vj : data.vehicle_journeys)
        std::cout  << vj->uri << "  " << vj->journey_pattern->uri << "  "<<
            vj->journey_pattern->route->uri << "  "<< vj->journey_pattern->route->line->uri <<
            std::endl;
    //Agency
    /*
    BOOST_REQUIRE_EQUAL(data.networks.size(), 2);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "RATP");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "ratp");
    BOOST_CHECK_EQUAL(data.networks[1]->name, "ACME");
    BOOST_CHECK_EQUAL(data.networks[1]->uri, "");

    //Stop areas
    BOOST_REQUIRE_EQUAL(data.stop_areas.size(), 5);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->name, "Doublon");
    BOOST_CHECK_EQUAL(data.stop_areas[0]->uri, "doublon_sa");
    BOOST_CHECK_EQUAL(data.stop_areas[0]->coord.lon(), 0);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->coord.lat(), 1);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->wheelchair_boarding, false);


    BOOST_CHECK_EQUAL(data.stop_areas[1]->name, "Stop Area avant");
    BOOST_CHECK_EQUAL(data.stop_areas[1]->uri, "sa_avant");
    BOOST_CHECK_EQUAL(data.stop_areas[1]->coord.lon(), 2.355022);
    BOOST_CHECK_EQUAL(data.stop_areas[1]->coord.lat(), 48.880342);
    BOOST_CHECK_EQUAL(data.stop_areas[1]->wheelchair_boarding, false);

    BOOST_CHECK_EQUAL(data.stop_areas[2]->name, "Stop Area apres");
    BOOST_CHECK_EQUAL(data.stop_areas[2]->uri, "sa_apres");
    BOOST_CHECK_EQUAL(data.stop_areas[2]->coord.lon(), 2.355022);
    BOOST_CHECK_EQUAL(data.stop_areas[2]->coord.lat(), 48.880342);
    BOOST_CHECK_EQUAL(data.stop_areas[2]->wheelchair_boarding, false);

    BOOST_CHECK_EQUAL(data.stop_areas[3]->name, "Stop area seul");
    BOOST_CHECK_EQUAL(data.stop_areas[3]->uri, "sa_seul");
    BOOST_CHECK_EQUAL(data.stop_areas[3]->coord.lon(), 2.355022);
    BOOST_CHECK_EQUAL(data.stop_areas[3]->coord.lat(), 48.880342);
    BOOST_CHECK_EQUAL(data.stop_areas[3]->wheelchair_boarding, false);

    BOOST_CHECK_EQUAL(data.stop_areas[4]->name, "Stop area wheelchair");
    BOOST_CHECK_EQUAL(data.stop_areas[4]->uri, "sa_wheelchair");
    BOOST_CHECK_EQUAL(data.stop_areas[4]->coord.lon(), 2.355022);
    BOOST_CHECK_EQUAL(data.stop_areas[4]->coord.lat(), 48.880342);
    BOOST_CHECK_EQUAL(data.stop_areas[4]->wheelchair_boarding, true);



    //Stop points
    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 9);

    BOOST_CHECK_EQUAL(data.stop_points[0]->name, "Doublon");
    BOOST_CHECK_EQUAL(data.stop_points[0]->uri, "doublon_sp");
    BOOST_CHECK_EQUAL(data.stop_points[0]->coord.lon(), 0);
    BOOST_CHECK_EQUAL(data.stop_points[0]->coord.lat(), 1);
    BOOST_CHECK_EQUAL(data.stop_points[0]->wheelchair_boarding, false);
    BOOST_REQUIRE(data.stop_points[0]->stop_area== NULL);

    BOOST_CHECK_EQUAL(data.stop_points[1]->name, "StopPoint apres sa 1");
    BOOST_CHECK_EQUAL(data.stop_points[1]->uri, "sa_avant:sp1");
    BOOST_CHECK_EQUAL(data.stop_points[1]->coord.lat(), 48.880531);
    BOOST_CHECK_EQUAL(data.stop_points[1]->coord.lon(), 2.355381);
    BOOST_CHECK_EQUAL(data.stop_points[1]->wheelchair_boarding, false);
    BOOST_REQUIRE(data.stop_points[1]->stop_area!= NULL);
    BOOST_CHECK_EQUAL(data.stop_points[1]->stop_area->uri, "sa_avant");

    BOOST_CHECK_EQUAL(data.stop_points[2]->name, "StopPoint apres sa 2");
    BOOST_CHECK_EQUAL(data.stop_points[2]->uri, "sa_avant:sp2");
    BOOST_CHECK_EQUAL(data.stop_points[2]->coord.lon(), 2.354786);
    BOOST_CHECK_EQUAL(data.stop_points[2]->coord.lat(), 48.880191);
    BOOST_CHECK_EQUAL(data.stop_points[2]->wheelchair_boarding, false);
    BOOST_REQUIRE(data.stop_points[2]->stop_area!= NULL);
    BOOST_CHECK_EQUAL(data.stop_points[2]->stop_area->uri, "sa_avant");

    BOOST_CHECK_EQUAL(data.stop_points[3]->name, "Stop Point avant sa 1");
    BOOST_CHECK_EQUAL(data.stop_points[3]->uri, "sa_apres:sp1");
    BOOST_CHECK_EQUAL(data.stop_points[3]->coord.lon(), 2.355381);
    BOOST_CHECK_EQUAL(data.stop_points[3]->coord.lat(), 48.880531);
    BOOST_CHECK_EQUAL(data.stop_points[3]->wheelchair_boarding, false);
    BOOST_REQUIRE(data.stop_points[3]->stop_area!= NULL);
    BOOST_CHECK_EQUAL(data.stop_points[3]->stop_area->uri, "sa_apres");

    BOOST_CHECK_EQUAL(data.stop_points[4]->name, "Stop Point avant sa 2");
    BOOST_CHECK_EQUAL(data.stop_points[4]->uri, "sa_apres:sp2");
    BOOST_CHECK_EQUAL(data.stop_points[4]->coord.lon(), 2.354786);
    BOOST_CHECK_EQUAL(data.stop_points[4]->coord.lat(), 48.880191);
    BOOST_CHECK_EQUAL(data.stop_points[4]->wheelchair_boarding, false);
    BOOST_REQUIRE(data.stop_points[4]->stop_area!= NULL);
    BOOST_CHECK_EQUAL(data.stop_points[4]->stop_area->uri, "sa_apres");

    BOOST_CHECK_EQUAL(data.stop_points[5]->name, "Stop point seul");
    BOOST_CHECK_EQUAL(data.stop_points[5]->uri, "sp_seul");
    BOOST_CHECK_EQUAL(data.stop_points[5]->coord.lon(), 2.355022);
    BOOST_CHECK_EQUAL(data.stop_points[5]->coord.lat(), 48.880342);
    BOOST_CHECK_EQUAL(data.stop_points[5]->wheelchair_boarding, false);
    BOOST_CHECK(data.stop_points[5]->stop_area == NULL);

    BOOST_CHECK_EQUAL(data.stop_points[6]->name, "Sp wheelchair");
    BOOST_CHECK_EQUAL(data.stop_points[6]->uri, "sp_wheelchair");
    BOOST_CHECK_EQUAL(data.stop_points[6]->coord.lat(), 4);
    BOOST_CHECK_EQUAL(data.stop_points[6]->coord.lon(), 2);
    BOOST_CHECK_EQUAL(data.stop_points[6]->wheelchair_boarding, true);

    BOOST_CHECK_EQUAL(data.stop_points[7]->name, "Stop point inherits");
    BOOST_CHECK_EQUAL(data.stop_points[7]->uri, "sa_wheelchair:sp1");
    BOOST_CHECK_EQUAL(data.stop_points[7]->coord.lon(), 2.355022);
    BOOST_CHECK_EQUAL(data.stop_points[7]->coord.lat(), 48.880342);
    BOOST_CHECK_EQUAL(data.stop_points[7]->wheelchair_boarding, true);
    BOOST_REQUIRE(data.stop_points[7]->stop_area!= NULL);
    BOOST_CHECK_EQUAL(data.stop_points[7]->stop_area->uri, "sa_wheelchair");

    BOOST_CHECK_EQUAL(data.stop_points[8]->name, "Stop point change");
    BOOST_CHECK_EQUAL(data.stop_points[8]->uri, "sa_wheelchair:sp2");
    BOOST_CHECK_EQUAL(data.stop_points[8]->coord.lon(), 2.355022);
    BOOST_CHECK_EQUAL(data.stop_points[8]->coord.lat(), 48.880342);
    BOOST_CHECK_EQUAL(data.stop_points[8]->wheelchair_boarding, false);
    BOOST_REQUIRE(data.stop_points[8]->stop_area!= NULL);
    BOOST_CHECK_EQUAL(data.stop_points[8]->stop_area->uri, "sa_wheelchair");

    //Transfers
    BOOST_REQUIRE_EQUAL(data.connections.size(), 9);
    BOOST_CHECK_EQUAL(data.connections[0]->departure_stop_point->uri, "sp_seul");
    BOOST_CHECK_EQUAL(data.connections[0]->destination_stop_point->uri, "sa_avant:sp1");
    BOOST_CHECK_EQUAL(data.connections[0]->duration, 150);

    BOOST_CHECK_EQUAL(data.connections[1]->departure_stop_point->uri,"sa_avant:sp1");
    BOOST_CHECK_EQUAL(data.connections[1]->destination_stop_point->uri, "sp_seul");
    BOOST_CHECK_EQUAL(data.connections[1]->duration, 160);

    BOOST_CHECK_EQUAL(data.connections[2]->departure_stop_point->uri,"sa_avant:sp2");
    BOOST_CHECK_EQUAL(data.connections[2]->destination_stop_point->uri, "sp_seul");
    BOOST_CHECK_EQUAL(data.connections[2]->duration, 160);

    BOOST_CHECK_EQUAL(data.connections[3]->departure_stop_point->uri, "sp_seul");
    BOOST_CHECK_EQUAL(data.connections[3]->destination_stop_point->uri, "sa_wheelchair:sp1");
    BOOST_CHECK_EQUAL(data.connections[3]->duration, 170);

    BOOST_CHECK_EQUAL(data.connections[4]->departure_stop_point->uri, "sp_seul");
    BOOST_CHECK_EQUAL(data.connections[4]->destination_stop_point->uri, "sa_wheelchair:sp2");
    BOOST_CHECK_EQUAL(data.connections[4]->duration, 170);

    BOOST_CHECK_EQUAL(data.connections[5]->departure_stop_point->uri, "sa_apres:sp1");
    BOOST_CHECK_EQUAL(data.connections[5]->destination_stop_point->uri, "sa_avant:sp1");
    BOOST_CHECK_EQUAL(data.connections[5]->duration, 180);

    BOOST_CHECK_EQUAL(data.connections[6]->departure_stop_point->uri, "sa_apres:sp1");
    BOOST_CHECK_EQUAL(data.connections[6]->destination_stop_point->uri, "sa_avant:sp2");
    BOOST_CHECK_EQUAL(data.connections[6]->duration, 180);

    BOOST_CHECK_EQUAL(data.connections[7]->departure_stop_point->uri, "sa_apres:sp2");
    BOOST_CHECK_EQUAL(data.connections[7]->destination_stop_point->uri, "sa_avant:sp1");
    BOOST_CHECK_EQUAL(data.connections[7]->duration, 180);

    BOOST_CHECK_EQUAL(data.connections[8]->departure_stop_point->uri, "sa_apres:sp2");
    BOOST_CHECK_EQUAL(data.connections[8]->destination_stop_point->uri, "sa_avant:sp2");
    BOOST_CHECK_EQUAL(data.connections[8]->duration, 180);


    // Lignes
    BOOST_REQUIRE_EQUAL(data.lines.size(), 3);
    for(auto l : data.lines)
        std::cout << l->uri << " " << l->name << "  " << l->id << std::endl;
    BOOST_CHECK_EQUAL(data.lines[0]->uri, "1");
    BOOST_CHECK_EQUAL(data.lines[0]->name, "Ligne 1");
    BOOST_REQUIRE(data.lines[0]->network!=NULL);
    BOOST_CHECK_EQUAL(data.lines[0]->network->uri, "ratp");

    BOOST_CHECK_EQUAL(data.lines[1]->uri, "2");
    BOOST_CHECK_EQUAL(data.lines[1]->name, "Ligne 2");
    BOOST_REQUIRE(data.lines[1]->network==NULL);

    //Trips
    BOOST_REQUIRE_EQUAL(data.vehicle_journeys.size(), 2);
    for(auto t : data.vehicle_journeys)
        std::cout  << t->uri << "  " << t->journey_pattern->route->line->uri <<
            std::endl;
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->uri, "myonetruetrip");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->name, "My one true headsign");
*/
}
