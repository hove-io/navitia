#pragma once
#include "raptor.h"
#include "type/type.pb.h"
#include "boost/date_time/gregorian_calendar.hpp"

namespace navitia { namespace routing { namespace raptor {

pbnavitia::Response make_response(std::vector<navitia::routing::Path> paths, const nt::Data & d);
void checkDateTime(int time, boost::gregorian::date date, boost::gregorian::date_period period);

struct badTime{};
struct badDate{};

}}}
