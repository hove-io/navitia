#pragma once
#include "type/type.h"
#include "routing/routing.h"
#include "get_stop_times.h"
#include "type/pb_converter.h"

namespace navitia { namespace timetables {

typedef std::vector<std::string> vector_string;
typedef std::pair<type::DateTime, const type::StopTime*> vector_date_time;

pbnavitia::Response route_schedule(const std::string & line_externalcode, const std::string &str_dt, uint32_t duration, const uint32_t max_depth, type::Data &d);

}}
