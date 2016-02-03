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
#include "type/pb_converter.h"
namespace navitia { namespace timetables {


void next_departures(PbCreator& pb_creator, const std::string &request,
        const std::vector<std::string>& forbidden_uris,
        const boost::posix_time::ptime datetime, uint32_t duration, uint32_t nb_stoptimes,
        const int depth, const type::AccessibiliteParams & accessibilite_params,
        const type::RTLevel rt_level, uint32_t count, uint32_t start_page);
void next_arrivals(PbCreator& pb_creator, const std::string &request,
        const std::vector<std::string>& forbidden_uris,
        const boost::posix_time::ptime datetime, uint32_t duration, uint32_t nb_stoptimes,
        const int depth, const type::AccessibiliteParams & accessibilite_params,
        const type::RTLevel rt_level, uint32_t count, uint32_t start_page);

}}
