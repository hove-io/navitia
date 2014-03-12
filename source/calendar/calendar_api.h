#pragma once
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/type.h"
#include "type/response.pb.h"

namespace navitia { namespace calendar {

pbnavitia::Response calendars(const navitia::type::Data &d,
                              const std::string &start_date,
                              const std::string &end_date,
                                const size_t depth,
                                size_t count,
                                size_t start_page, const std::string &filter,
                                const std::vector<std::string>& forbidden_uris);
}}
