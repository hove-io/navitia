#pragma once
#include "type/type.pb.h"

//Forward declare
namespace navitia {
namespace type {
    struct Data;
    struct GeographicalCoord;    
}
namespace georef{
    struct Path;
}
}




namespace navitia { namespace streetnetwork {

void create_pb(const navitia::georef::Path& path, const navitia::type::Data& data, pbnavitia::StreetNetwork* sn);

pbnavitia::Response street_network(const navitia::type::GeographicalCoord &origin, 
        const navitia::type::GeographicalCoord& destination, const navitia::type::Data &d);
}}
