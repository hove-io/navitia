#pragma once
#include "data.h"
#include "type/type.pb.h"

namespace navitia{
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::City* city, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::StopArea* stop_area, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::StopPoint* stop_point, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::Way* way, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::Line* line, int max_depth = 0);
}//namespace navitia
