#pragma once
#include "type.h"

class Data{

    public:
    Container<ValidityPattern> validity_patterns;

    Container<Line> lines;

    Container<Route> routes;

    Container<VehicleJourney> vehicle_journeys;

    Container<StopPoint> stop_points;

    Container<StopArea> stop_areas;

    std::vector<StopTime> stop_times;

    Index1ToN<StopArea, StopPoint> stoppoint_of_stoparea;

    public:
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & validity_patterns & lines & routes & vehicle_journeys & stop_points & stop_areas & stop_times;
    }


    void build_index();
};


