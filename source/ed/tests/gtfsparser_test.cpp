#include "ed/data.h"
#include "ed/connectors/gtfs_parser.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include <string>
#include "config.h"
#include "ed/build_helper.h"
#include "utils/csv.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

const std::string gtfs_path = "/ed/gtfs";

BOOST_AUTO_TEST_CASE(required_files) {
    std::vector<std::string> files = {"agency", "routes", "stop_times", "trips"};
    BOOST_REQUIRE_EQUAL(0, 1);
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

    using file_parser = ed::connectors::FileParser<ed::connectors::AgencyGtfsHandler>;
    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }

    //Check que les networks sont bien remplis
    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",") << "\n";
        sstream << "ratp, RATP,,,,,\n";
        sstream << ", ACME,,,,,";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        file_parser(parser.gtfs_data, sstream).fill(data);

        BOOST_REQUIRE_EQUAL(data.networks.size(), 2);
        BOOST_REQUIRE_EQUAL(data.networks.front()->uri, "ratp");
        BOOST_REQUIRE_EQUAL(data.networks.front()->name, "RATP");
        BOOST_REQUIRE_EQUAL(data.networks[1]->uri, "");
        BOOST_REQUIRE_EQUAL(data.networks[1]->name, "ACME");
    }
}

BOOST_AUTO_TEST_CASE(parse_stops) {
    std::vector<std::string> fields={"stop_id","stop_code", "stop_name", "stop_desc", "stop_lat",
                                     "stop_lon", "zone_id", "stop_url", "location_type", "parent_station",
                                     "stop_timezone", "wheelchair_boarding"},
            required_fields = {"stop_id", "stop_name", "stop_lat", "stop_lon"};

    using file_parser = ed::connectors::FileParser<ed::connectors::StopsGtfsHandler>;
    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(required_fields, ",") << "\n";
        sstream << "\"a\", \"A\",\"bad_lon\",\"bad_lat\"";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }
}

BOOST_AUTO_TEST_CASE(parse_transfers) {

    std::vector<std::string> fields={"from_stop_id","to_stop_id","transfer_type",
                                     "min_transfer_type"},
            required_fields = {"from_stop_id","to_stop_id"};

    using file_parser = ed::connectors::FileParser<ed::connectors::TransfersGtfsHandler>;
    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }
}

BOOST_AUTO_TEST_CASE(parse_lines) {
    std::vector<std::string> fields={"route_id","agency_id","route_short_name",
                                     "route_long_name", "route_desc",
                                     "route_type", "route_url", "route_color",
                                     "route_text_color"},
            required_fields = {"route_id", "route_short_name", "route_long_name",
                               "route_type"};

    using file_parser = ed::connectors::FileParser<ed::connectors::RouteGtfsHandler>;
    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }
}


BOOST_AUTO_TEST_CASE(parse_trips) {
    std::vector<std::string> fields={"route_id","service_id","trip_id",
                "trip_headsign", "trip_short_name", "direction_id", "block_id",
                "shape_id", "wheelchair_accessible"},
            required_fields = {"route_id", "service_id", "trip_id"};

    using file_parser = ed::connectors::FileParser<ed::connectors::TripsGtfsHandler>;
    //Check des champs obligatoires
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }
}

BOOST_AUTO_TEST_CASE(parse_raw_gtfs){
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
}

