#pragma once
#include "raptor.h"
#include "type/type.pb.h"
#include "boost/date_time/posix_time/ptime.hpp"
#include "georef/street_network.h"

namespace navitia { namespace routing { namespace raptor {

pbnavitia::Response make_response(RAPTOR &raptor,
                                  const type::EntryPoint &origin,
                                  const type::EntryPoint &destination,
                                  const std::vector<std::string> &datetimes,
                                  bool clockwise, const float walking_speed,
                                  const int walking_distance, const bool wheelchair,
                                  std::multimap<std::string, std::string> forbidden,
                                  streetnetwork::StreetNetwork & worker);

pbnavitia::Response make_isochrone(RAPTOR &raptor,
                                   type::EntryPoint origin,
                                   const std::string &datetime, bool clockwise,
                                   float walking_speed, int walking_distance,
                                   bool wheelchair,
                                   std::multimap<std::string, std::string> forbidden,
                                   streetnetwork::StreetNetwork & worker, int max_duration = 7200);


}}}
