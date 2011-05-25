#include <iostream>
#include <boost/foreach.hpp>

#include "connectors.h"

#include <fstream>

int main(int argc, char const* argv[])
{
    navimake::Data data;
    {
        navimake::connectors::CsvFusio connector("data/");
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

    navitia::type::Data nav_data;

    data.transform(nav_data);
    nav_data.save_flz("data.nav.flz");
    return 0;
}
