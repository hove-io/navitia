#pragma once
#include "raptor.h"
#include "type/type.pb.h"
#include "boost/date_time/posix_time/ptime.hpp"

namespace navitia { namespace routing { namespace raptor {

pbnavitia::Response make_response(RAPTOR &raptor,
                                  const type::EntryPoint &origin,
                                  const type::EntryPoint &destination,
                                  const boost::posix_time::ptime &datetime,
                                  const senscompute sens,
                                  streetnetwork::StreetNetworkWorker & worker);

}}}
