#include <iostream>
#include <boost/foreach.hpp>

#include "connectors.h"

int main (int argc, char const* argv[])
{
    navimake::Data data;
    {
        navimake::connectors::CsvFusio connector("/home/kinou/Public/BOD/");
        connector.fill(data);
    }

    std::cout << "line: " << data.lines.size() << std::endl;
    std::cout << "route: " << data.routes.size() << std::endl;
    std::cout << "stoparea: " << data.stop_areas.size() << std::endl;
    std::cout << "stoppoint: " << data.stop_points.size() << std::endl;
    std::cout << "vehiclejourney: " << data.vehicle_journeys.size() << std::endl;
    std::cout << "stop: " << data.stops.size() << std::endl;
    std::cout << "connection: " << data.connections.size() << std::endl;

    data.clean();
    data.sort();

    BOOST_FOREACH(navimake::types::Mode* mode, data.modes){
        std::cout << mode->mode_type->idx << " " << mode->mode_type->name << " " << mode->idx << " " << mode->name << " " << std::endl;
    }


    return 0;
}
