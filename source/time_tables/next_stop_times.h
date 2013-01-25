#pragma once
#include "type/pb_converter.h"
namespace navitia { namespace timetables {

pbnavitia::Response next_departures(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth, const bool wheelchair, type::Data & data);
pbnavitia::Response next_arrivals(const std::string &request, const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes, const int depth, const bool wheelchair, type::Data & data);

}}
