#pragma once
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/type.h"
#include "type/pt_data.h"

namespace navitia { namespace disruption {
typedef std::vector<const navitia::type::Line*> lines;
typedef std::pair<const navitia::type::Network*, lines> pair_network_line;
typedef std::vector<pair_network_line> network_line_list;

network_line_list disruptions_list(const std::string& filter,
                        const std::vector<std::string>& forbidden_uris,
                        const type::Data &d,
                        const boost::posix_time::time_period action_period,
                        const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());
}}

