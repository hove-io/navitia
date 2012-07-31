#pragma once
#include "type/type.pb.h"

//Forward declare
namespace navitia { namespace type {
    struct Data;
    struct GeographicalCoord;
}}

namespace navitia { namespace streetnetwork {


pbnavitia::Response street_network(const navitia::type::GeographicalCoord &origin, 
        const navitia::type::GeographicalCoord& destination, const navitia::type::Data &d);
}}
