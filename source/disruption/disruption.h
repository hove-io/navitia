#pragma once
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/type.h"
#include "type/pt_data.h"

namespace navitia { namespace disruption {
typedef std::vector<const navitia::type::Line*> lines;
typedef std::pair<const navitia::type::Network*, lines> pair_network_line;
typedef std::vector<pair_network_line> network_line;

network_line disruptions_list(const std::vector<type::idx_t>& line_idx,
                        const type::PT_Data &d,
                        const boost::posix_time::time_period action_period,
                        const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());
}}

