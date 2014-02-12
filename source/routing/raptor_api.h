#pragma once
#include "raptor.h"
#include "type/type.pb.h"
#include "type/response.pb.h"
#include "type/request.pb.h"
#include "boost/date_time/posix_time/ptime.hpp"
#include "georef/street_network.h"

namespace navitia { namespace routing {

pbnavitia::Response make_response(RAPTOR &raptor,
                                  const type::EntryPoint &origin,
                                  const type::EntryPoint &destination,
                                  const std::vector<std::string> &datetimes,
                                  bool clockwise,
                                  const type::AccessibiliteParams & accessibilite_params,
                                  std::vector<std::string> forbidden,
                                  georef::StreetNetwork & worker,
                                  bool disruption_active,
                                  uint32_t max_duration=std::numeric_limits<uint32_t>::max(),
                                  uint32_t max_transfers=std::numeric_limits<uint32_t>::max());

pbnavitia::Response make_isochrone(RAPTOR &raptor,
                                   type::EntryPoint origin,
                                   const std::string &datetime, bool clockwise,
                                   const type::AccessibiliteParams & accessibilite_params,
                                   std::vector<std::string> forbidden,
                                   georef::StreetNetwork & worker,
                                   bool disruption_active, int max_duration = 3600,
                                   uint32_t max_transfers=std::numeric_limits<uint32_t>::max());


}}
