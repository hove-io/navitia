#pragma once
#include "type/type.pb.h"
#include "type/request.pb.h"
#include "type/response.pb.h"

namespace navitia {

// Forward declare
namespace type {
struct Data;
enum class Type_e;
struct GeographicalCoord;
}

namespace proximitylist {
pbnavitia::Response find(const type::GeographicalCoord& coord, const double distance,
                         const std::vector<type::Type_e> & filter,
                         const uint32_t depth, const uint32_t count, const uint32_t start_page,
                         const type::Data & data);
}} // namespace navitia::proximitylist
