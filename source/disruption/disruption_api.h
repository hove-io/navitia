#pragma once
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/type.h"
#include "type/response.pb.h"
#include "disruption.h"

namespace navitia { namespace disruption_api {

pbnavitia::Response disruptions(const navitia::type::Data &d,
                            const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time(),
                            uint32_t depth = 1);
}}

