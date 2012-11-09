#pragma once
#include "data.h"
#include "type/type.pb.h"

namespace navitia{
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::City* city, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::StopArea* stop_area, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::StopPoint* stop_point, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::Way* way, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::Line* line, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::Route* route, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::Network* network, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::CommercialMode * commercial_mode, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::PhysicalMode * physical_mode, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::Connection * connection, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::RoutePoint * route_point, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::Company * company, int max_depth = 0);
void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::StopTime *stop_time, int max_depth = 0);

void fill_pb_placemark(const type::StopPoint & stop_point, const type::Data &data, pbnavitia::PlaceMark* pm, int max_depth = 0);
void fill_pb_placemark(const georef::Way & way, const type::Data &data, pbnavitia::PlaceMark* pm, int max_depth = 0, int house_number = -1);

void fill_road_section(const georef::Path & path, const type::Data &data, pbnavitia::Section* section, int max_depth = 0);
}//namespace navitia
