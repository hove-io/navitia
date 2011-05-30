#include <iostream>
#include <boost/foreach.hpp>

#include "connectors.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace pt = boost::posix_time;

int main(int argc, char const* argv[])
{
    pt::ptime start, end;
    int read, clean, sort, transform, save;

    navimake::Data data;
    {
        start = pt::microsec_clock::local_time();
        navimake::connectors::CsvFusio connector("data/");
        connector.fill(data);
        read = (pt::microsec_clock::local_time() - start).total_milliseconds();
    }

    std::cout << "line: " << data.lines.size() << std::endl;
    std::cout << "route: " << data.routes.size() << std::endl;
    std::cout << "stoparea: " << data.stop_areas.size() << std::endl;
    std::cout << "stoppoint: " << data.stop_points.size() << std::endl;
    std::cout << "vehiclejourney: " << data.vehicle_journeys.size() << std::endl;
    std::cout << "stop: " << data.stops.size() << std::endl;
    std::cout << "connection: " << data.connections.size() << std::endl;
    std::cout << "route points: " << data.route_points.size() << std::endl;

    start = pt::microsec_clock::local_time();
    data.clean();
    clean = (pt::microsec_clock::local_time() - start).total_milliseconds();
    start = pt::microsec_clock::local_time();
    data.sort();
    sort = (pt::microsec_clock::local_time() - start).total_milliseconds();

    navitia::type::Data nav_data;

    start = pt::microsec_clock::local_time();
    data.transform(nav_data);
    transform = (pt::microsec_clock::local_time() - start).total_milliseconds();
    start = pt::microsec_clock::local_time();
    nav_data.save_flz("data.nav.flz");
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    std::cout << "temps de traitement" << std::endl;
    std::cout << "\t lecture des fichiers " << read << "ms" << std::endl;
    std::cout << "\t netoyage des données " << clean << "ms" << std::endl;
    std::cout << "\t trie des données " << sort << "ms" << std::endl;
    std::cout << "\t transformation " << transform << "ms" << std::endl;
    std::cout << "\t serialization " << save << "ms" << std::endl;

    return 0;
}
