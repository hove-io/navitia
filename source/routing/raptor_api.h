#pragma once
#include "raptor.h"
#include "type/type.pb.h"
#include "boost/date_time/gregorian_calendar.hpp"

namespace navitia { namespace routing { namespace raptor {

pbnavitia::Response make_pathes(std::vector<navitia::routing::Path> &paths, const nt::Data & d);
bool checkTime(const int time);
pbnavitia::Response make_response(RAPTOR &raptor, const type::EntryPoint &departure, const type::EntryPoint &destination, const int time, const boost::gregorian::date &date, const senscompute sens, streetnetwork::StreetNetworkWorker & worker);

struct badTime : std::exception{};
struct badDate : std::exception{};

}}}
