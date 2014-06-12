/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "type/type.h"
#include "type/pt_data.h"
#include "utils/logger.h"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace navitia { namespace disruption {


struct disrupt{
    type::idx_t idx;
    type::idx_t network_idx;
    std::vector<type::idx_t> line_idx;
    std::vector<type::idx_t> stop_area_idx;
};

class Disruption{
private:

    std::vector<disrupt> disrupts;
    log4cplus::Logger logger;

    type::idx_t find_or_create(const type::Network* network);
    void add_stop_areas(const std::vector<type::idx_t>& network_idx,
                      const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());

    void add_networks(const std::vector<type::idx_t>& network_idx,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());
    void add_lines(const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());
    void sort_disruptions(const type::Data &d);
public:
    Disruption():logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"))){};

    void disruptions_list(const std::string& filter,
                    const std::vector<std::string>& forbidden_uris,
                    const type::Data &d,
                    const boost::posix_time::time_period action_period,
                    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());

    const std::vector<disrupt>& get_disrupts() const;
    size_t get_disrupts_size();
};
}}

