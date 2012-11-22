#pragma once
#include "type/type.h"
#include "routing/routing.h"
#include "next_departures.h"
namespace navitia { namespace timetables {

    typedef std::vector<dt_st> vector_stopTime;

pbnavitia::Response line_schedule(const std::string & line_externalcode, const std::string &str_dt, const std::string &str_max_dt, const int nb_departures, const uint32_t max_depth, type::Data &d);

}}
