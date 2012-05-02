#pragma once

#include "type/type.pb.h"
#include "type.h"

namespace navitia { namespace type {
void to_proto(navitia::type::GeographicalCoord geo, pbnavitia::GeographicalCoord* geopb);
void to_proto(navitia::type::StopArea sa, pbnavitia::StopArea * sapb);
void to_proto(navitia::type::StopPoint sp, pbnavitia::StopPoint* sppb);
void to_proto(navitia::type::StopTime st, pbnavitia::StopTime* stpb);
}}
