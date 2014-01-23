#pragma once
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/type.h"
#include "type/response.pb.h"

namespace navitia { namespace disruption {

pbnavitia::Response disruptions(const navitia::type::Data &d,
                                const std::string &str_dt,
                                const size_t period,
                                const size_t depth,
                                size_t count,
                                size_t start_page, const std::string &filter,
                                const std::vector<std::string>& forbidden_uris);
}}

