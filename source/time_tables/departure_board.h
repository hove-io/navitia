#pragma once
#include "type/pb_converter.h"
#include "routing/routing.h"
#include "get_stop_times.h"


namespace navitia { namespace timetables {
typedef std::vector<routing::DateTime> vector_datetime;

pbnavitia::Response departure_board(const std::string &filter, const std::string &date, const std::string &date_changetime, const type::Data &data);
}

}
