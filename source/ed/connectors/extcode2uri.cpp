#include "extcode2uri.h"


namespace ed{ namespace connectors{

ExtCode2Uri::ExtCode2Uri(const std::string& redis_connection):redis(redis_connection){
    init_logger();
    logger = log4cplus::Logger::getInstance("log");
    try{
        this->redis.redis_connect_with_timeout();
    }catch(const navitia::exception& ne){
        throw navitia::exception(ne.what());
    }
}

void ExtCode2Uri::to_redis(const ed::Data & data){
    // StopArea
    ed::add_redis(data.stop_areas, *this);
    // StopPoint
    ed::add_redis(data.stop_points, *this);
    // Line
    ed::add_redis(data.lines, *this);
    // VehicleJourney
    ed::add_redis(data.vehicle_journeys, *this);
    // Network
    ed::add_redis(data.networks, *this);
}

}}
