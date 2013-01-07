#pragma once
#include "type/pb_converter.h"
namespace navitia { namespace timetables {

pbnavitia::Response next_departures(const std::string &request, const std::string &str_dt, const std::string &str_max_dt, const int nb_departures, const int depth, const bool wheelchair, type::Data & data);
pbnavitia::Response next_arrivals(const std::string &request, const std::string &str_dt, const std::string &str_max_dt, const int nb_departures, const int depth, const bool wheelchair, type::Data & data);

}}
