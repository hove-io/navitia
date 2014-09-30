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
#include <string>
#include <map>
#include "type/message.h"

namespace navitia{namespace type{
    class Data;
}}

namespace ed{ namespace connectors{

struct RealtimeLoaderConfig{
    std::string connection_string;
    uint32_t shift_days;

    RealtimeLoaderConfig(const std::string& connectionstring, const uint32_t shiftdays) : connection_string(connectionstring), shift_days(shiftdays){}
};

navitia::type::new_disruption::DisruptionHolder load_disruptions(
        const RealtimeLoaderConfig& conf,
        const boost::posix_time::ptime& current_time);

void apply_messages(navitia::type::Data& data);

std::vector<navitia::type::AtPerturbation> load_at_perturbations(
        const RealtimeLoaderConfig& conf,
        const boost::posix_time::ptime& current_time);

}}//connectors
