#include <iostream>
#include <boost/foreach.hpp>

#include "connectors.h"

int main (int argc, char const* argv[])
{
    BO::connectors::CsvFusio connector("/home/kinou/Public/BOD/");
    BO::Data data;
    connector.fill(data);

    std::cout << "line: " << data.lines.size() << std::endl;
    std::cout << "route: " << data.routes.size() << std::endl;
    std::cout << "stoparea: " << data.stop_areas.size() << std::endl;
    std::cout << "stoppoint: " << data.stop_points.size() << std::endl;
    std::cout << "vehiclejourney: " << data.vehicle_journeys.size() << std::endl;
    std::cout << "stop: " << data.stops.size() << std::endl;
    std::cout << "connection: " << data.connections.size() << std::endl;
    return 0;
}
