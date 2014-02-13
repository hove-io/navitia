#pragma once
#include <proj_api.h>
#include"type/type.h"

namespace ed{ namespace connectors{

    struct Projection{
        std::string name;
        std::string definition;
        bool is_degree;

        Projection() : name("wgs84"), definition("+init=epsg:4326"), is_degree(true){}
        Projection(const std::string& name, const std::string& num_epsg, bool is_degree = false);
        Projection(const Projection& projection) :  name(projection.name),
            definition(projection.definition), is_degree(projection.is_degree){};
    };

    struct ConvCoord{
        Projection origin;
        Projection destination;
        ConvCoord(){};
        ConvCoord(const Projection& origin, const Projection& destination = Projection()): origin(origin), destination(destination){};
        ConvCoord(const ConvCoord& conv_coord): origin(conv_coord.origin), destination(conv_coord.destination){};

        navitia::type::GeographicalCoord convert_to(navitia::type::GeographicalCoord coord) const;
    };


}//namespace connectors
}//namespace ed
