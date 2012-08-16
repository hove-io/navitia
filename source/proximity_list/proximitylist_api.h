#pragma once
#include "type/data.h"
#include "type/type.pb.h"

namespace navitia { namespace proximitylist {
pbnavitia::Response find(type::GeographicalCoord coord, double distance, const std::vector<nt::Type_e> & filter, const type::Data & data);
}} // namespace navitia::proximitylist
