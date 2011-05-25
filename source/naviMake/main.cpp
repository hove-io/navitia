#include <iostream>
#include <boost/foreach.hpp>

#include "connectors.h"

#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include "filter.h"

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

    {
        std::ofstream ofs("data.nav.flz",std::ios::out|std::ios::binary|std::ios::trunc);
        boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
        out.push(FastLZCompressor(2048*500), 1024*500, 1024*500);
        out.push(ofs);
        boost::archive::binary_oarchive oa(out);
        oa << nav_data;
    } 

    return 0;
}
