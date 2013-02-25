#pragma once
#include "type/pb_converter.h"
#include "routing/routing.h"
#include "get_stop_times.h"


namespace navitia { namespace timetables {
typedef std::vector<type::DateTime> vector_datetime;
typedef std::pair<uint32_t, uint32_t> stop_point_line;
typedef std::vector<dt_st> vector_dt_st;

pbnavitia::Response departure_board(const std::string &filter, const std::string &date, uint32_t duration, const type::Data &data);
}

}