BOOST_AUTO_TEST_CASE(parse_gtfs){
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(FIXTURES_DIR) + gtfs_path + "_google_example");
    parser.fill(data);

    //Agency
    BOOST_REQUIRE_EQUAL(data.networks.size(), 1);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "Demo Transit Authority");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "DTA");

    //=> no stop area in the file, so one area has been created for each stop point
    //Stop areas
    BOOST_REQUIRE_EQUAL(data.stop_areas.size(), 9);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->uri, "FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(data.stop_areas[0]->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lat(), 36.425288, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lon(), -117.133162, 0.1);

    BOOST_CHECK_EQUAL(data.stop_areas[8]->uri, "AMV");
    BOOST_CHECK_EQUAL(data.stop_areas[8]->name, "Amargosa Valley (Demo)");
    BOOST_CHECK_CLOSE(data.stop_areas[8]->coord.lat(), 36.641496, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[8]->coord.lon(), -116.40094, 0.1);

    //Stop points
    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 9);
    BOOST_CHECK_EQUAL(data.stop_points[0]->uri, "FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(data.stop_points[0]->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lat(), 36.425288, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lon(), -117.133162, 0.1);

    BOOST_CHECK_EQUAL(data.stop_points[8]->uri, "AMV");
    BOOST_CHECK_EQUAL(data.stop_points[8]->name, "Amargosa Valley (Demo)");
    BOOST_CHECK_CLOSE(data.stop_points[8]->coord.lat(), 36.641496, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[8]->coord.lon(), -116.40094, 0.1);

    //Transfers
    BOOST_REQUIRE_EQUAL(data.stop_point_connections.size(), 0);

    // Lignes
    BOOST_REQUIRE_EQUAL(data.lines.size(), 5);
    for(auto l : data.lines)
        std::cout << l->uri << " " << l->name << "  " << l->id << std::endl;
    BOOST_CHECK_EQUAL(data.lines[0]->uri, "AB");
    BOOST_CHECK_EQUAL(data.lines[0]->name, "Airport - Bullfrog");
    BOOST_REQUIRE(data.lines[0]->network != nullptr);
    BOOST_CHECK_EQUAL(data.lines[0]->network->uri, "DTA");
    BOOST_REQUIRE(data.lines[0]->commercial_mode != nullptr);
    BOOST_CHECK_EQUAL(data.lines[0]->commercial_mode->id, "3");

    BOOST_CHECK_EQUAL(data.lines[4]->uri, "AAMV");
    BOOST_CHECK_EQUAL(data.lines[4]->name, "Airport - Amargosa Valley");
    BOOST_REQUIRE(data.lines[4]->network != nullptr);
    BOOST_CHECK_EQUAL(data.lines[4]->network->uri, "DTA");
    BOOST_REQUIRE(data.lines[4]->commercial_mode != nullptr);
    BOOST_CHECK_EQUAL(data.lines[4]->commercial_mode->id, "3");

    //Trips
    BOOST_REQUIRE_EQUAL(data.vehicle_journeys.size(), 11);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->uri, "AB1");
    BOOST_REQUIRE(data.vehicle_journeys[0]->tmp_line != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->tmp_line->uri, "AB");
    BOOST_REQUIRE(data.vehicle_journeys[0]->validity_pattern != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->validity_pattern->uri, "FULLW");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->name, "to Bullfrog");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->block_id, "1");

    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->uri, "AAMV4");
    BOOST_REQUIRE(data.vehicle_journeys[10]->tmp_line != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->tmp_line->uri, "AAMV");
    BOOST_REQUIRE(data.vehicle_journeys[10]->validity_pattern != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->validity_pattern->uri, "WE");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->name, "to Airport");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->block_id, "");

    //Calendar
    BOOST_REQUIRE_EQUAL(data.validity_patterns.size(), 2);
    BOOST_CHECK_EQUAL(data.validity_patterns[0]->uri, "FULLW");

    BOOST_CHECK_EQUAL(data.validity_patterns[1]->uri, "WE");

    //Stop time
    BOOST_REQUIRE_EQUAL(data.stops.size(), 28);
    BOOST_REQUIRE(data.stops[0]->vehicle_journey != nullptr);
    BOOST_CHECK_EQUAL(data.stops[0]->vehicle_journey->uri, "STBA");
    BOOST_CHECK_EQUAL(data.stops[0]->arrival_time, 6*3600);
    BOOST_CHECK_EQUAL(data.stops[0]->departure_time, 6*3600);
    BOOST_REQUIRE(data.stops[0]->tmp_stop_point != nullptr);
    BOOST_CHECK_EQUAL(data.stops[0]->tmp_stop_point->uri, "STAGECOACH");
    BOOST_CHECK_EQUAL(data.stops[0]->order, 1);
}
