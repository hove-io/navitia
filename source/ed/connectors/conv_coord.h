#pragma once
#include <proj_api.h>
#include"type/type.h"

namespace ed{ namespace connectors{

    struct Projection{
        std::string name;
        std::string definition;
        bool is_degree;
        projPJ proj_pj = nullptr;

        Projection(const std::string& name, const std::string& num_epsg, bool is_degree);
        Projection() : Projection("wgs84", "4326", true) {}
        Projection(const Projection&);
        ~Projection();
    };

    struct ConvCoord {
        Projection origin;
        Projection destination;
        ConvCoord(const Projection& origin, const Projection& destination = Projection()): origin(origin), destination(destination){}
        ConvCoord(const ConvCoord& conv_coord):origin(conv_coord.origin), destination(conv_coord.destination){}
        navitia::type::GeographicalCoord convert_to(navitia::type::GeographicalCoord coord) const;
    };

}//namespace connectors
}//namespace ed
