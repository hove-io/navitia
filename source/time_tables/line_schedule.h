#pragma once
#include "type/type.h"
#include "routing/routing.h"
#include "get_stop_times.h"
#include "type/pb_converter.h"

namespace navitia { namespace timetables {

typedef std::vector<dt_st> vector_stopTime;
typedef std::vector<std::string> vector_string;

pbnavitia::Response line_schedule(const std::string & line_externalcode, const std::string &str_dt, const std::string &str_max_dt, const uint32_t max_depth, type::Data &d);

}}
